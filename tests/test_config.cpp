#include <gtest/gtest.h>
#include "engine/Config.hpp"

using namespace gloaming;

TEST(ConfigTest, LoadFromValidString) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"name": "test", "value": 42})"));
    EXPECT_EQ(cfg.getString("name"), "test");
    EXPECT_EQ(cfg.getInt("value"), 42);
}

TEST(ConfigTest, LoadFromInvalidString) {
    Config cfg;
    EXPECT_FALSE(cfg.loadFromString("{invalid json}"));
}

TEST(ConfigTest, LoadFromMissingFile) {
    Config cfg;
    EXPECT_FALSE(cfg.loadFromFile("nonexistent_file.json"));
}

TEST(ConfigTest, DotNotation) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({
        "window": {
            "width": 1920,
            "height": 1080,
            "title": "Test",
            "fullscreen": true
        }
    })"));

    EXPECT_EQ(cfg.getInt("window.width"), 1920);
    EXPECT_EQ(cfg.getInt("window.height"), 1080);
    EXPECT_EQ(cfg.getString("window.title"), "Test");
    EXPECT_TRUE(cfg.getBool("window.fullscreen"));
}

TEST(ConfigTest, DefaultValues) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({})"));

    EXPECT_EQ(cfg.getString("missing", "fallback"), "fallback");
    EXPECT_EQ(cfg.getInt("missing", 99), 99);
    EXPECT_FLOAT_EQ(cfg.getFloat("missing", 3.14f), 3.14f);
    EXPECT_EQ(cfg.getBool("missing", true), true);
}

TEST(ConfigTest, HasKey) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"a": {"b": 1}})"));

    EXPECT_TRUE(cfg.hasKey("a"));
    EXPECT_TRUE(cfg.hasKey("a.b"));
    EXPECT_FALSE(cfg.hasKey("a.c"));
    EXPECT_FALSE(cfg.hasKey("x"));
}

TEST(ConfigTest, TypeMismatchReturnsDefault) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"name": "hello", "count": 5})"));

    // Requesting int from a string key should return default
    EXPECT_EQ(cfg.getInt("name", -1), -1);
    // Requesting string from an int key should return default
    EXPECT_EQ(cfg.getString("count", "nope"), "nope");
}

TEST(ConfigTest, FloatValues) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"pi": 3.14159, "int_as_float": 10})"));

    EXPECT_NEAR(cfg.getFloat("pi"), 3.14159f, 0.001f);
    // An integer should also work as a float
    EXPECT_FLOAT_EQ(cfg.getFloat("int_as_float"), 10.0f);
}

TEST(ConfigTest, DeepNesting) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"a": {"b": {"c": {"d": 42}}}})"));

    EXPECT_EQ(cfg.getInt("a.b.c.d"), 42);
    EXPECT_FALSE(cfg.hasKey("a.b.c.e"));
}

// =============================================================================
// Setter Tests
// =============================================================================

TEST(ConfigTest, SetString) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({})"));

    cfg.setString("name", "hello");
    EXPECT_EQ(cfg.getString("name"), "hello");
    EXPECT_TRUE(cfg.hasKey("name"));
}

TEST(ConfigTest, SetInt) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({})"));

    cfg.setInt("count", 42);
    EXPECT_EQ(cfg.getInt("count"), 42);
}

TEST(ConfigTest, SetFloat) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({})"));

    cfg.setFloat("ratio", 3.14f);
    EXPECT_NEAR(cfg.getFloat("ratio"), 3.14f, 0.01f);
}

TEST(ConfigTest, SetBool) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({})"));

    cfg.setBool("enabled", true);
    EXPECT_TRUE(cfg.getBool("enabled"));

    cfg.setBool("enabled", false);
    EXPECT_FALSE(cfg.getBool("enabled"));
}

TEST(ConfigTest, SetOverwritesExisting) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"name": "old"})"));

    cfg.setString("name", "new");
    EXPECT_EQ(cfg.getString("name"), "new");
}

