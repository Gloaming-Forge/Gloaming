#include "gameplay/TweenSystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "engine/Log.hpp"

#include <algorithm>
#include <cstdlib>
#include <unordered_map>

namespace gloaming {

EasingFunction getEasingByName(const std::string& name) {
    static const std::unordered_map<std::string, EasingFunction> easings = {
        {"linear",              Easing::linear},
        {"ease_in_quad",        Easing::easeInQuad},
        {"ease_out_quad",       Easing::easeOutQuad},
        {"ease_in_out_quad",    Easing::easeInOutQuad},
        {"ease_in_cubic",       Easing::easeInCubic},
        {"ease_out_cubic",      Easing::easeOutCubic},
        {"ease_in_out_cubic",   Easing::easeInOutCubic},
        {"ease_in_elastic",     Easing::easeInElastic},
        {"ease_out_elastic",    Easing::easeOutElastic},
        {"ease_in_bounce",      Easing::easeInBounce},
        {"ease_out_bounce",     Easing::easeOutBounce},
        {"ease_in_back",        Easing::easeInBack},
        {"ease_out_back",       Easing::easeOutBack},
        {"ease_in_out_back",    Easing::easeInOutBack},
    };

    auto it = easings.find(name);
    if (it != easings.end()) {
        return it->second;
    }
    return Easing::linear;
}

TweenId TweenSystem::tweenTo(Entity entity, TweenProperty property, float targetValue,
                              float duration, EasingFunction easing,
                              std::function<void()> onComplete) {
    Tween tween;
    tween.id = m_nextId++;
    tween.entity = entity;
    tween.property = property;
    tween.endValue = targetValue;
    tween.duration = std::max(0.001f, duration); // Prevent zero-duration
    tween.elapsed = 0.0f;
    tween.easing = easing ? easing : Easing::linear;
    tween.onComplete = std::move(onComplete);
    tween.alive = true;
    tween.started = false;

    m_tweens.push_back(std::move(tween));
    return m_tweens.back().id;
}

bool TweenSystem::cancel(TweenId id) {
    for (auto& tween : m_tweens) {
        if (tween.id == id && tween.alive) {
            tween.alive = false;
            return true;
        }
    }
    return false;
}

int TweenSystem::cancelAllForEntity(Entity entity) {
    int count = 0;
    for (auto& tween : m_tweens) {
        if (tween.entity == entity && tween.alive) {
            tween.alive = false;
            ++count;
        }
    }
    return count;
}

void TweenSystem::shake(float intensity, float duration, EasingFunction decay) {
    m_shake.intensity = intensity;
    m_shake.duration = duration;
    m_shake.elapsed = 0.0f;
    m_shake.decay = decay ? decay : Easing::easeOutQuad;
    m_shake.active = true;
    m_shake.offset = {0.0f, 0.0f};
}

void TweenSystem::update(float dt, Registry& registry) {
    // Collect completed tween callbacks to invoke after iteration
    // (callbacks might create new tweens)
    std::vector<std::function<void()>> completions;

    for (auto& tween : m_tweens) {
        if (!tween.alive) continue;

        // Check entity still valid
        if (!registry.valid(tween.entity)) {
            tween.alive = false;
            continue;
        }

        // Capture start value on first update
        if (!tween.started) {
            tween.startValue = getPropertyValue(tween.entity, tween.property, registry);
            tween.started = true;
        }

        tween.elapsed += dt;
        float t = std::min(tween.elapsed / tween.duration, 1.0f);
        float easedT = tween.easing(t);

        // Interpolate
        float value = tween.startValue + (tween.endValue - tween.startValue) * easedT;
        setPropertyValue(tween.entity, tween.property, value, registry);

        // Complete?
        if (t >= 1.0f) {
            tween.alive = false;
            if (tween.onComplete) {
                completions.push_back(std::move(tween.onComplete));
            }
        }
    }

    // Invoke completion callbacks
    for (auto& cb : completions) {
        try {
            cb();
        } catch (const std::exception& ex) {
            LOG_ERROR("Tween completion callback error: {}", ex.what());
        }
    }

    // Clean up dead tweens
    m_tweens.erase(
        std::remove_if(m_tweens.begin(), m_tweens.end(),
                        [](const Tween& tw) { return !tw.alive; }),
        m_tweens.end());

    // Update camera shake
    if (m_shake.active) {
        m_shake.elapsed += dt;
        if (m_shake.elapsed >= m_shake.duration) {
            m_shake.active = false;
            m_shake.offset = {0.0f, 0.0f};
        } else {
            float t = m_shake.elapsed / m_shake.duration;
            float decayFactor = 1.0f - m_shake.decay(t);
            float currentIntensity = m_shake.intensity * decayFactor;
            m_shake.offset.x = randomNormalized() * currentIntensity;
            m_shake.offset.y = randomNormalized() * currentIntensity;
        }
    }
}

size_t TweenSystem::activeCount() const {
    size_t count = 0;
    for (const auto& tw : m_tweens) {
        if (tw.alive) ++count;
    }
    return count;
}

void TweenSystem::clear() {
    m_tweens.clear();
    m_shake.active = false;
    m_shake.offset = {0.0f, 0.0f};
}

float TweenSystem::getPropertyValue(Entity entity, TweenProperty property,
                                     Registry& registry) {
    switch (property) {
        case TweenProperty::X: {
            auto* t = registry.tryGet<Transform>(entity);
            return t ? t->position.x : 0.0f;
        }
        case TweenProperty::Y: {
            auto* t = registry.tryGet<Transform>(entity);
            return t ? t->position.y : 0.0f;
        }
        case TweenProperty::Rotation: {
            auto* t = registry.tryGet<Transform>(entity);
            return t ? t->rotation : 0.0f;
        }
        case TweenProperty::ScaleX: {
            auto* t = registry.tryGet<Transform>(entity);
            return t ? t->scale.x : 1.0f;
        }
        case TweenProperty::ScaleY: {
            auto* t = registry.tryGet<Transform>(entity);
            return t ? t->scale.y : 1.0f;
        }
        case TweenProperty::Alpha: {
            auto* s = registry.tryGet<Sprite>(entity);
            return s ? static_cast<float>(s->tint.a) : 255.0f;
        }
    }
    return 0.0f;
}

void TweenSystem::setPropertyValue(Entity entity, TweenProperty property,
                                    float value, Registry& registry) {
    switch (property) {
        case TweenProperty::X: {
            auto* t = registry.tryGet<Transform>(entity);
            if (t) t->position.x = value;
            break;
        }
        case TweenProperty::Y: {
            auto* t = registry.tryGet<Transform>(entity);
            if (t) t->position.y = value;
            break;
        }
        case TweenProperty::Rotation: {
            auto* t = registry.tryGet<Transform>(entity);
            if (t) t->rotation = value;
            break;
        }
        case TweenProperty::ScaleX: {
            auto* t = registry.tryGet<Transform>(entity);
            if (t) t->scale.x = value;
            break;
        }
        case TweenProperty::ScaleY: {
            auto* t = registry.tryGet<Transform>(entity);
            if (t) t->scale.y = value;
            break;
        }
        case TweenProperty::Alpha: {
            auto* s = registry.tryGet<Sprite>(entity);
            if (s) {
                s->tint.a = static_cast<uint8_t>(
                    std::max(0.0f, std::min(255.0f, value)));
            }
            break;
        }
    }
}

float TweenSystem::randomNormalized() {
    return (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
}

} // namespace gloaming
