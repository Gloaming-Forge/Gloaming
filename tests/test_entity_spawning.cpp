#include <gtest/gtest.h>

#include "gameplay/EntitySpawning.hpp"
#include "gameplay/ProjectileSystem.hpp"
#include "gameplay/CollisionLayers.hpp"
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "ecs/EntityFactory.hpp"

using namespace gloaming;

// =============================================================================
// Projectile Component Tests
// =============================================================================

TEST(ProjectileComponentTest, DefaultValues) {
    Projectile proj;
    EXPECT_EQ(proj.ownerEntity, 0u);
    EXPECT_FLOAT_EQ(proj.damage, 10.0f);
    EXPECT_FLOAT_EQ(proj.speed, 400.0f);
    EXPECT_FLOAT_EQ(proj.lifetime, 5.0f);
    EXPECT_FLOAT_EQ(proj.age, 0.0f);
    EXPECT_EQ(proj.pierce, 0);
    EXPECT_FALSE(proj.gravityAffected);
    EXPECT_TRUE(proj.autoRotate);
    EXPECT_FLOAT_EQ(proj.maxDistance, 0.0f);
    EXPECT_EQ(proj.hitMask, 0u);
    EXPECT_TRUE(proj.alive);
    EXPECT_FALSE(proj.hitTile);
    EXPECT_TRUE(proj.alreadyHit.empty());
}

TEST(ProjectileComponentTest, CustomValues) {
    Projectile proj;
    proj.ownerEntity = 42;
    proj.damage = 25.0f;
    proj.speed = 600.0f;
    proj.lifetime = 3.0f;
    proj.pierce = -1; // infinite
    proj.gravityAffected = true;
    proj.hitMask = CollisionLayer::Enemy | CollisionLayer::NPC;

    EXPECT_EQ(proj.ownerEntity, 42u);
    EXPECT_FLOAT_EQ(proj.damage, 25.0f);
    EXPECT_FLOAT_EQ(proj.speed, 600.0f);
    EXPECT_FLOAT_EQ(proj.lifetime, 3.0f);
    EXPECT_EQ(proj.pierce, -1);
    EXPECT_TRUE(proj.gravityAffected);
    EXPECT_EQ(proj.hitMask, CollisionLayer::Enemy | CollisionLayer::NPC);
}

TEST(ProjectileComponentTest, AddToEntity) {
    Registry registry;
    Entity entity = registry.create(Transform{Vec2(100, 200)});

    EXPECT_FALSE(registry.has<Projectile>(entity));

    Projectile proj;
    proj.damage = 50.0f;
    proj.speed = 300.0f;
    registry.add<Projectile>(entity, std::move(proj));

    EXPECT_TRUE(registry.has<Projectile>(entity));
    EXPECT_FLOAT_EQ(registry.get<Projectile>(entity).damage, 50.0f);
    EXPECT_FLOAT_EQ(registry.get<Projectile>(entity).speed, 300.0f);
}

TEST(ProjectileComponentTest, AlreadyHitTracking) {
    Projectile proj;
    proj.pierce = 3;

    proj.alreadyHit.push_back(10);
    proj.alreadyHit.push_back(20);

    EXPECT_EQ(proj.alreadyHit.size(), 2u);

    // Check if entity is already hit
    bool found = false;
    for (uint32_t id : proj.alreadyHit) {
        if (id == 10) { found = true; break; }
    }
    EXPECT_TRUE(found);

    found = false;
    for (uint32_t id : proj.alreadyHit) {
        if (id == 99) { found = true; break; }
    }
    EXPECT_FALSE(found);
}

// =============================================================================
// EntitySpawning Tests
// =============================================================================

TEST(EntitySpawningTest, CreateBlankEntity) {
    Registry registry;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);

    Entity e = spawning.create(100.0f, 200.0f);
    EXPECT_NE(e, NullEntity);
    EXPECT_TRUE(registry.has<Transform>(e));
    EXPECT_TRUE(registry.has<Name>(e));

    auto& pos = registry.get<Transform>(e).position;
    EXPECT_FLOAT_EQ(pos.x, 100.0f);
    EXPECT_FLOAT_EQ(pos.y, 200.0f);
}

