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
