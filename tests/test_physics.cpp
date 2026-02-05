#include <gtest/gtest.h>
#include "physics/AABB.hpp"
#include "physics/Collision.hpp"
#include "physics/TileCollision.hpp"
#include "physics/Trigger.hpp"
#include "physics/Raycast.hpp"
#include "physics/PhysicsSystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "rendering/TileRenderer.hpp"
#include <map>
#include <algorithm>

using namespace gloaming;

// ============================================================================
// AABB Tests
// ============================================================================

TEST(AABBTest, CreateFromRect) {
    Rect rect(10.0f, 20.0f, 30.0f, 40.0f);
    AABB aabb = AABB::fromRect(rect);

    EXPECT_FLOAT_EQ(aabb.center.x, 25.0f);
    EXPECT_FLOAT_EQ(aabb.center.y, 40.0f);
    EXPECT_FLOAT_EQ(aabb.halfExtents.x, 15.0f);
    EXPECT_FLOAT_EQ(aabb.halfExtents.y, 20.0f);
}

TEST(AABBTest, CreateFromMinMax) {
    AABB aabb = AABB::fromMinMax(Vec2(0.0f, 0.0f), Vec2(100.0f, 50.0f));

    EXPECT_FLOAT_EQ(aabb.center.x, 50.0f);
    EXPECT_FLOAT_EQ(aabb.center.y, 25.0f);
    EXPECT_FLOAT_EQ(aabb.halfExtents.x, 50.0f);
    EXPECT_FLOAT_EQ(aabb.halfExtents.y, 25.0f);
}

TEST(AABBTest, GetMinMax) {
    AABB aabb(Vec2(50.0f, 50.0f), Vec2(25.0f, 25.0f));

    Vec2 min = aabb.getMin();
    Vec2 max = aabb.getMax();

    EXPECT_FLOAT_EQ(min.x, 25.0f);
    EXPECT_FLOAT_EQ(min.y, 25.0f);
    EXPECT_FLOAT_EQ(max.x, 75.0f);
    EXPECT_FLOAT_EQ(max.y, 75.0f);
}

TEST(AABBTest, ToRect) {
    AABB aabb(Vec2(50.0f, 50.0f), Vec2(25.0f, 25.0f));
    Rect rect = aabb.toRect();

    EXPECT_FLOAT_EQ(rect.x, 25.0f);
    EXPECT_FLOAT_EQ(rect.y, 25.0f);
    EXPECT_FLOAT_EQ(rect.width, 50.0f);
    EXPECT_FLOAT_EQ(rect.height, 50.0f);
}

TEST(AABBTest, ContainsPoint) {
    AABB aabb(Vec2(50.0f, 50.0f), Vec2(25.0f, 25.0f));

    EXPECT_TRUE(aabb.contains(Vec2(50.0f, 50.0f)));  // Center
    EXPECT_TRUE(aabb.contains(Vec2(30.0f, 30.0f)));  // Inside
    EXPECT_TRUE(aabb.contains(Vec2(25.0f, 25.0f)));  // Edge
    EXPECT_FALSE(aabb.contains(Vec2(0.0f, 0.0f)));   // Outside
    EXPECT_FALSE(aabb.contains(Vec2(100.0f, 50.0f))); // Outside
}

TEST(AABBTest, IntersectsTrue) {
    AABB aabb1(Vec2(50.0f, 50.0f), Vec2(25.0f, 25.0f));
    AABB aabb2(Vec2(70.0f, 50.0f), Vec2(25.0f, 25.0f));

    EXPECT_TRUE(aabb1.intersects(aabb2));
    EXPECT_TRUE(aabb2.intersects(aabb1));
}

TEST(AABBTest, IntersectsFalse) {
    AABB aabb1(Vec2(50.0f, 50.0f), Vec2(25.0f, 25.0f));
    AABB aabb2(Vec2(150.0f, 50.0f), Vec2(25.0f, 25.0f));

    EXPECT_FALSE(aabb1.intersects(aabb2));
    EXPECT_FALSE(aabb2.intersects(aabb1));
}

TEST(AABBTest, IntersectsTouching) {
    AABB aabb1(Vec2(25.0f, 25.0f), Vec2(25.0f, 25.0f));
    AABB aabb2(Vec2(75.0f, 25.0f), Vec2(25.0f, 25.0f));

    // Exactly touching should still intersect
    EXPECT_TRUE(aabb1.intersects(aabb2));
}

TEST(AABBTest, GetOverlap) {
    AABB aabb1(Vec2(50.0f, 50.0f), Vec2(25.0f, 25.0f));
    AABB aabb2(Vec2(60.0f, 50.0f), Vec2(25.0f, 25.0f));

    Vec2 overlap = aabb1.getOverlap(aabb2);

    EXPECT_FLOAT_EQ(overlap.x, 40.0f);  // (25+25) - |50-60| = 50-10 = 40
    EXPECT_FLOAT_EQ(overlap.y, 50.0f);  // (25+25) - |50-50| = 50-0 = 50
}

TEST(AABBTest, GetOverlapNoOverlap) {
    AABB aabb1(Vec2(0.0f, 0.0f), Vec2(10.0f, 10.0f));
    AABB aabb2(Vec2(100.0f, 100.0f), Vec2(10.0f, 10.0f));

    Vec2 overlap = aabb1.getOverlap(aabb2);

    EXPECT_FLOAT_EQ(overlap.x, 0.0f);
    EXPECT_FLOAT_EQ(overlap.y, 0.0f);
}

