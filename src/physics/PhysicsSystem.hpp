#pragma once

#include "physics/AABB.hpp"
#include "physics/Collision.hpp"
#include "physics/TileCollision.hpp"
#include "physics/Trigger.hpp"
#include "physics/Raycast.hpp"
#include "ecs/Systems.hpp"
#include "ecs/Components.hpp"
#include "ecs/Registry.hpp"
#include "world/TileMap.hpp"
#include <functional>
#include <vector>

namespace gloaming {

// Forward declaration
class Engine;

/// Physics world configuration
struct PhysicsConfig {
    Vec2 gravity{0.0f, 980.0f};       // Gravity in pixels/sec^2 (positive Y = down)
    float maxFallSpeed = 1000.0f;       // Terminal velocity
    float maxHorizontalSpeed = 500.0f;  // Max horizontal speed
    int collisionIterations = 4;        // Collision resolution iterations
    float skinWidth = 0.01f;            // Collision skin width
    float groundCheckDistance = 2.0f;   // Distance to check for ground
    bool enableSweepCollision = true;   // Use swept collision for fast objects
    float sweepThreshold = 100.0f;      // Speed above which to use swept collision
};

/// Collision event data
struct CollisionEvent {
    Entity entity = NullEntity;
    Vec2 normal;
    Vec2 point;
    bool withTile = false;
    bool withEntity = false;
    int tileX = 0;
    int tileY = 0;
    Entity otherEntity = NullEntity;
};

/// Callback type for collision events
using CollisionCallback = std::function<void(const CollisionEvent&)>;

/// Main physics system
/// Handles gravity, velocity, and collision detection/response
class PhysicsSystem : public System {
public:
    explicit PhysicsSystem(const PhysicsConfig& config = PhysicsConfig())
        : System("PhysicsSystem", 0), m_config(config) {}

    void init(Registry& registry, Engine& engine) override;
    void update(float dt) override;
    void shutdown() override {}

    /// Get/set configuration
    PhysicsConfig& getConfig() { return m_config; }
    const PhysicsConfig& getConfig() const { return m_config; }

    /// Set the tile map for tile collision
    void setTileMap(TileMap* tileMap);

    /// Get tile collision handler
    TileCollision& getTileCollision() { return m_tileCollision; }
    const TileCollision& getTileCollision() const { return m_tileCollision; }

    /// Get trigger system
    TriggerSystem& getTriggerSystem() { return m_triggerSystem; }
    const TriggerSystem& getTriggerSystem() const { return m_triggerSystem; }

    /// Register collision callback
    void onCollision(CollisionCallback callback) {
        m_collisionCallbacks.push_back(std::move(callback));
    }

    /// Clear collision callbacks
    void clearCollisionCallbacks() {
        m_collisionCallbacks.clear();
    }

    /// Raycast utilities
    RaycastHit raycast(Vec2 origin, Vec2 direction, float maxDistance,
                       uint32_t ignoreMask = 0, uint32_t ignoreEntity = 0);

    RaycastHit raycastTiles(Vec2 origin, Vec2 direction, float maxDistance);

    RaycastHit raycastEntities(Vec2 origin, Vec2 direction, float maxDistance,
                                uint32_t ignoreMask = 0, uint32_t ignoreEntity = 0);

    bool hasLineOfSight(Vec2 from, Vec2 to, uint32_t ignoreEntity = 0);

    /// Move an entity with collision handling
    /// Returns the movement result
    TileMoveResult moveEntity(Entity entity, Vec2 velocity, float dt);

    /// Check if an entity is on the ground
    bool isOnGround(Entity entity);

    /// Apply an impulse to an entity
    void applyImpulse(Entity entity, Vec2 impulse);

    /// Get physics statistics
    struct Stats {
        size_t entitiesProcessed = 0;
        size_t tileCollisions = 0;
        size_t entityCollisions = 0;
        size_t triggersProcessed = 0;
        float updateTimeMs = 0.0f;
    };
    const Stats& getStats() const { return m_stats; }

private:
    /// Apply gravity to all gravity-affected entities
    void applyGravity(float dt);

    /// Update entity positions based on velocity
    void updatePositions(float dt);

    /// Process a single entity's physics
    void processEntity(Entity entity, Transform& transform, Velocity& velocity,
                       Collider* collider, Gravity* gravity, float dt);

    /// Handle entity-to-entity collisions
    void handleEntityCollisions();

    /// Fire collision event
    void fireCollisionEvent(const CollisionEvent& event);

