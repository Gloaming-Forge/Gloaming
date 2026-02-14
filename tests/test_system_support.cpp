#include <gtest/gtest.h>
#include <cstdlib>
#include <string>
#include "engine/SteamIntegration.hpp"

using namespace gloaming;

// =============================================================================
// SteamIntegration — Initialization Without SDK
// =============================================================================

TEST(SteamIntegrationTest, InitFailsWithoutSteam) {
    SteamIntegration steam;
    // Without GLOAMING_STEAM compiled in (or without Steam running),
    // init() should return false and isAvailable() should be false.
    bool result = steam.init(480);  // 480 = Spacewar test app ID
    EXPECT_FALSE(result);
    EXPECT_FALSE(steam.isAvailable());
}

TEST(SteamIntegrationTest, InitWithZeroAppId) {
    SteamIntegration steam;
    bool result = steam.init(0);
    EXPECT_FALSE(result);
    EXPECT_FALSE(steam.isAvailable());
}

TEST(SteamIntegrationTest, DoubleInitReturnsConsistently) {
    SteamIntegration steam;
    bool first = steam.init(480);
    bool second = steam.init(480);
    // Without Steam SDK, both return false.
    // With Steam SDK, first would succeed and second would return true (guard).
    // In either case, both calls return the same value and don't crash.
    EXPECT_EQ(first, second);
    EXPECT_FALSE(steam.isAvailable());
}

// =============================================================================
// SteamIntegration — Shutdown Safety
// =============================================================================

TEST(SteamIntegrationTest, ShutdownWithoutInit) {
    SteamIntegration steam;
    // Shutdown without init should not crash
    steam.shutdown();
    EXPECT_FALSE(steam.isAvailable());
}

TEST(SteamIntegrationTest, DoubleShutdownSafe) {
    SteamIntegration steam;
    steam.init(480);
    steam.shutdown();
    steam.shutdown();  // Double shutdown should be safe
    EXPECT_FALSE(steam.isAvailable());
}

// =============================================================================
// SteamIntegration — Update Without Init
// =============================================================================

TEST(SteamIntegrationTest, UpdateWithoutInit) {
    SteamIntegration steam;
    // update() without init should not crash
    steam.update();
}

TEST(SteamIntegrationTest, UpdateAfterShutdown) {
    SteamIntegration steam;
    steam.init(480);
    steam.shutdown();
    steam.update();  // Should not crash
}

// =============================================================================
// SteamIntegration — Keyboard Methods (No-ops Without Steam)
// =============================================================================

TEST(SteamIntegrationTest, ShowKeyboardNoOp) {
    SteamIntegration steam;
    // Should not crash when Steam is unavailable
    steam.showOnScreenKeyboard("Enter name", "", 32);
}

TEST(SteamIntegrationTest, KeyboardResultDefaultFalse) {
    SteamIntegration steam;
    EXPECT_FALSE(steam.hasKeyboardResult());
    EXPECT_EQ(steam.getKeyboardResult(), "");
}

TEST(SteamIntegrationTest, KeyboardResultClearedOnUpdate) {
    SteamIntegration steam;
    // Even without Steam, update should safely clear keyboard state
    EXPECT_FALSE(steam.hasKeyboardResult());
    steam.update();
    EXPECT_FALSE(steam.hasKeyboardResult());
    EXPECT_EQ(steam.getKeyboardResult(), "");
}

// =============================================================================
// SteamIntegration — Glyph Path (No-ops Without Steam)
// =============================================================================

TEST(SteamIntegrationTest, GlyphPathEmptyWithoutSteam) {
    SteamIntegration steam;
    std::string path = steam.getGlyphPath(0);
    EXPECT_TRUE(path.empty());
}

TEST(SteamIntegrationTest, GlyphPathVariousOrigins) {
    SteamIntegration steam;
    // Multiple action origins should all return empty without Steam
    for (int origin = 0; origin < 10; ++origin) {
        EXPECT_TRUE(steam.getGlyphPath(origin).empty());
    }
}

// =============================================================================
// SteamIntegration — Overlay Detection (No-ops Without Steam)
// =============================================================================

TEST(SteamIntegrationTest, OverlayInactiveWithoutSteam) {
    SteamIntegration steam;
    EXPECT_FALSE(steam.isOverlayActive());
}

TEST(SteamIntegrationTest, OverlayAfterShutdown) {
    SteamIntegration steam;
    steam.init(480);
    steam.shutdown();
    EXPECT_FALSE(steam.isOverlayActive());
}

// =============================================================================
// SteamIntegration — Platform Detection (Static Methods)
// =============================================================================