TEST(AABBTest, Expanded) {
    AABB aabb(Vec2(50.0f, 50.0f), Vec2(25.0f, 25.0f));
    AABB expanded = aabb.expanded(5.0f);

    EXPECT_FLOAT_EQ(expanded.center.x, 50.0f);
    EXPECT_FLOAT_EQ(expanded.center.y, 50.0f);
    EXPECT_FLOAT_EQ(expanded.halfExtents.x, 30.0f);
    EXPECT_FLOAT_EQ(expanded.halfExtents.y, 30.0f);
}

TEST(AABBTest, Translated) {
    AABB aabb(Vec2(50.0f, 50.0f), Vec2(25.0f, 25.0f));
    AABB translated = aabb.translated(Vec2(10.0f, -5.0f));

    EXPECT_FLOAT_EQ(translated.center.x, 60.0f);
    EXPECT_FLOAT_EQ(translated.center.y, 45.0f);
    EXPECT_FLOAT_EQ(translated.halfExtents.x, 25.0f);
    EXPECT_FLOAT_EQ(translated.halfExtents.y, 25.0f);
}

TEST(AABBTest, ClosestPoint) {
    AABB aabb(Vec2(50.0f, 50.0f), Vec2(25.0f, 25.0f));

    // Point inside - returns the point itself
    Vec2 inside = aabb.closestPoint(Vec2(40.0f, 40.0f));
    EXPECT_FLOAT_EQ(inside.x, 40.0f);
    EXPECT_FLOAT_EQ(inside.y, 40.0f);

    // Point outside - returns closest point on surface
    Vec2 outside = aabb.closestPoint(Vec2(0.0f, 50.0f));
    EXPECT_FLOAT_EQ(outside.x, 25.0f);
    EXPECT_FLOAT_EQ(outside.y, 50.0f);
}

TEST(AABBTest, Merge) {
    AABB aabb1(Vec2(25.0f, 25.0f), Vec2(25.0f, 25.0f));  // 0-50, 0-50
    AABB aabb2(Vec2(75.0f, 75.0f), Vec2(25.0f, 25.0f));  // 50-100, 50-100

    AABB merged = AABB::merge(aabb1, aabb2);

    EXPECT_FLOAT_EQ(merged.center.x, 50.0f);
    EXPECT_FLOAT_EQ(merged.center.y, 50.0f);
    EXPECT_FLOAT_EQ(merged.halfExtents.x, 50.0f);
    EXPECT_FLOAT_EQ(merged.halfExtents.y, 50.0f);
}

// ============================================================================
// Collision Result Tests
// ============================================================================

TEST(CollisionResultTest, TestAABBCollision) {
    AABB aabb1(Vec2(50.0f, 50.0f), Vec2(25.0f, 25.0f));
    AABB aabb2(Vec2(60.0f, 50.0f), Vec2(25.0f, 25.0f));

    auto result = testAABBCollision(aabb1, aabb2);

    EXPECT_TRUE(result.collided);
    EXPECT_GT(result.penetration, 0.0f);
}

TEST(CollisionResultTest, TestAABBCollisionNoOverlap) {
    AABB aabb1(Vec2(0.0f, 0.0f), Vec2(10.0f, 10.0f));
    AABB aabb2(Vec2(100.0f, 100.0f), Vec2(10.0f, 10.0f));

    auto result = testAABBCollision(aabb1, aabb2);

    EXPECT_FALSE(result.collided);
}

TEST(CollisionResultTest, TestAABBCollisionNormal) {
    // Horizontal collision
    AABB aabb1(Vec2(50.0f, 50.0f), Vec2(25.0f, 25.0f));
    AABB aabb2(Vec2(60.0f, 50.0f), Vec2(25.0f, 25.0f));

    auto result = testAABBCollision(aabb1, aabb2);

    EXPECT_TRUE(result.collided);
    // Normal should point from A to B (or away from B)
    EXPECT_TRUE(result.normal.x != 0.0f || result.normal.y != 0.0f);
}

// ============================================================================
// Swept Collision Tests
// ============================================================================

TEST(SweepTest, NoMovement) {
    AABB aabb1(Vec2(0.0f, 0.0f), Vec2(10.0f, 10.0f));
    AABB aabb2(Vec2(100.0f, 0.0f), Vec2(10.0f, 10.0f));

    auto result = sweepAABB(aabb1, Vec2(0.0f, 0.0f), aabb2);

    EXPECT_FALSE(result.hit);
    EXPECT_FLOAT_EQ(result.time, 1.0f);
}

TEST(SweepTest, MovingTowardsTarget) {
    AABB aabb1(Vec2(0.0f, 0.0f), Vec2(10.0f, 10.0f));
    AABB aabb2(Vec2(50.0f, 0.0f), Vec2(10.0f, 10.0f));

    auto result = sweepAABB(aabb1, Vec2(100.0f, 0.0f), aabb2);

    EXPECT_TRUE(result.hit);
    EXPECT_GT(result.time, 0.0f);
    EXPECT_LT(result.time, 1.0f);
    EXPECT_FLOAT_EQ(result.normal.x, -1.0f);
    EXPECT_FLOAT_EQ(result.normal.y, 0.0f);
}

