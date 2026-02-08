#include "gameplay/ProjectileSystem.hpp"

namespace gloaming {

void ProjectileSystem::update(float dt) {
    auto& registry = getRegistry();

    std::unordered_set<Entity> toDestroy;

    // Process all projectile entities
    auto view = registry.view<Transform, Projectile>();
    for (auto entity : view) {
        auto& transform = view.get<Transform>(entity);
        auto& proj = view.get<Projectile>(entity);

        if (!proj.alive) {
            toDestroy.insert(entity);
            continue;
        }

        // Age the projectile
        proj.age += dt;
        if (proj.age >= proj.lifetime) {
            proj.alive = false;
            toDestroy.insert(entity);
            continue;
        }

        // Check max distance
        if (proj.maxDistance > 0.0f) {
            float dx = transform.position.x - proj.startPosition.x;
            float dy = transform.position.y - proj.startPosition.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist >= proj.maxDistance) {
                proj.alive = false;
                toDestroy.insert(entity);
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
            toDestroy.insert(entity);
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

void ProjectileSystem::checkEntityHits(Entity projEntity, const Transform& projTransform,
                                       Projectile& proj, std::unordered_set<Entity>& toDestroy) {
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
        if (proj.wasHit(targetId)) continue;

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
        proj.addHit(targetId);

        // Handle pierce
        if (proj.pierce == 0) {
            // Destroy on first hit
            proj.alive = false;
            toDestroy.insert(projEntity);
            return; // Stop checking further targets
        } else if (proj.pierce > 0) {
            proj.pierce--;
        }
        // pierce == -1 means infinite pierce, keep going
    }
}

} // namespace gloaming
