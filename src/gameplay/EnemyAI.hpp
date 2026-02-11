#pragma once

#include "ecs/Components.hpp"

#include <string>
#include <cstdint>

namespace gloaming {

/// Built-in AI behavior types.
/// These cover common patterns across all target game styles (side-scrolling,
/// top-down RPG, flight). Mods can also register fully custom behaviors via
/// fsm.add_state() for anything these don't cover.
///
/// | Behavior      | Game Style    | Description                                   |
/// |---------------|---------------|-----------------------------------------------|
/// | patrol_walk   | Side-view     | Walk back and forth, reverse at ledges/walls   |
/// | patrol_fly    | Side-view     | Fly in a sine-wave pattern                     |
/// | patrol_path   | Top-down      | Wander around home with random direction changes |
/// | chase         | Any           | Move toward target when in detection range      |
/// | flee          | Any           | Run from target when health is low              |
/// | guard         | Any           | Stay near home, attack targets that enter range |
/// | orbit         | Flight        | Circle around target at a set distance           |
/// | strafe_run    | Flight        | Attack-run toward target, then retreat           |
/// | idle          | Any           | Do nothing, just stand/hover in place            |
namespace AIBehavior {
    constexpr const char* Idle        = "idle";
    constexpr const char* PatrolWalk  = "patrol_walk";
    constexpr const char* PatrolFly   = "patrol_fly";
    constexpr const char* PatrolPath  = "patrol_path";
    constexpr const char* Chase       = "chase";
    constexpr const char* Flee        = "flee";
    constexpr const char* Guard       = "guard";
    constexpr const char* Orbit       = "orbit";
    constexpr const char* StrafeRun   = "strafe_run";
}

/// EnemyAI component — drives enemy decision-making each frame.
///
/// The `behavior` string references a built-in behavior or a mod-registered one.
/// Built-in behaviors are handled by EnemyAISystem. For custom Lua-driven AI,
/// set behavior to "custom" and use fsm.add_state() / fsm.set_state() on the
/// same entity — EnemyAISystem will skip entities whose behavior is "custom".
struct EnemyAI {
    std::string behavior = "idle";      ///< Active behavior name (from AIBehavior:: or custom)
    std::string defaultBehavior = "idle"; ///< Behavior to return to after chase/flee ends

    float detectionRange = 200.0f;      ///< Range at which the enemy detects the player
    float attackRange = 32.0f;          ///< Range at which the enemy attacks
    float moveSpeed = 60.0f;            ///< Movement speed (pixels/sec)
    float fleeHealthThreshold = 0.2f;   ///< Flee when health percentage drops below this

    Entity target = NullEntity;         ///< Current target entity (typically the player)
    float targetCheckInterval = 0.5f;   ///< Seconds between target acquisition scans
    float targetCheckTimer = 0.0f;

    // Patrol state
    Vec2 homePosition{0.0f, 0.0f};     ///< Spawn/home position for guard and patrol
    float patrolRadius = 100.0f;        ///< How far to patrol from home
    int patrolDirection = 1;            ///< +1 = right/down, -1 = left/up
    float patrolTimer = 0.0f;           ///< Time in current patrol segment

    // Attack state
    float attackCooldown = 1.0f;        ///< Seconds between attacks
    float attackTimer = 0.0f;           ///< Countdown to next attack
    int contactDamage = 10;             ///< Damage dealt on contact with player

    // Orbit/flight state
    float orbitDistance = 100.0f;       ///< Distance to maintain when orbiting
    float orbitSpeed = 2.0f;            ///< Radians per second for orbit
    float orbitAngle = 0.0f;            ///< Current angle in orbit

    // Despawn rules
    float despawnDistance = 1500.0f;    ///< Despawn when this far from nearest player (0 = never)
    float despawnTimer = 0.0f;          ///< Accumulated time out of range
    float despawnDelay = 5.0f;          ///< Seconds out of range before despawn

    EnemyAI() = default;
    explicit EnemyAI(const std::string& bhv) : behavior(bhv), defaultBehavior(bhv) {}
};

/// Configuration for the enemy spawn manager
struct EnemySpawnConfig {
    float spawnCheckInterval = 2.0f;    ///< Seconds between spawn attempts
    int maxEnemies = 50;                ///< Global enemy cap
    float spawnRangeMin = 400.0f;       ///< Min distance from player to spawn
    float spawnRangeMax = 800.0f;       ///< Max distance from player to spawn
    bool enabled = true;                ///< Master toggle

    // Side-view spawn settings
    float surfaceDepth = 0.0f;          ///< Y coordinate of the world surface
    bool requireSolidBelow = true;      ///< Require solid ground below spawn point

    // Top-down spawn settings
    float encounterChance = 0.1f;       ///< Chance per interval for random encounter

    // Wave/flight spawn settings
    int waveSize = 3;                   ///< Enemies per wave
    float waveCooldown = 10.0f;         ///< Seconds between waves
};

/// Runtime stats for the spawn manager (for debug display)
struct EnemySpawnStats {
    int activeEnemies = 0;
    int totalSpawned = 0;
    int totalDespawned = 0;
    int totalKilled = 0;
    float timeSinceLastSpawn = 0.0f;
};

} // namespace gloaming
