#include <gtest/gtest.h>

#include "gameplay/EnemyAI.hpp"
#include "gameplay/EnemyAISystem.hpp"
#include "gameplay/EnemySpawnSystem.hpp"
#include "gameplay/LootDropSystem.hpp"
#include "gameplay/GameplayLoop.hpp"
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "mod/ContentRegistry.hpp"

using namespace gloaming;

// =============================================================================
// EnemyAI Component Tests
// =============================================================================

TEST(EnemyAITest, DefaultConstruction) {
    EnemyAI ai;
    EXPECT_EQ(ai.behavior, "idle");
    EXPECT_EQ(ai.defaultBehavior, "idle");
    EXPECT_FLOAT_EQ(ai.detectionRange, 200.0f);
    EXPECT_FLOAT_EQ(ai.attackRange, 32.0f);
    EXPECT_FLOAT_EQ(ai.moveSpeed, 60.0f);
    EXPECT_FLOAT_EQ(ai.fleeHealthThreshold, 0.2f);
    EXPECT_EQ(ai.target, NullEntity);
    EXPECT_EQ(ai.contactDamage, 10);
    EXPECT_FLOAT_EQ(ai.despawnDistance, 1500.0f);
}

TEST(EnemyAITest, ExplicitBehavior) {
    EnemyAI ai("patrol_walk");
    EXPECT_EQ(ai.behavior, "patrol_walk");
    EXPECT_EQ(ai.defaultBehavior, "patrol_walk");
}

TEST(EnemyAITest, BehaviorConstants) {
    EXPECT_STREQ(AIBehavior::Idle, "idle");
    EXPECT_STREQ(AIBehavior::PatrolWalk, "patrol_walk");
    EXPECT_STREQ(AIBehavior::PatrolFly, "patrol_fly");
    EXPECT_STREQ(AIBehavior::PatrolPath, "patrol_path");
    EXPECT_STREQ(AIBehavior::Chase, "chase");
    EXPECT_STREQ(AIBehavior::Flee, "flee");
    EXPECT_STREQ(AIBehavior::Guard, "guard");
    EXPECT_STREQ(AIBehavior::Orbit, "orbit");
    EXPECT_STREQ(AIBehavior::StrafeRun, "strafe_run");
}

// =============================================================================
// EnemySpawnConfig Tests
// =============================================================================

TEST(EnemySpawnConfigTest, DefaultValues) {
    EnemySpawnConfig config;
    EXPECT_TRUE(config.enabled);
    EXPECT_EQ(config.maxEnemies, 50);
    EXPECT_FLOAT_EQ(config.spawnCheckInterval, 2.0f);
    EXPECT_FLOAT_EQ(config.spawnRangeMin, 400.0f);
    EXPECT_FLOAT_EQ(config.spawnRangeMax, 800.0f);
}

// =============================================================================
// SpawnRule Tests
// =============================================================================

TEST(SpawnRuleTest, DefaultValues) {
    SpawnRule rule;
    EXPECT_TRUE(rule.enemyId.empty());
    EXPECT_FLOAT_EQ(rule.weight, 1.0f);
    EXPECT_EQ(rule.maxAlive, 10);
    EXPECT_FALSE(rule.nightOnly);
    EXPECT_FALSE(rule.dayOnly);
    EXPECT_FLOAT_EQ(rule.lightLevelMax, 1.0f);
    EXPECT_TRUE(rule.biomes.empty());
}

TEST(SpawnRuleTest, CustomValues) {
    SpawnRule rule;
    rule.enemyId = "base:zombie";
    rule.weight = 2.0f;
    rule.maxAlive = 5;
    rule.nightOnly = true;
    rule.lightLevelMax = 0.3f;
    rule.biomes = {"forest", "plains"};
    rule.depthMin = 0.0f;
    rule.depthMax = 1000.0f;

    EXPECT_EQ(rule.enemyId, "base:zombie");
    EXPECT_FLOAT_EQ(rule.weight, 2.0f);
    EXPECT_EQ(rule.maxAlive, 5);
    EXPECT_TRUE(rule.nightOnly);
    EXPECT_FLOAT_EQ(rule.lightLevelMax, 0.3f);
    EXPECT_EQ(rule.biomes.size(), 2u);
}