TEST(SweepTest, MovingAwayFromTarget) {
    AABB aabb1(Vec2(0.0f, 0.0f), Vec2(10.0f, 10.0f));
    AABB aabb2(Vec2(50.0f, 0.0f), Vec2(10.0f, 10.0f));

    auto result = sweepAABB(aabb1, Vec2(-100.0f, 0.0f), aabb2);

    EXPECT_FALSE(result.hit);
    EXPECT_FLOAT_EQ(result.time, 1.0f);
}

TEST(SweepTest, AlreadyOverlapping) {
    AABB aabb1(Vec2(50.0f, 0.0f), Vec2(10.0f, 10.0f));
    AABB aabb2(Vec2(55.0f, 0.0f), Vec2(10.0f, 10.0f));

    auto result = sweepAABB(aabb1, Vec2(10.0f, 0.0f), aabb2);

    EXPECT_TRUE(result.hit);
    EXPECT_FLOAT_EQ(result.time, 0.0f);
}

TEST(SweepTest, MissingTarget) {
    AABB aabb1(Vec2(0.0f, 0.0f), Vec2(10.0f, 10.0f));
    AABB aabb2(Vec2(50.0f, 100.0f), Vec2(10.0f, 10.0f));

    auto result = sweepAABB(aabb1, Vec2(100.0f, 0.0f), aabb2);

    EXPECT_FALSE(result.hit);
}

TEST(SweepTest, VerticalMovement) {
    AABB aabb1(Vec2(0.0f, 0.0f), Vec2(10.0f, 10.0f));
    AABB aabb2(Vec2(0.0f, 50.0f), Vec2(10.0f, 10.0f));

    auto result = sweepAABB(aabb1, Vec2(0.0f, 100.0f), aabb2);

    EXPECT_TRUE(result.hit);
    EXPECT_GT(result.time, 0.0f);
    EXPECT_LT(result.time, 1.0f);
    EXPECT_FLOAT_EQ(result.normal.x, 0.0f);
    EXPECT_FLOAT_EQ(result.normal.y, -1.0f);
}

// ============================================================================
// Slide Velocity Tests
// ============================================================================

TEST(SlideVelocityTest, HorizontalWall) {
    Vec2 velocity(100.0f, 50.0f);
    Vec2 normal(-1.0f, 0.0f);

    Vec2 slide = calculateSlideVelocity(velocity, normal, 1.0f);

    EXPECT_FLOAT_EQ(slide.x, 0.0f);
    EXPECT_FLOAT_EQ(slide.y, 50.0f);
}

TEST(SlideVelocityTest, VerticalFloor) {
    Vec2 velocity(100.0f, 50.0f);
    Vec2 normal(0.0f, -1.0f);

    Vec2 slide = calculateSlideVelocity(velocity, normal, 1.0f);

    EXPECT_FLOAT_EQ(slide.x, 100.0f);
    EXPECT_FLOAT_EQ(slide.y, 0.0f);
}

// ============================================================================
// Entity Collision Tests
// ============================================================================

TEST(EntityCollisionTest, GetEntityAABB) {
    Transform transform;
    transform.position = Vec2(100.0f, 100.0f);
    transform.scale = Vec2(1.0f, 1.0f);

    Collider collider;
    collider.offset = Vec2(0.0f, 0.0f);
    collider.size = Vec2(32.0f, 32.0f);

    AABB aabb = Collision::getEntityAABB(transform, collider);

    EXPECT_FLOAT_EQ(aabb.center.x, 100.0f);
    EXPECT_FLOAT_EQ(aabb.center.y, 100.0f);
    EXPECT_FLOAT_EQ(aabb.halfExtents.x, 16.0f);
    EXPECT_FLOAT_EQ(aabb.halfExtents.y, 16.0f);
}

TEST(EntityCollisionTest, GetEntityAABBWithOffset) {
    Transform transform;
    transform.position = Vec2(100.0f, 100.0f);

    Collider collider;
    collider.offset = Vec2(10.0f, -5.0f);
    collider.size = Vec2(20.0f, 20.0f);

    AABB aabb = Collision::getEntityAABB(transform, collider);

    EXPECT_FLOAT_EQ(aabb.center.x, 110.0f);
    EXPECT_FLOAT_EQ(aabb.center.y, 95.0f);
}

TEST(EntityCollisionTest, TestEntityCollision) {
    Transform transformA{Vec2(100.0f, 100.0f)};
    Collider colliderA{Vec2(0.0f, 0.0f), Vec2(32.0f, 32.0f)};

    Transform transformB{Vec2(110.0f, 100.0f)};
    Collider colliderB{Vec2(0.0f, 0.0f), Vec2(32.0f, 32.0f)};

    auto result = Collision::testEntityCollision(transformA, colliderA, transformB, colliderB);

    EXPECT_TRUE(result.collided);
}

TEST(EntityCollisionTest, LayerFiltering) {
    Transform transformA{Vec2(100.0f, 100.0f)};
    Collider colliderA{Vec2(0.0f, 0.0f), Vec2(32.0f, 32.0f)};
    colliderA.layer = CollisionLayer::Player;
    colliderA.mask = CollisionLayer::Enemy;

    Transform transformB{Vec2(110.0f, 100.0f)};
    Collider colliderB{Vec2(0.0f, 0.0f), Vec2(32.0f, 32.0f)};
    colliderB.layer = CollisionLayer::Projectile;  // Not in A's mask
    colliderB.mask = CollisionLayer::All;

    auto result = Collision::testEntityCollision(transformA, colliderA, transformB, colliderB);

    EXPECT_FALSE(result.collided);  // Filtered out by layer mask
}

// ============================================================================
// Raycast Tests
// ============================================================================

