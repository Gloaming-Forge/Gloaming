#include <gtest/gtest.h>
#include "rendering/IRenderer.hpp"
#include "rendering/Camera.hpp"
#include "rendering/Texture.hpp"
#include "rendering/TileRenderer.hpp"

using namespace gloaming;

// =============================================================================
// Vec2 Tests
// =============================================================================

TEST(Vec2Test, DefaultConstruction) {
    Vec2 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
}

TEST(Vec2Test, ValueConstruction) {
    Vec2 v(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v.x, 3.0f);
    EXPECT_FLOAT_EQ(v.y, 4.0f);
}

TEST(Vec2Test, Addition) {
    Vec2 a(1.0f, 2.0f);
    Vec2 b(3.0f, 4.0f);
    Vec2 result = a + b;
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
}

TEST(Vec2Test, Subtraction) {
    Vec2 a(5.0f, 7.0f);
    Vec2 b(2.0f, 3.0f);
    Vec2 result = a - b;
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
}

TEST(Vec2Test, ScalarMultiplication) {
    Vec2 v(2.0f, 3.0f);
    Vec2 result = v * 2.0f;
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
}

TEST(Vec2Test, ScalarDivision) {
    Vec2 v(6.0f, 8.0f);
    Vec2 result = v / 2.0f;
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
}

TEST(Vec2Test, CompoundAddition) {
    Vec2 v(1.0f, 2.0f);
    v += Vec2(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v.x, 4.0f);
    EXPECT_FLOAT_EQ(v.y, 6.0f);
}

// =============================================================================
// Rect Tests
// =============================================================================

TEST(RectTest, DefaultConstruction) {
    Rect r;
    EXPECT_FLOAT_EQ(r.x, 0.0f);
    EXPECT_FLOAT_EQ(r.y, 0.0f);
    EXPECT_FLOAT_EQ(r.width, 0.0f);
    EXPECT_FLOAT_EQ(r.height, 0.0f);
}

TEST(RectTest, ValueConstruction) {
    Rect r(10.0f, 20.0f, 100.0f, 50.0f);
    EXPECT_FLOAT_EQ(r.x, 10.0f);
    EXPECT_FLOAT_EQ(r.y, 20.0f);
    EXPECT_FLOAT_EQ(r.width, 100.0f);
    EXPECT_FLOAT_EQ(r.height, 50.0f);
}

TEST(RectTest, ContainsPointInside) {
    Rect r(0.0f, 0.0f, 100.0f, 100.0f);
    EXPECT_TRUE(r.contains({50.0f, 50.0f}));
    EXPECT_TRUE(r.contains({0.0f, 0.0f}));  // Top-left corner
    EXPECT_TRUE(r.contains({99.0f, 99.0f}));  // Just inside bottom-right
}

TEST(RectTest, ContainsPointOutside) {
    Rect r(0.0f, 0.0f, 100.0f, 100.0f);
    EXPECT_FALSE(r.contains({-1.0f, 50.0f}));
    EXPECT_FALSE(r.contains({101.0f, 50.0f}));
    EXPECT_FALSE(r.contains({50.0f, -1.0f}));
    EXPECT_FALSE(r.contains({50.0f, 101.0f}));
    EXPECT_FALSE(r.contains({100.0f, 100.0f}));  // Right on boundary (exclusive)
}

TEST(RectTest, IntersectsOverlapping) {
    Rect a(0.0f, 0.0f, 100.0f, 100.0f);
    Rect b(50.0f, 50.0f, 100.0f, 100.0f);
    EXPECT_TRUE(a.intersects(b));
    EXPECT_TRUE(b.intersects(a));
}

TEST(RectTest, IntersectsContained) {
    Rect outer(0.0f, 0.0f, 200.0f, 200.0f);
    Rect inner(50.0f, 50.0f, 50.0f, 50.0f);
    EXPECT_TRUE(outer.intersects(inner));
    EXPECT_TRUE(inner.intersects(outer));
}

