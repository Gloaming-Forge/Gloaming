#include <gtest/gtest.h>
#include <csignal>
#include <atomic>
#include "engine/Time.hpp"
#include "mod/EventBus.hpp"

using namespace gloaming;

// =============================================================================
// Suspend/Resume — Time Delta Clamping Tests
// =============================================================================

TEST(SuspendResumeTest, LargeRawDeltaDetected) {
    Time time;

    // Simulate normal frame
    time.update(0.016);
    EXPECT_NEAR(time.rawDeltaTime(), 0.016, 0.001);
    EXPECT_NEAR(time.deltaTime(), 0.016, 0.001);

    // Simulate OS suspend: large raw delta (e.g., 5 seconds frozen)
    time.update(5.0);
    EXPECT_NEAR(time.rawDeltaTime(), 5.0, 0.001);
    // deltaTime should be clamped to MAX_DELTA (0.25)
    EXPECT_LE(time.deltaTime(), 0.25);
}

TEST(SuspendResumeTest, ClampNextDelta_AfterSuspend) {
    Time time;

    // Set a tight clamp for post-suspend frame
    time.clampNextDelta(0.1);

    // Simulate waking from suspend — large raw delta
    time.update(3.0);

    // Should be clamped to the one-shot value
    EXPECT_LE(time.deltaTime(), 0.1);
    EXPECT_NEAR(time.rawDeltaTime(), 3.0, 0.001);
}

TEST(SuspendResumeTest, ClampNextDelta_DoesNotAffectSubsequentFrames) {
    Time time;

    time.clampNextDelta(0.05);

    // Frame 1: clamped
    time.update(1.0);
    EXPECT_LE(time.deltaTime(), 0.05);

    // Frame 2: normal clamping (MAX_DELTA = 0.25)
    time.update(0.016);
    EXPECT_NEAR(time.deltaTime(), 0.016, 0.001);
}

TEST(SuspendResumeTest, NormalFrameNotAffectedBySuspendThreshold) {
    Time time;

    // Normal frames should pass through unmodified
    time.update(0.016);
    EXPECT_NEAR(time.deltaTime(), 0.016, 0.001);

    time.update(0.033);
    EXPECT_NEAR(time.deltaTime(), 0.033, 0.001);
}

TEST(SuspendResumeTest, MultipleConsecutiveLargeDeltas) {
    Time time;

    // Multiple large deltas (shouldn't happen in practice, but verify stability)
    time.update(2.0);
    EXPECT_LE(time.deltaTime(), 0.25);

    time.update(3.0);
    EXPECT_LE(time.deltaTime(), 0.25);

    // Return to normal
    time.update(0.016);
    EXPECT_NEAR(time.deltaTime(), 0.016, 0.001);
}

// =============================================================================
// Event Bus — Suspend/Resume Event Tests
// =============================================================================

TEST(SuspendResumeEventTest, SuspendEventEmitted) {
    EventBus bus;
    bool suspendFired = false;
    std::string suspendReason;

    bus.on("engine.suspend", [&](const EventData& data) -> bool {
        suspendFired = true;
        suspendReason = data.getString("reason");
        return false;
    });

    // Simulate engine emitting suspend event
    EventData data;
    data.setString("reason", "focus_lost");
    bus.emit("engine.suspend", data);

    EXPECT_TRUE(suspendFired);
    EXPECT_EQ(suspendReason, "focus_lost");
}

TEST(SuspendResumeEventTest, ResumeEventEmitted) {
    EventBus bus;
    bool resumeFired = false;

    bus.on("engine.resume", [&](const EventData&) -> bool {
        resumeFired = true;
        return false;
    });

    bus.emit("engine.resume");

    EXPECT_TRUE(resumeFired);
}

TEST(SuspendResumeEventTest, ShutdownEventEmitted) {
    EventBus bus;
    bool shutdownFired = false;

    bus.on("engine.shutdown", [&](const EventData&) -> bool {
        shutdownFired = true;
        return false;
    });

    bus.emit("engine.shutdown");

    EXPECT_TRUE(shutdownFired);
}

TEST(SuspendResumeEventTest, MultipleHandlersReceiveEvent) {
    EventBus bus;
    int callCount = 0;

    bus.on("engine.suspend", [&](const EventData&) -> bool {
        ++callCount;
        return false;
    });
    bus.on("engine.suspend", [&](const EventData&) -> bool {
        ++callCount;
        return false;
    });

    EventData data;
    data.setString("reason", "focus_lost");
    bus.emit("engine.suspend", data);

    EXPECT_EQ(callCount, 2);
}