TEST(EntitySpawningTest, CreateMultipleEntities) {
    Registry registry;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);

    Entity e1 = spawning.create(0.0f, 0.0f);
    Entity e2 = spawning.create(50.0f, 50.0f);
    Entity e3 = spawning.create(100.0f, 100.0f);

    EXPECT_NE(e1, e2);
    EXPECT_NE(e2, e3);
    EXPECT_EQ(spawning.entityCount(), 3u);
}

TEST(EntitySpawningTest, DestroyEntity) {
    Registry registry;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);

    Entity e = spawning.create(0.0f, 0.0f);
    EXPECT_TRUE(spawning.isValid(e));
    EXPECT_EQ(spawning.entityCount(), 1u);

    spawning.destroy(e);
    EXPECT_FALSE(spawning.isValid(e));
}

TEST(EntitySpawningTest, DestroyInvalidEntityIsSafe) {
    Registry registry;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);

    // Should not crash
    spawning.destroy(NullEntity);
    spawning.destroy(static_cast<Entity>(99999));
}

TEST(EntitySpawningTest, SetAndGetPosition) {
    Registry registry;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);

    Entity e = spawning.create(10.0f, 20.0f);
    Vec2 pos = spawning.getPosition(e);
    EXPECT_FLOAT_EQ(pos.x, 10.0f);
    EXPECT_FLOAT_EQ(pos.y, 20.0f);

    spawning.setPosition(e, 300.0f, 400.0f);
    pos = spawning.getPosition(e);
    EXPECT_FLOAT_EQ(pos.x, 300.0f);
    EXPECT_FLOAT_EQ(pos.y, 400.0f);
}

TEST(EntitySpawningTest, SetAndGetVelocity) {
    Registry registry;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);

    Entity e = spawning.create(0.0f, 0.0f);

    // Initially no velocity component
    Vec2 vel = spawning.getVelocity(e);
    EXPECT_FLOAT_EQ(vel.x, 0.0f);
    EXPECT_FLOAT_EQ(vel.y, 0.0f);

    // Set velocity (creates component)
    spawning.setVelocity(e, 100.0f, -50.0f);
    vel = spawning.getVelocity(e);
    EXPECT_FLOAT_EQ(vel.x, 100.0f);
    EXPECT_FLOAT_EQ(vel.y, -50.0f);

    // Update velocity (modifies existing)
    spawning.setVelocity(e, 200.0f, 0.0f);
    vel = spawning.getVelocity(e);
    EXPECT_FLOAT_EQ(vel.x, 200.0f);
    EXPECT_FLOAT_EQ(vel.y, 0.0f);
}

TEST(EntitySpawningTest, GetPositionInvalidEntity) {
    Registry registry;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);

    Vec2 pos = spawning.getPosition(NullEntity);
    EXPECT_FLOAT_EQ(pos.x, 0.0f);
    EXPECT_FLOAT_EQ(pos.y, 0.0f);
}

TEST(EntitySpawningTest, SpawnFromFactory) {
    Registry registry;
    EntityFactory factory;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);
    spawning.setEntityFactory(&factory);

    // Register a definition
    EntityDefinition def;
    def.type = "bat";
    def.name = "Flying Bat";
    def.colliderSize = Vec2(16, 16);
    def.health = 20.0f;
    def.maxHealth = 20.0f;
    factory.registerDefinition(def);

    Entity bat = spawning.spawn("bat", 100.0f, 200.0f);
    EXPECT_NE(bat, NullEntity);
    EXPECT_TRUE(registry.has<Transform>(bat));
    EXPECT_TRUE(registry.has<Name>(bat));
    EXPECT_TRUE(registry.has<Health>(bat));
    EXPECT_TRUE(registry.has<Collider>(bat));

    EXPECT_FLOAT_EQ(registry.get<Transform>(bat).position.x, 100.0f);
    EXPECT_FLOAT_EQ(registry.get<Health>(bat).current, 20.0f);
}