TEST(RectTest, IntersectsNonOverlapping) {
    Rect a(0.0f, 0.0f, 100.0f, 100.0f);
    Rect b(200.0f, 200.0f, 100.0f, 100.0f);
    EXPECT_FALSE(a.intersects(b));
    EXPECT_FALSE(b.intersects(a));
}

TEST(RectTest, IntersectsTouching) {
    Rect a(0.0f, 0.0f, 100.0f, 100.0f);
    Rect b(100.0f, 0.0f, 100.0f, 100.0f);  // Just touching on the right
    EXPECT_FALSE(a.intersects(b));  // Touching but not overlapping
}

// =============================================================================
// Color Tests
// =============================================================================

TEST(ColorTest, DefaultConstruction) {
    Color c;
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 255);
    EXPECT_EQ(c.b, 255);
    EXPECT_EQ(c.a, 255);
}

TEST(ColorTest, ValueConstruction) {
    Color c(100, 150, 200, 128);
    EXPECT_EQ(c.r, 100);
    EXPECT_EQ(c.g, 150);
    EXPECT_EQ(c.b, 200);
    EXPECT_EQ(c.a, 128);
}

TEST(ColorTest, ValueConstructionDefaultAlpha) {
    Color c(100, 150, 200);
    EXPECT_EQ(c.r, 100);
    EXPECT_EQ(c.g, 150);
    EXPECT_EQ(c.b, 200);
    EXPECT_EQ(c.a, 255);
}

TEST(ColorTest, StaticColors) {
    Color white = Color::White();
    EXPECT_EQ(white.r, 255);
    EXPECT_EQ(white.g, 255);
    EXPECT_EQ(white.b, 255);

    Color black = Color::Black();
    EXPECT_EQ(black.r, 0);
    EXPECT_EQ(black.g, 0);
    EXPECT_EQ(black.b, 0);

    Color red = Color::Red();
    EXPECT_EQ(red.r, 255);
    EXPECT_EQ(red.g, 0);
    EXPECT_EQ(red.b, 0);
}

// =============================================================================
// Camera Tests
// =============================================================================

TEST(CameraTest, DefaultConstruction) {
    Camera cam;
    EXPECT_FLOAT_EQ(cam.getPosition().x, 0.0f);
    EXPECT_FLOAT_EQ(cam.getPosition().y, 0.0f);
    EXPECT_FLOAT_EQ(cam.getZoom(), 1.0f);
    EXPECT_FLOAT_EQ(cam.getRotation(), 0.0f);
}

TEST(CameraTest, ConstructionWithSize) {
    Camera cam(1920.0f, 1080.0f);
    EXPECT_FLOAT_EQ(cam.getScreenSize().x, 1920.0f);
    EXPECT_FLOAT_EQ(cam.getScreenSize().y, 1080.0f);
}

TEST(CameraTest, SetPosition) {
    Camera cam(1280.0f, 720.0f);
    cam.setPosition(100.0f, 200.0f);
    EXPECT_FLOAT_EQ(cam.getPosition().x, 100.0f);
    EXPECT_FLOAT_EQ(cam.getPosition().y, 200.0f);
}

TEST(CameraTest, Move) {
    Camera cam(1280.0f, 720.0f);
    cam.setPosition(100.0f, 100.0f);
    cam.move(50.0f, -30.0f);
    EXPECT_FLOAT_EQ(cam.getPosition().x, 150.0f);
    EXPECT_FLOAT_EQ(cam.getPosition().y, 70.0f);
}

TEST(CameraTest, Zoom) {
    Camera cam(1280.0f, 720.0f);
    EXPECT_FLOAT_EQ(cam.getZoom(), 1.0f);

    cam.setZoom(2.0f);
    EXPECT_FLOAT_EQ(cam.getZoom(), 2.0f);

    cam.zoom(-0.5f);
    EXPECT_FLOAT_EQ(cam.getZoom(), 1.5f);
}

