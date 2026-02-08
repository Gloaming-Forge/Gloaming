#pragma once

#include "ecs/Systems.hpp"
#include "ecs/Components.hpp"
#include "ecs/Registry.hpp"
#include "physics/AABB.hpp"
#include "physics/Collision.hpp"

#include <functional>
#include <unordered_map>
#include <vector>
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
    /// Register an on-hit callback for a projectile entity
    void registerOnHit(Entity entity, ProjectileHitCallback callback) {
        m_callbacks[entity] = std::move(callback);
    }

    /// Remove the on-hit callback for an entity
    void removeOnHit(Entity entity) {
        m_callbacks.erase(entity);
    }

    /// Check if an entity has a callback
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

    /// Clear all callbacks
    void clear() {
        m_callbacks.clear();
    }

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

    /// Get the callback registry for registering on-hit callbacks
    ProjectileCallbackRegistry& getCallbacks() { return m_callbacks; }
    const ProjectileCallbackRegistry& getCallbacks() const { return m_callbacks; }

    void update(float dt) override {
        auto& registry = getRegistry();

        std::vector<Entity> toDestroy;

        // Process all projectile entities
        auto view = registry.view<Transform, Projectile>();
        for (auto entity : view) {
            auto& transform = view.get<Transform>(entity);
            auto& proj = view.get<Projectile>(entity);

            if (!proj.alive) {
                toDestroy.push_back(entity);
                continue;
            }

            // Age the projectile
            proj.age += dt;
            if (proj.age >= proj.lifetime) {
                proj.alive = false;
                toDestroy.push_back(entity);
                continue;
            }

            // Check max distance
            if (proj.maxDistance > 0.0f) {
                float dx = transform.position.x - proj.startPosition.x;
                float dy = transform.position.y - proj.startPosition.y;
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist >= proj.maxDistance) {
                    proj.alive = false;
                    toDestroy.push_back(entity);
                    continue;
                }
            }

            // Check if projectile hit a tile (set by collision callback)
            if (proj.hitTile) {
                ProjectileHitInfo info;
                info.projectile = entity;
                info.position = transform.position;
                info.hitTile = true;
                m_callbacks.fireOnHit(info);

                proj.alive = false;
                toDestroy.push_back(entity);
                continue;
            }

            // Auto-rotate to face velocity direction
            if (proj.autoRotate && registry.has<Velocity>(entity)) {
                auto& vel = registry.get<Velocity>(entity);
                if (vel.linear.x != 0.0f || vel.linear.y != 0.0f) {
                    float angle = std::atan2(vel.linear.y, vel.linear.x) * (180.0f / PI);
                    transform.rotation = angle;
                }
            }

            // Check entity overlaps for damage
            if (proj.hitMask != 0 && registry.has<Collider>(entity)) {
                checkEntityHits(entity, transform, proj, toDestroy);
            }
        }

        // Destroy dead projectiles
        for (Entity entity : toDestroy) {
            m_callbacks.removeOnHit(entity);
            if (registry.valid(entity)) {
                registry.destroy(entity);
            }
        }

        // Periodic callback cleanup (every ~60 frames)
        m_cleanupCounter++;
        if (m_cleanupCounter >= 60) {
            m_callbacks.cleanup(registry);
            m_cleanupCounter = 0;
        }
    }

private:
    void checkEntityHits(Entity projEntity, const Transform& projTransform,
                         Projectile& proj, std::vector<Entity>& toDestroy) {
        auto& registry = getRegistry();

        if (!registry.has<Collider>(projEntity)) return;
        auto& projCollider = registry.get<Collider>(projEntity);

        AABB projAABB = Collision::getEntityAABB(projTransform, projCollider);

        // Check against all entities with colliders
        auto targetView = registry.view<Transform, Collider>();
        for (auto targetEntity : targetView) {
            if (targetEntity == projEntity) continue;

            // Skip self and owner
            if (static_cast<uint32_t>(targetEntity) == proj.ownerEntity) continue;

            auto& targetCollider = targetView.get<Collider>(targetEntity);

            // Check if the target is on a layer the projectile can hit
            if ((targetCollider.layer & proj.hitMask) == 0) continue;
            if (!targetCollider.enabled) continue;

            // Skip already-hit entities (for piercing projectiles)
            uint32_t targetId = static_cast<uint32_t>(targetEntity);
            bool alreadyHit = false;
            for (uint32_t id : proj.alreadyHit) {
                if (id == targetId) {
                    alreadyHit = true;
                    break;
                }
            }
            if (alreadyHit) continue;

            // AABB overlap test
            auto& targetTransform = targetView.get<Transform>(targetEntity);
            AABB targetAABB = Collision::getEntityAABB(targetTransform, targetCollider);

            auto result = testAABBCollision(projAABB, targetAABB);
            if (!result.collided) continue;

            // --- HIT ---
            // Apply damage
            if (registry.has<Health>(targetEntity)) {
                registry.get<Health>(targetEntity).takeDamage(proj.damage);
            }

            // Fire on-hit callback
            ProjectileHitInfo info;
            info.projectile = projEntity;
            info.target = targetEntity;
            info.position = projTransform.position;
            info.hitTile = false;
            m_callbacks.fireOnHit(info);

            // Track hit for pierce
            proj.alreadyHit.push_back(targetId);

            // Handle pierce
            if (proj.pierce == 0) {
                // Destroy on first hit
                proj.alive = false;
                toDestroy.push_back(projEntity);
                return; // Stop checking further targets
            } else if (proj.pierce > 0) {
                proj.pierce--;
                if (proj.pierce < 0) proj.pierce = 0;
                // If pierce reached 0 after decrement, destroy next hit
            }
            // pierce == -1 means infinite pierce, keep going
        }
    }

    ProjectileCallbackRegistry m_callbacks;
    int m_cleanupCounter = 0;
};

} // namespace gloaming
