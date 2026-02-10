#include "gameplay/LootDropSystem.hpp"
#include "gameplay/EnemySpawnSystem.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "mod/ContentRegistry.hpp"
#include "mod/EventBus.hpp"
#include "gameplay/GameplayLoop.hpp"

#include <cmath>

namespace gloaming {

void LootDropSystem::init(Registry& registry, Engine& engine) {
    System::init(registry, engine);
    m_contentRegistry = &engine.getContentRegistry();
    m_eventBus = &engine.getEventBus();
    m_enemySpawnSystem = engine.getEnemySpawnSystem();
    m_viewMode = engine.getGameModeConfig().viewMode;
}

void LootDropSystem::shutdown() {
    m_contentRegistry = nullptr;
    m_eventBus = nullptr;
    m_enemySpawnSystem = nullptr;
}

void LootDropSystem::update(float dt) {
    auto& registry = getRegistry();
    m_viewMode = getEngine().getGameModeConfig().viewMode;

    // Find dead enemies to process
    std::vector<Entity> deadEnemies;

    registry.each<EnemyTag, Health, Transform>([&](Entity entity, const EnemyTag& tag,
                                                      const Health& health,
                                                      const Transform& transform) {
        if (health.isDead()) {
            deadEnemies.push_back(entity);
        }
    });

    // Process each dead enemy
    for (Entity enemy : deadEnemies) {
        if (!registry.valid(enemy)) continue;

        auto& tag = registry.get<EnemyTag>(enemy);
        auto& transform = registry.get<Transform>(enemy);

        // Spawn loot
        spawnLoot(enemy, tag.enemyType, transform.position);

        // Emit death event
        if (m_eventBus) {
            EventData data;
            data.setInt("entity", static_cast<int>(enemy));
            data.setString("enemy_id", tag.enemyType);
            data.setFloat("x", transform.position.x);
            data.setFloat("y", transform.position.y);
            m_eventBus->emit("enemy_killed", data);
        }

        // Update kill stats
        if (m_enemySpawnSystem) {
            m_enemySpawnSystem->incrementKilled();
        }

        // Destroy the enemy entity
        registry.destroy(enemy);
    }
}

void LootDropSystem::spawnLoot(Entity enemy, const std::string& enemyType,
                                const Vec2& position) {
    const EnemyDefinition* def = m_contentRegistry->getEnemy(enemyType);
    if (!def) return;

    std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);

    for (const auto& drop : def->drops) {
        // Roll for drop chance
        if (chanceDist(m_rng) > drop.chance) continue;

        // Determine count (random between min and max)
        int count = drop.countMin;
        if (drop.countMax > drop.countMin) {
            std::uniform_int_distribution<int> countDist(drop.countMin, drop.countMax);
            count = countDist(m_rng);
        }

        if (count <= 0) continue;
        if (drop.item.empty()) continue;

        // Spawn the item drop
        spawnItemDropEntity(drop.item, count, position.x, position.y);

        if (m_eventBus) {
            EventData data;
            data.setString("item", drop.item);
            data.setInt("count", count);
            data.setString("enemy_id", enemyType);
            data.setFloat("x", position.x);
            data.setFloat("y", position.y);
            m_eventBus->emit("loot_dropped", data);
        }
    }
}

Entity LootDropSystem::spawnItemDropEntity(const std::string& itemId, int count,
                                            float x, float y) {
    auto& registry = getRegistry();

    // Add small random offset so multiple drops don't stack perfectly
    std::uniform_real_distribution<float> offsetDist(-12.0f, 12.0f);
    float offsetX = offsetDist(m_rng);
    float offsetY = offsetDist(m_rng);

    Entity drop = registry.create(
        Transform{Vec2(x + offsetX, y + offsetY)},
        Name{itemId, "item_drop"}
    );

    // Add item drop component
    ItemDrop itemDrop(itemId, count);
    itemDrop.pickupDelay = 0.5f;   // Brief delay before pickup
    itemDrop.despawnTime = 300.0f;  // 5 minutes
    registry.add<ItemDrop>(drop, std::move(itemDrop));

    // Add collider for pickup detection
    Collider collider;
    collider.size = Vec2(8.0f, 8.0f);
    collider.layer = CollisionLayer::Item;
    collider.mask = CollisionLayer::Tile | CollisionLayer::Player;
    registry.add<Collider>(drop, collider);

    if (m_viewMode == ViewMode::SideView) {
        // Side-view: upward pop + gravity for visual feedback
        std::uniform_real_distribution<float> velDist(-30.0f, 30.0f);
        registry.add<Velocity>(drop, Vec2(velDist(m_rng), -60.0f));
        registry.add<Gravity>(drop, 1.0f);
    } else {
        // Top-down / flight: small random scatter, no gravity
        std::uniform_real_distribution<float> scatterDist(-20.0f, 20.0f);
        registry.add<Velocity>(drop, Vec2(scatterDist(m_rng), scatterDist(m_rng)));
    }

    return drop;
}

} // namespace gloaming