    PhysicsConfig m_config;
    TileMap* m_tileMap = nullptr;
    int m_tileSize = 16;
    TileCollision m_tileCollision;
    TriggerSystem m_triggerSystem;
    std::vector<CollisionCallback> m_collisionCallbacks;
    Stats m_stats;
};

// ============================================================================
// Implementation
// ============================================================================

inline void PhysicsSystem::init(Registry& registry, Engine& engine) {
    System::init(registry, engine);
    m_triggerSystem.init(registry);

    // Get tile size from config if available
    // Default is 16 pixels
    m_tileSize = 16;
    m_tileCollision.setTileSize(m_tileSize);
}

inline void PhysicsSystem::setTileMap(TileMap* tileMap) {
    m_tileMap = tileMap;
    m_tileCollision.setTileMap(tileMap);
    if (tileMap) {
        m_tileSize = tileMap->getTileSize();
        m_tileCollision.setTileSize(m_tileSize);
    }
}

inline void PhysicsSystem::update(float dt) {
    auto startTime = std::chrono::high_resolution_clock::now();

    m_stats = Stats{};

    // Apply gravity
    applyGravity(dt);

    // Update positions with collision
    updatePositions(dt);

    // Handle entity-entity collisions
    handleEntityCollisions();

    // Update trigger system
    m_triggerSystem.update();
    m_stats.triggersProcessed = m_triggerSystem.getTracker().getOverlapCount();

    auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.updateTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
}

inline void PhysicsSystem::applyGravity(float dt) {
    auto& registry = getRegistry();

    registry.each<Velocity, Gravity>([this, dt](auto entity, Velocity& velocity, Gravity& gravity) {
        if (!gravity.grounded) {
            velocity.linear.y += m_config.gravity.y * gravity.scale * dt;

            // Clamp to terminal velocity
            if (velocity.linear.y > m_config.maxFallSpeed) {
                velocity.linear.y = m_config.maxFallSpeed;
            }
        }
    });
}

inline void PhysicsSystem::updatePositions(float dt) {
    auto& registry = getRegistry();

    // Process entities with velocity
    auto view = registry.view<Transform, Velocity>();
    for (auto entity : view) {
        auto& transform = view.get<Transform>(entity);
        auto& velocity = view.get<Velocity>(entity);

        Collider* collider = registry.has<Collider>(entity)
            ? &registry.get<Collider>(entity)
            : nullptr;

        Gravity* gravity = registry.has<Gravity>(entity)
            ? &registry.get<Gravity>(entity)
            : nullptr;

        processEntity(entity, transform, velocity, collider, gravity, dt);
        ++m_stats.entitiesProcessed;
    }
}

inline void PhysicsSystem::processEntity(Entity entity, Transform& transform, Velocity& velocity,
                                          Collider* collider, Gravity* gravity, float dt) {
    // Clamp horizontal speed
    if (std::abs(velocity.linear.x) > m_config.maxHorizontalSpeed) {
        velocity.linear.x = velocity.linear.x > 0 ? m_config.maxHorizontalSpeed : -m_config.maxHorizontalSpeed;
    }

    Vec2 frameVelocity = velocity.linear * dt;

    // If no collider, just move
    if (!collider || !collider->enabled || collider->isTrigger) {
        transform.position = transform.position + frameVelocity;
        return;
    }

    // Create AABB for collision
    AABB aabb = Collision::getEntityAABB(transform, *collider);

    bool wasOnGround = gravity ? gravity->grounded : false;

    // Check if we should use swept collision
    bool useSweep = m_config.enableSweepCollision &&
                    frameVelocity.length() > m_config.sweepThreshold * dt;

    if (m_tileMap && !useSweep) {
        // Standard tile collision with resolution
        auto moveResult = m_tileCollision.moveAABB(aabb, frameVelocity, true, wasOnGround);

        // Update position
        Vec2 delta = moveResult.newPosition - aabb.center;
        transform.position = transform.position + delta;

        // Update velocity based on collisions
        if (moveResult.hitHorizontal) {
            velocity.linear.x = 0.0f;

            CollisionEvent event;
            event.entity = entity;
            event.withTile = true;
            event.normal = Vec2(frameVelocity.x > 0 ? -1.0f : 1.0f, 0.0f);
            if (!moveResult.collisions.empty()) {
                event.tileX = moveResult.collisions[0].tileX;
                event.tileY = moveResult.collisions[0].tileY;
            }
            fireCollisionEvent(event);
            ++m_stats.tileCollisions;
        }

        if (moveResult.hitVertical) {
            velocity.linear.y = 0.0f;

            CollisionEvent event;
            event.entity = entity;
            event.withTile = true;
            event.normal = Vec2(0.0f, frameVelocity.y > 0 ? -1.0f : 1.0f);
            if (!moveResult.collisions.empty()) {
                event.tileX = moveResult.collisions[0].tileX;
                event.tileY = moveResult.collisions[0].tileY;
            }
            fireCollisionEvent(event);
            ++m_stats.tileCollisions;
        }

        // Update grounded state
        if (gravity) {
            gravity->grounded = moveResult.onGround || moveResult.onSlope || moveResult.onPlatform;
        }
    } else if (m_tileMap && useSweep) {
        // Swept collision for fast objects
        auto sweep = m_tileCollision.sweepAABBTiles(aabb, frameVelocity);

        if (sweep.hit) {
            // Move to contact point
            Vec2 moveAmount = frameVelocity * sweep.time;
            transform.position = transform.position + moveAmount;

            // Calculate slide velocity
            Vec2 remaining = frameVelocity * (1.0f - sweep.time);
            Vec2 slide = calculateSlideVelocity(remaining, sweep.normal, 1.0f);

            // Try to slide
            AABB slideAABB = Collision::getEntityAABB(transform, *collider);
            auto slideResult = m_tileCollision.moveAABB(slideAABB, slide, true, wasOnGround);

            Vec2 slideDelta = slideResult.newPosition - slideAABB.center;
            transform.position = transform.position + slideDelta;

            // Update velocity
            if (std::abs(sweep.normal.x) > 0.5f) {
                velocity.linear.x = 0.0f;
            }
            if (std::abs(sweep.normal.y) > 0.5f) {
                velocity.linear.y = 0.0f;
            }

            // Update grounded
            if (gravity && sweep.normal.y < -0.5f) {
                gravity->grounded = true;
            }

            ++m_stats.tileCollisions;
        } else {
            // No collision, move freely
            transform.position = transform.position + frameVelocity;

            // Check if still grounded
            if (gravity) {
                AABB movedAABB = Collision::getEntityAABB(transform, *collider);
                gravity->grounded = m_tileCollision.checkGroundBelow(movedAABB, m_config.groundCheckDistance);
            }
        }
    } else {
        // No tile map, just move
        transform.position = transform.position + frameVelocity;

        if (gravity) {
            gravity->grounded = false;
        }
    }

    // Also update angular rotation
    if (velocity.angular != 0.0f) {
        transform.rotation += velocity.angular * dt;
        // Keep rotation in 0-360 range
        while (transform.rotation < 0.0f) transform.rotation += 360.0f;
        while (transform.rotation >= 360.0f) transform.rotation -= 360.0f;
    }
}

inline void PhysicsSystem::handleEntityCollisions() {
    auto& registry = getRegistry();

    // Find all entity-entity collisions
    Collision::findAllCollisions(registry, [this](const EntityCollision& collision) {
        if (collision.isTrigger) {
            // Triggers are handled by TriggerSystem
            return;
        }

        ++m_stats.entityCollisions;

        // Fire collision events for both entities
        CollisionEvent eventA;
        eventA.entity = collision.entityA;
        eventA.withEntity = true;
        eventA.normal = collision.normal;
        eventA.point = collision.point;
        eventA.otherEntity = collision.entityB;
        fireCollisionEvent(eventA);

        CollisionEvent eventB;
        eventB.entity = collision.entityB;
        eventB.withEntity = true;
        eventB.normal = collision.normal * -1.0f;
        eventB.point = collision.point;
        eventB.otherEntity = collision.entityA;
        fireCollisionEvent(eventB);

        // Separate overlapping entities and cancel colliding velocity.
        // Entities without a Velocity component are treated as static
        // (e.g. tile entities) and are never pushed.
        auto& registry = getRegistry();
        if (registry.has<Transform>(collision.entityA) && registry.has<Transform>(collision.entityB)) {
            auto& transformA = registry.get<Transform>(collision.entityA);
            auto& transformB = registry.get<Transform>(collision.entityB);

            bool dynamicA = registry.has<Velocity>(collision.entityA);
            bool dynamicB = registry.has<Velocity>(collision.entityB);

            // Determine push ratio: static entities receive none of the push
            float ratioA, ratioB;
            if (dynamicA && dynamicB) {
                ratioA = 0.5f;
                ratioB = 0.5f;
            } else if (dynamicA) {
                ratioA = 1.0f;
                ratioB = 0.0f;
            } else if (dynamicB) {
                ratioA = 0.0f;
                ratioB = 1.0f;
            } else {
                // Both static — no movement
                ratioA = 0.0f;
                ratioB = 0.0f;
            }

            Vec2 fullSeparation = collision.normal * collision.penetration;
            transformA.position = transformA.position + fullSeparation * ratioA;
            transformB.position = transformB.position - fullSeparation * ratioB;

            // Zero out velocity components along collision normal to prevent re-penetration
            if (dynamicA) {
                auto& velA = registry.get<Velocity>(collision.entityA);
                float dotA = Vec2::dot(velA.linear, collision.normal);
                if (dotA < 0.0f) { // Only cancel if moving into collision
                    velA.linear = velA.linear - collision.normal * dotA;
                }
                // Update grounded state when landing on a static entity
                if (!dynamicB && collision.normal.y < -0.5f && registry.has<Gravity>(collision.entityA)) {
                    registry.get<Gravity>(collision.entityA).grounded = true;
                }
            }
            if (dynamicB) {
                auto& velB = registry.get<Velocity>(collision.entityB);
                Vec2 normalB = collision.normal * -1.0f;
                float dotB = Vec2::dot(velB.linear, normalB);
                if (dotB < 0.0f) { // Only cancel if moving into collision
                    velB.linear = velB.linear - normalB * dotB;
                }
                // Update grounded state when landing on a static entity
                if (!dynamicA && normalB.y < -0.5f && registry.has<Gravity>(collision.entityB)) {
                    registry.get<Gravity>(collision.entityB).grounded = true;
                }
            }
        }
    });
}

inline void PhysicsSystem::fireCollisionEvent(const CollisionEvent& event) {
    for (auto& callback : m_collisionCallbacks) {
        callback(event);
    }
}

inline RaycastHit PhysicsSystem::raycast(Vec2 origin, Vec2 direction, float maxDistance,
                                          uint32_t ignoreMask, uint32_t ignoreEntity) {
    Ray ray(origin, direction);
    return Raycast::raycast(ray, maxDistance, m_tileMap, m_tileSize,
                            getRegistry(), ignoreMask, ignoreEntity);
}

inline RaycastHit PhysicsSystem::raycastTiles(Vec2 origin, Vec2 direction, float maxDistance) {
    Ray ray(origin, direction);
    return Raycast::raycastTiles(ray, maxDistance, m_tileMap, m_tileSize);
}

inline RaycastHit PhysicsSystem::raycastEntities(Vec2 origin, Vec2 direction, float maxDistance,
                                                   uint32_t ignoreMask, uint32_t ignoreEntity) {
    Ray ray(origin, direction);
    return Raycast::raycastEntities(ray, maxDistance, getRegistry(), ignoreMask, ignoreEntity);
}

inline bool PhysicsSystem::hasLineOfSight(Vec2 from, Vec2 to, uint32_t ignoreEntity) {
    return Raycast::hasLineOfSight(from, to, m_tileMap, m_tileSize, getRegistry(), ignoreEntity);
}

inline TileMoveResult PhysicsSystem::moveEntity(Entity entity, Vec2 velocity, float dt) {
    auto& registry = getRegistry();

    if (!registry.has<Transform>(entity) || !registry.has<Collider>(entity)) {
        return TileMoveResult{};
    }

    auto& transform = registry.get<Transform>(entity);
    auto& collider = registry.get<Collider>(entity);

    AABB aabb = Collision::getEntityAABB(transform, collider);
    bool wasOnGround = registry.has<Gravity>(entity) ? registry.get<Gravity>(entity).grounded : false;

    auto result = m_tileCollision.moveAABB(aabb, velocity * dt, true, wasOnGround);

    // Apply the movement
    Vec2 delta = result.newPosition - aabb.center;
    transform.position = transform.position + delta;

    // Update grounded state
    if (registry.has<Gravity>(entity)) {
        registry.get<Gravity>(entity).grounded = result.onGround;
    }

    return result;
}

inline bool PhysicsSystem::isOnGround(Entity entity) {
    auto& registry = getRegistry();
    if (registry.has<Gravity>(entity)) {
        return registry.get<Gravity>(entity).grounded;
    }
    return false;
}

inline void PhysicsSystem::applyImpulse(Entity entity, Vec2 impulse) {
    auto& registry = getRegistry();
    if (registry.has<Velocity>(entity)) {
        auto& velocity = registry.get<Velocity>(entity);
        velocity.linear = velocity.linear + impulse;
    }
}

} // namespace gloaming