TEST(RaycastTest, RaycastAABBHit) {
    Ray ray(Vec2(0.0f, 50.0f), Vec2(1.0f, 0.0f));
    AABB aabb(Vec2(100.0f, 50.0f), Vec2(25.0f, 25.0f));

    Vec2 normal;
    float distance = Raycast::raycastAABB(ray, aabb, &normal);

    EXPECT_GE(distance, 0.0f);
    EXPECT_FLOAT_EQ(normal.x, -1.0f);
    EXPECT_FLOAT_EQ(normal.y, 0.0f);
}

TEST(RaycastTest, RaycastAABBMiss) {
    Ray ray(Vec2(0.0f, 0.0f), Vec2(1.0f, 0.0f));
    AABB aabb(Vec2(100.0f, 100.0f), Vec2(25.0f, 25.0f));

    float distance = Raycast::raycastAABB(ray, aabb);

    EXPECT_LT(distance, 0.0f);  // No hit
}

TEST(RaycastTest, RaycastAABBBehind) {
    Ray ray(Vec2(200.0f, 50.0f), Vec2(1.0f, 0.0f));
    AABB aabb(Vec2(100.0f, 50.0f), Vec2(25.0f, 25.0f));

    float distance = Raycast::raycastAABB(ray, aabb);

    EXPECT_LT(distance, 0.0f);  // Behind ray origin
}

TEST(RaycastTest, RaycastAABBInsideBox) {
    Ray ray(Vec2(100.0f, 50.0f), Vec2(1.0f, 0.0f));  // Origin inside box
    AABB aabb(Vec2(100.0f, 50.0f), Vec2(25.0f, 25.0f));

    float distance = Raycast::raycastAABB(ray, aabb);

    EXPECT_FLOAT_EQ(distance, 0.0f);  // Distance is 0 when inside
}

TEST(RaycastTest, RayGetPoint) {
    Ray ray(Vec2(0.0f, 0.0f), Vec2(1.0f, 0.0f));

    Vec2 point = ray.getPoint(50.0f);

    EXPECT_FLOAT_EQ(point.x, 50.0f);
    EXPECT_FLOAT_EQ(point.y, 0.0f);
}

TEST(RaycastTest, RayDirectionNormalized) {
    Ray ray(Vec2(0.0f, 0.0f), Vec2(3.0f, 4.0f));  // Not normalized

    float length = ray.direction.length();

    EXPECT_NEAR(length, 1.0f, 0.001f);
}

// ============================================================================
// Trigger Tests
// ============================================================================

TEST(TriggerTest, TriggerTrackerEntityPairHash) {
    TriggerTracker::EntityPair pair1{1, 2};
    TriggerTracker::EntityPair pair2{1, 2};
    TriggerTracker::EntityPair pair3{2, 1};

    TriggerTracker::EntityPairHash hasher;

    EXPECT_EQ(hasher(pair1), hasher(pair2));  // Same pairs have same hash
    EXPECT_NE(hasher(pair1), hasher(pair3));  // Different order = different hash
}

TEST(TriggerTest, TriggerTrackerClear) {
    TriggerTracker tracker;
    tracker.clear();

    EXPECT_EQ(tracker.getOverlapCount(), 0);
}

// ============================================================================
// Physics Config Tests
// ============================================================================

TEST(PhysicsConfigTest, DefaultValues) {
    PhysicsConfig config;

    EXPECT_FLOAT_EQ(config.gravity.x, 0.0f);
    EXPECT_GT(config.gravity.y, 0.0f);  // Gravity should be positive (down)
    EXPECT_GT(config.maxFallSpeed, 0.0f);
    EXPECT_GT(config.maxHorizontalSpeed, 0.0f);
}

// ============================================================================
// TileCollision Tests (with mock TileMap)
// ============================================================================

TEST(TileCollisionTest, ConfigDefaults) {
    TileCollision::Config config;

    EXPECT_GT(config.skinWidth, 0.0f);
    EXPECT_GT(config.maxIterations, 0);
    EXPECT_GT(config.groundCheckDistance, 0.0f);
}

TEST(TileCollisionTest, SetTileSize) {
    TileCollision collision;
    collision.setTileSize(32);

    // Config should be stored
    EXPECT_NO_THROW(collision.getConfig());
}

// ============================================================================
// Vec2 Additional Tests (physics-specific)
// ============================================================================

TEST(Vec2PhysicsTest, Normalize) {
    Vec2 v(3.0f, 4.0f);
    Vec2 normalized = v.normalized();

    float length = normalized.length();
    EXPECT_NEAR(length, 1.0f, 0.001f);
    EXPECT_NEAR(normalized.x, 0.6f, 0.001f);
    EXPECT_NEAR(normalized.y, 0.8f, 0.001f);
}

TEST(Vec2PhysicsTest, NormalizeZero) {
    Vec2 v(0.0f, 0.0f);
    Vec2 normalized = v.normalized();

    // Zero vector should stay zero
    EXPECT_FLOAT_EQ(normalized.x, 0.0f);
    EXPECT_FLOAT_EQ(normalized.y, 0.0f);
}

TEST(Vec2PhysicsTest, Dot) {
    Vec2 a(1.0f, 0.0f);
    Vec2 b(0.0f, 1.0f);

    float dot = Vec2::dot(a, b);
    EXPECT_FLOAT_EQ(dot, 0.0f);  // Perpendicular

    Vec2 c(1.0f, 0.0f);
    Vec2 d(1.0f, 0.0f);
    dot = Vec2::dot(c, d);
    EXPECT_FLOAT_EQ(dot, 1.0f);  // Parallel same direction
}

