#include <gtest/gtest.h>
#include "engine/Log.hpp"

using namespace gloaming;

class LogTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Re-initialize for each test to avoid stale state
        spdlog::drop_all();
    }

    void TearDown() override {
        spdlog::drop_all();
    }
};

TEST_F(LogTest, InitWithDefaults) {
    ASSERT_NO_THROW(Log::init());
    EXPECT_NE(Log::getEngineLogger(), nullptr);
    EXPECT_NE(Log::getModLogger(), nullptr);
}

TEST_F(LogTest, InitWithLogLevel) {
    Log::init("", "warn");
    EXPECT_EQ(Log::getEngineLogger()->level(), spdlog::level::warn);
    EXPECT_EQ(Log::getModLogger()->level(), spdlog::level::warn);
}

TEST_F(LogTest, EngineAndModLoggersAreSeparate) {
    Log::init();
    EXPECT_NE(Log::getEngineLogger()->name(), Log::getModLogger()->name());
}

TEST_F(LogTest, LoggingDoesNotThrow) {
    Log::init();
    EXPECT_NO_THROW(Log::getEngineLogger()->info("Test message"));
    EXPECT_NO_THROW(Log::getModLogger()->info("Mod test message"));
}

TEST_F(LogTest, MacrosDoNotThrow) {
    Log::init();
    EXPECT_NO_THROW(LOG_INFO("Engine macro test"));
    EXPECT_NO_THROW(MOD_LOG_INFO("Mod macro test"));
}

TEST_F(LogTest, AllLogLevels) {
    for (const auto& level : {"trace", "debug", "info", "warn", "error", "critical"}) {
        spdlog::drop_all();
        ASSERT_NO_THROW(Log::init("", level));
    }
}