TEST(CameraTest, ZoomClampMin) {
    Camera cam(1280.0f, 720.0f);
    cam.setZoom(0.01f);  // Below minimum
    EXPECT_FLOAT_EQ(cam.getZoom(), 0.1f);  // Should be clamped to MIN_ZOOM
}

TEST(CameraTest, ZoomClampMax) {
    Camera cam(1280.0f, 720.0f);
    cam.setZoom(100.0f);  // Above maximum
    EXPECT_FLOAT_EQ(cam.getZoom(), 10.0f);  // Should be clamped to MAX_ZOOM
}

TEST(CameraTest, Rotation) {
    Camera cam(1280.0f, 720.0f);
    cam.setRotation(45.0f);
    EXPECT_FLOAT_EQ(cam.getRotation(), 45.0f);

    cam.rotate(30.0f);
    EXPECT_FLOAT_EQ(cam.getRotation(), 75.0f);
}

TEST(CameraTest, RotationNormalization) {
    Camera cam(1280.0f, 720.0f);
    cam.setRotation(400.0f);  // Should wrap to 40
    EXPECT_FLOAT_EQ(cam.getRotation(), 40.0f);

    cam.setRotation(-45.0f);  // Should wrap to 315
    EXPECT_FLOAT_EQ(cam.getRotation(), 315.0f);
}

TEST(CameraTest, WorldToScreen) {
    Camera cam(1280.0f, 720.0f);
    cam.setPosition(0.0f, 0.0f);

    // Camera at origin, no zoom: screen center should be at world origin
    Vec2 screenPos = cam.worldToScreen({0.0f, 0.0f});
    EXPECT_FLOAT_EQ(screenPos.x, 640.0f);  // Half of 1280
    EXPECT_FLOAT_EQ(screenPos.y, 360.0f);  // Half of 720
}

TEST(CameraTest, ScreenToWorld) {
    Camera cam(1280.0f, 720.0f);
    cam.setPosition(0.0f, 0.0f);

    // Screen center should map to camera position (world origin)
    Vec2 worldPos = cam.screenToWorld({640.0f, 360.0f});
    EXPECT_NEAR(worldPos.x, 0.0f, 0.001f);
    EXPECT_NEAR(worldPos.y, 0.0f, 0.001f);
}

TEST(CameraTest, WorldToScreenWithOffset) {
    Camera cam(1280.0f, 720.0f);
    cam.setPosition(100.0f, 50.0f);

    // With camera offset, world origin should be offset on screen
    Vec2 screenPos = cam.worldToScreen({0.0f, 0.0f});
    EXPECT_FLOAT_EQ(screenPos.x, 540.0f);  // 640 - 100
    EXPECT_FLOAT_EQ(screenPos.y, 310.0f);  // 360 - 50
}

TEST(CameraTest, WorldToScreenWithZoom) {
    Camera cam(1280.0f, 720.0f);
    cam.setPosition(0.0f, 0.0f);
    cam.setZoom(2.0f);

    // World point (100, 100) with 2x zoom
    Vec2 screenPos = cam.worldToScreen({100.0f, 100.0f});
    EXPECT_FLOAT_EQ(screenPos.x, 840.0f);  // 640 + (100 * 2)
    EXPECT_FLOAT_EQ(screenPos.y, 560.0f);  // 360 + (100 * 2)
}

TEST(CameraTest, VisibleArea) {
    Camera cam(1280.0f, 720.0f);
    cam.setPosition(0.0f, 0.0f);

    Rect visible = cam.getVisibleArea();
    EXPECT_FLOAT_EQ(visible.width, 1280.0f);
    EXPECT_FLOAT_EQ(visible.height, 720.0f);
    EXPECT_FLOAT_EQ(visible.x, -640.0f);  // Camera at center
    EXPECT_FLOAT_EQ(visible.y, -360.0f);
}