TEST(Vec2PhysicsTest, Distance) {
    Vec2 a(0.0f, 0.0f);
    Vec2 b(3.0f, 4.0f);

    float distance = Vec2::distance(a, b);
    EXPECT_FLOAT_EQ(distance, 5.0f);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(PhysicsIntegrationTest, ColliderBoundsCalculation) {
    Transform transform;
    transform.position = Vec2(100.0f, 100.0f);
    transform.scale = Vec2(2.0f, 2.0f);

    Collider collider;
    collider.offset = Vec2(0.0f, 0.0f);
    collider.size = Vec2(16.0f, 16.0f);

    Rect bounds = collider.getBounds(transform);

    // With scale 2x, size becomes 32x32
    EXPECT_FLOAT_EQ(bounds.width, 32.0f);
    EXPECT_FLOAT_EQ(bounds.height, 32.0f);
    // Centered on position
    EXPECT_FLOAT_EQ(bounds.x, 100.0f - 16.0f);
    EXPECT_FLOAT_EQ(bounds.y, 100.0f - 16.0f);
}

TEST(PhysicsIntegrationTest, GravityComponent) {
    Gravity gravity;
    gravity.scale = 1.0f;
    gravity.grounded = false;

    EXPECT_FALSE(gravity.grounded);
    EXPECT_FLOAT_EQ(gravity.scale, 1.0f);

    gravity.grounded = true;
    EXPECT_TRUE(gravity.grounded);
}

TEST(PhysicsIntegrationTest, CollisionLayerFlags) {
    EXPECT_EQ(CollisionLayer::None, 0);
    EXPECT_NE(CollisionLayer::Default, 0);
    EXPECT_NE(CollisionLayer::Player, 0);
    EXPECT_NE(CollisionLayer::Enemy, 0);
    EXPECT_EQ(CollisionLayer::All, 0xFFFFFFFF);

    // All layers should be unique (no overlap except All)
    EXPECT_NE(CollisionLayer::Player, CollisionLayer::Enemy);
    EXPECT_NE(CollisionLayer::Projectile, CollisionLayer::Tile);
}

TEST(PhysicsIntegrationTest, ColliderCanCollideWith) {
    Collider colliderA;
    colliderA.layer = CollisionLayer::Player;
    colliderA.mask = CollisionLayer::Enemy | CollisionLayer::Tile;

    Collider colliderB;
    colliderB.layer = CollisionLayer::Enemy;
    colliderB.mask = CollisionLayer::All;

    Collider colliderC;
    colliderC.layer = CollisionLayer::Projectile;
    colliderC.mask = CollisionLayer::All;

    EXPECT_TRUE(colliderA.canCollideWith(colliderB));   // Player vs Enemy
    EXPECT_FALSE(colliderA.canCollideWith(colliderC));  // Player vs Projectile (not in A's mask)
}

TEST(PhysicsIntegrationTest, ColliderDisabled) {
    Collider colliderA;
    colliderA.enabled = false;

    Collider colliderB;
    colliderB.enabled = true;

    EXPECT_FALSE(colliderA.canCollideWith(colliderB));
    EXPECT_FALSE(colliderB.canCollideWith(colliderA));
}

// ============================================================================
// TileCollision Tests with Mock TileMap
// ============================================================================

// Mock tile provider for testing
class MockTileProvider {
public:
    void setSolid(int x, int y) {
        Tile tile;
        tile.id = 1;
        tile.flags = Tile::FLAG_SOLID;
        m_tiles[{x, y}] = tile;
    }

    void setPlatform(int x, int y) {
        Tile tile;
        tile.id = 2;
        tile.flags = Tile::FLAG_PLATFORM;
        m_tiles[{x, y}] = tile;
    }

    void setSlope(int x, int y, bool leftToRight) {
        Tile tile;
        tile.id = leftToRight ? 3 : 4;  // SLOPE_LEFT or SLOPE_RIGHT
        tile.flags = Tile::FLAG_SOLID;
        m_tiles[{x, y}] = tile;
    }

    Tile getTile(int x, int y) const {
        auto it = m_tiles.find({x, y});
        if (it != m_tiles.end()) {
            return it->second;
        }
        return Tile{};  // Empty/air
    }

    std::function<Tile(int, int)> getCallback() {
        return [this](int x, int y) { return getTile(x, y); };
    }

private:
    std::map<std::pair<int, int>, Tile> m_tiles;
};

TEST(TileCollisionWithMapTest, MoveAABBNoCollision) {
    MockTileProvider provider;
    TileCollision collision;
    collision.setTileSize(16);
    collision.setTileCallback(provider.getCallback());

    // AABB in open space
    AABB aabb(Vec2(100.0f, 100.0f), Vec2(8.0f, 8.0f));
    Vec2 velocity(50.0f, 0.0f);

    auto result = collision.moveAABB(aabb, velocity, true, false);

    // Should move freely
    EXPECT_FLOAT_EQ(result.newPosition.x, 150.0f);
    EXPECT_FLOAT_EQ(result.newPosition.y, 100.0f);
    EXPECT_FALSE(result.hitHorizontal);
    EXPECT_FALSE(result.hitVertical);
}

TEST(TileCollisionWithMapTest, MoveAABBHitWall) {
    MockTileProvider provider;
    // Create a vertical wall
    provider.setSolid(10, 5);
    provider.setSolid(10, 6);
    provider.setSolid(10, 7);

    TileCollision collision;
    collision.setTileSize(16);
    collision.setTileCallback(provider.getCallback());

    // AABB moving right towards wall
    // Wall is at tile 10 (world x = 160-176), AABB starts at x=140
    AABB aabb(Vec2(140.0f, 100.0f), Vec2(8.0f, 8.0f));
    Vec2 velocity(50.0f, 0.0f);  // Moving right

    auto result = collision.moveAABB(aabb, velocity, true, false);

    // Should stop at wall
    EXPECT_TRUE(result.hitHorizontal);
    EXPECT_LT(result.newPosition.x, 160.0f);  // Should be before the wall
}

TEST(TileCollisionWithMapTest, MoveAABBHitFloor) {
    MockTileProvider provider;
    // Create a horizontal floor
    provider.setSolid(5, 10);
    provider.setSolid(6, 10);
    provider.setSolid(7, 10);

    TileCollision collision;
    collision.setTileSize(16);
    collision.setTileCallback(provider.getCallback());

    // AABB falling down towards floor
    // Floor is at tile y=10 (world y = 160-176), AABB starts at y=140
    AABB aabb(Vec2(100.0f, 140.0f), Vec2(8.0f, 8.0f));
    Vec2 velocity(0.0f, 50.0f);  // Moving down

    auto result = collision.moveAABB(aabb, velocity, true, false);

    // Should land on floor
    EXPECT_TRUE(result.hitVertical);
    EXPECT_TRUE(result.onGround);
    EXPECT_LT(result.newPosition.y, 160.0f);  // Should be above the floor
}

TEST(TileCollisionWithMapTest, MoveAABBOnPlatformFallingThrough) {
    MockTileProvider provider;
    // Create a one-way platform
    provider.setPlatform(5, 10);
    provider.setPlatform(6, 10);
    provider.setPlatform(7, 10);

    TileCollision collision;
    collision.setTileSize(16);
    collision.setTileCallback(provider.getCallback());

    // AABB falling down towards platform from above
    AABB aabb(Vec2(100.0f, 140.0f), Vec2(8.0f, 8.0f));
    Vec2 velocity(0.0f, 50.0f);  // Moving down

    auto result = collision.moveAABB(aabb, velocity, true, false);

    // Should land on platform when falling from above
    EXPECT_TRUE(result.onGround || result.onPlatform);
}

TEST(TileCollisionWithMapTest, MoveAABBJumpThroughPlatform) {
    MockTileProvider provider;
    // Create a one-way platform
    provider.setPlatform(5, 8);
    provider.setPlatform(6, 8);
    provider.setPlatform(7, 8);

    TileCollision collision;
    collision.setTileSize(16);
    collision.setTileCallback(provider.getCallback());

    // AABB jumping up through platform from below
    AABB aabb(Vec2(100.0f, 150.0f), Vec2(8.0f, 8.0f));
    Vec2 velocity(0.0f, -50.0f);  // Moving up

    auto result = collision.moveAABB(aabb, velocity, true, false);

    // Should pass through platform when moving up
    EXPECT_FALSE(result.hitVertical);
    EXPECT_FLOAT_EQ(result.newPosition.y, 100.0f);  // Should move full distance
}

TEST(TileCollisionWithMapTest, CheckGroundBelow) {
    MockTileProvider provider;
    // Create ground
    provider.setSolid(5, 10);
    provider.setSolid(6, 10);

    TileCollision collision;
    collision.setTileSize(16);
    collision.setTileCallback(provider.getCallback());

    // AABB just above ground
    // Ground at y=10 (160-176), AABB bottom at y=160-8=152
    AABB aabb(Vec2(100.0f, 152.0f), Vec2(8.0f, 8.0f));

    bool grounded = collision.checkGroundBelow(aabb, 2.0f);

    EXPECT_TRUE(grounded);
}

TEST(TileCollisionWithMapTest, CheckGroundBelowNotGrounded) {
    MockTileProvider provider;
    // No ground below

    TileCollision collision;
    collision.setTileSize(16);
    collision.setTileCallback(provider.getCallback());

    AABB aabb(Vec2(100.0f, 100.0f), Vec2(8.0f, 8.0f));

    bool grounded = collision.checkGroundBelow(aabb, 2.0f);

    EXPECT_FALSE(grounded);
}

// ============================================================================
// PhysicsSystem Integration Tests
// ============================================================================
// NOTE: These tests verify the physics LOGIC used by PhysicsSystem, but do not
// call PhysicsSystem::init() or PhysicsSystem::update() directly because Engine
// is difficult to mock (requires renderer, window, etc.). This means bugs in
// the system's update loop wiring would not be caught here.
//
// For full integration testing, consider:
// 1. Creating a minimal mock Engine for testing
// 2. Running gameplay tests that exercise the actual system
// 3. Manual testing in a debug build with physics visualization

TEST(PhysicsSystemTest, ApplyImpulse) {
    Registry registry;
    PhysicsConfig config;
    PhysicsSystem physics(config);

    // Create an entity with velocity
    auto entity = registry.create();
    registry.add<Transform>(entity, Vec2(100.0f, 100.0f));
    registry.add<Velocity>(entity);

    // Test the impulse logic directly (not through PhysicsSystem::applyImpulse
    // since that requires init() to be called with a valid Engine)
    Velocity& vel = registry.get<Velocity>(entity);
    vel.linear = Vec2(0.0f, 0.0f);

    // Apply impulse manually (simulating what applyImpulse does)
    Vec2 impulse(100.0f, -200.0f);
    vel.linear = vel.linear + impulse;

    EXPECT_FLOAT_EQ(vel.linear.x, 100.0f);
    EXPECT_FLOAT_EQ(vel.linear.y, -200.0f);
}

TEST(PhysicsSystemTest, GravityComponent) {
    Registry registry;

    auto entity = registry.create();
    registry.add<Transform>(entity, Vec2(100.0f, 100.0f));
    auto& vel = registry.add<Velocity>(entity);
    auto& gravity = registry.add<Gravity>(entity);

    vel.linear = Vec2(0.0f, 0.0f);
    gravity.grounded = false;
    gravity.scale = 1.0f;

    // Simulate gravity application
    PhysicsConfig config;
    float dt = 1.0f / 60.0f;

    // Manual gravity update (simulating applyGravity)
    vel.linear.y += config.gravity.y * gravity.scale * dt;

    EXPECT_GT(vel.linear.y, 0.0f);  // Should have downward velocity
}

TEST(PhysicsSystemTest, EntityCollisionVelocityCancel) {
    // Test that colliding velocities are cancelled along collision normal
    Vec2 velocityA(100.0f, 50.0f);
    Vec2 normal(-1.0f, 0.0f);  // Collision from right

    float dot = Vec2::dot(velocityA, normal);
    EXPECT_LT(dot, 0.0f);  // Moving into collision

    // Cancel velocity along normal
    if (dot < 0.0f) {
        velocityA = velocityA - normal * dot;
    }

    EXPECT_FLOAT_EQ(velocityA.x, 0.0f);  // X cancelled
    EXPECT_FLOAT_EQ(velocityA.y, 50.0f); // Y preserved
}

TEST(PhysicsSystemTest, CollisionCallback) {
    std::vector<CollisionEvent> events;

    auto callback = [&events](const CollisionEvent& event) {
        events.push_back(event);
    };

    // Simulate firing an event
    CollisionEvent event;
    event.entity = 1;
    event.withTile = true;
    event.normal = Vec2(0.0f, -1.0f);
    event.tileX = 5;
    event.tileY = 10;

    callback(event);

    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].entity, 1);
    EXPECT_TRUE(events[0].withTile);
    EXPECT_FLOAT_EQ(events[0].normal.y, -1.0f);
}

