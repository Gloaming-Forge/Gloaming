#pragma once

#include "ecs/Systems.hpp"
#include "ecs/Components.hpp"
#include "gameplay/EnemyAI.hpp"
#include "gameplay/GameMode.hpp"

#include <random>

namespace gloaming {

// Forward declarations
class ContentRegistry;
class EventBus;
class EnemySpawnSystem;

/// System that monitors enemy deaths and spawns loot drops from their loot tables.
///
/// When an entity with EnemyTag + Health has health <= 0, this system:
///   1. Looks up the enemy's loot table from the content registry
///   2. Rolls each drop entry (chance-based)
///   3. Spawns ItemDrop entities at the enemy's death position
///   4. Emits "enemy_killed" event with drop information
///   5. Destroys the enemy entity
///
/// This system runs at a low priority in PostUpdate so it processes deaths
/// after CombatSystem has applied damage and updated health.
class LootDropSystem : public System {
public:
    LootDropSystem() : System("LootDropSystem", 5) {}

    void init(Registry& registry, Engine& engine) override;
    void update(float dt) override;
    void shutdown() override;

private:
    /// Spawn item drops for a killed enemy
    void spawnLoot(Entity enemy, const std::string& enemyType, const Vec2& position);

    /// Create an ItemDrop entity at a position with a small random offset
    Entity spawnItemDropEntity(const std::string& itemId, int count,
                               float x, float y);

    ContentRegistry* m_contentRegistry = nullptr;
    EventBus* m_eventBus = nullptr;
    EnemySpawnSystem* m_enemySpawnSystem = nullptr;
    ViewMode m_viewMode = ViewMode::SideView;

    std::mt19937 m_rng{std::random_device{}()};
};

} // namespace gloaming
