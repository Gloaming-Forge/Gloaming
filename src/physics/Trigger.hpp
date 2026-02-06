#pragma once

#include "physics/AABB.hpp"
#include "physics/Collision.hpp"
#include "ecs/Components.hpp"
#include "ecs/Registry.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gloaming {

/// Tracks which entities are overlapping triggers
/// Used to determine enter/stay/exit events
class TriggerTracker {
public:
    /// Pair of entities for tracking overlaps
    struct EntityPair {
        Entity triggerEntity;
        Entity otherEntity;

        bool operator==(const EntityPair& other) const {
            return triggerEntity == other.triggerEntity &&
                   otherEntity == other.otherEntity;
        }
    };

    /// Hash function for EntityPair
    struct EntityPairHash {
        std::size_t operator()(const EntityPair& pair) const {
            std::size_t h1 = std::hash<uint32_t>{}(static_cast<uint32_t>(pair.triggerEntity));
            std::size_t h2 = std::hash<uint32_t>{}(static_cast<uint32_t>(pair.otherEntity));
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };

    /// Update trigger tracking and fire callbacks
    /// Should be called each frame after collision detection
    void update(Registry& registry) {
        // Find all current trigger overlaps
        std::unordered_set<EntityPair, EntityPairHash> currentOverlaps;
        findTriggerOverlaps(registry, currentOverlaps);

        // Process overlaps
        for (const auto& pair : currentOverlaps) {
            auto it = m_previousOverlaps.find(pair);
            if (it == m_previousOverlaps.end()) {
                // New overlap - fire onEnter
                fireEnterCallback(registry, pair.triggerEntity, pair.otherEntity);
            } else {
                // Continuing overlap - fire onStay
                fireStayCallback(registry, pair.triggerEntity, pair.otherEntity);
            }
        }

        // Check for exited overlaps
        for (const auto& pair : m_previousOverlaps) {
            if (currentOverlaps.find(pair) == currentOverlaps.end()) {
                // No longer overlapping - fire onExit
                fireExitCallback(registry, pair.triggerEntity, pair.otherEntity);
            }
        }

        // Store current as previous for next frame
        m_previousOverlaps = std::move(currentOverlaps);
    }

    /// Clear all tracked overlaps (e.g., when loading a new level)
    void clear() {
        m_previousOverlaps.clear();
    }

    /// Remove tracking for a specific entity (e.g., when entity is destroyed)
    void removeEntity(Entity entity) {
        std::vector<EntityPair> toRemove;
        for (const auto& pair : m_previousOverlaps) {
            if (pair.triggerEntity == entity || pair.otherEntity == entity) {
                toRemove.push_back(pair);
            }
        }
        for (const auto& pair : toRemove) {
            m_previousOverlaps.erase(pair);
        }
    }

    /// Get all entities currently inside a trigger
    std::vector<Entity> getEntitiesInTrigger(Entity triggerEntity) const {
        std::vector<Entity> entities;
        for (const auto& pair : m_previousOverlaps) {
            if (pair.triggerEntity == triggerEntity) {
                entities.push_back(pair.otherEntity);
            }
        }
        return entities;
    }

    /// Check if an entity is inside a trigger
    bool isEntityInTrigger(Entity triggerEntity, Entity otherEntity) const {
        return m_previousOverlaps.count({triggerEntity, otherEntity}) > 0;
    }

    /// Get count of tracked overlaps
    size_t getOverlapCount() const { return m_previousOverlaps.size(); }

private:
    void findTriggerOverlaps(Registry& registry,
                             std::unordered_set<EntityPair, EntityPairHash>& overlaps) {
        auto view = registry.view<Transform, Collider>();

        // Gather all entities with colliders
        std::vector<std::tuple<Entity, Transform*, Collider*, bool>> entities;
        for (auto entity : view) {
            auto& transform = view.get<Transform>(entity);
            auto& collider = view.get<Collider>(entity);
            if (collider.enabled) {
                bool hasTrigger = registry.has<Trigger>(entity);
                entities.emplace_back(
                    entity,
                    &transform,
                    &collider,
                    hasTrigger || collider.isTrigger
                );
            }
        }

        // Check trigger entities against all other entities.
        // Note: The inner loop starts at j=0 (not j=i+1) intentionally because:
        // 1. We only process pairs where entityA is a trigger (outer loop filter)
        // 2. Each trigger needs to detect ALL entities inside it, not just "later" ones
        // 3. If both A and B are triggers, (A,B) and (B,A) are separate tracked pairs
        //    since each trigger independently tracks what enters/exits it
        for (size_t i = 0; i < entities.size(); ++i) {
            auto& [entityA, transformA, colliderA, isTriggerA] = entities[i];

            // Only process if this entity is a trigger
            if (!isTriggerA) continue;

            for (size_t j = 0; j < entities.size(); ++j) {
                if (i == j) continue;

                auto& [entityB, transformB, colliderB, isTriggerB] = entities[j];

                if (!colliderA->canCollideWith(*colliderB)) continue;

                AABB aabbA = Collision::getEntityAABB(*transformA, *colliderA);
                AABB aabbB = Collision::getEntityAABB(*transformB, *colliderB);

                if (aabbA.intersects(aabbB)) {
                    overlaps.insert({entityA, entityB});
                }
            }
        }
    }

    void fireEnterCallback(Registry& registry, Entity triggerEntity, Entity otherEntity) {
        if (registry.has<Trigger>(triggerEntity)) {
            auto& trigger = registry.get<Trigger>(triggerEntity);
            if (trigger.onEnter) {
                trigger.onEnter(triggerEntity, otherEntity);
            }
        }
    }

    void fireStayCallback(Registry& registry, Entity triggerEntity, Entity otherEntity) {
        if (registry.has<Trigger>(triggerEntity)) {
            auto& trigger = registry.get<Trigger>(triggerEntity);
            if (trigger.onStay) {
                trigger.onStay(triggerEntity, otherEntity);
            }
        }
    }

    void fireExitCallback(Registry& registry, Entity triggerEntity, Entity otherEntity) {
        if (registry.has<Trigger>(triggerEntity)) {
            auto& trigger = registry.get<Trigger>(triggerEntity);
            if (trigger.onExit) {
                trigger.onExit(triggerEntity, otherEntity);
            }
        }
    }

    std::unordered_set<EntityPair, EntityPairHash> m_previousOverlaps;
};

/// System that updates trigger tracking each frame
/// This should be added to the Update phase after physics
class TriggerSystem {
public:
    TriggerSystem() = default;

    /// Initialize with registry reference
    void init(Registry& registry) {
        m_registry = &registry;
    }

    /// Update trigger tracking
    void update() {
        if (m_registry) {
            m_tracker.update(*m_registry);
        }
    }

    /// Get the trigger tracker
    TriggerTracker& getTracker() { return m_tracker; }
    const TriggerTracker& getTracker() const { return m_tracker; }

    /// Clear all tracked overlaps
    void clear() { m_tracker.clear(); }

    /// Notify that an entity was destroyed
    void onEntityDestroyed(Entity entity) {
        m_tracker.removeEntity(entity);
    }

private:
    Registry* m_registry = nullptr;
    TriggerTracker m_tracker;
};

} // namespace gloaming
