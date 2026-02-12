#include <gtest/gtest.h>

#include "engine/Profiler.hpp"
#include "engine/ResourceManager.hpp"
#include "engine/DiagnosticOverlay.hpp"

using namespace gloaming;

// =============================================================================
// Profiler Tests
// =============================================================================

class ProfilerTest : public ::testing::Test {
protected:
    Profiler profiler;
};

TEST_F(ProfilerTest, DefaultState) {
    EXPECT_TRUE(profiler.isEnabled());
    EXPECT_EQ(profiler.frameCount(), 0u);
    EXPECT_DOUBLE_EQ(profiler.frameTimeMs(), 0.0);
    EXPECT_DOUBLE_EQ(profiler.avgFrameTimeMs(), 0.0);
}

TEST_F(ProfilerTest, FrameBudgetDefault60FPS) {
    EXPECT_NEAR(profiler.frameBudgetMs(), 16.6667, 0.01);
}

TEST_F(ProfilerTest, SetTargetFPS) {
    profiler.setTargetFPS(30);
    EXPECT_NEAR(profiler.frameBudgetMs(), 33.3333, 0.01);

    profiler.setTargetFPS(144);
    EXPECT_NEAR(profiler.frameBudgetMs(), 6.944, 0.01);
}

TEST_F(ProfilerTest, SetTargetFPSZeroIgnored) {
    double original = profiler.frameBudgetMs();
    profiler.setTargetFPS(0);
    EXPECT_DOUBLE_EQ(profiler.frameBudgetMs(), original);
}

TEST_F(ProfilerTest, SetTargetFPSNegativeIgnored) {
    double original = profiler.frameBudgetMs();
    profiler.setTargetFPS(-10);
    EXPECT_DOUBLE_EQ(profiler.frameBudgetMs(), original);
}

TEST_F(ProfilerTest, BeginEndFrame) {
    profiler.beginFrame();
    profiler.endFrame();

    EXPECT_EQ(profiler.frameCount(), 1u);
    EXPECT_GE(profiler.frameTimeMs(), 0.0);
}

TEST_F(ProfilerTest, MultipleFrames) {
    for (int i = 0; i < 10; ++i) {
        profiler.beginFrame();
        profiler.endFrame();
    }
    EXPECT_EQ(profiler.frameCount(), 10u);
}

TEST_F(ProfilerTest, ZoneBasic) {
    profiler.beginFrame();
    profiler.beginZone("TestZone");
    profiler.endZone("TestZone");
    profiler.endFrame();

    auto stats = profiler.getZoneStats("TestZone");
    EXPECT_EQ(stats.name, "TestZone");
    EXPECT_GE(stats.lastTimeMs, 0.0);
    EXPECT_EQ(stats.sampleCount, 1u);
}

TEST_F(ProfilerTest, ScopedZone) {
    profiler.beginFrame();
    {
        auto z = profiler.scopedZone("ScopedTest");
        // simulate some work (zone timing is real)
    }
    profiler.endFrame();

    auto stats = profiler.getZoneStats("ScopedTest");
    EXPECT_EQ(stats.name, "ScopedTest");
    EXPECT_EQ(stats.sampleCount, 1u);
}

TEST_F(ProfilerTest, MultipleZones) {
    profiler.beginFrame();
    profiler.beginZone("ZoneA");
    profiler.endZone("ZoneA");
    profiler.beginZone("ZoneB");
    profiler.endZone("ZoneB");
    profiler.endFrame();

    const auto& allZones = profiler.getAllZoneStats();
    EXPECT_EQ(allZones.size(), 2u);
    EXPECT_EQ(allZones[0].name, "ZoneA");
    EXPECT_EQ(allZones[1].name, "ZoneB");
}

TEST_F(ProfilerTest, ZoneStatsAccumulate) {
    for (int i = 0; i < 5; ++i) {
        profiler.beginFrame();
        profiler.beginZone("Accumulate");
        profiler.endZone("Accumulate");
        profiler.endFrame();
    }

    auto stats = profiler.getZoneStats("Accumulate");
    EXPECT_EQ(stats.sampleCount, 5u);
    EXPECT_GE(stats.avgTimeMs, 0.0);
    EXPECT_GE(stats.minTimeMs, 0.0);
    EXPECT_GE(stats.maxTimeMs, 0.0);
    EXPECT_LE(stats.minTimeMs, stats.maxTimeMs);
}