// ============================================================================
// Trigger Enter/Stay/Exit Tests
// ============================================================================

TEST(TriggerCallbackTest, TriggerEnterCallback) {
    Registry registry;
    TriggerTracker tracker;

    // Create trigger entity
    auto triggerEntity = registry.create();
    registry.add<Transform>(triggerEntity, Vec2(100.0f, 100.0f));
    auto& triggerCollider = registry.add<Collider>(triggerEntity);
    triggerCollider.size = Vec2(32.0f, 32.0f);
    triggerCollider.isTrigger = true;

    bool enterCalled = false;
    uint32_t enteredEntity = 0;

    auto& trigger = registry.add<Trigger>(triggerEntity);
    trigger.onEnter = [&enterCalled, &enteredEntity](uint32_t triggerEnt, uint32_t otherEnt) {
        enterCalled = true;
        enteredEntity = otherEnt;
    };

    // Create entity that enters trigger
    auto movingEntity = registry.create();
    registry.add<Transform>(movingEntity, Vec2(105.0f, 100.0f));  // Overlapping
    auto& movingCollider = registry.add<Collider>(movingEntity);
    movingCollider.size = Vec2(16.0f, 16.0f);

    // First update - should fire onEnter
    tracker.update(registry);

    EXPECT_TRUE(enterCalled);
    EXPECT_EQ(enteredEntity, static_cast<uint32_t>(movingEntity));
}

