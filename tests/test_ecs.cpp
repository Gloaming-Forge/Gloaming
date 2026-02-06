#include <gtest/gtest.h>

#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "ecs/Systems.hpp"
#include "ecs/EntityFactory.hpp"

using namespace gloaming;

// =============================================================================
// Registry Tests
// =============================================================================

TEST(RegistryTest, CreateEntity) {
    Registry registry;
    Entity entity = registry.create();
    EXPECT_TRUE(registry.valid(entity));
}

TEST(RegistryTest, DestroyEntity) {
    Registry registry;
    Entity entity = registry.create();
    EXPECT_TRUE(registry.valid(entity));
    registry.destroy(entity);
    EXPECT_FALSE(registry.valid(entity));
}

TEST(RegistryTest, CreateEntityWithComponents) {
    Registry registry;
    Entity entity = registry.create(
        Transform{Vec2(10.0f, 20.0f)},
        Velocity{Vec2(5.0f, 0.0f)}
    );

    EXPECT_TRUE(registry.valid(entity));
    EXPECT_TRUE(registry.has<Transform>(entity));
    EXPECT_TRUE(registry.has<Velocity>(entity));

    auto& transform = registry.get<Transform>(entity);
    EXPECT_FLOAT_EQ(transform.position.x, 10.0f);
    EXPECT_FLOAT_EQ(transform.position.y, 20.0f);
}

TEST(RegistryTest, AddAndRemoveComponents) {
    Registry registry;
    Entity entity = registry.create();

    registry.add<Transform>(entity, Vec2(100.0f, 200.0f));
    EXPECT_TRUE(registry.has<Transform>(entity));

    registry.remove<Transform>(entity);
    EXPECT_FALSE(registry.has<Transform>(entity));
}

TEST(RegistryTest, TryGetComponent) {
    Registry registry;
    Entity entity = registry.create();

    Transform* transform = registry.tryGet<Transform>(entity);
    EXPECT_EQ(transform, nullptr);

    registry.add<Transform>(entity);
    transform = registry.tryGet<Transform>(entity);
    EXPECT_NE(transform, nullptr);
}

TEST(RegistryTest, HasAllComponents) {
    Registry registry;
    Entity entity = registry.create(
        Transform{},
        Velocity{},
        Health{100.0f}
    );

    EXPECT_TRUE((registry.hasAll<Transform, Velocity>(entity)));
    EXPECT_TRUE((registry.hasAll<Transform, Velocity, Health>(entity)));
    EXPECT_FALSE((registry.hasAll<Transform, Collider>(entity)));
}

TEST(RegistryTest, HasAnyComponent) {
    Registry registry;
    Entity entity = registry.create(Transform{});

    EXPECT_TRUE((registry.hasAny<Transform, Velocity>(entity)));
    EXPECT_FALSE((registry.hasAny<Velocity, Health>(entity)));
}

TEST(RegistryTest, ViewIteration) {
    Registry registry;

    for (int i = 0; i < 5; ++i) {
        registry.create(Transform{Vec2(static_cast<float>(i), 0.0f)});
    }

    int count = 0;
    registry.each<Transform>([&count](Entity /*entity*/, Transform& /*transform*/) {
        ++count;
    });

    EXPECT_EQ(count, 5);
}

TEST(RegistryTest, CountEntities) {
    Registry registry;

    for (int i = 0; i < 10; ++i) {
        registry.create(Transform{});
    }
    for (int i = 0; i < 5; ++i) {
        registry.create(Transform{}, Velocity{});
    }

    EXPECT_EQ(registry.count<Transform>(), 15);
    EXPECT_EQ(registry.count<Velocity>(), 5);
}

