#include <gtest/gtest.h>
#include "gameplay/Pathfinding.hpp"

using namespace gloaming;

// =============================================================================
// TilePos Tests
// =============================================================================

TEST(TilePosTest, DefaultConstruction) {
    TilePos p;
    EXPECT_EQ(p.x, 0);
    EXPECT_EQ(p.y, 0);
}

TEST(TilePosTest, Equality) {
    EXPECT_TRUE(TilePos(1, 2) == TilePos(1, 2));
    EXPECT_FALSE(TilePos(1, 2) == TilePos(1, 3));
    EXPECT_TRUE(TilePos(1, 2) != TilePos(3, 4));
}

TEST(TilePosTest, Hash) {
    TilePosHash hash;
    // Different positions should (generally) have different hashes
    EXPECT_NE(hash(TilePos(0, 0)), hash(TilePos(1, 0)));
    EXPECT_NE(hash(TilePos(0, 0)), hash(TilePos(0, 1)));
    // Same position should have same hash
    EXPECT_EQ(hash(TilePos(5, 5)), hash(TilePos(5, 5)));
}

// =============================================================================
// Pathfinder Configuration
// =============================================================================

class PathfinderTest : public ::testing::Test {
protected:
    Pathfinder pathfinder;

    // All tiles walkable
    static bool allWalkable(int, int) { return true; }

    // A simple 10x10 grid where all tiles are walkable
    static bool gridWalkable(int x, int y) {
        return x >= 0 && x < 10 && y >= 0 && y < 10;
    }
};

TEST_F(PathfinderTest, DefaultConfig) {
    EXPECT_FALSE(pathfinder.getAllowDiagonals());
    EXPECT_EQ(pathfinder.getMaxNodes(), 5000);
}

TEST_F(PathfinderTest, SetAllowDiagonals) {
    pathfinder.setAllowDiagonals(true);
    EXPECT_TRUE(pathfinder.getAllowDiagonals());
}

TEST_F(PathfinderTest, SetMaxNodes) {
    pathfinder.setMaxNodes(100);
    EXPECT_EQ(pathfinder.getMaxNodes(), 100);
}

// =============================================================================
// findPath - Basic Cases
// =============================================================================

TEST_F(PathfinderTest, StartEqualsGoal) {
    auto result = pathfinder.findPath(TilePos(5, 5), TilePos(5, 5), allWalkable);
    EXPECT_TRUE(result.found);
    ASSERT_EQ(result.path.size(), 1u);
    EXPECT_EQ(result.path[0], TilePos(5, 5));
}

TEST_F(PathfinderTest, AdjacentHorizontal) {
    auto result = pathfinder.findPath(TilePos(0, 0), TilePos(1, 0), allWalkable);
    EXPECT_TRUE(result.found);
    ASSERT_GE(result.path.size(), 2u);
    EXPECT_EQ(result.path.front(), TilePos(0, 0));
    EXPECT_EQ(result.path.back(), TilePos(1, 0));
}

TEST_F(PathfinderTest, AdjacentVertical) {
    auto result = pathfinder.findPath(TilePos(0, 0), TilePos(0, 1), allWalkable);
    EXPECT_TRUE(result.found);
    EXPECT_EQ(result.path.front(), TilePos(0, 0));
    EXPECT_EQ(result.path.back(), TilePos(0, 1));
}

TEST_F(PathfinderTest, StraightLine4Dir) {
    auto result = pathfinder.findPath(TilePos(0, 0), TilePos(5, 0), gridWalkable);
    EXPECT_TRUE(result.found);
    EXPECT_EQ(result.path.size(), 6u); // 0,1,2,3,4,5
    EXPECT_EQ(result.path.front(), TilePos(0, 0));
    EXPECT_EQ(result.path.back(), TilePos(5, 0));
}

TEST_F(PathfinderTest, GoalUnwalkable) {
    auto result = pathfinder.findPath(
        TilePos(0, 0), TilePos(5, 5),
        [](int x, int y) { return !(x == 5 && y == 5); }
    );
    EXPECT_FALSE(result.found);
}

// =============================================================================
// findPath - Obstacles
// =============================================================================

