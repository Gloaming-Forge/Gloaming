#pragma once

#include "ecs/Systems.hpp"
#include "ecs/Components.hpp"
#include "gameplay/EnemyAI.hpp"
#include "gameplay/GameMode.hpp"

#include <functional>
#include <unordered_map>
#include <string>

namespace gloaming {

// Forward declarations
class TileMap;
class Pathfinder;
class EventBus;

/// Custom AI behavior callback registered by mods.
/// Receives (entity, ai_component, dt) each frame.
using CustomAIBehavior = std::function<void(Entity, EnemyAI&, float)>;

/// System that executes AI behaviors for all entities with EnemyAI components.
///
/// Built-in behaviors handle common patterns for all target game styles.
/// Mods can register custom behaviors via enemy_ai.register_behavior() in Lua,
/// or use "custom" + FSM for fully scripted AI.
///
/// Processing order per entity per frame:
///   1. Despawn check (too far from player for too long)
///   2. Contact damage check (player overlapping enemy)
///   3. Target acquisition (periodic scan for nearest player)
///   4. Behavior-specific logic (movement, attack decisions)
///   5. Health-based transitions (flee when low HP)
class EnemyAISystem : public System {
public:
    EnemyAISystem() : System("EnemyAISystem", 12) {}

    void init(Registry& registry, Engine& engine) override;
    void update(float dt) override;
    void shutdown() override;

    /// Register a custom AI behavior that can be referenced by name
    void registerBehavior(const std::string& name, CustomAIBehavior behavior);

    /// Check if a custom behavior is registered
    bool hasBehavior(const std::string& name) const;

private:
    /// Find the nearest player to an entity
    Entity findNearestPlayer(const Vec2& position, float maxRange);

    /// Apply contact damage when player overlaps enemy
    void checkContactDamage(Entity enemy, const Transform& enemyTransform,
                            const EnemyAI& ai);

    /// Handle despawn logic (distance from player)
    bool checkDespawn(Entity enemy, const Transform& transform, EnemyAI& ai, float dt);

    // --- Built-in behavior implementations ---

    /// Idle: do nothing (for scripted or stationary enemies)
    void behaviorIdle(Entity entity, EnemyAI& ai, float dt);

    /// PatrolWalk: walk back and forth horizontally, reversing at walls/ledges
    void behaviorPatrolWalk(Entity entity, EnemyAI& ai, float dt);

    /// PatrolFly: fly in a sine-wave pattern around home position
    void behaviorPatrolFly(Entity entity, EnemyAI& ai, float dt);

    /// PatrolPath: follow A* path along patrol route (top-down games)
    void behaviorPatrolPath(Entity entity, EnemyAI& ai, float dt);

    /// Chase: move toward target entity
    void behaviorChase(Entity entity, EnemyAI& ai, float dt);

    /// Flee: move away from target entity
    void behaviorFlee(Entity entity, EnemyAI& ai, float dt);

    /// Guard: stay near home, chase if target enters detection range
    void behaviorGuard(Entity entity, EnemyAI& ai, float dt);

    /// Orbit: circle around target at fixed distance (flight games)
    void behaviorOrbit(Entity entity, EnemyAI& ai, float dt);

    /// StrafeRun: fly toward target, then retreat (flight attack runs)
    void behaviorStrafeRun(Entity entity, EnemyAI& ai, float dt);

    // Subsystem references
    TileMap* m_tileMap = nullptr;
    Pathfinder* m_pathfinder = nullptr;
    EventBus* m_eventBus = nullptr;
    ViewMode m_viewMode = ViewMode::SideView;

    // Custom behaviors registered by mods
    std::unordered_map<std::string, CustomAIBehavior> m_customBehaviors;
};

} // namespace gloaming