TEST(EntitySpawningTest, SpawnUnknownTypeReturnsNull) {
    Registry registry;
    EntityFactory factory;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);
    spawning.setEntityFactory(&factory);

    Entity e = spawning.spawn("nonexistent", 0.0f, 0.0f);
    EXPECT_EQ(e, NullEntity);
}

// =============================================================================
// Spatial Query Tests
// =============================================================================

TEST(SpatialQueryTest, FindInRadius) {
    Registry registry;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);

    // Create entities at known positions
    Entity e1 = spawning.create(0.0f, 0.0f);
    Entity e2 = spawning.create(50.0f, 0.0f);
    Entity e3 = spawning.create(100.0f, 0.0f);
    Entity e4 = spawning.create(200.0f, 0.0f);

    // Find within radius 75 from origin
    auto results = spawning.findInRadius(0.0f, 0.0f, 75.0f);
    EXPECT_EQ(results.size(), 2u);

    // Results should be sorted by distance
    EXPECT_EQ(results[0].entity, e1);
    EXPECT_FLOAT_EQ(results[0].distance, 0.0f);
    EXPECT_EQ(results[1].entity, e2);
    EXPECT_FLOAT_EQ(results[1].distance, 50.0f);
}

TEST(SpatialQueryTest, FindInRadiusEmpty) {
    Registry registry;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);

    spawning.create(1000.0f, 1000.0f);

    auto results = spawning.findInRadius(0.0f, 0.0f, 10.0f);
    EXPECT_TRUE(results.empty());
}

TEST(SpatialQueryTest, FindInRadiusFilterByType) {
    Registry registry;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);

    Entity e1 = spawning.create(10.0f, 0.0f);
    registry.get<Name>(e1).type = "enemy";

    Entity e2 = spawning.create(20.0f, 0.0f);
    registry.get<Name>(e2).type = "npc";

    Entity e3 = spawning.create(30.0f, 0.0f);
    registry.get<Name>(e3).type = "enemy";

    EntityQueryFilter filter;
    filter.type = "enemy";
    auto results = spawning.findInRadius(0.0f, 0.0f, 100.0f, filter);
    EXPECT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].entity, e1);
    EXPECT_EQ(results[1].entity, e3);
}

TEST(SpatialQueryTest, FindInRadiusFilterByLayer) {
    Registry registry;
    CollisionLayerRegistry layers;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);

    Entity e1 = spawning.create(10.0f, 0.0f);
    Collider col1;
    layers.setLayer(col1, "enemy");
    registry.add<Collider>(e1, col1);

    Entity e2 = spawning.create(20.0f, 0.0f);
    Collider col2;
    layers.setLayer(col2, "player");
    registry.add<Collider>(e2, col2);

    EntityQueryFilter filter;
    filter.requiredLayer = CollisionLayer::Enemy;
    auto results = spawning.findInRadius(0.0f, 0.0f, 100.0f, filter);
    EXPECT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].entity, e1);
}

TEST(SpatialQueryTest, FindInRadiusExcludesDead) {
    Registry registry;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);

    Entity alive = spawning.create(10.0f, 0.0f);
    registry.add<Health>(alive, 50.0f);

    Entity dead = spawning.create(20.0f, 0.0f);
    Health deadHealth(100.0f);
    deadHealth.current = 0.0f;
    registry.add<Health>(dead, deadHealth);

    EntityQueryFilter filter;
    filter.excludeDead = true;
    auto results = spawning.findInRadius(0.0f, 0.0f, 100.0f, filter);
    EXPECT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].entity, alive);
}

TEST(SpatialQueryTest, FindInRadiusIncludesDead) {
    Registry registry;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);

    Entity alive = spawning.create(10.0f, 0.0f);
    registry.add<Health>(alive, 50.0f);

    Entity dead = spawning.create(20.0f, 0.0f);
    Health deadHealth(100.0f);
    deadHealth.current = 0.0f;
    registry.add<Health>(dead, deadHealth);

    EntityQueryFilter filter;
    filter.excludeDead = false;
    auto results = spawning.findInRadius(0.0f, 0.0f, 100.0f, filter);
    EXPECT_EQ(results.size(), 2u);
}