TEST_F(PathfinderTest, PathAroundWall) {
    // A wall blocking direct horizontal path at y=0
    // Wall at (2,0), (2,1), (2,2) — must go around
    auto isWalkable = [](int x, int y) -> bool {
        if (x < 0 || x >= 10 || y < 0 || y >= 10) return false;
        if (x == 2 && y >= 0 && y <= 2) return false; // wall
        return true;
    };

    auto result = pathfinder.findPath(TilePos(0, 0), TilePos(4, 0), isWalkable);
    EXPECT_TRUE(result.found);
    EXPECT_EQ(result.path.front(), TilePos(0, 0));
    EXPECT_EQ(result.path.back(), TilePos(4, 0));

    // Path should not pass through the wall
    for (const auto& pos : result.path) {
        EXPECT_FALSE(pos.x == 2 && pos.y >= 0 && pos.y <= 2)
            << "Path passes through wall at (" << pos.x << ", " << pos.y << ")";
    }
}

TEST_F(PathfinderTest, NoPathBlocked) {
    // Completely surrounded goal
    auto isWalkable = [](int x, int y) -> bool {
        if (x >= 4 && x <= 6 && y >= 4 && y <= 6 && !(x == 5 && y == 5)) return false;
        return x >= 0 && x < 10 && y >= 0 && y < 10;
    };

    auto result = pathfinder.findPath(TilePos(0, 0), TilePos(5, 5), isWalkable);
    EXPECT_FALSE(result.found);
}

// =============================================================================
// findPath - Diagonal Movement
// =============================================================================

TEST_F(PathfinderTest, DiagonalPath) {
    pathfinder.setAllowDiagonals(true);
    auto result = pathfinder.findPath(TilePos(0, 0), TilePos(3, 3), gridWalkable);
    EXPECT_TRUE(result.found);
    // Diagonal should be shorter than 4-directional
    EXPECT_LE(result.path.size(), 5u);
}

TEST_F(PathfinderTest, DiagonalCornerCuttingPrevention) {
    pathfinder.setAllowDiagonals(true);

    // Block west and north of (5,5) to test SW/NW corner-cutting prevention
    // (the pathfinder prevents diagonal corner-cutting for SW and NW directions)
    auto isWalkable = [](int x, int y) -> bool {
        if (x < 0 || x >= 10 || y < 0 || y >= 10) return false;
        if (x == 4 && y == 5) return false;  // Block west of (5,5)
        if (x == 5 && y == 4) return false;  // Block north of (5,5)
        return true;
    };

    // From (5,5) going to (4,4): SW diagonal requires (4,5) and (5,4) walkable
    // Both are blocked, so the path must avoid diagonal from (5,5) to (4,4)
    auto result = pathfinder.findPath(TilePos(5, 5), TilePos(3, 3), isWalkable);
    EXPECT_TRUE(result.found);
    // Path should be longer than 3 steps since direct diagonal is blocked
    EXPECT_GT(result.path.size(), 3u);
}

// =============================================================================
// findPath - Max Nodes Budget
// =============================================================================

TEST_F(PathfinderTest, MaxNodesExceeded) {
    // Limit search to very few nodes — long path should fail
    auto result = pathfinder.findPath(
        TilePos(0, 0), TilePos(9, 9), gridWalkable, false, 5
    );
    // With only 5 nodes budget and a 9-tile Manhattan path, it should fail
    EXPECT_FALSE(result.found);
    EXPECT_GT(result.nodesExplored, 0);
}

TEST_F(PathfinderTest, SufficientMaxNodes) {
    auto result = pathfinder.findPath(
        TilePos(0, 0), TilePos(3, 0), gridWalkable, false, 100
    );
    EXPECT_TRUE(result.found);
}

// =============================================================================
// findPath - Weighted Tiles
// =============================================================================

TEST_F(PathfinderTest, WeightedTileCostAffectsPath) {
    // With tile costs, the path should differ from the unweighted path.
    // Make row y=0 very expensive except at endpoints.
    TileCostFunc expensiveRow0 = [](int x, int y) -> float {
        if (y == 0 && x > 0 && x < 5) return 100.0f;
        return 1.0f;
    };

    // Without cost: direct path along y=0 (5 steps)
    auto directResult = pathfinder.findPath(
        TilePos(0, 0), TilePos(5, 0), gridWalkable, false, 5000
    );
    EXPECT_TRUE(directResult.found);

    // With cost: should prefer going around via y=1
    auto costResult = pathfinder.findPath(
        TilePos(0, 0), TilePos(5, 0), gridWalkable, false, 5000, expensiveRow0
    );
    EXPECT_TRUE(costResult.found);

    // The weighted path should be longer (more tiles) but cheaper in total cost
    EXPECT_GT(costResult.path.size(), directResult.path.size());
}

// =============================================================================
// findPath - Path Correctness
// =============================================================================