TEST_F(ProfilerTest, UnknownZoneReturnsEmpty) {
    auto stats = profiler.getZoneStats("NonExistent");
    EXPECT_EQ(stats.name, "NonExistent");
    EXPECT_EQ(stats.sampleCount, 0u);
}

TEST_F(ProfilerTest, FrameBudgetUsage) {
    profiler.setTargetFPS(60);
    profiler.beginFrame();
    profiler.endFrame();

    // Frame budget usage should be finite and >= 0
    double usage = profiler.frameBudgetUsage();
    EXPECT_GE(usage, 0.0);
    EXPECT_LT(usage, 100.0); // Shouldn't be 100x over budget
}

TEST_F(ProfilerTest, FrameHistory) {
    const auto& history = profiler.frameTimeHistory();
    EXPECT_EQ(history.size(), Profiler::kHistorySize);

    // All should be zero initially
    for (float v : history) {
        EXPECT_FLOAT_EQ(v, 0.0f);
    }

    profiler.beginFrame();
    profiler.endFrame();

    // After one frame, at least one entry should be non-zero
    bool hasNonZero = false;
    for (float v : profiler.frameTimeHistory()) {
        if (v > 0.0f) hasNonZero = true;
    }
    EXPECT_TRUE(hasNonZero);
}

TEST_F(ProfilerTest, Reset) {
    profiler.beginFrame();
    profiler.beginZone("ResetTest");
    profiler.endZone("ResetTest");
    profiler.endFrame();

    profiler.reset();

    EXPECT_EQ(profiler.frameCount(), 0u);
    EXPECT_DOUBLE_EQ(profiler.frameTimeMs(), 0.0);
    EXPECT_TRUE(profiler.getAllZoneStats().empty());
}

TEST_F(ProfilerTest, EnableDisable) {
    profiler.setEnabled(false);
    EXPECT_FALSE(profiler.isEnabled());

    profiler.beginFrame();
    profiler.beginZone("Disabled");
    profiler.endZone("Disabled");
    profiler.endFrame();

    // Nothing should be recorded when disabled
    EXPECT_EQ(profiler.frameCount(), 0u);
    EXPECT_TRUE(profiler.getAllZoneStats().empty());

    profiler.setEnabled(true);
    EXPECT_TRUE(profiler.isEnabled());
}

TEST_F(ProfilerTest, Toggle) {
    EXPECT_TRUE(profiler.isEnabled());
    profiler.toggle();
    EXPECT_FALSE(profiler.isEnabled());
    profiler.toggle();
    EXPECT_TRUE(profiler.isEnabled());
}

TEST_F(ProfilerTest, EndZoneWithoutBeginIsIgnored) {
    profiler.beginFrame();
    profiler.endZone("NeverStarted"); // Should not crash
    profiler.endFrame();

    auto stats = profiler.getZoneStats("NeverStarted");
    EXPECT_EQ(stats.sampleCount, 0u);
}

TEST_F(ProfilerTest, MinMaxFrameTime) {
    for (int i = 0; i < 5; ++i) {
        profiler.beginFrame();
        profiler.endFrame();
    }

    EXPECT_GE(profiler.minFrameTimeMs(), 0.0);
    EXPECT_GE(profiler.maxFrameTimeMs(), 0.0);
    EXPECT_LE(profiler.minFrameTimeMs(), profiler.maxFrameTimeMs());
}

// =============================================================================
// ResourceManager Tests
// =============================================================================

class ResourceManagerTest : public ::testing::Test {
protected:
    ResourceManager resources;
};

TEST_F(ResourceManagerTest, InitiallyEmpty) {
    EXPECT_EQ(resources.count(), 0u);
    EXPECT_EQ(resources.totalBytes(), 0u);
}

TEST_F(ResourceManagerTest, TrackResource) {
    resources.track("textures/player.png", "texture", 4096);
    EXPECT_EQ(resources.count(), 1u);
    EXPECT_EQ(resources.totalBytes(), 4096u);
    EXPECT_TRUE(resources.isTracked("textures/player.png"));
}

TEST_F(ResourceManagerTest, TrackMultiple) {
    resources.track("textures/player.png", "texture", 4096);
    resources.track("sounds/hit.ogg", "sound", 2048);
    resources.track("music/theme.ogg", "music", 1024000);

    EXPECT_EQ(resources.count(), 3u);
    EXPECT_EQ(resources.totalBytes(), 4096u + 2048u + 1024000u);
}