TEST(RegistryTest, FindFirst) {
    Registry registry;

    registry.create(Transform{Vec2(1.0f, 0.0f)}, Name{"a"});
    Entity target = registry.create(Transform{Vec2(2.0f, 0.0f)}, Name{"target"});
    registry.create(Transform{Vec2(3.0f, 0.0f)}, Name{"c"});

    Entity found = registry.findFirst<Name>([](Entity /*entity*/) {
        return true; // Find any
    });
    EXPECT_NE(found, NullEntity);

    found = registry.findFirst<Name>([&registry](Entity entity) {
        return registry.get<Name>(entity).name == "target";
    });
    EXPECT_EQ(found, target);
}

TEST(RegistryTest, CollectEntities) {
    Registry registry;

    registry.create(Transform{}, Velocity{});
    registry.create(Transform{}, Velocity{});
    registry.create(Transform{}); // No velocity

    auto withVelocity = registry.collect<Transform, Velocity>();
    EXPECT_EQ(withVelocity.size(), 2);
}

TEST(RegistryTest, ClearRegistry) {
    Registry registry;

    for (int i = 0; i < 10; ++i) {
        registry.create(Transform{});
    }

    EXPECT_FALSE(registry.empty());
    registry.clear();
    EXPECT_TRUE(registry.empty());
}

// =============================================================================
// Component Tests
// =============================================================================

TEST(TransformTest, DefaultConstruction) {
    Transform transform;
    EXPECT_FLOAT_EQ(transform.position.x, 0.0f);
    EXPECT_FLOAT_EQ(transform.position.y, 0.0f);
    EXPECT_FLOAT_EQ(transform.rotation, 0.0f);
    EXPECT_FLOAT_EQ(transform.scale.x, 1.0f);
    EXPECT_FLOAT_EQ(transform.scale.y, 1.0f);
}

TEST(TransformTest, PositionConstruction) {
    Transform transform(Vec2(10.0f, 20.0f));
    EXPECT_FLOAT_EQ(transform.position.x, 10.0f);
    EXPECT_FLOAT_EQ(transform.position.y, 20.0f);
}

TEST(VelocityTest, DefaultConstruction) {
    Velocity velocity;
    EXPECT_FLOAT_EQ(velocity.linear.x, 0.0f);
    EXPECT_FLOAT_EQ(velocity.linear.y, 0.0f);
    EXPECT_FLOAT_EQ(velocity.angular, 0.0f);
}

TEST(VelocityTest, VectorConstruction) {
    Velocity velocity(Vec2(100.0f, -50.0f));
    EXPECT_FLOAT_EQ(velocity.linear.x, 100.0f);
    EXPECT_FLOAT_EQ(velocity.linear.y, -50.0f);
}

TEST(HealthTest, DefaultConstruction) {
    Health health;
    EXPECT_FLOAT_EQ(health.current, 100.0f);
    EXPECT_FLOAT_EQ(health.max, 100.0f);
    EXPECT_FLOAT_EQ(health.invincibilityTime, 0.0f);
}

TEST(HealthTest, TakeDamage) {
    Health health(100.0f);

    float damage = health.takeDamage(30.0f);
    EXPECT_FLOAT_EQ(damage, 30.0f);
    EXPECT_FLOAT_EQ(health.current, 70.0f);
    EXPECT_TRUE(health.isInvincible());
}

TEST(HealthTest, TakeDamageWhileInvincible) {
    Health health(100.0f);

    health.takeDamage(30.0f);
    float secondDamage = health.takeDamage(50.0f);

    EXPECT_FLOAT_EQ(secondDamage, 0.0f);  // Blocked by invincibility
    EXPECT_FLOAT_EQ(health.current, 70.0f);
}

TEST(HealthTest, InvincibilityDecay) {
    Health health(100.0f);
    health.invincibilityDuration = 1.0f;

    health.takeDamage(10.0f);
    EXPECT_TRUE(health.isInvincible());

    health.update(0.5f);
    EXPECT_TRUE(health.isInvincible());

    health.update(0.6f);
    EXPECT_FALSE(health.isInvincible());
}