// =============================================================================
// EnemySpawnStats Tests
// =============================================================================

TEST(EnemySpawnStatsTest, DefaultZero) {
    EnemySpawnStats stats;
    EXPECT_EQ(stats.activeEnemies, 0);
    EXPECT_EQ(stats.totalSpawned, 0);
    EXPECT_EQ(stats.totalDespawned, 0);
    EXPECT_EQ(stats.totalKilled, 0);
    EXPECT_FLOAT_EQ(stats.timeSinceLastSpawn, 0.0f);
}

// =============================================================================
// EnemyAI + ECS Integration Tests
// =============================================================================

TEST(EnemyAIIntegrationTest, AddComponentToEntity) {
    Registry registry;
    Entity entity = registry.create(
        Transform{Vec2(100.0f, 200.0f)},
        Name{"slime", "base:slime"},
        EnemyTag{"base:slime"}
    );

    EnemyAI ai("patrol_walk");
    ai.homePosition = Vec2(100.0f, 200.0f);
    ai.contactDamage = 15;
    registry.add<EnemyAI>(entity, std::move(ai));

    EXPECT_TRUE(registry.has<EnemyAI>(entity));
    EXPECT_TRUE(registry.has<EnemyTag>(entity));

    auto& retrievedAI = registry.get<EnemyAI>(entity);
    EXPECT_EQ(retrievedAI.behavior, "patrol_walk");
    EXPECT_EQ(retrievedAI.contactDamage, 15);
    EXPECT_FLOAT_EQ(retrievedAI.homePosition.x, 100.0f);
    EXPECT_FLOAT_EQ(retrievedAI.homePosition.y, 200.0f);
}

TEST(EnemyAIIntegrationTest, BehaviorSwitch) {
    Registry registry;
    Entity entity = registry.create(Transform{Vec2(0.0f, 0.0f)});
    EnemyAI ai("patrol_walk");
    ai.defaultBehavior = "patrol_walk";
    registry.add<EnemyAI>(entity, std::move(ai));

    auto& retrievedAI = registry.get<EnemyAI>(entity);

    // Simulate switching to chase
    retrievedAI.behavior = AIBehavior::Chase;
    EXPECT_EQ(retrievedAI.behavior, "chase");

    // Switch back to default
    retrievedAI.behavior = retrievedAI.defaultBehavior;
    EXPECT_EQ(retrievedAI.behavior, "patrol_walk");
}

TEST(EnemyAIIntegrationTest, FleeThresholdCheck) {
    Registry registry;
    Entity entity = registry.create(Transform{Vec2(0.0f, 0.0f)});
    registry.add<Health>(entity, 100.0f, 100.0f);
    registry.add<EnemyAI>(entity, EnemyAI("patrol_walk"));

    auto& health = registry.get<Health>(entity);
    auto& ai = registry.get<EnemyAI>(entity);

    // At full health, should not flee
    EXPECT_GT(health.getPercentage(), ai.fleeHealthThreshold);

    // Take heavy damage
    health.takeDamage(85.0f);
    EXPECT_LT(health.getPercentage(), ai.fleeHealthThreshold);
}

TEST(EnemyAIIntegrationTest, DespawnTimerAccumulates) {
    EnemyAI ai;
    ai.despawnDistance = 100.0f;
    ai.despawnDelay = 5.0f;

    // Simulate being out of range for multiple frames
    ai.despawnTimer += 2.0f;
    EXPECT_FLOAT_EQ(ai.despawnTimer, 2.0f);
    EXPECT_LT(ai.despawnTimer, ai.despawnDelay); // Not yet time

    ai.despawnTimer += 3.0f;
    EXPECT_GE(ai.despawnTimer, ai.despawnDelay); // Should despawn now

    // Simulate coming back in range
    ai.despawnTimer = 0.0f;
    EXPECT_FLOAT_EQ(ai.despawnTimer, 0.0f);
}