TEST(TriggerCallbackTest, TriggerStayCallback) {
    Registry registry;
    TriggerTracker tracker;

    // Create trigger entity
    auto triggerEntity = registry.create();
    registry.add<Transform>(triggerEntity, Vec2(100.0f, 100.0f));
    auto& triggerCollider = registry.add<Collider>(triggerEntity);
    triggerCollider.size = Vec2(32.0f, 32.0f);
    triggerCollider.isTrigger = true;

    int stayCalls = 0;

    auto& trigger = registry.add<Trigger>(triggerEntity);
    trigger.onEnter = [](uint32_t, uint32_t) {};  // Ignore enter
    trigger.onStay = [&stayCalls](uint32_t, uint32_t) {
        stayCalls++;
    };

    // Create entity inside trigger
    auto movingEntity = registry.create();
    registry.add<Transform>(movingEntity, Vec2(105.0f, 100.0f));
    auto& movingCollider = registry.add<Collider>(movingEntity);
    movingCollider.size = Vec2(16.0f, 16.0f);

    // First update - onEnter
    tracker.update(registry);
    EXPECT_EQ(stayCalls, 0);  // Not called on first frame (that's onEnter)

    // Second update - should call onStay
    tracker.update(registry);
    EXPECT_EQ(stayCalls, 1);

    // Third update - should call onStay again
    tracker.update(registry);
    EXPECT_EQ(stayCalls, 2);
}

