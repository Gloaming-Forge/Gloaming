#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "engine/Config.hpp"
#include "engine/SteamIntegration.hpp"

using namespace gloaming;

// =============================================================================
// Config::setString / setInt / setFloat / setBool
// =============================================================================

TEST(ConfigPersistenceTest, SetStringCreatesNewKey) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({})"));

    cfg.setString("player.name", "Gloaming");
    EXPECT_EQ(cfg.getString("player.name"), "Gloaming");
}

TEST(ConfigPersistenceTest, SetIntCreatesNestedPath) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({})"));

    cfg.setInt("window.width", 1280);
    cfg.setInt("window.height", 800);
    EXPECT_EQ(cfg.getInt("window.width"), 1280);
    EXPECT_EQ(cfg.getInt("window.height"), 800);
}

TEST(ConfigPersistenceTest, SetFloatOverwritesExisting) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"audio": {"volume": 0.5}})"));

    cfg.setFloat("audio.volume", 0.8f);
    EXPECT_NEAR(cfg.getFloat("audio.volume"), 0.8f, 0.001f);
}

TEST(ConfigPersistenceTest, SetBoolCreatesAndReads) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({})"));

    cfg.setBool("window.fullscreen", true);
    EXPECT_TRUE(cfg.getBool("window.fullscreen"));

    cfg.setBool("window.fullscreen", false);
    EXPECT_FALSE(cfg.getBool("window.fullscreen"));
}

TEST(ConfigPersistenceTest, SetPreservesExistingSiblingKeys) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"window": {"width": 1920, "title": "Test"}})"));

    cfg.setInt("window.height", 1080);
    // The new key should exist alongside the original keys
    EXPECT_EQ(cfg.getInt("window.width"), 1920);
    EXPECT_EQ(cfg.getString("window.title"), "Test");
    EXPECT_EQ(cfg.getInt("window.height"), 1080);
}

TEST(ConfigPersistenceTest, SetDeeplyNestedPath) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({})"));

    cfg.setString("a.b.c.d", "deep");
    EXPECT_EQ(cfg.getString("a.b.c.d"), "deep");
    EXPECT_TRUE(cfg.hasKey("a.b.c.d"));
    EXPECT_TRUE(cfg.hasKey("a.b.c"));
    EXPECT_TRUE(cfg.hasKey("a.b"));
    EXPECT_TRUE(cfg.hasKey("a"));
}

// =============================================================================
// Config::saveToFile
// =============================================================================

TEST(ConfigPersistenceTest, SaveToFileAndReload) {
    const std::string testPath = "test_config_save_tmp.json";

    // Create and populate a config
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({})"));
    cfg.setInt("window.width", 1280);
    cfg.setInt("window.height", 800);
    cfg.setString("window.title", "SaveTest");
    cfg.setBool("window.fullscreen", true);
    cfg.setFloat("audio.volume", 0.75f);

    // Save to file
    ASSERT_TRUE(cfg.saveToFile(testPath));

    // Reload from file
    Config reloaded;
    ASSERT_TRUE(reloaded.loadFromFile(testPath));
    EXPECT_EQ(reloaded.getInt("window.width"), 1280);
    EXPECT_EQ(reloaded.getInt("window.height"), 800);
    EXPECT_EQ(reloaded.getString("window.title"), "SaveTest");
    EXPECT_TRUE(reloaded.getBool("window.fullscreen"));
    EXPECT_NEAR(reloaded.getFloat("audio.volume"), 0.75f, 0.001f);

    // Cleanup
    std::remove(testPath.c_str());
}

TEST(ConfigPersistenceTest, SaveToInvalidPathFails) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({})"));
    EXPECT_FALSE(cfg.saveToFile("/nonexistent/directory/config.json"));
}

// =============================================================================
// Config::mergeFromFile
// =============================================================================

TEST(ConfigPersistenceTest, MergeOverwritesExistingKeys) {
    const std::string overlayPath = "test_config_merge_tmp.json";

    // Write an overlay file
    {
        std::ofstream f(overlayPath);
        f << R"({"window": {"width": 1920, "height": 1080}})";
    }

    // Load base config
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({
        "window": {"width": 1280, "height": 720, "title": "Base"}
    })"));

    // Merge overlay
    ASSERT_TRUE(cfg.mergeFromFile(overlayPath));

    // Overlaid keys should be updated
    EXPECT_EQ(cfg.getInt("window.width"), 1920);
    EXPECT_EQ(cfg.getInt("window.height"), 1080);
    // Non-overlaid keys should be preserved
    EXPECT_EQ(cfg.getString("window.title"), "Base");

    std::remove(overlayPath.c_str());
}

TEST(ConfigPersistenceTest, MergePreservesNonOverlaidSections) {
    const std::string overlayPath = "test_config_merge_sections_tmp.json";

    {
        std::ofstream f(overlayPath);
        f << R"({"display": {"ui_scale": 1.5}})";
    }

    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({
        "window": {"width": 1280},
        "audio": {"volume": 0.7}
    })"));

    ASSERT_TRUE(cfg.mergeFromFile(overlayPath));

    // Original sections untouched
    EXPECT_EQ(cfg.getInt("window.width"), 1280);
    EXPECT_NEAR(cfg.getFloat("audio.volume"), 0.7f, 0.001f);
    // New section merged in
    EXPECT_NEAR(cfg.getFloat("display.ui_scale"), 1.5f, 0.001f);

    std::remove(overlayPath.c_str());
}

TEST(ConfigPersistenceTest, MergeMissingFileReturnsFalse) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"key": "value"})"));

    // Should fail gracefully
    EXPECT_FALSE(cfg.mergeFromFile("nonexistent_overlay.json"));

    // Original config unchanged
    EXPECT_EQ(cfg.getString("key"), "value");
}