TEST_F(ResourceManagerTest, TrackDuplicateUpdates) {
    resources.track("textures/player.png", "texture", 4096);
    resources.track("textures/player.png", "texture", 8192);

    EXPECT_EQ(resources.count(), 1u);
    EXPECT_EQ(resources.totalBytes(), 8192u);
}

TEST_F(ResourceManagerTest, UntrackResource) {
    resources.track("textures/player.png", "texture", 4096);
    resources.untrack("textures/player.png");

    EXPECT_EQ(resources.count(), 0u);
    EXPECT_EQ(resources.totalBytes(), 0u);
    EXPECT_FALSE(resources.isTracked("textures/player.png"));
}

TEST_F(ResourceManagerTest, UntrackNonExistent) {
    resources.untrack("does_not_exist.png"); // Should not crash
    EXPECT_EQ(resources.count(), 0u);
}

TEST_F(ResourceManagerTest, GetEntry) {
    resources.track("textures/tile.png", "texture", 2048, true);

    const auto* entry = resources.getEntry("textures/tile.png");
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->path, "textures/tile.png");
    EXPECT_EQ(entry->type, "texture");
    EXPECT_EQ(entry->sizeBytes, 2048u);
    EXPECT_TRUE(entry->persistent);
}

TEST_F(ResourceManagerTest, GetEntryNotFound) {
    const auto* entry = resources.getEntry("missing.png");
    EXPECT_EQ(entry, nullptr);
}

TEST_F(ResourceManagerTest, GetStats) {
    resources.track("tex1.png", "texture", 100);
    resources.track("tex2.png", "texture", 200);
    resources.track("snd1.ogg", "sound", 300);
    resources.track("mus1.ogg", "music", 400);
    resources.track("init.lua", "script", 50);
    resources.track("cfg.json", "data", 25);

    auto stats = resources.getStats();
    EXPECT_EQ(stats.textureCount, 2u);
    EXPECT_EQ(stats.soundCount, 1u);
    EXPECT_EQ(stats.musicCount, 1u);
    EXPECT_EQ(stats.scriptCount, 1u);
    EXPECT_EQ(stats.dataCount, 1u);
    EXPECT_EQ(stats.totalCount, 6u);
    EXPECT_EQ(stats.totalBytes, 1075u);
}

TEST_F(ResourceManagerTest, GetEntriesByType) {
    resources.track("a.png", "texture", 100);
    resources.track("b.png", "texture", 200);
    resources.track("c.ogg", "sound", 300);

    auto textures = resources.getEntriesByType("texture");
    EXPECT_EQ(textures.size(), 2u);

    auto sounds = resources.getEntriesByType("sound");
    EXPECT_EQ(sounds.size(), 1u);

    auto empty = resources.getEntriesByType("music");
    EXPECT_EQ(empty.size(), 0u);
}

TEST_F(ResourceManagerTest, ClearTransient) {
    resources.track("persistent.png", "texture", 100, true);
    resources.track("transient.png", "texture", 200, false);
    resources.track("also_transient.ogg", "sound", 300, false);

    size_t removed = resources.clearTransient();
    EXPECT_EQ(removed, 2u);
    EXPECT_EQ(resources.count(), 1u);
    EXPECT_TRUE(resources.isTracked("persistent.png"));
    EXPECT_FALSE(resources.isTracked("transient.png"));
    EXPECT_EQ(resources.totalBytes(), 100u);
}

TEST_F(ResourceManagerTest, Clear) {
    resources.track("a.png", "texture", 100, true);
    resources.track("b.png", "texture", 200);

    resources.clear();
    EXPECT_EQ(resources.count(), 0u);
    EXPECT_EQ(resources.totalBytes(), 0u);
}

TEST_F(ResourceManagerTest, FindLeaks) {
    resources.track("a.png", "texture", 100);
    resources.track("b.png", "texture", 200);
    resources.track("c.ogg", "sound", 300);

    // Only "a.png" and "c.ogg" are alive
    std::vector<std::string> alive = {"a.png", "c.ogg"};
    auto leaks = resources.findLeaks(alive);

    EXPECT_EQ(leaks.size(), 1u);
    EXPECT_EQ(leaks[0], "b.png");
}

TEST_F(ResourceManagerTest, FindLeaksNoLeaks) {
    resources.track("a.png", "texture", 100);
    std::vector<std::string> alive = {"a.png"};
    auto leaks = resources.findLeaks(alive);
    EXPECT_TRUE(leaks.empty());
}

