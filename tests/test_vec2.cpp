#include <gtest/gtest.h>
#include "engine/Vec2.hpp"

#include <cmath>

using namespace gloaming;

// =============================================================================
// Vec2 Extended Tests
// Note: Basic construction, addition, subtraction, scalar ops, and compound
// addition are already covered in test_rendering.cpp. This file covers
// additional operations: compound operators, comparisons, length, normalized,
// dot product, distance, and edge cases.
// =============================================================================

// Compound operators not in test_rendering.cpp
TEST(Vec2ExtTest, SubAssign) {
    Vec2 v(5.0f, 7.0f);
    v -= Vec2(2.0f, 3.0f);
    EXPECT_FLOAT_EQ(v.x, 3.0f);
    EXPECT_FLOAT_EQ(v.y, 4.0f);
}

TEST(Vec2ExtTest, MulAssign) {
    Vec2 v(3.0f, 4.0f);
    v *= 2.0f;
    EXPECT_FLOAT_EQ(v.x, 6.0f);
    EXPECT_FLOAT_EQ(v.y, 8.0f);
}

TEST(Vec2ExtTest, NegativeValues) {
    Vec2 v(-1.5f, -2.5f);
    EXPECT_FLOAT_EQ(v.x, -1.5f);
    EXPECT_FLOAT_EQ(v.y, -2.5f);
}

TEST(Vec2ExtTest, MultiplyByZero) {
    Vec2 v(3.0f, 4.0f);
    Vec2 result = v * 0.0f;
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
}

TEST(Vec2ExtTest, MultiplyByNegative) {
    Vec2 v(3.0f, 4.0f);
    Vec2 result = v * -1.0f;
    EXPECT_FLOAT_EQ(result.x, -3.0f);
    EXPECT_FLOAT_EQ(result.y, -4.0f);
}

// =============================================================================
// Comparison Operators
// =============================================================================

TEST(Vec2ExtTest, Equality) {
    Vec2 a(1.0f, 2.0f);
    Vec2 b(1.0f, 2.0f);
    EXPECT_TRUE(a == b);
}

TEST(Vec2ExtTest, Inequality) {
    Vec2 a(1.0f, 2.0f);
    Vec2 b(1.0f, 3.0f);
    EXPECT_TRUE(a != b);
}

TEST(Vec2ExtTest, EqualityBothZero) {
    Vec2 a;
    Vec2 b(0.0f, 0.0f);
    EXPECT_TRUE(a == b);
}

TEST(Vec2ExtTest, InequalityX) {
    Vec2 a(1.0f, 2.0f);
    Vec2 b(99.0f, 2.0f);
    EXPECT_TRUE(a != b);
}

// =============================================================================
// Length & LengthSquared
// =============================================================================

TEST(Vec2ExtTest, Length345) {
    Vec2 v(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v.length(), 5.0f);
}

TEST(Vec2ExtTest, LengthZero) {
    Vec2 v;
    EXPECT_FLOAT_EQ(v.length(), 0.0f);
}

TEST(Vec2ExtTest, LengthUnit) {
    Vec2 v(1.0f, 0.0f);
    EXPECT_FLOAT_EQ(v.length(), 1.0f);
}

TEST(Vec2ExtTest, LengthSquared) {
    Vec2 v(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v.lengthSquared(), 25.0f);
}

TEST(Vec2ExtTest, LengthSquaredZero) {
    Vec2 v;
    EXPECT_FLOAT_EQ(v.lengthSquared(), 0.0f);
}

// =============================================================================
// Normalized
// =============================================================================

TEST(Vec2ExtTest, Normalized) {
    Vec2 v(3.0f, 4.0f);
    Vec2 n = v.normalized();
    EXPECT_NEAR(n.x, 0.6f, 0.0001f);
    EXPECT_NEAR(n.y, 0.8f, 0.0001f);
    EXPECT_NEAR(n.length(), 1.0f, 0.0001f);
}

TEST(Vec2ExtTest, NormalizedAlreadyUnit) {
    Vec2 v(1.0f, 0.0f);
    Vec2 n = v.normalized();
    EXPECT_FLOAT_EQ(n.x, 1.0f);
    EXPECT_FLOAT_EQ(n.y, 0.0f);
}

TEST(Vec2ExtTest, NormalizedZeroVector) {
    Vec2 v;
    Vec2 n = v.normalized();
    EXPECT_FLOAT_EQ(n.x, 0.0f);
    EXPECT_FLOAT_EQ(n.y, 0.0f);
}

TEST(Vec2ExtTest, NormalizedDiagonal) {
    Vec2 v(1.0f, 1.0f);
    Vec2 n = v.normalized();
    EXPECT_NEAR(n.length(), 1.0f, 0.0001f);
    EXPECT_NEAR(n.x, n.y, 0.0001f);
}

// =============================================================================
// Dot Product
// =============================================================================

TEST(Vec2ExtTest, DotProductPerpendicular) {
    Vec2 a(1.0f, 0.0f);
    Vec2 b(0.0f, 1.0f);
    EXPECT_FLOAT_EQ(Vec2::dot(a, b), 0.0f);
}

TEST(Vec2ExtTest, DotProductParallel) {
    Vec2 a(2.0f, 3.0f);
    Vec2 b(2.0f, 3.0f);
    EXPECT_FLOAT_EQ(Vec2::dot(a, b), 13.0f);
}

TEST(Vec2ExtTest, DotProductOpposite) {
    Vec2 a(1.0f, 0.0f);
    Vec2 b(-1.0f, 0.0f);
    EXPECT_FLOAT_EQ(Vec2::dot(a, b), -1.0f);
}

TEST(Vec2ExtTest, DotProductWithZero) {
    Vec2 a(5.0f, 10.0f);
    Vec2 b(0.0f, 0.0f);
    EXPECT_FLOAT_EQ(Vec2::dot(a, b), 0.0f);
}

// =============================================================================
// Distance
// =============================================================================

TEST(Vec2ExtTest, Distance345) {
    Vec2 a(0.0f, 0.0f);
    Vec2 b(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(Vec2::distance(a, b), 5.0f);
}

TEST(Vec2ExtTest, DistanceSamePoint) {
    Vec2 a(5.0f, 5.0f);
    EXPECT_FLOAT_EQ(Vec2::distance(a, a), 0.0f);
}

TEST(Vec2ExtTest, DistanceIsSymmetric) {
    Vec2 a(1.0f, 2.0f);
    Vec2 b(4.0f, 6.0f);
    EXPECT_FLOAT_EQ(Vec2::distance(a, b), Vec2::distance(b, a));
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(Vec2ExtTest, LargeValues) {
    Vec2 v(1e6f, 1e6f);
    EXPECT_FLOAT_EQ(v.lengthSquared(), 2e12f);
}

TEST(Vec2ExtTest, ChainedOperations) {
    Vec2 v(1.0f, 1.0f);
    Vec2 result = (v + Vec2(2.0f, 3.0f)) * 2.0f;
    EXPECT_FLOAT_EQ(result.x, 6.0f);
    EXPECT_FLOAT_EQ(result.y, 8.0f);
}

TEST(Vec2ExtTest, SubtractionSelf) {
    Vec2 v(5.0f, 10.0f);
    Vec2 result = v - v;
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
}