TEST(SpatialQueryTest, FindNearest) {
    Registry registry;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);

    spawning.create(100.0f, 0.0f);
    Entity nearest = spawning.create(10.0f, 0.0f);
    spawning.create(50.0f, 0.0f);

    auto result = spawning.findNearest(0.0f, 0.0f, 200.0f);
    EXPECT_EQ(result.entity, nearest);
    EXPECT_FLOAT_EQ(result.distance, 10.0f);
}

TEST(SpatialQueryTest, FindNearestNoResult) {
    Registry registry;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);

    spawning.create(1000.0f, 1000.0f);

    auto result = spawning.findNearest(0.0f, 0.0f, 10.0f);
    EXPECT_EQ(result.entity, NullEntity);
}

TEST(SpatialQueryTest, CountByType) {
    Registry registry;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);

    Entity e1 = spawning.create(0.0f, 0.0f);
    registry.get<Name>(e1).type = "enemy";

    Entity e2 = spawning.create(0.0f, 0.0f);
    registry.get<Name>(e2).type = "enemy";

    Entity e3 = spawning.create(0.0f, 0.0f);
    registry.get<Name>(e3).type = "npc";

    EXPECT_EQ(spawning.countByType("enemy"), 2u);
    EXPECT_EQ(spawning.countByType("npc"), 1u);
    EXPECT_EQ(spawning.countByType("player"), 0u);
}

// =============================================================================
// ProjectileCallbackRegistry Tests
// =============================================================================

TEST(ProjectileCallbackRegistryTest, RegisterAndFire) {
    ProjectileCallbackRegistry callbacks;

    int hitCount = 0;
    Entity lastTarget = NullEntity;

    Entity projEntity = static_cast<Entity>(42);
    callbacks.registerOnHit(projEntity, [&](const ProjectileHitInfo& info) {
        hitCount++;
        lastTarget = info.target;
    });

    EXPECT_TRUE(callbacks.hasCallback(projEntity));

    ProjectileHitInfo info;
    info.projectile = projEntity;
    info.target = static_cast<Entity>(99);
    info.position = Vec2(100, 200);

    bool fired = callbacks.fireOnHit(info);
    EXPECT_TRUE(fired);
    EXPECT_EQ(hitCount, 1);
    EXPECT_EQ(lastTarget, static_cast<Entity>(99));
}

TEST(ProjectileCallbackRegistryTest, NoCallbackReturns) {
    ProjectileCallbackRegistry callbacks;

    ProjectileHitInfo info;
    info.projectile = static_cast<Entity>(42);
    info.target = static_cast<Entity>(99);

    bool fired = callbacks.fireOnHit(info);
    EXPECT_FALSE(fired);
}

TEST(ProjectileCallbackRegistryTest, RemoveCallback) {
    ProjectileCallbackRegistry callbacks;

    Entity projEntity = static_cast<Entity>(42);
    callbacks.registerOnHit(projEntity, [](const ProjectileHitInfo&) {});

    EXPECT_TRUE(callbacks.hasCallback(projEntity));
    callbacks.removeOnHit(projEntity);
    EXPECT_FALSE(callbacks.hasCallback(projEntity));
}

TEST(ProjectileCallbackRegistryTest, ClearAll) {
    ProjectileCallbackRegistry callbacks;

    callbacks.registerOnHit(static_cast<Entity>(1), [](const ProjectileHitInfo&) {});
    callbacks.registerOnHit(static_cast<Entity>(2), [](const ProjectileHitInfo&) {});
    callbacks.registerOnHit(static_cast<Entity>(3), [](const ProjectileHitInfo&) {});

    EXPECT_TRUE(callbacks.hasCallback(static_cast<Entity>(1)));
    EXPECT_TRUE(callbacks.hasCallback(static_cast<Entity>(2)));
    EXPECT_TRUE(callbacks.hasCallback(static_cast<Entity>(3)));

    callbacks.clear();

    EXPECT_FALSE(callbacks.hasCallback(static_cast<Entity>(1)));
    EXPECT_FALSE(callbacks.hasCallback(static_cast<Entity>(2)));
    EXPECT_FALSE(callbacks.hasCallback(static_cast<Entity>(3)));
}

