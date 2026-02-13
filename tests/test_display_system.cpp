#include <gtest/gtest.h>
#include "rendering/ViewportScaler.hpp"
#include "ui/UIScaling.hpp"
#include "engine/Time.hpp"
#include "engine/Window.hpp"

using namespace gloaming;

// =============================================================================
// ViewportScaler Tests
// =============================================================================

TEST(ViewportScalerTest, DefaultConfig) {
    ViewportScaler vs;
    ViewportConfig cfg;
    vs.configure(cfg);
    vs.update(1280, 720);

    EXPECT_EQ(vs.getEffectiveWidth(), 1280);
    EXPECT_EQ(vs.getEffectiveHeight(), 720);
    EXPECT_FLOAT_EQ(vs.getScale(), 1.0f);
}

TEST(ViewportScalerTest, ExpandMode_WiderWindow) {
    ViewportScaler vs;
    ViewportConfig cfg;
    cfg.designWidth = 1280;
    cfg.designHeight = 720;
    cfg.scaleMode = ScaleMode::Expand;
    vs.configure(cfg);

    // 16:10 aspect ratio (Steam Deck native)
    vs.update(1280, 800);

    // Scale is determined by width (1280/1280 = 1.0)
    EXPECT_FLOAT_EQ(vs.getScale(), 1.0f);
    // Width stays the same, height expands to show more world
    EXPECT_EQ(vs.getEffectiveWidth(), 1280);
    EXPECT_EQ(vs.getEffectiveHeight(), 800);

    // Viewport fills entire window
    auto vp = vs.getViewport();
    EXPECT_FLOAT_EQ(vp.x, 0.0f);
    EXPECT_FLOAT_EQ(vp.y, 0.0f);
    EXPECT_FLOAT_EQ(vp.width, 1280.0f);
    EXPECT_FLOAT_EQ(vp.height, 800.0f);
}

TEST(ViewportScalerTest, ExpandMode_TallerWindow) {
    ViewportScaler vs;
    ViewportConfig cfg;
    cfg.designWidth = 1280;
    cfg.designHeight = 720;
    cfg.scaleMode = ScaleMode::Expand;
    vs.configure(cfg);

    // Taller than design aspect
    vs.update(800, 600);

    // Window is taller than 16:9 design, so scale by width
    float expectedScale = 800.0f / 1280.0f;
    EXPECT_FLOAT_EQ(vs.getScale(), expectedScale);
    EXPECT_EQ(vs.getEffectiveWidth(), 1280);
    // Height expands
    int expectedHeight = static_cast<int>(std::round(600.0f / expectedScale));
    EXPECT_EQ(vs.getEffectiveHeight(), expectedHeight);
}

TEST(ViewportScalerTest, ExpandMode_ExactDesignResolution) {
    ViewportScaler vs;
    ViewportConfig cfg;
    cfg.designWidth = 1280;
    cfg.designHeight = 720;
    cfg.scaleMode = ScaleMode::Expand;
    vs.configure(cfg);

    vs.update(1280, 720);

    EXPECT_FLOAT_EQ(vs.getScale(), 1.0f);
    EXPECT_EQ(vs.getEffectiveWidth(), 1280);
    EXPECT_EQ(vs.getEffectiveHeight(), 720);
}

TEST(ViewportScalerTest, FitLetterbox_WiderWindow) {
    ViewportScaler vs;
    ViewportConfig cfg;
    cfg.designWidth = 1280;
    cfg.designHeight = 720;
    cfg.scaleMode = ScaleMode::FitLetterbox;
    vs.configure(cfg);

    // Window is wider than design
    vs.update(1920, 720);

    // Scale limited by height: 720/720 = 1.0
    EXPECT_FLOAT_EQ(vs.getScale(), 1.0f);
    // Design resolution preserved
    EXPECT_EQ(vs.getEffectiveWidth(), 1280);
    EXPECT_EQ(vs.getEffectiveHeight(), 720);

    // Pillarbox bars expected (viewport centered)
    auto vp = vs.getViewport();
    EXPECT_FLOAT_EQ(vp.width, 1280.0f);
    EXPECT_FLOAT_EQ(vp.height, 720.0f);
    EXPECT_GT(vp.x, 0.0f);  // Centered horizontally
}