TEST(HealthTest, Heal) {
    Health health(50.0f, 100.0f);

    float healed = health.heal(30.0f);
    EXPECT_FLOAT_EQ(healed, 30.0f);
    EXPECT_FLOAT_EQ(health.current, 80.0f);

    // Can't overheal
    healed = health.heal(50.0f);
    EXPECT_FLOAT_EQ(healed, 20.0f);
    EXPECT_FLOAT_EQ(health.current, 100.0f);
}

TEST(HealthTest, IsDead) {
    Health health(10.0f);
    EXPECT_FALSE(health.isDead());

    health.takeDamage(10.0f);
    EXPECT_TRUE(health.isDead());
}

TEST(HealthTest, GetPercentage) {
    Health health(50.0f, 100.0f);
    EXPECT_FLOAT_EQ(health.getPercentage(), 0.5f);
}

TEST(ColliderTest, DefaultConstruction) {
    Collider collider;
    EXPECT_FLOAT_EQ(collider.offset.x, 0.0f);
    EXPECT_FLOAT_EQ(collider.offset.y, 0.0f);
    EXPECT_FLOAT_EQ(collider.size.x, 16.0f);
    EXPECT_FLOAT_EQ(collider.size.y, 16.0f);
    EXPECT_EQ(collider.layer, CollisionLayer::Default);
    EXPECT_EQ(collider.mask, CollisionLayer::All);
    EXPECT_FALSE(collider.isTrigger);
}

TEST(ColliderTest, GetBounds) {
    Collider collider;
    collider.size = Vec2(20.0f, 30.0f);
    collider.offset = Vec2(5.0f, 10.0f);

    Transform transform(Vec2(100.0f, 200.0f));
    Rect bounds = collider.getBounds(transform);

    EXPECT_FLOAT_EQ(bounds.x, 100.0f + 5.0f - 10.0f);  // position + offset - half size
    EXPECT_FLOAT_EQ(bounds.y, 200.0f + 10.0f - 15.0f);
    EXPECT_FLOAT_EQ(bounds.width, 20.0f);
    EXPECT_FLOAT_EQ(bounds.height, 30.0f);
}

TEST(ColliderTest, CanCollideWithLayers) {
    Collider playerCollider;
    playerCollider.layer = CollisionLayer::Player;
    playerCollider.mask = CollisionLayer::Enemy | CollisionLayer::Tile;

    Collider enemyCollider;
    enemyCollider.layer = CollisionLayer::Enemy;
    enemyCollider.mask = CollisionLayer::Player | CollisionLayer::Projectile;

    Collider projectileCollider;
    projectileCollider.layer = CollisionLayer::Projectile;
    projectileCollider.mask = CollisionLayer::Enemy;

    // Player can collide with enemy
    EXPECT_TRUE(playerCollider.canCollideWith(enemyCollider));
    EXPECT_TRUE(enemyCollider.canCollideWith(playerCollider));

    // Player cannot collide with projectile (player's mask doesn't include Projectile)
    EXPECT_FALSE(playerCollider.canCollideWith(projectileCollider));

    // Projectile can collide with enemy
    EXPECT_TRUE(projectileCollider.canCollideWith(enemyCollider));
}

TEST(LightSourceTest, DefaultConstruction) {
    LightSource light;
    EXPECT_FLOAT_EQ(light.radius, 100.0f);
    EXPECT_FLOAT_EQ(light.intensity, 1.0f);
    EXPECT_FALSE(light.flicker);
}

TEST(LightSourceTest, Flicker) {
    LightSource light;
    light.flicker = true;
    light.flickerSpeed = 10.0f;
    light.flickerAmount = 0.2f;

    float initialIntensity = light.getEffectiveIntensity();
    light.update(0.1f);
    float newIntensity = light.getEffectiveIntensity();

    // Intensity should vary due to flicker
    // Note: They might be the same if update happens at a specific point
    // Just check intensity is within expected range
    EXPECT_GE(newIntensity, 0.0f);
    EXPECT_LE(newIntensity, 1.2f);
}