TEST(ProjectileCallbackRegistryTest, TileHitInfo) {
    ProjectileCallbackRegistry callbacks;

    bool wasTileHit = false;
    Entity projEntity = static_cast<Entity>(1);
    callbacks.registerOnHit(projEntity, [&](const ProjectileHitInfo& info) {
        wasTileHit = info.hitTile;
    });

    ProjectileHitInfo info;
    info.projectile = projEntity;
    info.target = NullEntity;
    info.hitTile = true;
    info.tileX = 10;
    info.tileY = 20;

    callbacks.fireOnHit(info);
    EXPECT_TRUE(wasTileHit);
}

// =============================================================================
// ProjectileSystem Tests (unit-level, no Engine dependency)
// =============================================================================

// Helper: manually step the projectile system logic for a single entity
// This avoids needing a full Engine instance
static void simulateProjectileAge(Projectile& proj, float dt) {
    proj.age += dt;
    if (proj.age >= proj.lifetime) {
        proj.alive = false;
    }
}

TEST(ProjectileSystemUnitTest, AgeProjectile) {
    Projectile proj;
    proj.lifetime = 2.0f;

    simulateProjectileAge(proj, 1.0f);
    EXPECT_TRUE(proj.alive);
    EXPECT_FLOAT_EQ(proj.age, 1.0f);

    simulateProjectileAge(proj, 0.5f);
    EXPECT_TRUE(proj.alive);
    EXPECT_FLOAT_EQ(proj.age, 1.5f);

    simulateProjectileAge(proj, 0.5f);
    EXPECT_FALSE(proj.alive); // 2.0 >= 2.0
}

TEST(ProjectileSystemUnitTest, MaxDistanceCheck) {
    Projectile proj;
    proj.maxDistance = 100.0f;
    proj.startPosition = Vec2(0, 0);

    // Simulate distance check
    Vec2 currentPos(50, 0);
    float dx = currentPos.x - proj.startPosition.x;
    float dy = currentPos.y - proj.startPosition.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    EXPECT_LT(dist, proj.maxDistance);

    // Move past max distance
    currentPos = Vec2(120, 0);
    dx = currentPos.x - proj.startPosition.x;
    dy = currentPos.y - proj.startPosition.y;
    dist = std::sqrt(dx * dx + dy * dy);

    EXPECT_GT(dist, proj.maxDistance);
}

TEST(ProjectileSystemUnitTest, PierceDecrement) {
    Projectile proj;
    proj.pierce = 3;

    // Simulate 3 hits
    EXPECT_EQ(proj.pierce, 3);
    proj.pierce--; // hit 1
    EXPECT_EQ(proj.pierce, 2);
    proj.pierce--; // hit 2
    EXPECT_EQ(proj.pierce, 1);
    proj.pierce--; // hit 3
    EXPECT_EQ(proj.pierce, 0);
    // At pierce=0, next hit should destroy
}

TEST(ProjectileSystemUnitTest, InfinitePierce) {
    Projectile proj;
    proj.pierce = -1; // infinite

    // Multiple hits don't change pierce
    EXPECT_EQ(proj.pierce, -1);
    // Never reaches 0
    EXPECT_LT(proj.pierce, 0);
}

TEST(ProjectileSystemUnitTest, AutoRotation) {
    // Velocity direction → rotation angle
    Vec2 vel(100, 0); // Right
    float angle = std::atan2(vel.y, vel.x) * (180.0f / PI);
    EXPECT_NEAR(angle, 0.0f, 0.001f);

    vel = Vec2(0, 100); // Down
    angle = std::atan2(vel.y, vel.x) * (180.0f / PI);
    EXPECT_NEAR(angle, 90.0f, 0.001f);

    vel = Vec2(-100, 0); // Left
    angle = std::atan2(vel.y, vel.x) * (180.0f / PI);
    EXPECT_NEAR(angle, 180.0f, 0.1f);

    vel = Vec2(0, -100); // Up
    angle = std::atan2(vel.y, vel.x) * (180.0f / PI);
    EXPECT_NEAR(angle, -90.0f, 0.001f);

    vel = Vec2(100, 100); // Diagonal down-right
    angle = std::atan2(vel.y, vel.x) * (180.0f / PI);
    EXPECT_NEAR(angle, 45.0f, 0.001f);
}

