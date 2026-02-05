#include <gtest/gtest.h>
#include "physics/AABB.hpp"
#include "physics/Collision.hpp"
#include "physics/TileCollision.hpp"
#include "physics/Trigger.hpp"
#include "physics/Raycast.hpp"
#include "physics/PhysicsSystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"

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