// =============================================================================
// EnemyAISystem Custom Behavior Registration Tests
// =============================================================================

TEST(EnemyAISystemTest, RegisterCustomBehavior) {
    EnemyAISystem system;
    EXPECT_FALSE(system.hasBehavior("custom_spin"));

    bool callbackCalled = false;
    system.registerBehavior("custom_spin", [&callbackCalled](Entity, EnemyAI&, float) {
        callbackCalled = true;
    });

    EXPECT_TRUE(system.hasBehavior("custom_spin"));
}

TEST(EnemyAISystemTest, BuiltInBehaviorsNotRegistered) {
    EnemyAISystem system;
    // Built-in behaviors are handled by the system directly, not in the custom map
    EXPECT_FALSE(system.hasBehavior("idle"));
    EXPECT_FALSE(system.hasBehavior("patrol_walk"));
    EXPECT_FALSE(system.hasBehavior("chase"));
}

// =============================================================================
// EnemyDefinition AI Fields Tests
// =============================================================================

TEST(EnemyDefinitionTest, DefaultAIFields) {
    EnemyDefinition def;
    EXPECT_TRUE(def.aiBehavior.empty());
    EXPECT_FLOAT_EQ(def.detectionRange, 200.0f);
    EXPECT_FLOAT_EQ(def.attackRange, 32.0f);
    EXPECT_FLOAT_EQ(def.moveSpeed, 60.0f);
    EXPECT_FLOAT_EQ(def.patrolRadius, 100.0f);
    EXPECT_FLOAT_EQ(def.fleeThreshold, 0.2f);
    EXPECT_FLOAT_EQ(def.despawnDistance, 1500.0f);
    EXPECT_FLOAT_EQ(def.colliderWidth, 16.0f);
    EXPECT_FLOAT_EQ(def.colliderHeight, 16.0f);
}

TEST(EnemyDefinitionTest, CustomAIFields) {
    EnemyDefinition def;
    def.aiBehavior = "guard";
    def.detectionRange = 300.0f;
    def.attackRange = 48.0f;
    def.moveSpeed = 80.0f;
    def.patrolRadius = 150.0f;
    def.fleeThreshold = 0.1f;
    def.despawnDistance = 2000.0f;
    def.colliderWidth = 24.0f;
    def.colliderHeight = 32.0f;

    EXPECT_EQ(def.aiBehavior, "guard");
    EXPECT_FLOAT_EQ(def.detectionRange, 300.0f);
    EXPECT_FLOAT_EQ(def.attackRange, 48.0f);
    EXPECT_FLOAT_EQ(def.moveSpeed, 80.0f);
    EXPECT_FLOAT_EQ(def.colliderWidth, 24.0f);
    EXPECT_FLOAT_EQ(def.colliderHeight, 32.0f);
}

// =============================================================================
// Content Registry Enemy Registration (Integration)
// =============================================================================

TEST(ContentRegistryEnemyTest, RegisterAndRetrieve) {
    ContentRegistry registry;

    EnemyDefinition def;
    def.id = "slime";
    def.qualifiedId = "base:slime";
    def.name = "Green Slime";
    def.health = 50.0f;
    def.damage = 10;
    def.aiBehavior = "patrol_walk";
    def.detectionRange = 150.0f;
    def.moveSpeed = 40.0f;

    EnemyDropDef drop;
    drop.item = "base:gel";
    drop.countMin = 1;
    drop.countMax = 3;
    drop.chance = 1.0f;
    def.drops.push_back(drop);

    registry.registerEnemy(def);

    EXPECT_TRUE(registry.hasEnemy("base:slime"));
    EXPECT_EQ(registry.enemyCount(), 1u);

    const EnemyDefinition* retrieved = registry.getEnemy("base:slime");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->name, "Green Slime");
    EXPECT_FLOAT_EQ(retrieved->health, 50.0f);
    EXPECT_EQ(retrieved->damage, 10);
    EXPECT_EQ(retrieved->aiBehavior, "patrol_walk");
    EXPECT_FLOAT_EQ(retrieved->detectionRange, 150.0f);
    EXPECT_FLOAT_EQ(retrieved->moveSpeed, 40.0f);
    EXPECT_EQ(retrieved->drops.size(), 1u);
    EXPECT_EQ(retrieved->drops[0].item, "base:gel");
}