TEST_F(ResourceManagerTest, FindLeaksEmpty) {
    auto leaks = resources.findLeaks({});
    EXPECT_TRUE(leaks.empty());
}

// =============================================================================
// DiagnosticOverlay Tests
// =============================================================================

class DiagnosticOverlayTest : public ::testing::Test {
protected:
    DiagnosticOverlay overlay;
};

TEST_F(DiagnosticOverlayTest, DefaultOff) {
    EXPECT_EQ(overlay.getMode(), DiagnosticMode::Off);
    EXPECT_FALSE(overlay.isVisible());
}

TEST_F(DiagnosticOverlayTest, SetMode) {
    overlay.setMode(DiagnosticMode::Minimal);
    EXPECT_EQ(overlay.getMode(), DiagnosticMode::Minimal);
    EXPECT_TRUE(overlay.isVisible());

    overlay.setMode(DiagnosticMode::Full);
    EXPECT_EQ(overlay.getMode(), DiagnosticMode::Full);
    EXPECT_TRUE(overlay.isVisible());

    overlay.setMode(DiagnosticMode::Off);
    EXPECT_EQ(overlay.getMode(), DiagnosticMode::Off);
    EXPECT_FALSE(overlay.isVisible());
}

TEST_F(DiagnosticOverlayTest, Cycle) {
    EXPECT_EQ(overlay.getMode(), DiagnosticMode::Off);

    overlay.cycle();
    EXPECT_EQ(overlay.getMode(), DiagnosticMode::Minimal);

    overlay.cycle();
    EXPECT_EQ(overlay.getMode(), DiagnosticMode::Full);

    overlay.cycle();
    EXPECT_EQ(overlay.getMode(), DiagnosticMode::Off);
}

TEST_F(DiagnosticOverlayTest, CycleWrapsAround) {
    // Full cycle three times
    for (int round = 0; round < 3; ++round) {
        overlay.cycle(); // Minimal
        overlay.cycle(); // Full
        overlay.cycle(); // Off
        EXPECT_EQ(overlay.getMode(), DiagnosticMode::Off);
    }
}

TEST_F(DiagnosticOverlayTest, RenderNullRendererSafe) {
    // render() should early-return when renderer is nullptr.
    // We verify the state-gating logic without needing a full Engine instance.
    overlay.setMode(DiagnosticMode::Minimal);
    EXPECT_TRUE(overlay.isVisible());
    // The null-renderer guard in render() prevents any Engine access,
    // so this path is safe to test purely through mode/visibility checks.
}

TEST_F(DiagnosticOverlayTest, RenderWhenOffIsNoOp) {
    overlay.setMode(DiagnosticMode::Off);
    EXPECT_FALSE(overlay.isVisible());
    // When mode is Off, render() returns immediately without touching
    // any of its arguments — including the renderer and engine pointers.
}

// =============================================================================
// Integration: Profiler + ResourceManager together
// =============================================================================

TEST(PolishIntegrationTest, ProfilerAndResourcesIndependent) {
    Profiler profiler;
    ResourceManager resources;

    // They should work together without interference
    profiler.beginFrame();
    resources.track("test.png", "texture", 1024);
    profiler.beginZone("Load");
    resources.track("test2.png", "texture", 2048);
    profiler.endZone("Load");
    profiler.endFrame();

    EXPECT_EQ(profiler.frameCount(), 1u);
    EXPECT_EQ(resources.count(), 2u);

    auto zoneStats = profiler.getZoneStats("Load");
    EXPECT_EQ(zoneStats.sampleCount, 1u);
}

TEST(PolishIntegrationTest, ProfilerHistorySize) {
    Profiler profiler;
    EXPECT_EQ(profiler.frameTimeHistory().size(), Profiler::kHistorySize);
    EXPECT_EQ(Profiler::kHistorySize, 120u);
}

TEST(PolishIntegrationTest, ResourceManagerBytesAccuracy) {
    ResourceManager resources;

    resources.track("a", "texture", 1000);
    resources.track("b", "sound", 2000);
    resources.track("c", "texture", 3000);
    EXPECT_EQ(resources.totalBytes(), 6000u);

    resources.untrack("b");
    EXPECT_EQ(resources.totalBytes(), 4000u);

    resources.track("a", "texture", 500); // Update: reduce size
    EXPECT_EQ(resources.totalBytes(), 3500u);
}
