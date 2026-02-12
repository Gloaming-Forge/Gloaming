#include <gtest/gtest.h>

#include "gameplay/ParticleSystem.hpp"
#include "gameplay/TweenSystem.hpp"
#include "gameplay/DebugDrawSystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"

using namespace gloaming;

// =============================================================================
// ParticleSystem Tests
// =============================================================================

class ParticleSystemTest : public ::testing::Test {
protected:
    // ParticleSystem derives from System, so we can't easily test without
    // a full Engine/Registry setup. We test the config structures and
    // public API behavior at a unit level.
};

TEST(ParticleConfigTest, RangeFDefaults) {
    RangeF r;
    EXPECT_FLOAT_EQ(r.min, 0.0f);
    EXPECT_FLOAT_EQ(r.max, 0.0f);
}

TEST(ParticleConfigTest, RangeFSingleValue) {
    RangeF r(5.0f);
    EXPECT_FLOAT_EQ(r.min, 5.0f);
    EXPECT_FLOAT_EQ(r.max, 5.0f);
}

TEST(ParticleConfigTest, RangeFRange) {
    RangeF r(1.0f, 10.0f);
    EXPECT_FLOAT_EQ(r.min, 1.0f);
    EXPECT_FLOAT_EQ(r.max, 10.0f);
}

TEST(ParticleConfigTest, SizeCurveEvaluate) {
    SizeCurve curve(10.0f, 2.0f);
    EXPECT_FLOAT_EQ(curve.evaluate(0.0f), 10.0f);
    EXPECT_FLOAT_EQ(curve.evaluate(1.0f), 2.0f);
    EXPECT_FLOAT_EQ(curve.evaluate(0.5f), 6.0f);
}

TEST(ParticleConfigTest, ColorFLerp) {
    ColorF a(0, 0, 0, 255);
    ColorF b(255, 255, 255, 0);
    ColorF mid = ColorF::lerp(a, b, 0.5f);
    EXPECT_NEAR(mid.r, 127.5f, 0.01f);
    EXPECT_NEAR(mid.g, 127.5f, 0.01f);
    EXPECT_NEAR(mid.b, 127.5f, 0.01f);
    EXPECT_NEAR(mid.a, 127.5f, 0.01f);
}

TEST(ParticleConfigTest, ColorFToColor) {
    ColorF cf(128.0f, 64.0f, 200.0f, 100.0f);
    Color c = cf.toColor();
    EXPECT_EQ(c.r, 128);
    EXPECT_EQ(c.g, 64);
    EXPECT_EQ(c.b, 200);
    EXPECT_EQ(c.a, 100);
}

TEST(ParticleConfigTest, ColorFClamp) {
    ColorF cf(300.0f, -10.0f, 128.0f, 255.0f);
    Color c = cf.toColor();
    EXPECT_EQ(c.r, 255); // Clamped
    EXPECT_EQ(c.g, 0);   // Clamped
    EXPECT_EQ(c.b, 128);
    EXPECT_EQ(c.a, 255);
}

TEST(ParticleConfigTest, EmitterConfigDefaults) {
    ParticleEmitterConfig config;
    EXPECT_FLOAT_EQ(config.rate, 10.0f);
    EXPECT_EQ(config.count, 0);
    EXPECT_FLOAT_EQ(config.gravity, 0.0f);
    EXPECT_FALSE(config.followCamera);
    EXPECT_TRUE(config.worldSpace);
    EXPECT_TRUE(config.fade);
}

// =============================================================================
// TweenSystem Tests
// =============================================================================

class TweenSystemTest : public ::testing::Test {
protected:
    Registry registry;
    TweenSystem tweenSystem;
    Entity testEntity;

    void SetUp() override {
        testEntity = registry.create(Transform{Vec2(0, 0)}, Sprite{});
    }
};

TEST_F(TweenSystemTest, TweenToBasic) {
    TweenId id = tweenSystem.tweenTo(testEntity, TweenProperty::X, 100.0f,
                                      1.0f, Easing::linear);
    EXPECT_NE(id, InvalidTweenId);
    EXPECT_EQ(tweenSystem.activeCount(), 1u);
}

TEST_F(TweenSystemTest, TweenUpdatesProperty) {
    tweenSystem.tweenTo(testEntity, TweenProperty::X, 100.0f,
                        1.0f, Easing::linear);

    // After 0.5 seconds, X should be ~50
    tweenSystem.update(0.5f, registry);
    auto* transform = registry.tryGet<Transform>(testEntity);
    ASSERT_NE(transform, nullptr);
    EXPECT_NEAR(transform->position.x, 50.0f, 1.0f);
}

