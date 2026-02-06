#pragma once

#include "physics/AABB.hpp"
#include "ecs/Components.hpp"
#include "ecs/Registry.hpp"
#include <vector>
#include <functional>

namespace gloaming {

/// Information about a collision between two entities
struct EntityCollision {
    Entity entityA = NullEntity;
    Entity entityB = NullEntity;
    Vec2 normal{0.0f, 0.0f};
    float penetration = 0.0f;
    Vec2 point{0.0f, 0.0f};
    bool isTrigger = false;         // True if either entity is a trigger
};

/// Configuration for collision detection
struct CollisionConfig {
    float skinWidth = 0.01f;        // Small buffer to prevent floating point issues
    int maxIterations = 4;          // Max collision resolution iterations
    float slopeTolerance = 0.01f;   // Tolerance for slope detection
};

/// Collision detection utilities for entities
class Collision {
public:
    /// Get AABB from an entity's transform and collider
    static AABB getEntityAABB(const Transform& transform, const Collider& collider) {
        Rect bounds = collider.getBounds(transform);
        return AABB::fromRect(bounds);
    }

    /// Test collision between two entities
    static CollisionResult testEntityCollision(
        const Transform& transformA, const Collider& colliderA,
        const Transform& transformB, const Collider& colliderB
    ) {
        if (!colliderA.canCollideWith(colliderB)) {
            return CollisionResult{};
        }

        AABB a = getEntityAABB(transformA, colliderA);
        AABB b = getEntityAABB(transformB, colliderB);

        return testAABBCollision(a, b);
    }

    /// Swept collision test between a moving entity and a static entity
    static SweepResult sweepEntityCollision(
        const Transform& transformA, const Collider& colliderA, Vec2 velocity,
        const Transform& transformB, const Collider& colliderB
    ) {
        if (!colliderA.canCollideWith(colliderB)) {
            return SweepResult{};
        }

        AABB a = getEntityAABB(transformA, colliderA);
        AABB b = getEntityAABB(transformB, colliderB);

        return sweepAABB(a, velocity, b);
    }

    /// Find all entity collisions in the registry
    /// @param registry The ECS registry to query
    /// @param callback Called for each collision found
    static void findAllCollisions(
        Registry& registry,
        const std::function<void(const EntityCollision&)>& callback
    ) {
        auto view = registry.view<Transform, Collider>();

        // Simple O(n²) broad phase - suitable for up to ~200-300 entities per frame.
        // For larger entity counts, implement spatial partitioning (grid, quadtree).
        // Performance: ~45,000 pair checks at 300 entities, ~10ms on typical hardware.
        std::vector<std::tuple<Entity, Transform*, Collider*>> entities;

        for (auto entity : view) {
            auto& transform = view.get<Transform>(entity);
            auto& collider = view.get<Collider>(entity);
            if (collider.enabled) {
                entities.emplace_back(
                    entity,
                    &transform,
                    &collider
                );
            }
        }

        for (size_t i = 0; i < entities.size(); ++i) {
            for (size_t j = i + 1; j < entities.size(); ++j) {
                auto& [entityA, transformA, colliderA] = entities[i];
                auto& [entityB, transformB, colliderB] = entities[j];

                if (!colliderA->canCollideWith(*colliderB)) {
                    continue;
                }

                AABB a = getEntityAABB(*transformA, *colliderA);
                AABB b = getEntityAABB(*transformB, *colliderB);

                auto result = testAABBCollision(a, b);
                if (result.collided) {
                    EntityCollision collision;
                    collision.entityA = entityA;
                    collision.entityB = entityB;
                    collision.normal = result.normal;
                    collision.penetration = result.penetration;
                    collision.point = result.point;
                    collision.isTrigger = colliderA->isTrigger || colliderB->isTrigger;

                    callback(collision);
                }
            }
        }
    }

    /// Find collisions for a specific entity
    static std::vector<EntityCollision> findCollisionsFor(
        Registry& registry,
        Entity entity,
        const Transform& transform,
        const Collider& collider
    ) {
        std::vector<EntityCollision> collisions;

        if (!collider.enabled) {
            return collisions;
        }

        AABB entityAABB = getEntityAABB(transform, collider);

        auto view = registry.view<Transform, Collider>();
        for (auto other : view) {
            if (other == entity) {
                continue;
            }

            auto& otherTransform = view.get<Transform>(other);
            auto& otherCollider = view.get<Collider>(other);

            if (!otherCollider.enabled || !collider.canCollideWith(otherCollider)) {
                continue;
            }

            AABB otherAABB = getEntityAABB(otherTransform, otherCollider);
            auto result = testAABBCollision(entityAABB, otherAABB);

            if (result.collided) {
                EntityCollision collision;
                collision.entityA = entity;
                collision.entityB = other;
                collision.normal = result.normal;
                collision.penetration = result.penetration;
                collision.point = result.point;
                collision.isTrigger = collider.isTrigger || otherCollider.isTrigger;
                collisions.push_back(collision);
            }
        }

        return collisions;
    }

    /// Perform swept collision against all entities
    /// Returns the earliest collision if any
    static SweepResult sweepAgainstEntities(
        Registry& registry,
        Entity entity,
        const Transform& transform,
        const Collider& collider,
        Vec2 velocity,
        Entity* hitEntity = nullptr
    ) {
        SweepResult earliest;
        earliest.time = 1.0f;

        if (!collider.enabled || (velocity.x == 0.0f && velocity.y == 0.0f)) {
            return earliest;
        }

        AABB entityAABB = getEntityAABB(transform, collider);

        auto view = registry.view<Transform, Collider>();
        for (auto other : view) {
            if (other == entity) {
                continue;
            }

            auto& otherTransform = view.get<Transform>(other);
            auto& otherCollider = view.get<Collider>(other);

            if (!otherCollider.enabled || !collider.canCollideWith(otherCollider)) {
                continue;
            }

            // Skip triggers for physical collision
            if (otherCollider.isTrigger) {
                continue;
            }

            AABB otherAABB = getEntityAABB(otherTransform, otherCollider);
            auto result = sweepAABB(entityAABB, velocity, otherAABB);

            if (result.hit && result.time < earliest.time) {
                earliest = result;
                if (hitEntity) {
                    *hitEntity = other;
                }
            }
        }

        return earliest;
    }
};

/// Helper to resolve penetration between two entities
inline Vec2 resolvePenetration(const EntityCollision& collision, float ratio = 0.5f) {
    return collision.normal * (collision.penetration * ratio);
}

} // namespace gloaming