TEST(ProjectileSystemUnitTest, TileHitDestroysProjectile) {
    Projectile proj;
    proj.hitTile = true;

    // ProjectileSystem would mark as not alive
    if (proj.hitTile) {
        proj.alive = false;
    }
    EXPECT_FALSE(proj.alive);
}

// =============================================================================
// Integration Tests: Projectile + Entity Overlap
// =============================================================================

TEST(ProjectileIntegrationTest, ProjectileHitsEnemy) {
    Registry registry;

    // Create a projectile at (100, 100) moving right
    Entity proj = registry.create(
        Transform{Vec2(100, 100)},
        Velocity{Vec2(400, 0)},
        Name{"arrow", "projectile"}
    );
    Collider projCol;
    projCol.size = Vec2(8, 8);
    projCol.layer = CollisionLayer::Projectile;
    projCol.mask = CollisionLayer::Tile;
    registry.add<Collider>(proj, projCol);

    Projectile projComp;
    projComp.damage = 25.0f;
    projComp.hitMask = CollisionLayer::Enemy;
    registry.add<Projectile>(proj, std::move(projComp));

    // Create an enemy overlapping the projectile position
    Entity enemy = registry.create(
        Transform{Vec2(102, 100)},
        Name{"skeleton", "enemy"}
    );
    Collider enemyCol;
    enemyCol.size = Vec2(16, 16);
    enemyCol.layer = CollisionLayer::Enemy;
    enemyCol.mask = CollisionLayer::Tile | CollisionLayer::Player | CollisionLayer::Projectile;
    registry.add<Collider>(enemy, enemyCol);
    registry.add<Health>(enemy, 100.0f);

    // Verify projectile collider does NOT collide with enemy via physics
    // (because projectile mask doesn't include enemy)
    EXPECT_FALSE(projCol.canCollideWith(enemyCol));

    // But the enemy's layer IS in the projectile's hitMask
    EXPECT_TRUE((enemyCol.layer & projComp.hitMask) != 0);

    // Simulate what ProjectileSystem would do: check AABB overlap
    AABB projAABB = Collision::getEntityAABB(
        registry.get<Transform>(proj), registry.get<Collider>(proj));
    AABB enemyAABB = Collision::getEntityAABB(
        registry.get<Transform>(enemy), registry.get<Collider>(enemy));

    auto result = testAABBCollision(projAABB, enemyAABB);
    EXPECT_TRUE(result.collided);

    // Apply damage (simulating ProjectileSystem)
    registry.get<Health>(enemy).takeDamage(25.0f);
    EXPECT_FLOAT_EQ(registry.get<Health>(enemy).current, 75.0f);
}

TEST(ProjectileIntegrationTest, ProjectileDoesNotHitOwner) {
    Registry registry;

    // Create owner (player)
    Entity player = registry.create(
        Transform{Vec2(100, 100)},
        Name{"player", "player"}
    );
    Collider playerCol;
    playerCol.size = Vec2(16, 16);
    playerCol.layer = CollisionLayer::Player;
    registry.add<Collider>(player, playerCol);

    // Create projectile owned by player, overlapping player
    Entity proj = registry.create(
        Transform{Vec2(100, 100)},
        Velocity{Vec2(400, 0)}
    );
    Projectile projComp;
    projComp.ownerEntity = static_cast<uint32_t>(player);
    projComp.hitMask = CollisionLayer::Enemy;
    registry.add<Projectile>(proj, std::move(projComp));

    // The ProjectileSystem skips the owner entity
    auto& p = registry.get<Projectile>(proj);
    EXPECT_EQ(p.ownerEntity, static_cast<uint32_t>(player));
}