TEST_F(TweenSystemTest, TweenCompletes) {
    bool completed = false;
    tweenSystem.tweenTo(testEntity, TweenProperty::X, 100.0f,
                        1.0f, Easing::linear, [&completed]() {
        completed = true;
    });

    tweenSystem.update(1.0f, registry);
    EXPECT_TRUE(completed);
    EXPECT_EQ(tweenSystem.activeCount(), 0u);

    auto* transform = registry.tryGet<Transform>(testEntity);
    EXPECT_NEAR(transform->position.x, 100.0f, 0.01f);
}

TEST_F(TweenSystemTest, TweenCancel) {
    TweenId id = tweenSystem.tweenTo(testEntity, TweenProperty::X, 100.0f,
                                      1.0f, Easing::linear);
    EXPECT_TRUE(tweenSystem.cancel(id));
    tweenSystem.update(0.5f, registry);
    EXPECT_EQ(tweenSystem.activeCount(), 0u);
}

TEST_F(TweenSystemTest, CancelAllForEntity) {
    tweenSystem.tweenTo(testEntity, TweenProperty::X, 100.0f, 1.0f);
    tweenSystem.tweenTo(testEntity, TweenProperty::Y, 200.0f, 1.0f);
    tweenSystem.tweenTo(testEntity, TweenProperty::Rotation, 45.0f, 1.0f);

    int cancelled = tweenSystem.cancelAllForEntity(testEntity);
    EXPECT_EQ(cancelled, 3);
    EXPECT_EQ(tweenSystem.activeCount(), 0u);
}

TEST_F(TweenSystemTest, CameraShake) {
    EXPECT_FALSE(tweenSystem.isShaking());
    tweenSystem.shake(10.0f, 0.5f);
    EXPECT_TRUE(tweenSystem.isShaking());

    tweenSystem.update(0.1f, registry);
    // Offset should be non-zero during shake
    // (probabilistic, but with intensity 10 it's very unlikely to be exactly 0)
    EXPECT_TRUE(tweenSystem.isShaking());
    (void)tweenSystem.getShakeOffset(); // Verify it doesn't crash

    // Finish the shake
    tweenSystem.update(0.5f, registry);
    EXPECT_FALSE(tweenSystem.isShaking());
    Vec2 finalOffset = tweenSystem.getShakeOffset();
    EXPECT_FLOAT_EQ(finalOffset.x, 0.0f);
    EXPECT_FLOAT_EQ(finalOffset.y, 0.0f);
}

TEST_F(TweenSystemTest, TweenEntityDestroyed) {
    tweenSystem.tweenTo(testEntity, TweenProperty::X, 100.0f, 1.0f);
    registry.destroy(testEntity);
    tweenSystem.update(0.1f, registry);
    EXPECT_EQ(tweenSystem.activeCount(), 0u);
}

TEST_F(TweenSystemTest, TweenAlpha) {
    tweenSystem.tweenTo(testEntity, TweenProperty::Alpha, 0.0f,
                        1.0f, Easing::linear);
    tweenSystem.update(1.0f, registry);

    auto* sprite = registry.tryGet<Sprite>(testEntity);
    ASSERT_NE(sprite, nullptr);
    EXPECT_EQ(sprite->tint.a, 0);
}

TEST_F(TweenSystemTest, ClearAll) {
    tweenSystem.tweenTo(testEntity, TweenProperty::X, 100.0f, 1.0f);
    tweenSystem.shake(5.0f, 1.0f);
    tweenSystem.clear();
    EXPECT_EQ(tweenSystem.activeCount(), 0u);
    EXPECT_FALSE(tweenSystem.isShaking());
}

// =============================================================================
// Easing Function Tests
// =============================================================================

