#pragma once

#include "ecs/Entity.hpp"
#include "rendering/IRenderer.hpp"

#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <cmath>

namespace gloaming {

// Forward declarations
class Registry;
class Camera;

/// Unique identifier for a tween
using TweenId = uint32_t;

/// Invalid tween ID sentinel
constexpr TweenId InvalidTweenId = 0;

/// Easing function type
using EasingFunction = float(*)(float t);

/// Standard easing functions
namespace Easing {
    // Linear
    inline float linear(float t) { return t; }

    // Quadratic
    inline float easeInQuad(float t) { return t * t; }
    inline float easeOutQuad(float t) { return t * (2.0f - t); }
    inline float easeInOutQuad(float t) {
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    }

    // Cubic
    inline float easeInCubic(float t) { return t * t * t; }
    inline float easeOutCubic(float t) { float u = t - 1.0f; return u * u * u + 1.0f; }
    inline float easeInOutCubic(float t) {
        return t < 0.5f ? 4.0f * t * t * t
                        : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
    }

    // Elastic
    inline float easeOutElastic(float t) {
        if (t <= 0.0f) return 0.0f;
        if (t >= 1.0f) return 1.0f;
        constexpr float p = 0.3f;
        return std::pow(2.0f, -10.0f * t) *
               std::sin((t - p / 4.0f) * (2.0f * PI / p)) + 1.0f;
    }
    inline float easeInElastic(float t) {
        return 1.0f - easeOutElastic(1.0f - t);
    }

    // Bounce
    inline float easeOutBounce(float t) {
        if (t < 1.0f / 2.75f) {
            return 7.5625f * t * t;
        } else if (t < 2.0f / 2.75f) {
            t -= 1.5f / 2.75f;
            return 7.5625f * t * t + 0.75f;
        } else if (t < 2.5f / 2.75f) {
            t -= 2.25f / 2.75f;
            return 7.5625f * t * t + 0.9375f;
        } else {
            t -= 2.625f / 2.75f;
            return 7.5625f * t * t + 0.984375f;
        }
    }
    inline float easeInBounce(float t) {
        return 1.0f - easeOutBounce(1.0f - t);
    }

    // Back (overshoot)
    inline float easeInBack(float t) {
        constexpr float s = 1.70158f;
        return t * t * ((s + 1.0f) * t - s);
    }
    inline float easeOutBack(float t) {
        constexpr float s = 1.70158f;
        float u = t - 1.0f;
        return u * u * ((s + 1.0f) * u + s) + 1.0f;
    }
    inline float easeInOutBack(float t) {
        constexpr float s = 1.70158f * 1.525f;
        t *= 2.0f;
        if (t < 1.0f) {
            return 0.5f * (t * t * ((s + 1.0f) * t - s));
        }
        t -= 2.0f;
        return 0.5f * (t * t * ((s + 1.0f) * t + s) + 2.0f);
    }
}

/// Resolve an easing function by name string
EasingFunction getEasingByName(const std::string& name);

/// Property being tweened
enum class TweenProperty {
    X,          // Transform.position.x
    Y,          // Transform.position.y
    Rotation,   // Transform.rotation
    ScaleX,     // Transform.scale.x
    ScaleY,     // Transform.scale.y
    Alpha,      // Sprite.tint.a
};

/// A single active tween
struct Tween {
    TweenId id = InvalidTweenId;
    Entity entity = NullEntity;
    TweenProperty property = TweenProperty::X;
    float startValue = 0.0f;
    float endValue = 0.0f;
    float duration = 0.0f;
    float elapsed = 0.0f;
    EasingFunction easing = Easing::linear;
    std::function<void()> onComplete;
    bool alive = true;
    bool started = false;  // Whether startValue has been captured
};

/// Camera shake state
struct CameraShake {
    float intensity = 0.0f;
    float duration = 0.0f;
    float elapsed = 0.0f;
    EasingFunction decay = Easing::easeOutQuad;
    bool active = false;
    Vec2 offset{0.0f, 0.0f};  // Current shake offset
};

/// Tween / Easing system.
///
/// Provides:
///   - Tween any numeric entity property (position, scale, rotation, alpha)
///   - Standard easing functions (linear, quad, cubic, elastic, bounce, back)
///   - Chainable tweens (on_complete callback can start another tween)
///   - Tween cancellation
///   - Camera shake helper
class TweenSystem {
public:
    TweenSystem() = default;

    /// Tween an entity property to a target value
    /// @param entity Target entity
    /// @param property Which property to tween
    /// @param targetValue Target value to tween toward
    /// @param duration Duration in seconds
    /// @param easing Easing function name
    /// @param onComplete Optional callback when tween finishes
    /// @return Tween ID for cancellation
    TweenId tweenTo(Entity entity, TweenProperty property, float targetValue,
                    float duration, EasingFunction easing = Easing::linear,
                    std::function<void()> onComplete = nullptr);

    /// Cancel a tween
    /// @return true if the tween existed and was cancelled
    bool cancel(TweenId id);

    /// Cancel all tweens for an entity
    /// @return number of tweens cancelled
    int cancelAllForEntity(Entity entity);

    /// Start a camera shake
    void shake(float intensity, float duration,
               EasingFunction decay = Easing::easeOutQuad);

    /// Get current camera shake offset (add to camera position)
    Vec2 getShakeOffset() const { return m_shake.active ? m_shake.offset : Vec2{0, 0}; }

    /// Is a camera shake currently active?
    bool isShaking() const { return m_shake.active; }

    /// Update all tweens. Call once per frame.
    /// @param dt Delta time in seconds
    /// @param registry ECS registry for property access
    void update(float dt, Registry& registry);

    /// Get number of active tweens
    size_t activeCount() const;

    /// Clear all tweens
    void clear();

private:
    /// Read the current value of a property from an entity
    float getPropertyValue(Entity entity, TweenProperty property, Registry& registry);

    /// Write a value to an entity property
    void setPropertyValue(Entity entity, TweenProperty property, float value,
                          Registry& registry);

    /// Random float in range [-1, 1]
    static float randomNormalized();

    std::vector<Tween> m_tweens;
    CameraShake m_shake;
    TweenId m_nextId = 1;
};

} // namespace gloaming