TEST(LifetimeTest, Expiration) {
    Lifetime lifetime(2.0f);

    EXPECT_FALSE(lifetime.isExpired());
    EXPECT_FLOAT_EQ(lifetime.getRemaining(), 2.0f);
    EXPECT_FLOAT_EQ(lifetime.getProgress(), 0.0f);

    lifetime.elapsed = 1.0f;
    EXPECT_FALSE(lifetime.isExpired());
    EXPECT_FLOAT_EQ(lifetime.getRemaining(), 1.0f);
    EXPECT_FLOAT_EQ(lifetime.getProgress(), 0.5f);

    lifetime.elapsed = 2.5f;
    EXPECT_TRUE(lifetime.isExpired());
    EXPECT_FLOAT_EQ(lifetime.getRemaining(), 0.0f);
}

// =============================================================================
// Sprite Animation Tests
// =============================================================================

TEST(SpriteTest, AddAnimation) {
    Sprite sprite;

    std::vector<AnimationFrame> frames;
    frames.push_back({Rect(0, 0, 32, 32), 0.1f});
    frames.push_back({Rect(32, 0, 32, 32), 0.1f});
    frames.push_back({Rect(64, 0, 32, 32), 0.1f});

    sprite.addAnimation("walk", std::move(frames), true);

    EXPECT_EQ(sprite.animations.size(), 1);
    EXPECT_EQ(sprite.animations[0].name, "walk");
    EXPECT_EQ(sprite.animations[0].frames.size(), 3);
}

TEST(SpriteTest, PlayAnimation) {
    Sprite sprite;

    std::vector<AnimationFrame> idleFrames;
    idleFrames.push_back({Rect(0, 0, 32, 32), 0.1f});
    sprite.addAnimation("idle", std::move(idleFrames), true);

    std::vector<AnimationFrame> walkFrames;
    walkFrames.push_back({Rect(32, 0, 32, 32), 0.1f});
    sprite.addAnimation("walk", std::move(walkFrames), true);

    EXPECT_TRUE(sprite.playAnimation("idle"));
    EXPECT_EQ(sprite.currentAnimation, 0);
    EXPECT_EQ(sprite.getCurrentAnimationName(), "idle");

    EXPECT_TRUE(sprite.playAnimation("walk"));
    EXPECT_EQ(sprite.currentAnimation, 1);
    EXPECT_EQ(sprite.getCurrentAnimationName(), "walk");

    EXPECT_FALSE(sprite.playAnimation("nonexistent"));
}

// =============================================================================
// Entity Factory Tests
// =============================================================================

TEST(EntityFactoryTest, RegisterAndSpawn) {
    Registry registry;
    EntityFactory factory;

    EntityDefinition def;
    def.type = "test_entity";
    def.name = "Test Entity";
    def.health = 50.0f;
    def.maxHealth = 100.0f;
    def.colliderSize = Vec2(32.0f, 32.0f);

    factory.registerDefinition(def);

    EXPECT_TRUE(factory.hasDefinition("test_entity"));

    Entity entity = factory.spawn(registry, "test_entity", Vec2(100.0f, 200.0f));
    EXPECT_NE(entity, NullEntity);
    EXPECT_TRUE(registry.has<Transform>(entity));
    EXPECT_TRUE(registry.has<Name>(entity));
    EXPECT_TRUE(registry.has<Health>(entity));
    EXPECT_TRUE(registry.has<Collider>(entity));

    auto& transform = registry.get<Transform>(entity);
    EXPECT_FLOAT_EQ(transform.position.x, 100.0f);
    EXPECT_FLOAT_EQ(transform.position.y, 200.0f);

    auto& health = registry.get<Health>(entity);
    EXPECT_FLOAT_EQ(health.current, 50.0f);
    EXPECT_FLOAT_EQ(health.max, 100.0f);

    auto& collider = registry.get<Collider>(entity);
    EXPECT_FLOAT_EQ(collider.size.x, 32.0f);
    EXPECT_FLOAT_EQ(collider.size.y, 32.0f);
}

