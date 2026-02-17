#include <gtest/gtest.h>
#include "engine/Time.hpp"

using namespace gloaming;

TEST(TimeTest, InitialState) {
    Time time;
    EXPECT_DOUBLE_EQ(time.deltaTime(), 0.0);
    EXPECT_DOUBLE_EQ(time.elapsedTime(), 0.0);
    EXPECT_EQ(time.frameCount(), 0u);
}

TEST(TimeTest, SingleUpdate) {
    Time time;
    time.update(0.016); // ~60fps

    EXPECT_NEAR(time.deltaTime(), 0.016, 0.0001);
    EXPECT_NEAR(time.elapsedTime(), 0.016, 0.0001);
    EXPECT_EQ(time.frameCount(), 1u);
}

TEST(TimeTest, MultipleUpdates) {
    Time time;
    time.update(0.016);
    time.update(0.016);
    time.update(0.020);

    EXPECT_NEAR(time.deltaTime(), 0.020, 0.0001);
    EXPECT_NEAR(time.elapsedTime(), 0.052, 0.0001);
    EXPECT_EQ(time.frameCount(), 3u);
}

TEST(TimeTest, DeltaTimeClamped) {
    Time time;
    // Simulate a huge lag spike (2 seconds)
    time.update(2.0);

    // Should be clamped to MAX_DELTA (0.25)
    EXPECT_LE(time.deltaTime(), 0.25);
    EXPECT_LE(time.elapsedTime(), 0.25);
}

TEST(TimeTest, FpsCalculation) {
    Time time;

    // Simulate 60 frames at ~60fps to trigger FPS calculation
    for (int i = 0; i < 60; ++i) {
        time.update(1.0 / 60.0);
    }

    // FPS should be approximately 60
    EXPECT_NEAR(time.fps(), 60.0, 1.0);
}

TEST(TimeTest, ElapsedTimeAccumulates) {
    Time time;
    for (int i = 0; i < 100; ++i) {
        time.update(0.01);
    }

    EXPECT_NEAR(time.elapsedTime(), 1.0, 0.01);
    EXPECT_EQ(time.frameCount(), 100u);
}

// =============================================================================
// Raw Delta Time
// =============================================================================

TEST(TimeTest, RawDeltaTimePreserved) {
    Time time;
    time.update(2.0);  // Over MAX_DELTA

    // deltaTime should be clamped
    EXPECT_LE(time.deltaTime(), 0.25);
    // rawDeltaTime should preserve the original value
    EXPECT_NEAR(time.rawDeltaTime(), 2.0, 0.001);
}

TEST(TimeTest, RawDeltaTimeNormal) {
    Time time;
    time.update(0.016);

    EXPECT_NEAR(time.rawDeltaTime(), 0.016, 0.0001);
    EXPECT_NEAR(time.deltaTime(), 0.016, 0.0001);
}

TEST(TimeTest, RawDeltaTimeInitiallyZero) {
    Time time;
    EXPECT_DOUBLE_EQ(time.rawDeltaTime(), 0.0);
}

// =============================================================================
// Target FPS
// =============================================================================

TEST(TimeTest, TargetFPSDefault) {
    Time time;
    EXPECT_EQ(time.getTargetFPS(), 0);
}

// =============================================================================
// Clamp Next Delta
// =============================================================================

TEST(TimeTest, ClampNextDeltaLimitsFrame) {
    Time time;
    time.clampNextDelta(0.05);
    time.update(0.2);

    EXPECT_LE(time.deltaTime(), 0.05 + 0.001);
}

TEST(TimeTest, ClampNextDeltaOneShotOnly) {
    Time time;
    time.clampNextDelta(0.05);
    time.update(0.2);

    // Second update should use normal clamping (0.1 < MAX_DELTA so not clamped)
    time.update(0.1);
    EXPECT_NEAR(time.deltaTime(), 0.1, 0.001);
}

TEST(TimeTest, ClampNextDeltaDoesNotAffectRaw) {
    Time time;
    time.clampNextDelta(0.05);
    time.update(0.2);

    EXPECT_NEAR(time.rawDeltaTime(), 0.2, 0.001);
}

// =============================================================================
// Delta Time Edge Cases
// =============================================================================

TEST(TimeTest, ZeroDeltaTime) {
    Time time;
    time.update(0.0);

    EXPECT_DOUBLE_EQ(time.deltaTime(), 0.0);
    EXPECT_EQ(time.frameCount(), 1u);
}

TEST(TimeTest, ExactlyAtMaxDelta) {
    Time time;
    time.update(0.25);

    EXPECT_NEAR(time.deltaTime(), 0.25, 0.001);
}

TEST(TimeTest, JustOverMaxDelta) {
    Time time;
    time.update(0.251);

    EXPECT_LE(time.deltaTime(), 0.25);
}

// =============================================================================
// FPS Calculation Details
// =============================================================================

TEST(TimeTest, FpsInitiallyZero) {
    Time time;
    EXPECT_DOUBLE_EQ(time.fps(), 0.0);
}

TEST(TimeTest, FpsCalculatedEvery60Frames) {
    Time time;
    // FPS is computed in batches of 60 frames
    // At 30fps: 60 frames * (1/30)s = 2s -> fps = 60/2 = 30
    for (int i = 0; i < 60; ++i) {
        time.update(1.0 / 30.0);
    }
    EXPECT_NEAR(time.fps(), 30.0, 2.0);
}

TEST(TimeTest, FpsUpdatesAfterNextBatch) {
    Time time;
    // First batch at 60fps
    for (int i = 0; i < 60; ++i) {
        time.update(1.0 / 60.0);
    }
    EXPECT_NEAR(time.fps(), 60.0, 2.0);

    // Second batch at 30fps
    for (int i = 0; i < 60; ++i) {
        time.update(1.0 / 30.0);
    }
    EXPECT_NEAR(time.fps(), 30.0, 2.0);
}