TEST(ViewportScalerTest, FitLetterbox_TallerWindow) {
    ViewportScaler vs;
    ViewportConfig cfg;
    cfg.designWidth = 1280;
    cfg.designHeight = 720;
    cfg.scaleMode = ScaleMode::FitLetterbox;
    vs.configure(cfg);

    // Window is taller than design
    vs.update(1280, 1024);

    // Scale limited by width: 1280/1280 = 1.0
    EXPECT_FLOAT_EQ(vs.getScale(), 1.0f);
    // Design resolution preserved
    EXPECT_EQ(vs.getEffectiveWidth(), 1280);
    EXPECT_EQ(vs.getEffectiveHeight(), 720);

    // Letterbox bars expected (viewport centered)
    auto vp = vs.getViewport();
    EXPECT_FLOAT_EQ(vp.width, 1280.0f);
    EXPECT_FLOAT_EQ(vp.height, 720.0f);
    EXPECT_GT(vp.y, 0.0f);  // Centered vertically
}

TEST(ViewportScalerTest, FillCrop_WiderWindow) {
    ViewportScaler vs;
    ViewportConfig cfg;
    cfg.designWidth = 1280;
    cfg.designHeight = 720;
    cfg.scaleMode = ScaleMode::FillCrop;
    vs.configure(cfg);

    vs.update(1920, 720);

    // Scale to fill: max(1920/1280, 720/720) = 1.5
    EXPECT_FLOAT_EQ(vs.getScale(), 1.5f);
    // Design resolution preserved (cropped content)
    EXPECT_EQ(vs.getEffectiveWidth(), 1280);
    EXPECT_EQ(vs.getEffectiveHeight(), 720);
}

TEST(ViewportScalerTest, StretchMode) {
    ViewportScaler vs;
    ViewportConfig cfg;
    cfg.designWidth = 1280;
    cfg.designHeight = 720;
    cfg.scaleMode = ScaleMode::Stretch;
    vs.configure(cfg);

    vs.update(1920, 1080);

    // Viewport fills window
    auto vp = vs.getViewport();
    EXPECT_FLOAT_EQ(vp.x, 0.0f);
    EXPECT_FLOAT_EQ(vp.y, 0.0f);
    EXPECT_FLOAT_EQ(vp.width, 1920.0f);
    EXPECT_FLOAT_EQ(vp.height, 1080.0f);

    // Design resolution preserved
    EXPECT_EQ(vs.getEffectiveWidth(), 1280);
    EXPECT_EQ(vs.getEffectiveHeight(), 720);
}

TEST(ViewportScalerTest, ScreenToGame_Identity) {
    ViewportScaler vs;
    ViewportConfig cfg;
    cfg.scaleMode = ScaleMode::Expand;
    vs.configure(cfg);
    vs.update(1280, 720);

    Vec2 result = vs.screenToGame({640.0f, 360.0f});
    EXPECT_FLOAT_EQ(result.x, 640.0f);
    EXPECT_FLOAT_EQ(result.y, 360.0f);
}

TEST(ViewportScalerTest, ScreenToGame_Scaled) {
    ViewportScaler vs;
    ViewportConfig cfg;
    cfg.designWidth = 1280;
    cfg.designHeight = 720;
    cfg.scaleMode = ScaleMode::Expand;
    vs.configure(cfg);
    vs.update(2560, 1440);  // 2x scale

    Vec2 result = vs.screenToGame({1280.0f, 720.0f});
    EXPECT_FLOAT_EQ(result.x, 640.0f);
    EXPECT_FLOAT_EQ(result.y, 360.0f);
}