TEST(EasingTest, LinearBoundaries) {
    EXPECT_FLOAT_EQ(Easing::linear(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(Easing::linear(1.0f), 1.0f);
    EXPECT_FLOAT_EQ(Easing::linear(0.5f), 0.5f);
}

TEST(EasingTest, QuadBoundaries) {
    EXPECT_FLOAT_EQ(Easing::easeInQuad(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(Easing::easeInQuad(1.0f), 1.0f);
    EXPECT_FLOAT_EQ(Easing::easeOutQuad(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(Easing::easeOutQuad(1.0f), 1.0f);
    EXPECT_FLOAT_EQ(Easing::easeInOutQuad(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(Easing::easeInOutQuad(1.0f), 1.0f);
}

TEST(EasingTest, CubicBoundaries) {
    EXPECT_FLOAT_EQ(Easing::easeInCubic(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(Easing::easeInCubic(1.0f), 1.0f);
    EXPECT_FLOAT_EQ(Easing::easeOutCubic(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(Easing::easeOutCubic(1.0f), 1.0f);
}

TEST(EasingTest, ElasticBoundaries) {
    EXPECT_FLOAT_EQ(Easing::easeOutElastic(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(Easing::easeOutElastic(1.0f), 1.0f);
}

TEST(EasingTest, BounceBoundaries) {
    EXPECT_FLOAT_EQ(Easing::easeOutBounce(0.0f), 0.0f);
    EXPECT_NEAR(Easing::easeOutBounce(1.0f), 1.0f, 0.001f);
}

TEST(EasingTest, BackBoundaries) {
    EXPECT_FLOAT_EQ(Easing::easeInBack(0.0f), 0.0f);
    EXPECT_NEAR(Easing::easeInBack(1.0f), 1.0f, 0.001f);
    EXPECT_FLOAT_EQ(Easing::easeOutBack(0.0f), 0.0f);
    EXPECT_NEAR(Easing::easeOutBack(1.0f), 1.0f, 0.001f);
}

TEST(EasingTest, GetEasingByName) {
    auto fn = getEasingByName("linear");
    EXPECT_EQ(fn, Easing::linear);

    fn = getEasingByName("ease_out_quad");
    EXPECT_EQ(fn, Easing::easeOutQuad);

    fn = getEasingByName("ease_out_elastic");
    EXPECT_EQ(fn, Easing::easeOutElastic);

    // Unknown name returns linear
    fn = getEasingByName("nonexistent");
    EXPECT_EQ(fn, Easing::linear);
}

TEST(EasingTest, EaseOutQuadMonotonic) {
    // ease_out_quad should be monotonically increasing
    float prev = 0.0f;
    for (float t = 0.0f; t <= 1.0f; t += 0.01f) {
        float val = Easing::easeOutQuad(t);
        EXPECT_GE(val, prev);
        prev = val;
    }
}

// =============================================================================
// DebugDrawSystem Tests
// =============================================================================

TEST(DebugDrawTest, DefaultDisabled) {
    DebugDrawSystem debug;
    EXPECT_FALSE(debug.isEnabled());
}

TEST(DebugDrawTest, EnableDisable) {
    DebugDrawSystem debug;
    debug.setEnabled(true);
    EXPECT_TRUE(debug.isEnabled());
    debug.setEnabled(false);
    EXPECT_FALSE(debug.isEnabled());
}

TEST(DebugDrawTest, Toggle) {
    DebugDrawSystem debug;
    debug.toggle();
    EXPECT_TRUE(debug.isEnabled());
    debug.toggle();
    EXPECT_FALSE(debug.isEnabled());
}

TEST(DebugDrawTest, QueueCommands) {
    DebugDrawSystem debug;

    debug.drawRect(0, 0, 10, 10, Color::Red());
    debug.drawCircle(5, 5, 3, Color::Green());
    debug.drawLine(0, 0, 10, 10, Color::Blue());
    debug.drawText("hello", 0, 0);
    debug.drawPoint(5, 5, Color::White());
    debug.drawTextScreen("screen text", 10, 10);
    debug.drawRectScreen(0, 0, 100, 100, Color::Gray());
    debug.drawLineScreen(0, 0, 50, 50, Color::Red());

    // 8 commands queued
    EXPECT_EQ(debug.commandCount(), 8u);
}

TEST(DebugDrawTest, PathDrawing) {
    DebugDrawSystem debug;

    std::vector<Vec2> points = {{0, 0}, {10, 10}, {20, 0}};
    debug.drawPath(points, Color::Red());

    EXPECT_EQ(debug.commandCount(), 1u); // 1 path
}

TEST(DebugDrawTest, SinglePointPathIgnored) {
    DebugDrawSystem debug;

    std::vector<Vec2> points = {{0, 0}};
    debug.drawPath(points, Color::Red());

    EXPECT_EQ(debug.commandCount(), 0u); // Too few points
}

TEST(DebugDrawTest, RectOutline) {
    DebugDrawSystem debug;
    debug.drawRectOutline(0, 0, 50, 50, Color::Red(), 2.0f);
    EXPECT_EQ(debug.commandCount(), 1u);
}

TEST(DebugDrawTest, CircleOutline) {
    DebugDrawSystem debug;
    debug.drawCircleOutline(10, 10, 20, Color::Blue(), 1.5f);
    EXPECT_EQ(debug.commandCount(), 1u);
}
