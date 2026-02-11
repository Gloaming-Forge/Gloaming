#pragma once

#include <sol/sol.hpp>

namespace gloaming {

// Forward declarations
class Engine;
class EnemySpawnSystem;
class EnemyAISystem;

/// Registers all Stage 14 Enemy & AI Lua APIs onto the given Lua state.
///
/// Provides:
///   - enemy_spawns.add_rule()       — register a spawn rule
///   - enemy_spawns.clear_rules()    — clear all spawn rules
///   - enemy_spawns.spawn_at()       — manually spawn an enemy at a position
///   - enemy_spawns.set_enabled()    — toggle spawning
///   - enemy_spawns.set_max()        — set global enemy cap
///   - enemy_spawns.set_interval()   — set spawn check interval
///   - enemy_spawns.set_range()      — set spawn distance range
///   - enemy_spawns.get_count()      — get active enemy count
///   - enemy_spawns.get_count_type() — get count of specific enemy type
///   - enemy_spawns.get_stats()      — get spawn statistics
///
///   - enemy_ai.set_behavior()       — set an enemy's AI behavior
///   - enemy_ai.get_behavior()       — get an enemy's current behavior
///   - enemy_ai.set_detection()      — set detection range
///   - enemy_ai.set_attack()         — configure attack (range, cooldown, damage)
///   - enemy_ai.set_patrol()         — configure patrol (radius, speed)
///   - enemy_ai.set_flee()           — set flee health threshold
///   - enemy_ai.set_despawn()        — configure despawn rules
///   - enemy_ai.set_orbit()          — configure orbit behavior (distance, speed)
///   - enemy_ai.set_target()         — manually set AI target
///   - enemy_ai.get_target()         — get current AI target
///   - enemy_ai.register_behavior()  — register a custom AI behavior from Lua
///   - enemy_ai.get_home()           — get home position
///   - enemy_ai.set_home()           — set home position
void bindEnemyAPI(sol::state& lua, Engine& engine,
                  EnemySpawnSystem& spawnSystem,
                  EnemyAISystem& aiSystem);

} // namespace gloaming