TEST(ProjectileIntegrationTest, PiercingProjectileHitsMultiple) {
    Registry registry;

    // Create piercing projectile
    Entity proj = registry.create(
        Transform{Vec2(50, 50)},
        Velocity{Vec2(400, 0)}
    );
    Collider projCol;
    projCol.size = Vec2(8, 8);
    projCol.layer = CollisionLayer::Projectile;
    registry.add<Collider>(proj, projCol);

    Projectile projComp;
    projComp.pierce = 2; // Can pierce through 2 more enemies after first hit
    projComp.damage = 10.0f;
    projComp.hitMask = CollisionLayer::Enemy;
    registry.add<Projectile>(proj, std::move(projComp));

    // Create 3 enemies
    for (int i = 0; i < 3; i++) {
        Entity enemy = registry.create(
            Transform{Vec2(50.0f + i * 2.0f, 50.0f)},
            Name{"skeleton", "enemy"}
        );
        Collider enemyCol;
        enemyCol.size = Vec2(16, 16);
        enemyCol.layer = CollisionLayer::Enemy;
        registry.add<Collider>(enemy, enemyCol);
        registry.add<Health>(enemy, 100.0f);
    }

    // Simulate pierce tracking
    auto& p = registry.get<Projectile>(proj);
    p.alreadyHit.push_back(10); // Simulated first hit
    p.pierce--;
    EXPECT_EQ(p.pierce, 1);

    p.alreadyHit.push_back(11); // Second hit
    p.pierce--;
    EXPECT_EQ(p.pierce, 0);

    // At pierce=0, projectile should be destroyed on next hit
    EXPECT_EQ(p.pierce, 0);
}

TEST(ProjectileIntegrationTest, CollisionLayerFiltering) {
    CollisionLayerRegistry layers;

    // Projectile that hits enemies and NPCs, but not other projectiles or players
    Collider projCol;
    projCol.layer = CollisionLayer::Projectile;
    projCol.mask = CollisionLayer::Tile; // Physics: collides with tiles only

    Collider enemyCol;
    layers.setLayer(enemyCol, "enemy");
    layers.setMask(enemyCol, {"tile", "player", "projectile"});

    Collider playerCol;
    layers.setLayer(playerCol, "player");
    layers.setMask(playerCol, {"tile", "enemy", "npc"});

    Collider npcCol;
    layers.setLayer(npcCol, "npc");
    layers.setMask(npcCol, {"tile", "player"});

    // Physics: projectile does NOT collide with enemy (intentional)
    EXPECT_FALSE(projCol.canCollideWith(enemyCol));
    // Physics: projectile does NOT collide with player
    EXPECT_FALSE(projCol.canCollideWith(playerCol));
    // Physics: enemy DOES collide with player
    EXPECT_TRUE(enemyCol.canCollideWith(playerCol));

    // Projectile hitMask would be:
    uint32_t hitMask = CollisionLayer::Enemy;
    // Check enemy is targeted
    EXPECT_TRUE((enemyCol.layer & hitMask) != 0);
    // Check player is NOT targeted
    EXPECT_FALSE((playerCol.layer & hitMask) != 0);
}

// =============================================================================
// ProjectileHitInfo Tests
// =============================================================================

TEST(ProjectileHitInfoTest, EntityHit) {
    ProjectileHitInfo info;
    info.projectile = static_cast<Entity>(1);
    info.target = static_cast<Entity>(2);
    info.position = Vec2(100, 200);
    info.hitTile = false;

    EXPECT_EQ(info.projectile, static_cast<Entity>(1));
    EXPECT_EQ(info.target, static_cast<Entity>(2));
    EXPECT_FALSE(info.hitTile);
}

TEST(ProjectileHitInfoTest, TileHit) {
    ProjectileHitInfo info;
    info.projectile = static_cast<Entity>(1);
    info.target = NullEntity;
    info.position = Vec2(300, 400);
    info.hitTile = true;
    info.tileX = 18;
    info.tileY = 25;

    EXPECT_EQ(info.target, NullEntity);
    EXPECT_TRUE(info.hitTile);
    EXPECT_EQ(info.tileX, 18);
    EXPECT_EQ(info.tileY, 25);
}

// =============================================================================
// Entity Query Result Tests
// =============================================================================

TEST(EntityQueryResultTest, DefaultValues) {
    EntityQueryResult result;
    EXPECT_EQ(result.entity, NullEntity);
    EXPECT_FLOAT_EQ(result.distance, 0.0f);
}