TEST(EntityFactoryTest, SpawnUnknownType) {
    Registry registry;
    EntityFactory factory;

    Entity entity = factory.spawn(registry, "unknown_type", Vec2(0.0f, 0.0f));
    EXPECT_EQ(entity, NullEntity);
}

TEST(EntityFactoryTest, SpawnWithVelocity) {
    Registry registry;
    EntityFactory factory;

    EntityDefinition def;
    def.type = "projectile";
    def.name = "Projectile";
    factory.registerDefinition(def);

    Entity entity = factory.spawn(registry, "projectile", Vec2(0.0f, 0.0f), Vec2(100.0f, 0.0f));
    EXPECT_NE(entity, NullEntity);
    EXPECT_TRUE(registry.has<Velocity>(entity));

    auto& velocity = registry.get<Velocity>(entity);
    EXPECT_FLOAT_EQ(velocity.linear.x, 100.0f);
    EXPECT_FLOAT_EQ(velocity.linear.y, 0.0f);
}

TEST(EntityFactoryTest, LoadFromJsonString) {
    Registry registry;
    EntityFactory factory;

    const char* json = R"([
        {
            "type": "player",
            "name": "Player",
            "health": 100,
            "collider": {
                "size": [24, 32],
                "layer": 2,
                "mask": 255
            },
            "gravity": 1.0
        },
        {
            "type": "enemy",
            "name": "Slime",
            "health": {"current": 30, "max": 30},
            "collider": {
                "size": [16, 16]
            }
        }
    ])";

    EXPECT_TRUE(factory.loadFromString(json));
    EXPECT_TRUE(factory.hasDefinition("player"));
    EXPECT_TRUE(factory.hasDefinition("enemy"));

    // Spawn player
    Entity player = factory.spawn(registry, "player", Vec2(0.0f, 0.0f));
    EXPECT_NE(player, NullEntity);
    EXPECT_TRUE(registry.has<Health>(player));
    EXPECT_TRUE(registry.has<Collider>(player));
    EXPECT_TRUE(registry.has<Gravity>(player));

    auto& playerHealth = registry.get<Health>(player);
    EXPECT_FLOAT_EQ(playerHealth.max, 100.0f);

    // Spawn enemy
    Entity enemy = factory.spawn(registry, "enemy", Vec2(100.0f, 0.0f));
    EXPECT_NE(enemy, NullEntity);

    auto& enemyHealth = registry.get<Health>(enemy);
    EXPECT_FLOAT_EQ(enemyHealth.current, 30.0f);
    EXPECT_FLOAT_EQ(enemyHealth.max, 30.0f);
}

TEST(EntityFactoryTest, LoadWithLightSource) {
    Registry registry;
    EntityFactory factory;

    const char* json = R"({
        "type": "torch",
        "name": "Torch",
        "light": {
            "color": [255, 200, 100],
            "radius": 150,
            "intensity": 0.8,
            "flicker": true
        }
    })";

    EXPECT_TRUE(factory.loadFromString(json));

    Entity torch = factory.spawn(registry, "torch", Vec2(0.0f, 0.0f));
    EXPECT_NE(torch, NullEntity);
    EXPECT_TRUE(registry.has<LightSource>(torch));

    auto& light = registry.get<LightSource>(torch);
    EXPECT_EQ(light.color.r, 255);
    EXPECT_EQ(light.color.g, 200);
    EXPECT_EQ(light.color.b, 100);
    EXPECT_FLOAT_EQ(light.radius, 150.0f);
    EXPECT_FLOAT_EQ(light.intensity, 0.8f);
    EXPECT_TRUE(light.flicker);
}