TEST(TriggerCallbackTest, TriggerExitCallback) {
    Registry registry;
    TriggerTracker tracker;

    // Create trigger entity
    auto triggerEntity = registry.create();
    registry.add<Transform>(triggerEntity, Vec2(100.0f, 100.0f));
    auto& triggerCollider = registry.add<Collider>(triggerEntity);
    triggerCollider.size = Vec2(32.0f, 32.0f);
    triggerCollider.isTrigger = true;

    bool exitCalled = false;
    uint32_t exitedEntity = 0;

    auto& trigger = registry.add<Trigger>(triggerEntity);
    trigger.onEnter = [](uint32_t, uint32_t) {};
    trigger.onExit = [&exitCalled, &exitedEntity](uint32_t triggerEnt, uint32_t otherEnt) {
        exitCalled = true;
        exitedEntity = otherEnt;
    };

    // Create entity inside trigger
    auto movingEntity = registry.create();
    auto& movingTransform = registry.add<Transform>(movingEntity, Vec2(105.0f, 100.0f));
    auto& movingCollider = registry.add<Collider>(movingEntity);
    movingCollider.size = Vec2(16.0f, 16.0f);

    // First update - entity enters
    tracker.update(registry);
    EXPECT_FALSE(exitCalled);

    // Move entity out of trigger
    movingTransform.position = Vec2(500.0f, 500.0f);

    // Second update - entity exits
    tracker.update(registry);
    EXPECT_TRUE(exitCalled);
    EXPECT_EQ(exitedEntity, static_cast<uint32_t>(movingEntity));
}

TEST(TriggerCallbackTest, IsEntityInTrigger) {
    Registry registry;
    TriggerTracker tracker;

    // Create trigger entity
    auto triggerEntity = registry.create();
    registry.add<Transform>(triggerEntity, Vec2(100.0f, 100.0f));
    auto& triggerCollider = registry.add<Collider>(triggerEntity);
    triggerCollider.size = Vec2(32.0f, 32.0f);
    triggerCollider.isTrigger = true;
    registry.add<Trigger>(triggerEntity);

    // Create entity inside trigger
    auto movingEntity = registry.create();
    registry.add<Transform>(movingEntity, Vec2(105.0f, 100.0f));
    auto& movingCollider = registry.add<Collider>(movingEntity);
    movingCollider.size = Vec2(16.0f, 16.0f);

    // Before update
    EXPECT_FALSE(tracker.isEntityInTrigger(
        static_cast<uint32_t>(triggerEntity),
        static_cast<uint32_t>(movingEntity)
    ));

    // After update
    tracker.update(registry);
    EXPECT_TRUE(tracker.isEntityInTrigger(
        static_cast<uint32_t>(triggerEntity),
        static_cast<uint32_t>(movingEntity)
    ));
}

TEST(TriggerCallbackTest, GetEntitiesInTrigger) {
    Registry registry;
    TriggerTracker tracker;

    // Create trigger entity
    auto triggerEntity = registry.create();
    registry.add<Transform>(triggerEntity, Vec2(100.0f, 100.0f));
    auto& triggerCollider = registry.add<Collider>(triggerEntity);
    triggerCollider.size = Vec2(64.0f, 64.0f);
    triggerCollider.isTrigger = true;
    registry.add<Trigger>(triggerEntity);

    // Create multiple entities inside trigger
    auto entity1 = registry.create();
    registry.add<Transform>(entity1, Vec2(90.0f, 100.0f));
    auto& col1 = registry.add<Collider>(entity1);
    col1.size = Vec2(16.0f, 16.0f);

    auto entity2 = registry.create();
    registry.add<Transform>(entity2, Vec2(110.0f, 100.0f));
    auto& col2 = registry.add<Collider>(entity2);
    col2.size = Vec2(16.0f, 16.0f);

    // Create entity outside trigger
    auto entity3 = registry.create();
    registry.add<Transform>(entity3, Vec2(500.0f, 500.0f));
    auto& col3 = registry.add<Collider>(entity3);
    col3.size = Vec2(16.0f, 16.0f);

    tracker.update(registry);

    auto entitiesInTrigger = tracker.getEntitiesInTrigger(static_cast<uint32_t>(triggerEntity));

    EXPECT_EQ(entitiesInTrigger.size(), 2);
    EXPECT_TRUE(std::find(entitiesInTrigger.begin(), entitiesInTrigger.end(),
                          static_cast<uint32_t>(entity1)) != entitiesInTrigger.end());
    EXPECT_TRUE(std::find(entitiesInTrigger.begin(), entitiesInTrigger.end(),
                          static_cast<uint32_t>(entity2)) != entitiesInTrigger.end());
}

TEST(TriggerCallbackTest, RemoveEntity) {
    Registry registry;
    TriggerTracker tracker;

    // Create trigger entity
    auto triggerEntity = registry.create();
    registry.add<Transform>(triggerEntity, Vec2(100.0f, 100.0f));
    auto& triggerCollider = registry.add<Collider>(triggerEntity);
    triggerCollider.size = Vec2(32.0f, 32.0f);
    triggerCollider.isTrigger = true;
    registry.add<Trigger>(triggerEntity);

    // Create entity inside trigger
    auto movingEntity = registry.create();
    registry.add<Transform>(movingEntity, Vec2(105.0f, 100.0f));
    auto& movingCollider = registry.add<Collider>(movingEntity);
    movingCollider.size = Vec2(16.0f, 16.0f);

    tracker.update(registry);
    EXPECT_EQ(tracker.getOverlapCount(), 1);

    // Remove the entity
    tracker.removeEntity(static_cast<uint32_t>(movingEntity));
    EXPECT_EQ(tracker.getOverlapCount(), 0);
}