TEST(PlatformDetectionTest, IsSteamDeck_Default) {
    // In a test environment, SteamDeck env var is not set
    // (unless someone is running tests on an actual Deck)
    const char* val = std::getenv("SteamDeck");
    if (val == nullptr || std::string(val) != "1") {
        EXPECT_FALSE(SteamIntegration::isSteamDeck());
    } else {
        EXPECT_TRUE(SteamIntegration::isSteamDeck());
    }
}

TEST(PlatformDetectionTest, IsSteamOS_Default) {
    // isSteamOS() checks that SteamOS env var exists AND equals "1"
    const char* val = std::getenv("SteamOS");
    if (val == nullptr || std::string(val) != "1") {
        EXPECT_FALSE(SteamIntegration::isSteamOS());
    } else {
        EXPECT_TRUE(SteamIntegration::isSteamOS());
    }
}

TEST(PlatformDetectionTest, IsSteamDeck_And_IsSteamOS_Consistent) {
    // Both use the same pattern: check env var exists and equals "1"
    // Verify neither crashes and both return a bool
    bool deck = SteamIntegration::isSteamDeck();
    bool os = SteamIntegration::isSteamOS();
    // On a real Steam Deck, both would be true; in CI, both are false
    (void)deck;
    (void)os;
}

// =============================================================================
// SteamIntegration — Full Lifecycle
// =============================================================================

TEST(SteamIntegrationTest, FullLifecycle) {
    SteamIntegration steam;

    // Phase 1: Uninitialized state
    EXPECT_FALSE(steam.isAvailable());
    EXPECT_FALSE(steam.isOverlayActive());
    EXPECT_FALSE(steam.hasKeyboardResult());
    EXPECT_TRUE(steam.getGlyphPath(0).empty());

    // Phase 2: Attempt init (will fail without Steam running)
    steam.init(480);
    // All features should still work (as no-ops)
    steam.update();
    steam.showOnScreenKeyboard("test", "", 10);
    EXPECT_FALSE(steam.hasKeyboardResult());
    EXPECT_FALSE(steam.isOverlayActive());

    // Phase 3: Shutdown
    steam.shutdown();
    EXPECT_FALSE(steam.isAvailable());

    // Phase 4: Post-shutdown operations should be safe
    steam.update();
    steam.showOnScreenKeyboard("test", "", 10);
    EXPECT_FALSE(steam.hasKeyboardResult());
}

// =============================================================================
// CI Build Verification — Compile-Time Checks
// =============================================================================

TEST(LinuxBuildTest, CompilerVersion) {
    // Verify we're building with a C++20-capable compiler
#if defined(__GNUC__) && !defined(__clang__)
    EXPECT_GE(__GNUC__, 11) << "GCC 11+ required for C++20 support";
#elif defined(__clang__)
    EXPECT_GE(__clang_major__, 13) << "Clang 13+ required for C++20 support";
#else
    FAIL() << "Unknown compiler — expected GCC or Clang";
#endif
}

TEST(LinuxBuildTest, CppStandard) {
    // Verify C++20 is active
    EXPECT_GE(__cplusplus, 202002L) << "C++20 standard is required";
}

TEST(LinuxBuildTest, DependencyHeadersAvailable) {
    // These includes are pulled in transitively by SteamIntegration.hpp
    // and the engine headers. If this test compiles and links, all
    // FetchContent dependencies (Raylib, EnTT, sol2, spdlog, nlohmann_json,
    // Lua) resolved correctly.
    SteamIntegration steam;
    EXPECT_FALSE(steam.isAvailable());
}

// =============================================================================
// Conditional Compilation — GLOAMING_STEAM Flag
// =============================================================================

TEST(ConditionalCompilationTest, SteamFlagState) {
#ifdef GLOAMING_STEAM
    // When Steam is enabled, the STEAM_CALLBACK macro must have compiled
    // successfully, which means the header included steam_api.h.
    // Init will still fail without a running Steam client.
    SteamIntegration steam;
    // May succeed or fail depending on environment — just verify no crash
    steam.init(480);
    steam.shutdown();
#else
    // When Steam is disabled, SteamIntegration is pure no-op
    SteamIntegration steam;
    EXPECT_FALSE(steam.init(480));
    EXPECT_FALSE(steam.isAvailable());
#endif
}

TEST(ConditionalCompilationTest, NoSteamHeadersRequired) {
    // This test verifies that the SteamIntegration header can be
    // included and all public methods called without the Steamworks SDK.
    // If this test compiles and runs, the abstraction layer is correct.
    SteamIntegration steam;
    EXPECT_FALSE(steam.isAvailable());
    EXPECT_FALSE(steam.isOverlayActive());
    EXPECT_FALSE(steam.hasKeyboardResult());
    EXPECT_EQ(steam.getKeyboardResult(), "");
    EXPECT_TRUE(steam.getGlyphPath(0).empty());
    // Static methods
    (void)SteamIntegration::isSteamDeck();
    (void)SteamIntegration::isSteamOS();
}
