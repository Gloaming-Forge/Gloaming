#include <gtest/gtest.h>
#include "gameplay/TweenSystem.hpp"

#include <cmath>

using namespace gloaming;

// =============================================================================
// All easing functions should satisfy f(0)=0 and f(1)=1
// =============================================================================

class EasingBoundaryTest : public ::testing::TestWithParam<EasingFunction> {};

TEST_P(EasingBoundaryTest, StartsAtZero) {
    EXPECT_NEAR(GetParam()(0.0f), 0.0f, 0.001f);
}

TEST_P(EasingBoundaryTest, EndsAtOne) {
    EXPECT_NEAR(GetParam()(1.0f), 1.0f, 0.001f);
}

INSTANTIATE_TEST_SUITE_P(AllEasings, EasingBoundaryTest,
    ::testing::Values(
        Easing::linear,
        Easing::easeInQuad,
        Easing::easeOutQuad,
        Easing::easeInOutQuad,
        Easing::easeInCubic,
        Easing::easeOutCubic,
        Easing::easeInOutCubic,
        Easing::easeOutElastic,
        Easing::easeInElastic,
        Easing::easeOutBounce,
        Easing::easeInBounce,
        Easing::easeInBack,
        Easing::easeOutBack,
        Easing::easeInOutBack
    )
);

// =============================================================================
// Monotonicity for simple easings (no overshoot)
// =============================================================================

class EasingMonotonicity : public ::testing::TestWithParam<EasingFunction> {};

TEST_P(EasingMonotonicity, IsMonotonic) {
    float prev = GetParam()(0.0f);
    for (int i = 1; i <= 100; ++i) {
        float t = static_cast<float>(i) / 100.0f;
        float val = GetParam()(t);
        EXPECT_GE(val, prev - 0.001f)
            << "Easing not monotonic at t=" << t;
        prev = val;
    }
}

INSTANTIATE_TEST_SUITE_P(MonotonicEasings, EasingMonotonicity,
    ::testing::Values(
        Easing::linear,
        Easing::easeInQuad,
        Easing::easeOutQuad,
        Easing::easeInOutQuad,
        Easing::easeInCubic,
        Easing::easeOutCubic,
        Easing::easeInOutCubic
    )
);

// =============================================================================
// Linear Easing
// =============================================================================

TEST(EasingTest, LinearMidpoint) {
    EXPECT_FLOAT_EQ(Easing::linear(0.5f), 0.5f);
}

TEST(EasingTest, LinearQuarter) {
    EXPECT_FLOAT_EQ(Easing::linear(0.25f), 0.25f);
}

// =============================================================================
// Quadratic Easing
// =============================================================================

TEST(EasingTest, EaseInQuadMidpoint) {
    // easeInQuad(0.5) = 0.25
    EXPECT_FLOAT_EQ(Easing::easeInQuad(0.5f), 0.25f);
}

TEST(EasingTest, EaseOutQuadMidpoint) {
    // easeOutQuad(0.5) = 0.75
    EXPECT_FLOAT_EQ(Easing::easeOutQuad(0.5f), 0.75f);
}

TEST(EasingTest, EaseInOutQuadMidpoint) {
    EXPECT_NEAR(Easing::easeInOutQuad(0.5f), 0.5f, 0.001f);
}

// =============================================================================
// Cubic Easing
// =============================================================================

TEST(EasingTest, EaseInCubicMidpoint) {
    // easeInCubic(0.5) = 0.125
    EXPECT_FLOAT_EQ(Easing::easeInCubic(0.5f), 0.125f);
}

TEST(EasingTest, EaseOutCubicMidpoint) {
    // easeOutCubic(0.5) = 0.875
    EXPECT_FLOAT_EQ(Easing::easeOutCubic(0.5f), 0.875f);
}

// =============================================================================
// Elastic Easing — can overshoot
// =============================================================================

TEST(EasingTest, EaseOutElasticOvershoots) {
    // Elastic easing typically overshoots past 1.0 before settling
    bool overshot = false;
    for (int i = 0; i <= 100; ++i) {
        float t = static_cast<float>(i) / 100.0f;
        if (Easing::easeOutElastic(t) > 1.01f) {
            overshot = true;
            break;
        }
    }
    EXPECT_TRUE(overshot) << "easeOutElastic should overshoot past 1.0";
}