TEST(CameraTest, VisibleAreaWithZoom) {
    Camera cam(1280.0f, 720.0f);
    cam.setPosition(0.0f, 0.0f);
    cam.setZoom(2.0f);

    Rect visible = cam.getVisibleArea();
    EXPECT_FLOAT_EQ(visible.width, 640.0f);   // 1280 / 2
    EXPECT_FLOAT_EQ(visible.height, 360.0f);  // 720 / 2
}

TEST(CameraTest, IsVisible) {
    Camera cam(1280.0f, 720.0f);
    cam.setPosition(0.0f, 0.0f);

    // Object at origin should be visible
    EXPECT_TRUE(cam.isVisible(Rect(-50.0f, -50.0f, 100.0f, 100.0f)));

    // Object far away should not be visible
    EXPECT_FALSE(cam.isVisible(Rect(2000.0f, 2000.0f, 100.0f, 100.0f)));
}

TEST(CameraTest, WorldBounds) {
    Camera cam(1280.0f, 720.0f);
    cam.setWorldBounds(Rect(0.0f, 0.0f, 2000.0f, 1000.0f));

    EXPECT_TRUE(cam.hasWorldBounds());

    // Try to move camera to negative position
    cam.setPosition(-500.0f, -500.0f);

    // Camera should be clamped to stay within bounds
    Vec2 pos = cam.getPosition();
    EXPECT_GE(pos.x, 0.0f);
    EXPECT_GE(pos.y, 0.0f);
}

TEST(CameraTest, ClearWorldBounds) {
    Camera cam(1280.0f, 720.0f);
    cam.setWorldBounds(Rect(0.0f, 0.0f, 1000.0f, 1000.0f));
    EXPECT_TRUE(cam.hasWorldBounds());

    cam.clearWorldBounds();
    EXPECT_FALSE(cam.hasWorldBounds());
}

// =============================================================================
// TextureAtlas Tests
// =============================================================================

TEST(TextureAtlasTest, AddRegion) {
    TextureAtlas atlas;
    atlas.addRegion("player_idle", Rect(0, 0, 32, 32));

    EXPECT_TRUE(atlas.hasRegion("player_idle"));
    EXPECT_FALSE(atlas.hasRegion("nonexistent"));
}

TEST(TextureAtlasTest, GetRegion) {
    TextureAtlas atlas;
    atlas.addRegion("tile", Rect(64, 32, 16, 16));

    const AtlasRegion* region = atlas.getRegion("tile");
    ASSERT_NE(region, nullptr);
    EXPECT_FLOAT_EQ(region->bounds.x, 64.0f);
    EXPECT_FLOAT_EQ(region->bounds.y, 32.0f);
    EXPECT_FLOAT_EQ(region->bounds.width, 16.0f);
    EXPECT_FLOAT_EQ(region->bounds.height, 16.0f);
}

TEST(TextureAtlasTest, GetRegionNotFound) {
    TextureAtlas atlas;
    const AtlasRegion* region = atlas.getRegion("nonexistent");
    EXPECT_EQ(region, nullptr);
}

TEST(TextureAtlasTest, AddGrid) {
    TextureAtlas atlas;
    atlas.addGrid("tile", 0, 0, 16, 16, 4, 2);  // 4x2 grid of 16x16 tiles

    // Should have 8 regions (4 columns * 2 rows)
    EXPECT_TRUE(atlas.hasRegion("tile_0"));
    EXPECT_TRUE(atlas.hasRegion("tile_3"));
    EXPECT_TRUE(atlas.hasRegion("tile_4"));
    EXPECT_TRUE(atlas.hasRegion("tile_7"));
    EXPECT_FALSE(atlas.hasRegion("tile_8"));  // Beyond grid

    // Check positions
    const AtlasRegion* tile0 = atlas.getRegion("tile_0");
    ASSERT_NE(tile0, nullptr);
    EXPECT_FLOAT_EQ(tile0->bounds.x, 0.0f);
    EXPECT_FLOAT_EQ(tile0->bounds.y, 0.0f);

    const AtlasRegion* tile3 = atlas.getRegion("tile_3");
    ASSERT_NE(tile3, nullptr);
    EXPECT_FLOAT_EQ(tile3->bounds.x, 48.0f);  // 3 * 16
    EXPECT_FLOAT_EQ(tile3->bounds.y, 0.0f);

    const AtlasRegion* tile4 = atlas.getRegion("tile_4");
    ASSERT_NE(tile4, nullptr);
    EXPECT_FLOAT_EQ(tile4->bounds.x, 0.0f);
    EXPECT_FLOAT_EQ(tile4->bounds.y, 16.0f);  // Second row
}

