#include <gtest/gtest.h>
#include "gameplay/GridMovement.hpp"

using namespace gloaming;

// =============================================================================
// GridMovement Component Tests
// =============================================================================

class GridMovementTest : public ::testing::Test {
protected:
    GridMovement grid;
};

TEST_F(GridMovementTest, DefaultConstruction) {
    EXPECT_EQ(grid.gridSize, 16);
    EXPECT_FLOAT_EQ(grid.moveSpeed, 4.0f);
    EXPECT_EQ(grid.facing, FacingDirection::Down);
    EXPECT_FALSE(grid.isMoving);
    EXPECT_FALSE(grid.tileInitialized);
}

TEST_F(GridMovementTest, ParameterizedConstruction) {
    GridMovement g(32, 8.0f);
    EXPECT_EQ(g.gridSize, 32);
    EXPECT_FLOAT_EQ(g.moveSpeed, 8.0f);
}

// =============================================================================
// snapToGrid
// =============================================================================

TEST_F(GridMovementTest, SnapToGridExact) {
    Vec2 result = grid.snapToGrid(Vec2(32.0f, 48.0f));
    EXPECT_EQ(grid.tileX, 2);
    EXPECT_EQ(grid.tileY, 3);
    EXPECT_FLOAT_EQ(result.x, 32.0f);
    EXPECT_FLOAT_EQ(result.y, 48.0f);
    EXPECT_TRUE(grid.tileInitialized);
}

TEST_F(GridMovementTest, SnapToGridRounding) {
    // 25 is closer to 32 (tile 2) than to 16 (tile 1)
    Vec2 result = grid.snapToGrid(Vec2(25.0f, 0.0f));
    EXPECT_EQ(grid.tileX, 2);
    EXPECT_FLOAT_EQ(result.x, 32.0f);
}

TEST_F(GridMovementTest, SnapToGridOrigin) {
    Vec2 result = grid.snapToGrid(Vec2(0.0f, 0.0f));
    EXPECT_EQ(grid.tileX, 0);
    EXPECT_EQ(grid.tileY, 0);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
}

TEST_F(GridMovementTest, SnapToGridNegative) {
    Vec2 result = grid.snapToGrid(Vec2(-16.0f, -32.0f));
    EXPECT_EQ(grid.tileX, -1);
    EXPECT_EQ(grid.tileY, -2);
    EXPECT_FLOAT_EQ(result.x, -16.0f);
    EXPECT_FLOAT_EQ(result.y, -32.0f);
}

TEST_F(GridMovementTest, SnapToGridDifferentSizes) {
    GridMovement g(32);
    Vec2 result = g.snapToGrid(Vec2(64.0f, 96.0f));
    EXPECT_EQ(g.tileX, 2);
    EXPECT_EQ(g.tileY, 3);
    EXPECT_FLOAT_EQ(result.x, 64.0f);
    EXPECT_FLOAT_EQ(result.y, 96.0f);
}

// =============================================================================
// tileToWorldPos
// =============================================================================

TEST_F(GridMovementTest, TileToWorldPos) {
    grid.tileX = 3;
    grid.tileY = 5;
    Vec2 world = grid.tileToWorldPos();
    EXPECT_FLOAT_EQ(world.x, 48.0f);
    EXPECT_FLOAT_EQ(world.y, 80.0f);
}

TEST_F(GridMovementTest, TileToWorldPosOrigin) {
    grid.tileX = 0;
    grid.tileY = 0;
    Vec2 world = grid.tileToWorldPos();
    EXPECT_FLOAT_EQ(world.x, 0.0f);
    EXPECT_FLOAT_EQ(world.y, 0.0f);
}

TEST_F(GridMovementTest, TileToWorldPosNegative) {
    grid.tileX = -2;
    grid.tileY = -3;
    Vec2 world = grid.tileToWorldPos();
    EXPECT_FLOAT_EQ(world.x, -32.0f);
    EXPECT_FLOAT_EQ(world.y, -48.0f);
}

// =============================================================================
// worldToTile
// =============================================================================

TEST_F(GridMovementTest, WorldToTile) {
    EXPECT_EQ(grid.worldToTile(32.0f), 2);
    EXPECT_EQ(grid.worldToTile(48.0f), 3);
    EXPECT_EQ(grid.worldToTile(0.0f), 0);
}

TEST_F(GridMovementTest, WorldToTileMidCell) {
    // 20 / 16 = 1.25, floor = 1
    EXPECT_EQ(grid.worldToTile(20.0f), 1);
}

TEST_F(GridMovementTest, WorldToTileNegative) {
    // -10 / 16 = -0.625, floor = -1
    EXPECT_EQ(grid.worldToTile(-10.0f), -1);
}

// =============================================================================
// tileToWorld
// =============================================================================

TEST_F(GridMovementTest, TileToWorld) {
    EXPECT_FLOAT_EQ(grid.tileToWorld(0), 0.0f);
    EXPECT_FLOAT_EQ(grid.tileToWorld(1), 16.0f);
    EXPECT_FLOAT_EQ(grid.tileToWorld(5), 80.0f);
    EXPECT_FLOAT_EQ(grid.tileToWorld(-2), -32.0f);
}

// =============================================================================
// FacingDirection Enum
// =============================================================================

TEST(FacingDirectionTest, Values) {
    EXPECT_EQ(static_cast<uint8_t>(FacingDirection::Down), 0);
    EXPECT_EQ(static_cast<uint8_t>(FacingDirection::Left), 1);
    EXPECT_EQ(static_cast<uint8_t>(FacingDirection::Up), 2);
    EXPECT_EQ(static_cast<uint8_t>(FacingDirection::Right), 3);
}

// =============================================================================
// Input Buffering State
// =============================================================================

TEST_F(GridMovementTest, PendingInputDefault) {
    EXPECT_FALSE(grid.hasPendingInput);
    EXPECT_EQ(grid.pendingDirection, FacingDirection::Down);
}

// =============================================================================
// Movement State
// =============================================================================

TEST_F(GridMovementTest, MovementProgress) {
    EXPECT_FLOAT_EQ(grid.moveProgress, 0.0f);
    EXPECT_FALSE(grid.isMoving);
}

// =============================================================================
// Snap then Convert Roundtrip
// =============================================================================

TEST_F(GridMovementTest, SnapThenTileToWorld) {
    Vec2 snapped = grid.snapToGrid(Vec2(100.0f, 200.0f));
    Vec2 fromTile = grid.tileToWorldPos();
    EXPECT_FLOAT_EQ(snapped.x, fromTile.x);
    EXPECT_FLOAT_EQ(snapped.y, fromTile.y);
}