TEST(ConfigTest, SetWithDotNotation) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"window": {}})"));

    cfg.setInt("window.width", 1920);
    cfg.setInt("window.height", 1080);
    EXPECT_EQ(cfg.getInt("window.width"), 1920);
    EXPECT_EQ(cfg.getInt("window.height"), 1080);
}

TEST(ConfigTest, SetCreatesIntermediateObjects) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({})"));

    cfg.setString("a.b.c", "deep");
    EXPECT_EQ(cfg.getString("a.b.c"), "deep");
    EXPECT_TRUE(cfg.hasKey("a"));
    EXPECT_TRUE(cfg.hasKey("a.b"));
    EXPECT_TRUE(cfg.hasKey("a.b.c"));
}

// =============================================================================
// Dirty Keys Tracking
// =============================================================================

TEST(ConfigTest, DirtyKeysInitiallyEmpty) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({})"));
    EXPECT_TRUE(cfg.dirtyKeys().empty());
}

TEST(ConfigTest, DirtyKeysAfterSet) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({})"));

    cfg.setString("name", "test");
    cfg.setInt("count", 5);

    const auto& dirty = cfg.dirtyKeys();
    EXPECT_EQ(dirty.size(), 2u);
    EXPECT_TRUE(dirty.count("name") > 0);
    EXPECT_TRUE(dirty.count("count") > 0);
}

TEST(ConfigTest, DirtyKeysNotSetByLoad) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"name": "test"})"));
    EXPECT_TRUE(cfg.dirtyKeys().empty());
}

// =============================================================================
// Raw JSON Access
// =============================================================================

TEST(ConfigTest, RawAccess) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"x": 10})"));

    const auto& raw = cfg.raw();
    EXPECT_TRUE(raw.contains("x"));
    EXPECT_EQ(raw["x"], 10);
}

TEST(ConfigTest, RawEmptyConfig) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({})"));
    EXPECT_TRUE(cfg.raw().empty());
}

// =============================================================================
// Save and Load Roundtrip (via temp file)
// =============================================================================

TEST(ConfigTest, SaveToFileAndReload) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({
        "name": "test_game",
        "window": {"width": 800, "height": 600}
    })"));

    std::string tmpPath = "/tmp/gloaming_test_config.json";
    ASSERT_TRUE(cfg.saveToFile(tmpPath));

    Config cfg2;
    ASSERT_TRUE(cfg2.loadFromFile(tmpPath));
    EXPECT_EQ(cfg2.getString("name"), "test_game");
    EXPECT_EQ(cfg2.getInt("window.width"), 800);
    EXPECT_EQ(cfg2.getInt("window.height"), 600);

    std::remove(tmpPath.c_str());
}

TEST(ConfigTest, SaveOverridesOnly) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"base": "value", "count": 0})"));

    cfg.setInt("count", 42);
    cfg.setString("new_key", "hello");

    std::string tmpPath = "/tmp/gloaming_test_overrides.json";
    ASSERT_TRUE(cfg.saveOverridesToFile(tmpPath));

    Config overrides;
    ASSERT_TRUE(overrides.loadFromFile(tmpPath));
    EXPECT_EQ(overrides.getInt("count"), 42);
    EXPECT_EQ(overrides.getString("new_key"), "hello");
    // Base key should NOT be in overrides file
    EXPECT_FALSE(overrides.hasKey("base"));

    std::remove(tmpPath.c_str());
}

TEST(ConfigTest, SaveOverridesEmptyReturnsFalse) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"name": "test"})"));
    EXPECT_FALSE(cfg.saveOverridesToFile("/tmp/gloaming_no_overrides.json"));
}

TEST(ConfigTest, SaveToInvalidPathFails) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({})"));
    EXPECT_FALSE(cfg.saveToFile("/nonexistent/directory/config.json"));
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(ConfigTest, BooleanValues) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"on": true, "off": false})"));
    EXPECT_TRUE(cfg.getBool("on"));
    EXPECT_FALSE(cfg.getBool("off"));
}

TEST(ConfigTest, ArrayValuesReturnDefault) {
    Config cfg;
    ASSERT_TRUE(cfg.loadFromString(R"({"list": [1, 2, 3]})"));
    EXPECT_EQ(cfg.getInt("list", -1), -1);
}
