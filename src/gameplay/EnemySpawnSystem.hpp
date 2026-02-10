#pragma once

#include "ecs/Systems.hpp"
#include "ecs/Components.hpp"
#include "gameplay/EnemyAI.hpp"
#include "gameplay/GameMode.hpp"

#include <string>
#include <vector>
#include <functional>
#include <random>

namespace gloaming {

// Forward declarations
class ContentRegistry;
class TileMap;
class EventBus;
class LightingSystem;

/// Spawn rule registered by mods — overrides content-registry defaults.
/// Mods call enemy_spawns.add_rule() to create these.
struct SpawnRule {
    std::string enemyId;                ///< Qualified enemy ID (e.g. "base:slime")
    float weight = 1.0f;               ///< Relative probability weight
    int maxAlive = 10;                  ///< Max alive at once for this type

    // Conditions (all must be satisfied for the enemy to be eligible)
    std::vector<std::string> biomes;    ///< Required biomes (empty = any)
    float depthMin = -1e6f;             ///< Min world Y (negative = sky)
    float depthMax = 1e6f;              ///< Max world Y (positive = underground)
    float lightLevelMax = 1.0f;         ///< Max light level (0-1; 1 = spawn anywhere)
    bool nightOnly = false;             ///< Only spawn at night
    bool dayOnly = false;               ///< Only spawn during daytime
};

/// System that handles spawning enemies around active players.
///
/// Genre awareness:
///   - **SideView**: Spawns enemies on solid ground near the player at
///     configurable depth ranges. Uses biome/depth/light conditions.
///   - **TopDown**: Spawns enemies in walkable tiles around the player.
///     Supports encounter-chance-based spawning (Pokemon random encounters).
///   - **Flight/Custom**: Wave-based spawning at screen edges in the
///     direction of travel.
///
/// Spawn flow:
///   1. Timer tick -> check if under enemy cap
///   2. Gather player positions
///   3. For each player, pick random spawn position in [spawnRangeMin, spawnRangeMax]
///   4. Filter eligible enemies by conditions (biome, depth, light, day/night)
///   5. Weighted random selection from eligible pool
///   6. Spawn entity, attach EnemyTag + EnemyAI + Health + Collider etc.
///   7. Emit "enemy_spawned" event
class EnemySpawnSystem : public System {
public:
    EnemySpawnSystem() : System("EnemySpawnSystem", 20) {}

    void init(Registry& registry, Engine& engine) override;
    void update(float dt) override;
    void shutdown() override;

    // Configuration
    EnemySpawnConfig& getConfig() { return m_config; }
    const EnemySpawnConfig& getConfig() const { return m_config; }

    // Spawn rules (registered by mods via Lua)
    void addSpawnRule(const SpawnRule& rule);
    void clearSpawnRules();
    const std::vector<SpawnRule>& getSpawnRules() const { return m_spawnRules; }

    // Manual spawn (from Lua: enemy_spawns.spawn_at)
    Entity spawnEnemy(const std::string& enemyId, float x, float y);

    // Stats
    const EnemySpawnStats& getStats() const { return m_stats; }
    void incrementKilled() { m_stats.totalKilled++; }
    void incrementDespawned() { m_stats.totalDespawned++; }

    // Enemy count
    int getActiveEnemyCount();
    int getEnemyCountByType(const std::string& type);

private:
    /// Try to spawn enemies around a player position
    void trySpawnAroundPlayer(const Vec2& playerPos);

    /// Pick a spawn position relative to the player, respecting view mode
    Vec2 pickSpawnPosition(const Vec2& playerPos);

    /// Check whether a spawn position is valid (not inside solid tiles, etc.)
    bool isValidSpawnPosition(float x, float y) const;

    /// Gather eligible enemies for a given spawn position
    std::vector<const SpawnRule*> getEligibleEnemies(float x, float y) const;

    /// Select one enemy from the eligible pool via weighted random
    const SpawnRule* weightedSelect(const std::vector<const SpawnRule*>& eligible);

    /// Create the actual enemy entity from a content-registry definition
    Entity createEnemyEntity(const std::string& enemyId, float x, float y);

    /// Auto-generate spawn rules from content registry if none were explicitly added
    void generateDefaultRules();

    EnemySpawnConfig m_config;
    EnemySpawnStats m_stats;
    std::vector<SpawnRule> m_spawnRules;
    bool m_rulesGenerated = false;

    float m_spawnTimer = 0.0f;

    ContentRegistry* m_contentRegistry = nullptr;
    TileMap* m_tileMap = nullptr;
    EventBus* m_eventBus = nullptr;
    LightingSystem* m_lightingSystem = nullptr;
    ViewMode m_viewMode = ViewMode::SideView;

    std::mt19937 m_rng{std::random_device{}()};
};

} // namespace gloaming