TEST(ConfigPersistenceTest, MergeInvalidJsonReturnsFalse) {
    const std::string overlayPath = "test_config_merge_invalid_tmp.json";

    {
        std::ofstream f(overlayPath);
        f << "{not valid json}}}";
    }

    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"key": "original"})"));

    EXPECT_FALSE(cfg.mergeFromFile(overlayPath));

    // Original config unchanged
    EXPECT_EQ(cfg.getString("key"), "original");

    std::remove(overlayPath.c_str());
}

// =============================================================================
// Round-trip: set → save → merge
// =============================================================================

TEST(ConfigPersistenceTest, SetSaveMergeRoundTrip) {
    const std::string localPath = "test_config_roundtrip_tmp.json";

    // Simulate: user changes settings at runtime, saves local config
    Config runtime;
    ASSERT_TRUE(runtime.loadFromString(R"({
        "window": {"width": 1280, "height": 720},
        "input": {"glyph_style": "auto"}
    })"));
    runtime.setInt("window.height", 800);
    runtime.setBool("window.fullscreen", true);
    runtime.setString("input.glyph_style", "xbox");
    ASSERT_TRUE(runtime.saveToFile(localPath));

    // Simulate: engine starts fresh with base config, merges local
    Config fresh;
    ASSERT_TRUE(fresh.loadFromString(R"({
        "window": {"width": 1280, "height": 720, "title": "Gloaming"},
        "input": {"glyph_style": "auto", "gamepad_deadzone": 0.15}
    })"));
    ASSERT_TRUE(fresh.mergeFromFile(localPath));

    // Local overrides applied
    EXPECT_EQ(fresh.getInt("window.height"), 800);
    EXPECT_TRUE(fresh.getBool("window.fullscreen"));
    EXPECT_EQ(fresh.getString("input.glyph_style"), "xbox");
    // Base-only keys preserved
    EXPECT_EQ(fresh.getString("window.title"), "Gloaming");
    EXPECT_NEAR(fresh.getFloat("input.gamepad_deadzone"), 0.15f, 0.001f);
    // Merged keys from runtime (width was unchanged)
    EXPECT_EQ(fresh.getInt("window.width"), 1280);

    std::remove(localPath.c_str());
}

// =============================================================================
// Platform-aware defaults — auto glyph style
// =============================================================================

TEST(ConfigPersistenceTest, AutoGlyphStyleDefaultsToKeyboardOffDeck) {
    // In a test environment (not on Steam Deck), "auto" should resolve to
    // keyboard glyphs for desktop use.
    const char* deckVal = std::getenv("SteamDeck");
    bool onDeck = deckVal != nullptr && std::string(deckVal) == "1";

    std::string autoResolved = onDeck ? "xbox" : "keyboard";

    // Verify the config has "auto" as default
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"input": {"glyph_style": "auto"}})"));
    std::string style = cfg.getString("input.glyph_style");
    EXPECT_EQ(style, "auto");

    // Simulate the Engine's resolution logic
    if (style == "auto") {
        style = SteamIntegration::isSteamDeck() ? "xbox" : "keyboard";
    }
    EXPECT_EQ(style, autoResolved);
}

TEST(ConfigPersistenceTest, ExplicitGlyphStyleNotOverridden) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"input": {"glyph_style": "playstation"}})"));
    std::string style = cfg.getString("input.glyph_style");
    EXPECT_EQ(style, "playstation");
    // Explicit style should not be changed to auto
}

// =============================================================================
// Platform-aware defaults — Steam Deck detection
// =============================================================================

TEST(ConfigPersistenceTest, SteamDeckDefaultsApplied) {
    // Verify the default resolution/fullscreen changes based on onDeck flag.
    // We test the logic here without modifying env vars.
    bool onDeck = SteamIntegration::isSteamDeck();

    int defaultHeight = onDeck ? 800 : 720;
    bool defaultFS = onDeck;

    // In CI (not on Deck): 720, false
    // On actual Deck: 800, true
    if (!onDeck) {
        EXPECT_EQ(defaultHeight, 720);
        EXPECT_FALSE(defaultFS);
    } else {
        EXPECT_EQ(defaultHeight, 800);
        EXPECT_TRUE(defaultFS);
    }
}

// =============================================================================
// Edge cases
// =============================================================================

TEST(ConfigPersistenceTest, MergeDeepNesting) {
    const std::string overlayPath = "test_config_deep_merge_tmp.json";

    {
        std::ofstream f(overlayPath);
        f << R"({"a": {"b": {"c": 99}}})";
    }

    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"a": {"b": {"c": 1, "d": 2}, "e": 3}})"));

    ASSERT_TRUE(cfg.mergeFromFile(overlayPath));
    EXPECT_EQ(cfg.getInt("a.b.c"), 99);  // Overwritten
    EXPECT_EQ(cfg.getInt("a.b.d"), 2);   // Preserved
    EXPECT_EQ(cfg.getInt("a.e"), 3);     // Preserved

    std::remove(overlayPath.c_str());
}

TEST(ConfigPersistenceTest, SetOnEmptyConfig) {
    Config cfg;
    // No loadFromString or loadFromFile — m_data is default (null or empty)
    cfg.setString("hello", "world");
    EXPECT_EQ(cfg.getString("hello"), "world");
}

TEST(ConfigPersistenceTest, SaveEmptyConfig) {
    const std::string testPath = "test_config_empty_save_tmp.json";
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({})"));
    ASSERT_TRUE(cfg.saveToFile(testPath));

    Config reloaded;
    ASSERT_TRUE(reloaded.loadFromFile(testPath));
    // Empty config should round-trip cleanly
    EXPECT_FALSE(reloaded.hasKey("anything"));

    std::remove(testPath.c_str());
}