TEST(ViewportScalerTest, GameToScreen_Scaled) {
    ViewportScaler vs;
    ViewportConfig cfg;
    cfg.designWidth = 1280;
    cfg.designHeight = 720;
    cfg.scaleMode = ScaleMode::Expand;
    vs.configure(cfg);
    vs.update(2560, 1440);  // 2x scale

    Vec2 result = vs.gameToScreen({640.0f, 360.0f});
    EXPECT_FLOAT_EQ(result.x, 1280.0f);
    EXPECT_FLOAT_EQ(result.y, 720.0f);
}

TEST(ViewportScalerTest, ScreenToGameRoundTrip) {
    ViewportScaler vs;
    ViewportConfig cfg;
    cfg.designWidth = 1280;
    cfg.designHeight = 720;
    cfg.scaleMode = ScaleMode::FitLetterbox;
    vs.configure(cfg);
    vs.update(1920, 1080);

    Vec2 original(500.0f, 300.0f);
    Vec2 screen = vs.gameToScreen(original);
    Vec2 back = vs.screenToGame(screen);

    EXPECT_NEAR(back.x, original.x, 0.01f);
    EXPECT_NEAR(back.y, original.y, 0.01f);
}

TEST(ViewportScalerTest, StretchMode_ScreenToGame) {
    ViewportScaler vs;
    ViewportConfig cfg;
    cfg.designWidth = 1280;
    cfg.designHeight = 720;
    cfg.scaleMode = ScaleMode::Stretch;
    vs.configure(cfg);
    vs.update(1920, 1080);

    // Screen center should map to game center
    Vec2 result = vs.screenToGame({960.0f, 540.0f});
    EXPECT_FLOAT_EQ(result.x, 640.0f);
    EXPECT_FLOAT_EQ(result.y, 360.0f);
}

TEST(ViewportScalerTest, ZeroWindowSize) {
    ViewportScaler vs;
    ViewportConfig cfg;
    vs.configure(cfg);

    // Should not crash with zero dimensions
    vs.update(0, 0);
    // Values remain at defaults
    EXPECT_EQ(vs.getEffectiveWidth(), 1280);
    EXPECT_EQ(vs.getEffectiveHeight(), 720);
}

TEST(ViewportScalerTest, SteamDeck_1280x800_Expand) {
    ViewportScaler vs;
    ViewportConfig cfg;
    cfg.designWidth = 1280;
    cfg.designHeight = 720;
    cfg.scaleMode = ScaleMode::Expand;
    vs.configure(cfg);

    // Steam Deck native resolution
    vs.update(1280, 800);

    // At 1280 width, scale = 1.0, and height expands from 720 to 800
    EXPECT_FLOAT_EQ(vs.getScale(), 1.0f);
    EXPECT_EQ(vs.getEffectiveWidth(), 1280);
    EXPECT_EQ(vs.getEffectiveHeight(), 800);
}

// =============================================================================
// UIScaling Tests
// =============================================================================

TEST(UIScalingTest, DefaultConfig) {
    UIScaling scaling;
    UIScalingConfig cfg;
    scaling.configure(cfg);

    EXPECT_FLOAT_EQ(scaling.getScale(), 1.0f);
    EXPECT_EQ(scaling.scaleFontSize(16), 16);
    EXPECT_FLOAT_EQ(scaling.scaleDimension(100.0f), 100.0f);
}

TEST(UIScalingTest, MinFontSizeEnforcement) {
    UIScaling scaling;
    UIScalingConfig cfg;
    cfg.minFontSize = 12;
    scaling.configure(cfg);

    // A font size of 8 should be clamped to the minimum of 12
    EXPECT_EQ(scaling.scaleFontSize(8), 12);
    // A font size of 16 should remain 16
    EXPECT_EQ(scaling.scaleFontSize(16), 16);
    // A font size of 12 should remain 12
    EXPECT_EQ(scaling.scaleFontSize(12), 12);
}

