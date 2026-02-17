#include <gtest/gtest.h>
#include "rendering/Camera.hpp"

using namespace gloaming;

// =============================================================================
// Camera Extended Tests
// Note: Basic construction, position, move, zoom clamping, rotation,
// world/screen conversion, visible area, isVisible(Rect), and world bounds
// are already covered in test_rendering.cpp. This file covers additional
// functionality: follow, isVisible(Vec2), offset, roundtrip conversions,
// set screen size, additional position/zoom edge cases.
// =============================================================================

class CameraExtTest : public ::testing::Test {
protected:
    Camera camera{800.0f, 600.0f};
};

// =============================================================================
// Screen Size
// =============================================================================

TEST_F(CameraExtTest, SetScreenSize) {
    camera.setScreenSize(1920.0f, 1080.0f);
    Vec2 size = camera.getScreenSize();
    EXPECT_FLOAT_EQ(size.x, 1920.0f);
    EXPECT_FLOAT_EQ(size.y, 1080.0f);
}

TEST_F(CameraExtTest, SetScreenSizeVec2) {
    camera.setScreenSize(Vec2{640.0f, 480.0f});
    Vec2 size = camera.getScreenSize();
    EXPECT_FLOAT_EQ(size.x, 640.0f);
    EXPECT_FLOAT_EQ(size.y, 480.0f);
}

// =============================================================================
// Position Edge Cases
// =============================================================================

TEST_F(CameraExtTest, SetPositionVec2) {
    camera.setPosition(Vec2{100.0f, 200.0f});
    EXPECT_FLOAT_EQ(camera.getPosition().x, 100.0f);
    EXPECT_FLOAT_EQ(camera.getPosition().y, 200.0f);
}

TEST_F(CameraExtTest, MoveVec2) {
    camera.setPosition(0.0f, 0.0f);
    camera.move(Vec2{5.0f, 10.0f});
    EXPECT_FLOAT_EQ(camera.getPosition().x, 5.0f);
    EXPECT_FLOAT_EQ(camera.getPosition().y, 10.0f);
}

TEST_F(CameraExtTest, MoveAccumulates) {
    camera.move(Vec2{10.0f, 0.0f});
    camera.move(Vec2{10.0f, 0.0f});
    camera.move(Vec2{10.0f, 0.0f});
    EXPECT_FLOAT_EQ(camera.getPosition().x, 30.0f);
}

// =============================================================================
// Zoom Edge Cases
// =============================================================================

TEST_F(CameraExtTest, ZoomNegativeClamped) {
    camera.setZoom(-1.0f);
    EXPECT_GE(camera.getZoom(), 0.1f);
}

TEST_F(CameraExtTest, ZoomDeltaPositive) {
    camera.setZoom(1.0f);
    camera.zoom(0.5f);
    EXPECT_FLOAT_EQ(camera.getZoom(), 1.5f);
}

TEST_F(CameraExtTest, ZoomDeltaNegative) {
    camera.setZoom(2.0f);
    camera.zoom(-0.5f);
    EXPECT_FLOAT_EQ(camera.getZoom(), 1.5f);
}

// =============================================================================
// Rotation Edge Cases
// =============================================================================

TEST_F(CameraExtTest, RotateNegative) {
    camera.setRotation(0.0f);
    camera.rotate(-45.0f);
    // Rotation wraps: -45 -> 315
    float rot = camera.getRotation();
    EXPECT_TRUE(rot == 315.0f || rot == -45.0f);
}

// =============================================================================
// Coordinate Conversion Roundtrip
// =============================================================================

TEST_F(CameraExtTest, ScreenToWorldRoundTrip) {
    camera.setPosition(100.0f, 200.0f);
    camera.setZoom(2.0f);

    Vec2 original(150.0f, 250.0f);
    Vec2 world = camera.screenToWorld(original);
    Vec2 backToScreen = camera.worldToScreen(world);

    EXPECT_NEAR(backToScreen.x, original.x, 0.5f);
    EXPECT_NEAR(backToScreen.y, original.y, 0.5f);
}

TEST_F(CameraExtTest, WorldToScreenRoundTrip) {
    camera.setPosition(50.0f, 75.0f);

    Vec2 worldPt(200.0f, 300.0f);
    Vec2 screen = camera.worldToScreen(worldPt);
    Vec2 backToWorld = camera.screenToWorld(screen);

    EXPECT_NEAR(backToWorld.x, worldPt.x, 0.5f);
    EXPECT_NEAR(backToWorld.y, worldPt.y, 0.5f);
}