TEST_F(PathfinderTest, PathStartsAtStart) {
    auto result = pathfinder.findPath(TilePos(1, 1), TilePos(5, 5), gridWalkable);
    EXPECT_TRUE(result.found);
    EXPECT_EQ(result.path.front(), TilePos(1, 1));
}

TEST_F(PathfinderTest, PathEndsAtGoal) {
    auto result = pathfinder.findPath(TilePos(1, 1), TilePos(5, 5), gridWalkable);
    EXPECT_TRUE(result.found);
    EXPECT_EQ(result.path.back(), TilePos(5, 5));
}

TEST_F(PathfinderTest, PathStepsAreAdjacent) {
    auto result = pathfinder.findPath(TilePos(0, 0), TilePos(5, 5), gridWalkable);
    EXPECT_TRUE(result.found);

    for (size_t i = 0; i + 1 < result.path.size(); ++i) {
        int dx = std::abs(result.path[i+1].x - result.path[i].x);
        int dy = std::abs(result.path[i+1].y - result.path[i].y);
        // In 4-directional mode, steps should differ by exactly 1 in one axis
        EXPECT_EQ(dx + dy, 1)
            << "Non-adjacent step from (" << result.path[i].x << "," << result.path[i].y
            << ") to (" << result.path[i+1].x << "," << result.path[i+1].y << ")";
    }
}

TEST_F(PathfinderTest, DiagonalPathStepsAreAdjacent) {
    pathfinder.setAllowDiagonals(true);
    auto result = pathfinder.findPath(TilePos(0, 0), TilePos(5, 5), gridWalkable);
    EXPECT_TRUE(result.found);

    for (size_t i = 0; i + 1 < result.path.size(); ++i) {
        int dx = std::abs(result.path[i+1].x - result.path[i].x);
        int dy = std::abs(result.path[i+1].y - result.path[i].y);
        // In 8-directional mode, each step differs by at most 1 in each axis
        EXPECT_LE(dx, 1);
        EXPECT_LE(dy, 1);
        EXPECT_GT(dx + dy, 0); // Must move somewhere
    }
}

// =============================================================================
// isReachable
// =============================================================================

TEST_F(PathfinderTest, IsReachableSamePoint) {
    EXPECT_TRUE(pathfinder.isReachable(TilePos(5, 5), TilePos(5, 5), gridWalkable));
}

TEST_F(PathfinderTest, IsReachableAdjacent) {
    EXPECT_TRUE(pathfinder.isReachable(TilePos(0, 0), TilePos(1, 0), gridWalkable));
}

TEST_F(PathfinderTest, IsReachableGoalUnwalkable) {
    auto blocked = [](int x, int y) { return !(x == 5 && y == 5); };
    EXPECT_FALSE(pathfinder.isReachable(TilePos(0, 0), TilePos(5, 5), blocked));
}

TEST_F(PathfinderTest, IsReachableWithWall) {
    // Goal completely walled off
    auto isWalkable = [](int x, int y) -> bool {
        if (x >= 4 && x <= 6 && y >= 4 && y <= 6 && !(x == 5 && y == 5)) return false;
        return x >= 0 && x < 10 && y >= 0 && y < 10;
    };
    EXPECT_FALSE(pathfinder.isReachable(TilePos(0, 0), TilePos(5, 5), isWalkable));
}

TEST_F(PathfinderTest, IsReachableDistanceLimit) {
    // Should not find path beyond maxDistance
    EXPECT_FALSE(pathfinder.isReachable(TilePos(0, 0), TilePos(9, 9), gridWalkable, 5));
}

TEST_F(PathfinderTest, IsReachableWithinDistance) {
    EXPECT_TRUE(pathfinder.isReachable(TilePos(0, 0), TilePos(3, 0), gridWalkable, 10));
}

// =============================================================================
// Explicit Parameters Overload
// =============================================================================

TEST_F(PathfinderTest, ExplicitParamsOverride) {
    pathfinder.setAllowDiagonals(false);
    pathfinder.setMaxNodes(10);

    // Use explicit params that differ from instance config
    auto result = pathfinder.findPath(
        TilePos(0, 0), TilePos(3, 3), gridWalkable, true, 5000
    );
    EXPECT_TRUE(result.found);
}

TEST_F(PathfinderTest, NodesExploredReported) {
    auto result = pathfinder.findPath(TilePos(0, 0), TilePos(5, 5), gridWalkable);
    EXPECT_TRUE(result.found);
    EXPECT_GT(result.nodesExplored, 0);
}