TEST(UIScalingTest, MinFontSize_SteamDeckRequirement) {
    UIScaling scaling;
    UIScalingConfig cfg;
    cfg.minFontSize = 9;  // Valve minimum for Steam Deck Verified
    scaling.configure(cfg);

    // Even a font size of 1 should be clamped to 9
    EXPECT_GE(scaling.scaleFontSize(1), 9);
}

TEST(UIScalingTest, BaseScaleMultiplier) {
    UIScaling scaling;
    UIScalingConfig cfg;
    cfg.baseScale = 1.5f;
    cfg.minFontSize = 1;
    scaling.configure(cfg);

    EXPECT_FLOAT_EQ(scaling.getScale(), 1.5f);
    EXPECT_EQ(scaling.scaleFontSize(10), 15);  // 10 * 1.5 = 15
    EXPECT_FLOAT_EQ(scaling.scaleDimension(100.0f), 150.0f);
}

TEST(UIScalingTest, DpiScale) {
    UIScaling scaling;
    UIScalingConfig cfg;
    cfg.baseScale = 1.0f;
    cfg.dpiScale = 2.0f;
    cfg.minFontSize = 1;
    scaling.configure(cfg);

    // Effective scale = baseScale * dpiScale = 2.0
    EXPECT_FLOAT_EQ(scaling.getScale(), 2.0f);
    EXPECT_EQ(scaling.scaleFontSize(10), 20);
}

TEST(UIScalingTest, CombinedScaling) {
    UIScaling scaling;
    UIScalingConfig cfg;
    cfg.baseScale = 1.5f;
    cfg.dpiScale = 2.0f;
    cfg.minFontSize = 1;
    scaling.configure(cfg);

    // Effective scale = 1.5 * 2.0 = 3.0
    EXPECT_FLOAT_EQ(scaling.getScale(), 3.0f);
    EXPECT_EQ(scaling.scaleFontSize(10), 30);
}

TEST(UIScalingTest, AutoDetect_NativeResolution) {
    UIScaling scaling;
    UIScalingConfig cfg;
    cfg.minFontSize = 1;
    scaling.configure(cfg);

    // At 1280x720 (reference resolution), dpiScale should be 1.0
    scaling.autoDetect(1280, 720);
    EXPECT_FLOAT_EQ(scaling.getScale(), 1.0f);
}

TEST(UIScalingTest, AutoDetect_HigherResolution) {
    UIScaling scaling;
    UIScalingConfig cfg;
    cfg.minFontSize = 1;
    scaling.configure(cfg);

    // At 2560x1440 (2x the reference), dpiScale should be 2.0
    scaling.autoDetect(2560, 1440);
    EXPECT_FLOAT_EQ(scaling.getScale(), 2.0f);
}

TEST(UIScalingTest, AutoDetect_SteamDeck) {
    UIScaling scaling;
    UIScalingConfig cfg;
    cfg.minFontSize = 1;
    scaling.configure(cfg);

    // Steam Deck: 1280x800, reference is 1280x720
    // scaleX = 1280/1280 = 1.0, scaleY = 800/720 = 1.11
    // Takes the minimum: 1.0
    scaling.autoDetect(1280, 800);
    EXPECT_FLOAT_EQ(scaling.getScale(), 1.0f);
}

TEST(UIScalingTest, AutoDetect_LowerResolution) {
    UIScaling scaling;
    UIScalingConfig cfg;
    cfg.minFontSize = 1;
    scaling.configure(cfg);

    // At 640x360 (half), dpiScale should be 0.5
    scaling.autoDetect(640, 360);
    EXPECT_FLOAT_EQ(scaling.getScale(), 0.5f);
}

TEST(UIScalingTest, ScalePosition) {
    UIScaling scaling;
    UIScalingConfig cfg;
    cfg.baseScale = 2.0f;
    cfg.minFontSize = 1;
    scaling.configure(cfg);

    Vec2 result = scaling.scalePosition({100.0f, 200.0f});
    EXPECT_FLOAT_EQ(result.x, 200.0f);
    EXPECT_FLOAT_EQ(result.y, 400.0f);
}

