#pragma once

#include "ecs/Systems.hpp"
#include "ecs/Components.hpp"
#include "rendering/SpriteBatch.hpp"
#include "rendering/Camera.hpp"

namespace gloaming {

/// System that updates entity positions based on velocity
class MovementSystem : public System {
public:
    MovementSystem() : System("MovementSystem", 0) {}

    void update(float dt) override {
        getRegistry().each<Transform, Velocity>([dt](Entity /*entity*/, Transform& transform, Velocity& velocity) {
            transform.position += velocity.linear * dt;
            transform.rotation += velocity.angular * dt;

            // Normalize rotation to 0-360
            while (transform.rotation >= 360.0f) transform.rotation -= 360.0f;
            while (transform.rotation < 0.0f) transform.rotation += 360.0f;
        });
    }
};

/// System that destroys entities that have exceeded their lifetime
class LifetimeSystem : public System {
public:
    LifetimeSystem() : System("LifetimeSystem", 100) {}

    void update(float dt) override {
        std::vector<Entity> toDestroy;

        getRegistry().each<Lifetime>([dt, &toDestroy](Entity entity, Lifetime& lifetime) {
            lifetime.elapsed += dt;
            if (lifetime.isExpired()) {
                toDestroy.push_back(entity);
            }
        });

        for (Entity entity : toDestroy) {
            getRegistry().destroy(entity);
        }
    }
};

/// System that updates health invincibility timers
class HealthSystem : public System {
public:
    HealthSystem() : System("HealthSystem", 20) {}

    void update(float dt) override {
        getRegistry().each<Health>([dt](Entity /*entity*/, Health& health) {
            health.update(dt);
        });
    }
};

/// System that updates light source flicker
class LightUpdateSystem : public System {
public:
    LightUpdateSystem() : System("LightUpdateSystem", 30) {}

    void update(float dt) override {
        getRegistry().each<LightSource>([dt](Entity /*entity*/, LightSource& light) {
            light.update(dt);
        });
    }
};

/// System that renders sprites using SpriteBatch
class SpriteRenderSystem : public System {
public:
    SpriteRenderSystem() : System("SpriteRenderSystem", 0) {}

    void init(Registry& registry, Engine& engine) override {
        System::init(registry, engine);
        m_spriteBatch = &engine.getSpriteBatch();
        m_camera = &engine.getCamera();
    }

    void update(float /*dt*/) override {
        if (!m_spriteBatch || !m_camera) return;

        m_spriteBatch->begin();

        getRegistry().each<Transform, Sprite>([this](Entity /*entity*/, const Transform& transform, const Sprite& sprite) {
            if (!sprite.visible || !sprite.texture) return;

            // Check if in view (basic culling)
            Rect spriteBounds(
                transform.position.x - sprite.sourceRect.width * sprite.origin.x * transform.scale.x,
                transform.position.y - sprite.sourceRect.height * sprite.origin.y * transform.scale.y,
                sprite.sourceRect.width * transform.scale.x,
                sprite.sourceRect.height * transform.scale.y
            );

            if (!m_camera->isVisible(spriteBounds)) return;

            // Create sprite data for batch
            SpriteData batchSprite;
            batchSprite.texture = sprite.texture;
            batchSprite.sourceRect = sprite.sourceRect;
            batchSprite.position = transform.position;
            batchSprite.origin = sprite.origin;
            batchSprite.scale = transform.scale;

            // Handle flip
            if (sprite.flipX) batchSprite.scale.x = -batchSprite.scale.x;
            if (sprite.flipY) batchSprite.scale.y = -batchSprite.scale.y;

            batchSprite.rotation = transform.rotation;
            batchSprite.tint = sprite.tint;
            batchSprite.layer = sprite.layer;

            m_spriteBatch->draw(batchSprite);
        });

        m_spriteBatch->end();
    }

private:
    SpriteBatch* m_spriteBatch = nullptr;
    Camera* m_camera = nullptr;
};

/// System that renders debug information for colliders
class ColliderDebugRenderSystem : public System {
public:
    ColliderDebugRenderSystem() : System("ColliderDebugRenderSystem", 1000) {}

    void init(Registry& registry, Engine& engine) override {
        System::init(registry, engine);
        m_renderer = engine.getRenderer();
        m_camera = &engine.getCamera();
    }

    void setEnabled(bool enabled) { m_debugEnabled = enabled; System::setEnabled(enabled); }
    void setDrawEnabled(bool enabled) { m_debugEnabled = enabled; }
    bool isDrawEnabled() const { return m_debugEnabled; }

    void update(float /*dt*/) override {
        if (!m_debugEnabled || !m_renderer || !m_camera) return;

        getRegistry().each<Transform, Collider>([this](Entity /*entity*/, const Transform& transform, const Collider& collider) {
            if (!collider.enabled) return;

            Rect bounds = collider.getBounds(transform);

            // Transform to screen space
            Vec2 screenPos = m_camera->worldToScreen(Vec2(bounds.x, bounds.y));
            float zoom = m_camera->getZoom();

            Rect screenBounds(
                screenPos.x,
                screenPos.y,
                bounds.width * zoom,
                bounds.height * zoom
            );

            // Don't draw if off-screen
            if (!m_camera->isVisible(bounds)) return;

            Color color = collider.isTrigger ? Color(0, 255, 0, 100) : Color(255, 0, 0, 100);
            m_renderer->drawRectangleOutline(screenBounds, color, 1.0f);
        });
    }

private:
    IRenderer* m_renderer = nullptr;
    Camera* m_camera = nullptr;
    bool m_debugEnabled = false;
};

/// Helper function to register all core systems with the scheduler
inline void registerCoreSystems(SystemScheduler& scheduler) {
    // Update phase systems
    scheduler.addSystem<MovementSystem>(SystemPhase::Update);
    scheduler.addSystem<HealthSystem>(SystemPhase::Update);
    scheduler.addSystem<LightUpdateSystem>(SystemPhase::Update);
    scheduler.addSystem<LifetimeSystem>(SystemPhase::PostUpdate);

    // Render phase systems
    scheduler.addSystem<SpriteRenderSystem>(SystemPhase::Render);
    scheduler.addSystem<ColliderDebugRenderSystem>(SystemPhase::PostRender);
}

} // namespace gloaming