TEST(SuspendResumeEventTest, HandlerCanBeUnsubscribed) {
    EventBus bus;
    int callCount = 0;

    auto id = bus.on("engine.suspend", [&](const EventData&) -> bool {
        ++callCount;
        return false;
    });

    EventData data;
    data.setString("reason", "focus_lost");
    bus.emit("engine.suspend", data);
    EXPECT_EQ(callCount, 1);

    bus.off(id);
    bus.emit("engine.suspend", data);
    EXPECT_EQ(callCount, 1);  // No additional call
}

// =============================================================================
// Graceful Exit — Signal Flag Tests
// =============================================================================

TEST(GracefulExitTest, AtomicSignalFlag) {
    // Verify that std::atomic<bool> behaves correctly for signal-flag pattern
    std::atomic<bool> flag{false};
    EXPECT_FALSE(flag.load(std::memory_order_relaxed));

    flag.store(true, std::memory_order_relaxed);
    EXPECT_TRUE(flag.load(std::memory_order_relaxed));

    // Reset
    flag.store(false, std::memory_order_relaxed);
    EXPECT_FALSE(flag.load(std::memory_order_relaxed));
}

TEST(GracefulExitTest, ShutdownEventData) {
    EventBus bus;
    bool received = false;

    bus.on("engine.shutdown", [&](const EventData&) -> bool {
        received = true;
        return false;
    });

    bus.emit("engine.shutdown");
    EXPECT_TRUE(received);
}

// =============================================================================
// Platform Warning Suppression (§4.1) — EventData for Suspend Reason
// =============================================================================

TEST(PlatformPolicyTest, SuspendReasonField) {
    EventData data;
    data.setString("reason", "focus_lost");

    EXPECT_TRUE(data.hasString("reason"));
    EXPECT_EQ(data.getString("reason"), "focus_lost");
}

TEST(PlatformPolicyTest, EventDataMissingField) {
    EventData data;

    EXPECT_FALSE(data.hasString("reason"));
    EXPECT_EQ(data.getString("reason", "default"), "default");
}

// =============================================================================
// Suspend/Resume State Consistency Tests
// =============================================================================

TEST(SuspendStateTest, SuspendThresholdConstant) {
    // The engine uses a 1.0 second threshold. Verify the concept:
    // raw delta > threshold means suspend detected.
    constexpr float SUSPEND_THRESHOLD = 1.0f;

    // Normal frame: no suspend
    EXPECT_FALSE(0.016f > SUSPEND_THRESHOLD);
    // Slow frame: no suspend
    EXPECT_FALSE(0.5f > SUSPEND_THRESHOLD);
    // At threshold: no suspend (must exceed, not equal)
    EXPECT_FALSE(1.0f > SUSPEND_THRESHOLD);
    // Just over threshold: suspend!
    EXPECT_TRUE(1.001f > SUSPEND_THRESHOLD);
    // Long suspend: definitely detected
    EXPECT_TRUE(30.0f > SUSPEND_THRESHOLD);
}

TEST(SuspendStateTest, FocusTimerAccumulation) {
    // Simulate focus timer accumulation logic
    float unfocusedTimer = 0.0f;
    bool wasSuspended = false;
    constexpr float SUSPEND_THRESHOLD = 1.0f;

    // Simulate frames while unfocused — use enough frames to safely exceed
    // the threshold despite float accumulation (63 * 0.016 = 1.008)
    for (int i = 0; i < 63; ++i) {
        float dt = 0.016f;
        unfocusedTimer += dt;
        if (!wasSuspended && unfocusedTimer >= SUSPEND_THRESHOLD) {
            wasSuspended = true;
        }
    }

    EXPECT_TRUE(wasSuspended);
    EXPECT_GE(unfocusedTimer, SUSPEND_THRESHOLD);
}

TEST(SuspendStateTest, BriefFocusLossDoesNotSuspend) {
    float unfocusedTimer = 0.0f;
    bool wasSuspended = false;
    constexpr float SUSPEND_THRESHOLD = 1.0f;

    // Simulate 30 frames unfocused (~0.5 seconds)
    for (int i = 0; i < 30; ++i) {
        unfocusedTimer += 0.016f;
        if (!wasSuspended && unfocusedTimer >= SUSPEND_THRESHOLD) {
            wasSuspended = true;
        }
    }

    EXPECT_FALSE(wasSuspended);

    // Regain focus: reset
    unfocusedTimer = 0.0f;
    wasSuspended = false;
    EXPECT_FALSE(wasSuspended);
    EXPECT_FLOAT_EQ(unfocusedTimer, 0.0f);
}

TEST(SuspendStateTest, ResumeResetsTimer) {
    float unfocusedTimer = 2.0f;  // Was unfocused for 2 seconds
    bool wasSuspended = true;

    // Simulate focus regain
    if (wasSuspended) {
        // Resume actions would happen here
    }
    wasSuspended = false;
    unfocusedTimer = 0.0f;

    EXPECT_FALSE(wasSuspended);
    EXPECT_FLOAT_EQ(unfocusedTimer, 0.0f);
}