TEST(TextureAtlasTest, AddGridWithPadding) {
    TextureAtlas atlas;
    atlas.addGrid("sprite", 0, 0, 16, 16, 2, 2, 2, 2);  // 2x2 grid with 2px padding

    const AtlasRegion* sprite0 = atlas.getRegion("sprite_0");
    ASSERT_NE(sprite0, nullptr);
    EXPECT_FLOAT_EQ(sprite0->bounds.x, 0.0f);
    EXPECT_FLOAT_EQ(sprite0->bounds.y, 0.0f);

    const AtlasRegion* sprite1 = atlas.getRegion("sprite_1");
    ASSERT_NE(sprite1, nullptr);
    EXPECT_FLOAT_EQ(sprite1->bounds.x, 18.0f);  // 16 + 2 padding
    EXPECT_FLOAT_EQ(sprite1->bounds.y, 0.0f);

    const AtlasRegion* sprite2 = atlas.getRegion("sprite_2");
    ASSERT_NE(sprite2, nullptr);
    EXPECT_FLOAT_EQ(sprite2->bounds.x, 0.0f);
    EXPECT_FLOAT_EQ(sprite2->bounds.y, 18.0f);  // 16 + 2 padding
}

TEST(TextureAtlasTest, GetRegionNames) {
    TextureAtlas atlas;
    atlas.addRegion("a", Rect(0, 0, 16, 16));
    atlas.addRegion("b", Rect(16, 0, 16, 16));
    atlas.addRegion("c", Rect(32, 0, 16, 16));

    std::vector<std::string> names = atlas.getRegionNames();
    EXPECT_EQ(names.size(), 3u);

    // Check all names are present (order may vary due to unordered_map)
    EXPECT_TRUE(std::find(names.begin(), names.end(), "a") != names.end());
    EXPECT_TRUE(std::find(names.begin(), names.end(), "b") != names.end());
    EXPECT_TRUE(std::find(names.begin(), names.end(), "c") != names.end());
}

// =============================================================================
// Tile Tests
// =============================================================================

TEST(TileTest, DefaultConstruction) {
    Tile tile;
    EXPECT_EQ(tile.id, 0);
    EXPECT_EQ(tile.variant, 0);
    EXPECT_EQ(tile.flags, 0);
    EXPECT_TRUE(tile.isEmpty());
}

TEST(TileTest, IsEmpty) {
    Tile empty;
    empty.id = 0;
    EXPECT_TRUE(empty.isEmpty());

    Tile solid;
    solid.id = 1;
    EXPECT_FALSE(solid.isEmpty());
}

TEST(TileTest, IsSolid) {
    Tile tile;
    tile.id = 1;
    tile.flags = 0;
    EXPECT_FALSE(tile.isSolid());

    tile.flags = Tile::FLAG_SOLID;
    EXPECT_TRUE(tile.isSolid());
}

TEST(TileTest, MultipleFlags) {
    Tile tile;
    tile.flags = Tile::FLAG_SOLID | Tile::FLAG_PLATFORM;
    EXPECT_TRUE(tile.isSolid());
    EXPECT_TRUE((tile.flags & Tile::FLAG_PLATFORM) != 0);
}