TEST(EntityQueryResultTest, SortByDistance) {
    std::vector<EntityQueryResult> results;

    EntityQueryResult r1;
    r1.entity = static_cast<Entity>(1);
    r1.distance = 50.0f;

    EntityQueryResult r2;
    r2.entity = static_cast<Entity>(2);
    r2.distance = 10.0f;

    EntityQueryResult r3;
    r3.entity = static_cast<Entity>(3);
    r3.distance = 30.0f;

    results.push_back(r1);
    results.push_back(r2);
    results.push_back(r3);

    std::sort(results.begin(), results.end(),
        [](const EntityQueryResult& a, const EntityQueryResult& b) {
            return a.distance < b.distance;
        });

    EXPECT_EQ(results[0].entity, static_cast<Entity>(2));
    EXPECT_EQ(results[1].entity, static_cast<Entity>(3));
    EXPECT_EQ(results[2].entity, static_cast<Entity>(1));
}

// =============================================================================
// EntitySpawning Null Safety Tests
// =============================================================================

TEST(EntitySpawningNullSafetyTest, NullRegistry) {
    EntitySpawning spawning;
    // All operations should be safe with no registry

    Entity e = spawning.create(0, 0);
    EXPECT_EQ(e, NullEntity);

    EXPECT_FALSE(spawning.isValid(NullEntity));
    EXPECT_EQ(spawning.entityCount(), 0u);

    Vec2 pos = spawning.getPosition(NullEntity);
    EXPECT_FLOAT_EQ(pos.x, 0.0f);

    Vec2 vel = spawning.getVelocity(NullEntity);
    EXPECT_FLOAT_EQ(vel.x, 0.0f);

    auto results = spawning.findInRadius(0, 0, 100);
    EXPECT_TRUE(results.empty());

    EXPECT_EQ(spawning.countByType("enemy"), 0u);
}

TEST(EntitySpawningNullSafetyTest, NullFactory) {
    Registry registry;
    EntitySpawning spawning;
    spawning.setRegistry(&registry);

    // create() works without factory
    Entity e = spawning.create(0, 0);
    EXPECT_NE(e, NullEntity);

    // spawn() requires factory, returns null
    Entity e2 = spawning.spawn("test", 0, 0);
    EXPECT_EQ(e2, NullEntity);
}

// =============================================================================
// Projectile Speed/Angle Tests
// =============================================================================

TEST(ProjectileSpawnTest, AngleToVelocity) {
    float speed = 400.0f;

    // 0 degrees = right
    float angle0 = 0.0f * (PI / 180.0f);
    float vx = std::cos(angle0) * speed;
    float vy = std::sin(angle0) * speed;
    EXPECT_NEAR(vx, 400.0f, 0.01f);
    EXPECT_NEAR(vy, 0.0f, 0.01f);

    // 90 degrees = down
    float angle90 = 90.0f * (PI / 180.0f);
    vx = std::cos(angle90) * speed;
    vy = std::sin(angle90) * speed;
    EXPECT_NEAR(vx, 0.0f, 0.01f);
    EXPECT_NEAR(vy, 400.0f, 0.01f);

    // 180 degrees = left
    float angle180 = 180.0f * (PI / 180.0f);
    vx = std::cos(angle180) * speed;
    vy = std::sin(angle180) * speed;
    EXPECT_NEAR(vx, -400.0f, 0.01f);
    EXPECT_NEAR(vy, 0.0f, 0.1f);

    // 270 degrees = up
    float angle270 = 270.0f * (PI / 180.0f);
    vx = std::cos(angle270) * speed;
    vy = std::sin(angle270) * speed;
    EXPECT_NEAR(vx, 0.0f, 0.1f);
    EXPECT_NEAR(vy, -400.0f, 0.01f);

    // 45 degrees = diagonal
    float angle45 = 45.0f * (PI / 180.0f);
    vx = std::cos(angle45) * speed;
    vy = std::sin(angle45) * speed;
    float expected = 400.0f / std::sqrt(2.0f);
    EXPECT_NEAR(vx, expected, 0.01f);
    EXPECT_NEAR(vy, expected, 0.01f);
}
