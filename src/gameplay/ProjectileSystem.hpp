#pragma once

#include "ecs/Systems.hpp"
#include "ecs/Components.hpp"
#include "ecs/Registry.hpp"
#include "physics/AABB.hpp"
#include "physics/Collision.hpp"

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <cmath>

namespace gloaming {

/// Callback invoked when a projectile hits a target entity or tile
struct ProjectileHitInfo {
    Entity projectile = NullEntity;
    Entity target = NullEntity;         // NullEntity if tile hit
    Vec2 position{0.0f, 0.0f};
    bool hitTile = false;
    int tileX = 0;
    int tileY = 0;
};

using ProjectileHitCallback = std::function<void(const ProjectileHitInfo&)>;

/// Registry for per-entity on-hit callbacks (supports Lua callbacks)
class ProjectileCallbackRegistry {
public:
    void registerOnHit(Entity entity, ProjectileHitCallback callback) {
        m_callbacks[entity] = std::move(callback);
    }

    void removeOnHit(Entity entity) {
        m_callbacks.erase(entity);
    }

    bool hasCallback(Entity entity) const {
        return m_callbacks.find(entity) != m_callbacks.end();
    }

    /// Fire the on-hit callback if registered. Returns true if a callback was found.
    bool fireOnHit(const ProjectileHitInfo& info) {
        auto it = m_callbacks.find(info.projectile);
        if (it != m_callbacks.end()) {
            it->second(info);
            return true;
        }
        return false;
    }

    /// Clean up callbacks for destroyed entities
    void cleanup(Registry& registry) {
        for (auto it = m_callbacks.begin(); it != m_callbacks.end(); ) {
            if (!registry.valid(it->first)) {
                it = m_callbacks.erase(it);
            } else {
                ++it;
            }
        }
    }

    void clear() { m_callbacks.clear(); }

private:
    std::unordered_map<Entity, ProjectileHitCallback> m_callbacks;
};

/// System that manages projectile lifecycle: aging, collision, damage, despawn.
///
/// Runs in the Update phase after PhysicsSystem. For each entity with a
/// Projectile component it:
///   1. Ages the projectile and checks lifetime
///   2. Checks max travel distance
///   3. Auto-rotates sprite to face velocity direction
///   4. Detects overlap with target entities (via hitMask)
///   5. Applies damage, fires on-hit callbacks, handles pierce
///   6. Destroys projectiles that are no longer alive
class ProjectileSystem : public System {
public:
    ProjectileSystem() : System("ProjectileSystem", 15) {}

    void init(Registry& registry, Engine& engine) override {
        System::init(registry, engine);
    }

    void shutdown() override {
        m_callbacks.clear();
    }

    ProjectileCallbackRegistry& getCallbacks() { return m_callbacks; }
    const ProjectileCallbackRegistry& getCallbacks() const { return m_callbacks; }

    void update(float dt) override;

private:
    void checkEntityHits(Entity projEntity, const Transform& projTransform,
                         Projectile& proj, std::unordered_set<Entity>& toDestroy);

    ProjectileCallbackRegistry m_callbacks;
    int m_cleanupCounter = 0;
};

} // namespace gloaming