TEST(ContentRegistryEnemyTest, GetEnemyIds) {
    ContentRegistry registry;

    EnemyDefinition slime;
    slime.id = "slime";
    slime.qualifiedId = "base:slime";
    slime.name = "Slime";
    registry.registerEnemy(slime);

    EnemyDefinition zombie;
    zombie.id = "zombie";
    zombie.qualifiedId = "base:zombie";
    zombie.name = "Zombie";
    registry.registerEnemy(zombie);

    auto ids = registry.getEnemyIds();
    EXPECT_EQ(ids.size(), 2u);

    // IDs should contain both enemies (order may vary)
    bool hasSlime = false, hasZombie = false;
    for (const auto& id : ids) {
        if (id == "base:slime") hasSlime = true;
        if (id == "base:zombie") hasZombie = true;
    }
    EXPECT_TRUE(hasSlime);
    EXPECT_TRUE(hasZombie);
}

// =============================================================================
// Multiple Enemy Entity Creation Test
// =============================================================================

TEST(EnemyEntityTest, MultipleEnemiesWithDifferentBehaviors) {
    Registry registry;

    // Create a side-view enemy
    Entity patrol = registry.create(
        Transform{Vec2(100.0f, 300.0f)},
        EnemyTag{"base:slime"}
    );
    registry.add<Health>(patrol, 50.0f, 50.0f);
    registry.add<Velocity>(patrol);
    registry.add<EnemyAI>(patrol, EnemyAI("patrol_walk"));

    // Create a flying enemy
    Entity flyer = registry.create(
        Transform{Vec2(200.0f, 100.0f)},
        EnemyTag{"base:bat"}
    );
    registry.add<Health>(flyer, 30.0f, 30.0f);
    registry.add<Velocity>(flyer);
    registry.add<EnemyAI>(flyer, EnemyAI("patrol_fly"));

    // Create a top-down enemy
    Entity guard = registry.create(
        Transform{Vec2(400.0f, 400.0f)},
        EnemyTag{"base:guard"}
    );
    registry.add<Health>(guard, 100.0f, 100.0f);
    registry.add<Velocity>(guard);
    registry.add<EnemyAI>(guard, EnemyAI("guard"));

    // Verify all have different behaviors
    EXPECT_EQ(registry.get<EnemyAI>(patrol).behavior, "patrol_walk");
    EXPECT_EQ(registry.get<EnemyAI>(flyer).behavior, "patrol_fly");
    EXPECT_EQ(registry.get<EnemyAI>(guard).behavior, "guard");

    // Count enemies
    int enemyCount = 0;
    registry.each<EnemyTag>([&enemyCount](Entity, const EnemyTag&) { ++enemyCount; });
    EXPECT_EQ(enemyCount, 3);
}

// =============================================================================
// Enemy Death and Loot (Integration Simulation)
// =============================================================================

TEST(EnemyDeathTest, DeathDetectedByHealthCheck) {
    Registry registry;

    Entity enemy = registry.create(
        Transform{Vec2(100.0f, 200.0f)},
        EnemyTag{"base:slime"},
        Name{"slime", "base:slime"}
    );
    registry.add<Health>(enemy, 50.0f, 50.0f);
    registry.add<EnemyAI>(enemy, EnemyAI("patrol_walk"));

    // Kill the enemy
    auto& health = registry.get<Health>(enemy);
    health.takeDamage(50.0f);
    EXPECT_TRUE(health.isDead());

    // Verify we can detect dead enemies with the ECS query
    int deadCount = 0;
    registry.each<EnemyTag, Health>([&](Entity, const EnemyTag&, const Health& h) {
        if (h.isDead()) deadCount++;
    });
    EXPECT_EQ(deadCount, 1);
}