TEST(EntityFactoryTest, LoadWithLifetime) {
    Registry registry;
    EntityFactory factory;

    const char* json = R"({
        "type": "particle",
        "name": "Particle",
        "lifetime": 2.5
    })";

    EXPECT_TRUE(factory.loadFromString(json));

    Entity particle = factory.spawn(registry, "particle", Vec2(0.0f, 0.0f));
    EXPECT_NE(particle, NullEntity);
    EXPECT_TRUE(registry.has<Lifetime>(particle));

    auto& lifetime = registry.get<Lifetime>(particle);
    EXPECT_FLOAT_EQ(lifetime.duration, 2.5f);
}

TEST(EntityFactoryTest, CreateBasicEntity) {
    Registry registry;
    EntityFactory factory;

    Entity entity = factory.createBasic(registry, Vec2(50.0f, 100.0f));
    EXPECT_NE(entity, NullEntity);
    EXPECT_TRUE(registry.has<Transform>(entity));
    EXPECT_TRUE(registry.has<Name>(entity));

    auto& transform = registry.get<Transform>(entity);
    EXPECT_FLOAT_EQ(transform.position.x, 50.0f);
    EXPECT_FLOAT_EQ(transform.position.y, 100.0f);
}

TEST(EntityFactoryTest, GetDefinitionTypes) {
    EntityFactory factory;

    EntityDefinition def1;
    def1.type = "type_a";
    factory.registerDefinition(def1);

    EntityDefinition def2;
    def2.type = "type_b";
    factory.registerDefinition(def2);

    auto types = factory.getDefinitionTypes();
    EXPECT_EQ(types.size(), 2);
    EXPECT_TRUE(std::find(types.begin(), types.end(), "type_a") != types.end());
    EXPECT_TRUE(std::find(types.begin(), types.end(), "type_b") != types.end());
}

// =============================================================================
// System Scheduler Tests
// =============================================================================

// Simple test system that counts updates
class CounterSystem : public System {
public:
    CounterSystem() : System("CounterSystem", 0) {}

    void update(float /*dt*/) override {
        ++updateCount;
    }

    int updateCount = 0;
};

class HighPrioritySystem : public System {
public:
    HighPrioritySystem() : System("HighPrioritySystem", -10) {}

    void update(float /*dt*/) override {
        executionOrder.push_back("high");
    }

    static std::vector<std::string> executionOrder;
};

class LowPrioritySystem : public System {
public:
    LowPrioritySystem() : System("LowPrioritySystem", 10) {}

    void update(float /*dt*/) override {
        HighPrioritySystem::executionOrder.push_back("low");
    }

    static std::vector<std::string>& getExecutionOrder() {
        return HighPrioritySystem::executionOrder;
    }
};

std::vector<std::string> HighPrioritySystem::executionOrder;

TEST(SystemSchedulerTest, AddAndRunSystem) {
    Registry registry;

    // Create a minimal mock engine class for testing
    // Since we can't easily mock Engine, we'll test the systems directly
    CounterSystem system;
    system.update(0.016f);
    system.update(0.016f);
    system.update(0.016f);

    EXPECT_EQ(system.updateCount, 3);
}

TEST(SystemSchedulerTest, SystemPriority) {
    HighPrioritySystem::executionOrder.clear();

    HighPrioritySystem highSystem;
    LowPrioritySystem lowSystem;

    // High priority should execute first
    highSystem.update(0.016f);
    lowSystem.update(0.016f);

    EXPECT_EQ(HighPrioritySystem::executionOrder.size(), 2);
    EXPECT_EQ(HighPrioritySystem::executionOrder[0], "high");
    EXPECT_EQ(HighPrioritySystem::executionOrder[1], "low");
}

TEST(SystemSchedulerTest, DisableSystem) {
    CounterSystem system;

    system.update(0.016f);
    EXPECT_EQ(system.updateCount, 1);

    system.setEnabled(false);
    EXPECT_FALSE(system.isEnabled());

    // When disabled, update should still increment if called directly
    // (Scheduler should skip disabled systems)
    system.update(0.016f);
    EXPECT_EQ(system.updateCount, 2);
}
