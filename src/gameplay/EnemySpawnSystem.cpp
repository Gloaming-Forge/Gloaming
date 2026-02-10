#include "gameplay/EnemySpawnSystem.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "mod/ContentRegistry.hpp"
#include "mod/EventBus.hpp"
#include "world/TileMap.hpp"
#include "lighting/LightingSystem.hpp"

#include <cmath>
#include <algorithm>

namespace gloaming {

void EnemySpawnSystem::init(Registry& registry, Engine& engine) {
    System::init(registry, engine);
    m_contentRegistry = &engine.getContentRegistry();
    m_tileMap = &engine.getTileMap();
    m_eventBus = &engine.getEventBus();
    m_lightingSystem = engine.getLightingSystem();
    m_viewMode = engine.getGameModeConfig().viewMode;
}

void EnemySpawnSystem::shutdown() {
    m_spawnRules.clear();
}

void EnemySpawnSystem::addSpawnRule(const SpawnRule& rule) {
    m_spawnRules.push_back(rule);
}

void EnemySpawnSystem::clearSpawnRules() {
    m_spawnRules.clear();
    m_rulesGenerated = false;
}

int EnemySpawnSystem::getActiveEnemyCount() {
    int count = 0;
    getRegistry().each<EnemyTag>([&count](Entity, const EnemyTag&) {
        ++count;
    });
    return count;
}

int EnemySpawnSystem::getEnemyCountByType(const std::string& type) {
    int count = 0;
    getRegistry().each<EnemyTag>([&count, &type](Entity, const EnemyTag& tag) {
        if (tag.enemyType == type) ++count;
    });
    return count;
}

void EnemySpawnSystem::update(float dt) {
    if (!m_config.enabled) return;

    // Read current view mode each frame (mods can change it)
    m_viewMode = getEngine().getGameModeConfig().viewMode;

    // Auto-generate spawn rules from content registry on first update
    if (!m_rulesGenerated && m_spawnRules.empty()) {
        generateDefaultRules();
        m_rulesGenerated = true;
    }

    if (m_spawnRules.empty()) return;

    // Update spawn timer
    m_spawnTimer += dt;
    m_stats.timeSinceLastSpawn += dt;

    if (m_spawnTimer < m_config.spawnCheckInterval) return;
    m_spawnTimer = 0.0f;

    // Check enemy cap
    m_stats.activeEnemies = getActiveEnemyCount();
    if (m_stats.activeEnemies >= m_config.maxEnemies) return;

    // Gather player positions
    auto& registry = getRegistry();
    std::vector<Vec2> playerPositions;
    registry.each<PlayerTag, Transform>([&](Entity, const PlayerTag&, const Transform& t) {
        playerPositions.push_back(t.position);
    });

    if (playerPositions.empty()) return;

    // Try to spawn around each player
    for (const auto& pos : playerPositions) {
        if (m_stats.activeEnemies >= m_config.maxEnemies) break;
        trySpawnAroundPlayer(pos);
    }
}

void EnemySpawnSystem::trySpawnAroundPlayer(const Vec2& playerPos) {
    Vec2 spawnPos = pickSpawnPosition(playerPos);

    if (!isValidSpawnPosition(spawnPos.x, spawnPos.y)) return;

    auto eligible = getEligibleEnemies(spawnPos.x, spawnPos.y);
    if (eligible.empty()) return;

    const SpawnRule* selected = weightedSelect(eligible);
    if (!selected) return;

    // Check per-type cap
    int typeCount = getEnemyCountByType(selected->enemyId);
    if (typeCount >= selected->maxAlive) return;

    Entity enemy = spawnEnemy(selected->enemyId, spawnPos.x, spawnPos.y);
    if (enemy != NullEntity) {
        m_stats.totalSpawned++;
        m_stats.timeSinceLastSpawn = 0.0f;
        m_stats.activeEnemies++;
    }
}

Vec2 EnemySpawnSystem::pickSpawnPosition(const Vec2& playerPos) {
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> distDist(m_config.spawnRangeMin, m_config.spawnRangeMax);

    float distance = distDist(m_rng);

    switch (m_viewMode) {
        case ViewMode::SideView: {
            // Spawn to the left or right of the player, at a random depth offset
            std::uniform_int_distribution<int> sideDist(0, 1);
            float xOff = (sideDist(m_rng) == 0 ? -1.0f : 1.0f) * distance;
            std::uniform_real_distribution<float> yOff(-200.0f, 200.0f);
            return Vec2(playerPos.x + xOff, playerPos.y + yOff(m_rng));
        }
        case ViewMode::TopDown: {
            // Spawn at a random angle around the player
            float angle = angleDist(m_rng);
            return Vec2(
                playerPos.x + std::cos(angle) * distance,
                playerPos.y + std::sin(angle) * distance
            );
        }
        case ViewMode::Custom:
        default: {
            // Flight / custom: spawn ahead of the player (to the right for auto-scroll)
            // or at screen edges
            std::uniform_real_distribution<float> yOff(-300.0f, 300.0f);
            return Vec2(playerPos.x + distance, playerPos.y + yOff(m_rng));
        }
    }
}

bool EnemySpawnSystem::isValidSpawnPosition(float x, float y) const {
    if (!m_tileMap) return true;

    int tileX = static_cast<int>(std::floor(x / 16.0f));
    int tileY = static_cast<int>(std::floor(y / 16.0f));

    // Don't spawn inside solid tiles
    Tile tile = m_tileMap->getTile(tileX, tileY);
    if (tile.isSolid()) return false;

    if (m_viewMode == ViewMode::SideView && m_config.requireSolidBelow) {
        // For side-view, require solid ground within 3 tiles below
        for (int dy = 1; dy <= 3; ++dy) {
            Tile below = m_tileMap->getTile(tileX, tileY + dy);
            if (below.isSolid()) return true;
        }
        return false; // No ground found — skip (unless flying enemy, handled by caller)
    }

    return true;
}

std::vector<const SpawnRule*> EnemySpawnSystem::getEligibleEnemies(float x, float y) const {
    std::vector<const SpawnRule*> eligible;

    float depth = y;
    float lightLevel = 1.0f;

    // Get light level at position if lighting system is available
    if (m_lightingSystem && m_lightingSystem->getConfig().enabled) {
        lightLevel = m_lightingSystem->getDayNightCycle().getSkyBrightness();
    }

    bool isNight = false;
    bool isDay = true;
    if (m_lightingSystem) {
        auto tod = m_lightingSystem->getDayNightCycle().getTimeOfDay();
        isNight = (tod == TimeOfDay::Night);
        isDay = (tod == TimeOfDay::Day || tod == TimeOfDay::Dawn);
    }

    for (const auto& rule : m_spawnRules) {
        // Depth check
        if (depth < rule.depthMin || depth > rule.depthMax) continue;

        // Light level check
        if (lightLevel > rule.lightLevelMax) continue;

        // Day/night check
        if (rule.nightOnly && !isNight) continue;
        if (rule.dayOnly && !isDay) continue;

        // Biome check (empty = any biome)
        // TODO: integrate with biome system when available at runtime

        // Enemy definition must exist
        if (!m_contentRegistry->hasEnemy(rule.enemyId)) continue;

        eligible.push_back(&rule);
    }

    return eligible;
}

const SpawnRule* EnemySpawnSystem::weightedSelect(const std::vector<const SpawnRule*>& eligible) {
    if (eligible.empty()) return nullptr;
    if (eligible.size() == 1) return eligible[0];

    float totalWeight = 0.0f;
    for (const auto* rule : eligible) {
        totalWeight += rule->weight;
    }

    if (totalWeight <= 0.0f) return eligible[0];

    std::uniform_real_distribution<float> dist(0.0f, totalWeight);
    float roll = dist(m_rng);

    float cumulative = 0.0f;
    for (const auto* rule : eligible) {
        cumulative += rule->weight;
        if (roll <= cumulative) return rule;
    }

    return eligible.back();
}

Entity EnemySpawnSystem::spawnEnemy(const std::string& enemyId, float x, float y) {
    return createEnemyEntity(enemyId, x, y);
}

Entity EnemySpawnSystem::createEnemyEntity(const std::string& enemyId, float x, float y) {
    const EnemyDefinition* def = m_contentRegistry->getEnemy(enemyId);
    if (!def) {
        LOG_WARN("EnemySpawnSystem: unknown enemy '{}'", enemyId);
        return NullEntity;
    }

    auto& registry = getRegistry();

    // Create entity with core components
    Entity entity = registry.create(
        Transform{Vec2(x, y)},
        Name{def->name, enemyId},
        EnemyTag{enemyId}
    );

    // Health
    registry.add<Health>(entity, def->health, def->health);

    // Velocity
    registry.add<Velocity>(entity);

    // Collider
    Collider collider;
    collider.size = Vec2(def->colliderWidth, def->colliderHeight);
    collider.layer = CollisionLayer::Enemy;
    collider.mask = CollisionLayer::Player | CollisionLayer::Projectile | CollisionLayer::Tile;
    registry.add<Collider>(entity, collider);

    // Gravity (for side-view games)
    if (m_viewMode == ViewMode::SideView) {
        registry.add<Gravity>(entity, 1.0f);
    }

    // EnemyAI component
    EnemyAI ai;
    if (!def->behaviorScript.empty()) {
        // If a behavior script is specified, use "custom" to let FSM handle it
        ai.behavior = "custom";
        ai.defaultBehavior = "custom";
    } else if (!def->aiBehavior.empty()) {
        // Use behavior specified in enemy definition
        ai.behavior = def->aiBehavior;
        ai.defaultBehavior = def->aiBehavior;
    } else {
        // Pick a default behavior based on view mode
        switch (m_viewMode) {
            case ViewMode::SideView:
                ai.behavior = AIBehavior::PatrolWalk;
                ai.defaultBehavior = AIBehavior::PatrolWalk;
                break;
            case ViewMode::TopDown:
                ai.behavior = AIBehavior::PatrolPath;
                ai.defaultBehavior = AIBehavior::PatrolPath;
                break;
            case ViewMode::Custom:
                ai.behavior = AIBehavior::Orbit;
                ai.defaultBehavior = AIBehavior::Orbit;
                break;
        }
    }
    ai.homePosition = Vec2(x, y);
    ai.contactDamage = def->damage;
    ai.detectionRange = def->detectionRange;
    ai.attackRange = def->attackRange;
    ai.moveSpeed = def->moveSpeed;
    ai.patrolRadius = def->patrolRadius;
    ai.fleeHealthThreshold = def->fleeThreshold;
    ai.despawnDistance = def->despawnDistance;
    ai.orbitDistance = def->orbitDistance;
    ai.orbitSpeed = def->orbitSpeed;
    registry.add<EnemyAI>(entity, std::move(ai));

    // Emit spawn event
    if (m_eventBus) {
        EventData data;
        data.setInt("entity", static_cast<int>(entity));
        data.setString("enemy_id", enemyId);
        data.setFloat("x", x);
        data.setFloat("y", y);
        m_eventBus->emit("enemy_spawned", data);
    }

    return entity;
}

void EnemySpawnSystem::generateDefaultRules() {
    if (!m_contentRegistry) return;

    auto enemyIds = m_contentRegistry->getEnemyIds();
    for (const auto& id : enemyIds) {
        const EnemyDefinition* def = m_contentRegistry->getEnemy(id);
        if (!def) continue;

        SpawnRule rule;
        rule.enemyId = id;
        rule.weight = 1.0f;
        rule.maxAlive = 10;

        // Copy conditions from enemy definition
        rule.biomes = def->spawnConditions.biomes;
        rule.depthMin = def->spawnConditions.depthMin;
        rule.depthMax = def->spawnConditions.depthMax;
        rule.lightLevelMax = def->spawnConditions.lightLevelMax;

        m_spawnRules.push_back(std::move(rule));
    }

    if (!enemyIds.empty()) {
        LOG_INFO("EnemySpawnSystem: auto-generated {} spawn rules from content registry",
                 m_spawnRules.size());
    }
}

} // namespace gloaming