// =============================================================================
// Bounce Easing
// =============================================================================

TEST(EasingTest, EaseOutBounceStaysInRange) {
    for (int i = 0; i <= 100; ++i) {
        float t = static_cast<float>(i) / 100.0f;
        float val = Easing::easeOutBounce(t);
        EXPECT_GE(val, -0.01f) << "easeOutBounce < 0 at t=" << t;
        EXPECT_LE(val, 1.01f) << "easeOutBounce > 1 at t=" << t;
    }
}

TEST(EasingTest, EaseInBounceStaysInRange) {
    for (int i = 0; i <= 100; ++i) {
        float t = static_cast<float>(i) / 100.0f;
        float val = Easing::easeInBounce(t);
        EXPECT_GE(val, -0.01f) << "easeInBounce < 0 at t=" << t;
        EXPECT_LE(val, 1.01f) << "easeInBounce > 1 at t=" << t;
    }
}

// =============================================================================
// Back Easing — intentional overshoot
// =============================================================================

TEST(EasingTest, EaseInBackUndershoots) {
    // easeInBack should go negative initially (pull back before accelerating)
    bool undershot = false;
    for (int i = 0; i <= 100; ++i) {
        float t = static_cast<float>(i) / 100.0f;
        if (Easing::easeInBack(t) < -0.01f) {
            undershot = true;
            break;
        }
    }
    EXPECT_TRUE(undershot) << "easeInBack should undershoot below 0";
}

TEST(EasingTest, EaseOutBackOvershoots) {
    bool overshot = false;
    for (int i = 0; i <= 100; ++i) {
        float t = static_cast<float>(i) / 100.0f;
        if (Easing::easeOutBack(t) > 1.01f) {
            overshot = true;
            break;
        }
    }
    EXPECT_TRUE(overshot) << "easeOutBack should overshoot past 1.0";
}

// =============================================================================
// TweenSystem Basic Tests (no Registry needed)
// =============================================================================

TEST(TweenSystemTest, InitiallyEmpty) {
    TweenSystem tweens;
    EXPECT_EQ(tweens.activeCount(), 0u);
    EXPECT_FALSE(tweens.isShaking());
}

TEST(TweenSystemTest, ShakeOffset) {
    TweenSystem tweens;
    Vec2 offset = tweens.getShakeOffset();
    EXPECT_FLOAT_EQ(offset.x, 0.0f);
    EXPECT_FLOAT_EQ(offset.y, 0.0f);
}

TEST(TweenSystemTest, Clear) {
    TweenSystem tweens;
    tweens.clear();
    EXPECT_EQ(tweens.activeCount(), 0u);
}

// =============================================================================
// CameraShake Tests
// =============================================================================

TEST(CameraShakeTest, DefaultState) {
    CameraShake shake;
    EXPECT_FALSE(shake.active);
    EXPECT_FLOAT_EQ(shake.intensity, 0.0f);
    EXPECT_FLOAT_EQ(shake.duration, 0.0f);
}

// =============================================================================
// getEasingByName (if available)
// =============================================================================

TEST(EasingByNameTest, LinearByName) {
    auto fn = getEasingByName("linear");
    ASSERT_NE(fn, nullptr);
    EXPECT_FLOAT_EQ(fn(0.5f), 0.5f);
}

TEST(EasingByNameTest, EaseInQuadByName) {
    // getEasingByName uses snake_case (Lua convention)
    auto fn = getEasingByName("ease_in_quad");
    ASSERT_NE(fn, nullptr);
    EXPECT_FLOAT_EQ(fn(0.5f), 0.25f);
}

TEST(EasingByNameTest, UnknownNameReturnsLinear) {
    auto fn = getEasingByName("nonexistent");
    // Should return a fallback (likely linear)
    ASSERT_NE(fn, nullptr);
    EXPECT_NEAR(fn(0.5f), 0.5f, 0.001f);
}

// =============================================================================
// Tween Struct
// =============================================================================

TEST(TweenTest, InvalidTweenIdConstant) {
    EXPECT_EQ(InvalidTweenId, 0u);
}

TEST(TweenTest, DefaultTweenState) {
    Tween tween;
    EXPECT_EQ(tween.id, InvalidTweenId);
    EXPECT_EQ(tween.entity, NullEntity);
    EXPECT_TRUE(tween.alive);
    EXPECT_FALSE(tween.started);
}