// =============================================================================
// isVisible (Vec2 point overload)
// =============================================================================

TEST_F(CameraExtTest, IsVisiblePointCenter) {
    camera.setPosition(0.0f, 0.0f);
    EXPECT_TRUE(camera.isVisible(Vec2{0.0f, 0.0f}));
}

TEST_F(CameraExtTest, IsVisiblePointFarAway) {
    camera.setPosition(0.0f, 0.0f);
    EXPECT_FALSE(camera.isVisible(Vec2{5000.0f, 5000.0f}));
}

TEST_F(CameraExtTest, IsVisiblePointNearEdge) {
    camera.setPosition(0.0f, 0.0f);
    // At zoom=1 with 800x600 screen, visible area is (-400,-300) to (400,300)
    EXPECT_TRUE(camera.isVisible(Vec2{399.0f, 299.0f}));
}

// =============================================================================
// Follow
// =============================================================================

TEST_F(CameraExtTest, FollowInstant) {
    camera.setPosition(0.0f, 0.0f);
    camera.follow(Vec2{100.0f, 100.0f}, 0.0f, 0.016f);
    EXPECT_NEAR(camera.getPosition().x, 100.0f, 1.0f);
    EXPECT_NEAR(camera.getPosition().y, 100.0f, 1.0f);
}

TEST_F(CameraExtTest, FollowSmooth) {
    camera.setPosition(0.0f, 0.0f);
    camera.follow(Vec2{100.0f, 0.0f}, 10.0f, 0.016f);
    EXPECT_GT(camera.getPosition().x, 0.0f);
    EXPECT_LT(camera.getPosition().x, 100.0f);
}

TEST_F(CameraExtTest, FollowAlreadyAtTarget) {
    camera.setPosition(50.0f, 50.0f);
    camera.follow(Vec2{50.0f, 50.0f}, 5.0f, 0.016f);
    EXPECT_NEAR(camera.getPosition().x, 50.0f, 0.5f);
    EXPECT_NEAR(camera.getPosition().y, 50.0f, 0.5f);
}

// =============================================================================
// Offset
// =============================================================================

TEST_F(CameraExtTest, OffsetIsCenterOfScreen) {
    camera.setPosition(0.0f, 0.0f);
    Vec2 offset = camera.getOffset();
    EXPECT_FLOAT_EQ(offset.x, 400.0f);
    EXPECT_FLOAT_EQ(offset.y, 300.0f);
}

TEST_F(CameraExtTest, OffsetUnaffectedByZoom) {
    camera.setPosition(0.0f, 0.0f);
    camera.setZoom(2.0f);
    Vec2 offset = camera.getOffset();
    // getOffset() returns half screen size regardless of zoom
    EXPECT_FLOAT_EQ(offset.x, 400.0f);
    EXPECT_FLOAT_EQ(offset.y, 300.0f);
}

// =============================================================================
// Visible Area with Position
// =============================================================================

TEST_F(CameraExtTest, VisibleAreaMovesWithCamera) {
    camera.setPosition(100.0f, 200.0f);
    Rect area = camera.getVisibleArea();
    // Center of visible area should be at camera position
    float centerX = area.x + area.width / 2.0f;
    float centerY = area.y + area.height / 2.0f;
    EXPECT_NEAR(centerX, 100.0f, 1.0f);
    EXPECT_NEAR(centerY, 200.0f, 1.0f);
}

// =============================================================================
// World Bounds Extended
// =============================================================================

TEST_F(CameraExtTest, GetWorldBounds) {
    camera.setWorldBounds(Rect{10.0f, 20.0f, 500.0f, 400.0f});
    Rect bounds = camera.getWorldBounds();
    EXPECT_FLOAT_EQ(bounds.x, 10.0f);
    EXPECT_FLOAT_EQ(bounds.y, 20.0f);
    EXPECT_FLOAT_EQ(bounds.width, 500.0f);
    EXPECT_FLOAT_EQ(bounds.height, 400.0f);
}

TEST_F(CameraExtTest, NoBoundsInitially) {
    EXPECT_FALSE(camera.hasWorldBounds());
}
