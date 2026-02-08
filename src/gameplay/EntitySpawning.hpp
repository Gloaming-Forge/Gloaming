#pragma once

#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "ecs/EntityFactory.hpp"

#include <vector>
#include <string>
#include <cmath>

namespace gloaming {

/// Query filter for spatial entity searches
struct EntityQueryFilter {
    std::string type;                    // Filter by Name::type (empty = any)
    uint32_t requiredLayer = 0;          // Filter by collision layer (0 = any)
    bool excludeDead = true;             // Exclude entities with Health <= 0
};

/// Result of a spatial entity query
struct EntityQueryResult {
    Entity entity = NullEntity;
    float distance = 0.0f;
    Vec2 position{0.0f, 0.0f};
};

/// Utility class for entity spatial queries and helper operations.
/// Wraps the Registry and EntityFactory to provide a convenient API
/// for Lua bindings.
class EntitySpawning {
public:
    EntitySpawning() = default;

    void setRegistry(Registry* registry) { m_registry = registry; }
    void setEntityFactory(EntityFactory* factory) { m_factory = factory; }

    /// Create a blank entity with just a Transform and Name
    Entity create(float x, float y) {
        if (!m_registry) return NullEntity;
        return m_registry->create(
            Transform{Vec2(x, y)},
            Name{"entity"}
        );
    }

    /// Spawn a registered entity type at a position
    Entity spawn(const std::string& type, float x, float y) {
        if (!m_registry || !m_factory) return NullEntity;
        return m_factory->spawn(*m_registry, type, Vec2(x, y));
    }

    /// Destroy an entity
    void destroy(Entity entity) {
        if (!m_registry || !m_registry->valid(entity)) return;
        m_registry->destroy(entity);
    }

    /// Check if an entity is valid (exists and hasn't been destroyed)
    bool isValid(Entity entity) const {
        return m_registry && m_registry->valid(entity);
    }

    /// Get total alive entity count
    size_t entityCount() const {
        return m_registry ? m_registry->alive() : 0;
    }

    /// Set entity position
    void setPosition(Entity entity, float x, float y) {
        if (!m_registry || !m_registry->valid(entity)) return;
        if (!m_registry->has<Transform>(entity)) {
            m_registry->add<Transform>(entity, Vec2(x, y));
        } else {
            m_registry->get<Transform>(entity).position = Vec2(x, y);
        }
    }

    /// Get entity position (returns {0,0} if invalid)
    Vec2 getPosition(Entity entity) const {
        if (!m_registry || !m_registry->valid(entity)) return Vec2();
        if (!m_registry->has<Transform>(entity)) return Vec2();
        return m_registry->get<Transform>(entity).position;
    }

    /// Set entity velocity
    void setVelocity(Entity entity, float vx, float vy) {
        if (!m_registry || !m_registry->valid(entity)) return;
        if (!m_registry->has<Velocity>(entity)) {
            m_registry->add<Velocity>(entity, Vec2(vx, vy));
        } else {
            m_registry->get<Velocity>(entity).linear = Vec2(vx, vy);
        }
    }

    /// Get entity velocity (returns {0,0} if invalid)
    Vec2 getVelocity(Entity entity) const {
        if (!m_registry || !m_registry->valid(entity)) return Vec2();
        if (!m_registry->has<Velocity>(entity)) return Vec2();
        return m_registry->get<Velocity>(entity).linear;
    }

    /// Find all entities within a radius of a point (O(N) brute force scan)
    std::vector<EntityQueryResult> findInRadius(
            float cx, float cy, float radius,
            const EntityQueryFilter& filter = EntityQueryFilter{}) const;

    /// Find the nearest entity to a point matching a filter
    EntityQueryResult findNearest(
            float cx, float cy, float maxRadius,
            const EntityQueryFilter& filter = EntityQueryFilter{}) const;

    /// Count entities matching a type filter (O(N) scan)
    size_t countByType(const std::string& type) const;

private:
    Registry* m_registry = nullptr;
    EntityFactory* m_factory = nullptr;
};

} // namespace gloaming
