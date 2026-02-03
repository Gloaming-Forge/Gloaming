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