TEST(UIScalingTest, SetBaseScale_Clamp) {
    UIScaling scaling;
    UIScalingConfig cfg;
    scaling.configure(cfg);

    scaling.setBaseScale(0.0f);
    // Should be clamped to minimum of 0.1
    EXPECT_GE(scaling.getScale(), 0.1f);
}

TEST(UIScalingTest, SetMinFontSize) {
    UIScaling scaling;
    UIScalingConfig cfg;
    cfg.minFontSize = 8;
    scaling.configure(cfg);

    EXPECT_EQ(scaling.scaleFontSize(4), 8);

    scaling.setMinFontSize(14);
    EXPECT_EQ(scaling.scaleFontSize(10), 14);
}

TEST(UIScalingTest, AutoDetect_ZeroSize) {
    UIScaling scaling;
    UIScalingConfig cfg;
    scaling.configure(cfg);

    // Should not crash or produce invalid scale
    scaling.autoDetect(0, 0);
    EXPECT_FLOAT_EQ(scaling.getScale(), 1.0f);  // Unchanged from default
}

// =============================================================================
// Time Target FPS Tests
// =============================================================================

TEST(TimeTargetFPSTest, DefaultTargetFPS) {
    Time time;
    EXPECT_EQ(time.getTargetFPS(), 0);
}

TEST(TimeTargetFPSTest, ClampNextDelta) {
    Time time;
    time.clampNextDelta(0.05);

    // Simulate a large delta (e.g., after suspend)
    time.update(1.0);

    // Delta should be clamped to 0.05, not 1.0 or MAX_DELTA
    EXPECT_LE(time.deltaTime(), 0.05);
}

TEST(TimeTargetFPSTest, ClampNextDelta_OneShotOnly) {
    Time time;
    time.clampNextDelta(0.05);

    // First update: clamped
    time.update(1.0);
    EXPECT_LE(time.deltaTime(), 0.05);

    // Second update: normal MAX_DELTA applies
    time.update(0.5);
    EXPECT_LE(time.deltaTime(), 0.25);
    EXPECT_GT(time.deltaTime(), 0.05);
}

// =============================================================================
// FullscreenMode Enum Tests
// =============================================================================

TEST(WindowConfigTest, DefaultFullscreenMode) {
    WindowConfig cfg;
    EXPECT_EQ(cfg.fullscreenMode, FullscreenMode::BorderlessFullscreen);
}

TEST(WindowConfigTest, FullscreenModeEnum) {
    // Verify all enum values are distinct
    EXPECT_NE(static_cast<int>(FullscreenMode::Windowed),
              static_cast<int>(FullscreenMode::Fullscreen));
    EXPECT_NE(static_cast<int>(FullscreenMode::Fullscreen),
              static_cast<int>(FullscreenMode::BorderlessFullscreen));
    EXPECT_NE(static_cast<int>(FullscreenMode::Windowed),
              static_cast<int>(FullscreenMode::BorderlessFullscreen));
}

// =============================================================================
// ScaleMode Enum Tests
// =============================================================================

TEST(ScaleModeTest, AllModesDistinct) {
    EXPECT_NE(static_cast<int>(ScaleMode::FillCrop),
              static_cast<int>(ScaleMode::FitLetterbox));
    EXPECT_NE(static_cast<int>(ScaleMode::FitLetterbox),
              static_cast<int>(ScaleMode::Stretch));
    EXPECT_NE(static_cast<int>(ScaleMode::Stretch),
              static_cast<int>(ScaleMode::Expand));
}

TEST(ScaleModeTest, DefaultConfigIsExpand) {
    ViewportConfig cfg;
    EXPECT_EQ(cfg.scaleMode, ScaleMode::Expand);
}