TEST(EnemyDeathTest, ItemDropCreation) {
    Registry registry;

    // Simulate loot spawning (what LootDropSystem does)
    Entity drop = registry.create(
        Transform{Vec2(100.0f, 200.0f)},
        Name{"base:gel", "item_drop"}
    );
    registry.add<ItemDrop>(drop, ItemDrop("base:gel", 2));
    registry.add<Velocity>(drop, Vec2(5.0f, -60.0f));
    registry.add<Gravity>(drop, 1.0f);

    EXPECT_TRUE(registry.has<ItemDrop>(drop));
    auto& itemDrop = registry.get<ItemDrop>(drop);
    EXPECT_EQ(itemDrop.itemId, "base:gel");
    EXPECT_EQ(itemDrop.count, 2);
    EXPECT_FALSE(itemDrop.canPickup()); // Pickup delay not elapsed
}

// =============================================================================
// Orbit Behavior State Test
// =============================================================================

TEST(EnemyAITest, OrbitAngleAdvances) {
    EnemyAI ai("orbit");
    ai.orbitSpeed = 2.0f;
    ai.orbitAngle = 0.0f;

    // Simulate advancing the orbit
    float dt = 0.5f;
    ai.orbitAngle += ai.orbitSpeed * dt;
    EXPECT_FLOAT_EQ(ai.orbitAngle, 1.0f);

    ai.orbitAngle += ai.orbitSpeed * dt;
    EXPECT_FLOAT_EQ(ai.orbitAngle, 2.0f);
}

// =============================================================================
// Spawn Rule Filtering Test
// =============================================================================

TEST(SpawnRuleTest, NightOnlyFilter) {
    SpawnRule nightRule;
    nightRule.enemyId = "base:zombie";
    nightRule.nightOnly = true;
    nightRule.dayOnly = false;

    SpawnRule dayRule;
    dayRule.enemyId = "base:slime";
    dayRule.nightOnly = false;
    dayRule.dayOnly = true;

    SpawnRule anyTimeRule;
    anyTimeRule.enemyId = "base:bat";
    anyTimeRule.nightOnly = false;
    anyTimeRule.dayOnly = false;

    // During night: zombie and bat should be eligible, slime should not
    bool isNight = true;
    bool isDay = false;

    EXPECT_TRUE(!nightRule.nightOnly || isNight);   // zombie: eligible
    EXPECT_FALSE(!dayRule.dayOnly || isDay);          // slime: not eligible (dayOnly + not day)
    EXPECT_TRUE(!anyTimeRule.nightOnly || isNight);  // bat: eligible

    // During day: slime and bat should be eligible, zombie should not
    isNight = false;
    isDay = true;

    EXPECT_FALSE(!nightRule.nightOnly || isNight);    // zombie: not eligible (nightOnly + not night)
    EXPECT_TRUE(!dayRule.dayOnly || isDay);            // slime: eligible
    EXPECT_TRUE(!anyTimeRule.nightOnly || isNight);   // bat: eligible (not nightOnly)
}

TEST(SpawnRuleTest, DepthRangeFilter) {
    SpawnRule surfaceRule;
    surfaceRule.depthMin = -100.0f;
    surfaceRule.depthMax = 500.0f;

    SpawnRule deepRule;
    deepRule.depthMin = 500.0f;
    deepRule.depthMax = 5000.0f;

    float surfaceY = 200.0f;
    float deepY = 2000.0f;

    // Surface rule should match surface position
    EXPECT_TRUE(surfaceY >= surfaceRule.depthMin && surfaceY <= surfaceRule.depthMax);
    // Surface rule should NOT match deep position
    EXPECT_FALSE(deepY >= surfaceRule.depthMin && deepY <= surfaceRule.depthMax);

    // Deep rule should match deep position
    EXPECT_TRUE(deepY >= deepRule.depthMin && deepY <= deepRule.depthMax);
    // Deep rule should NOT match surface position
    EXPECT_FALSE(surfaceY >= deepRule.depthMin && surfaceY <= deepRule.depthMax);
}
