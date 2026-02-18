#include <gtest/gtest.h>
#include "engine/Log.hpp"

#include <filesystem>
#include <fstream>
#include <string>

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

TEST_F(LogTest, ShutdownSafe) {
    Log::init();
    EXPECT_NO_THROW(Log::shutdown());
}

TEST_F(LogTest, ShutdownWithoutInitSafe) {
    EXPECT_NO_THROW(Log::shutdown());
}

TEST_F(LogTest, ReinitAfterShutdown) {
    Log::init();
    Log::shutdown();
    spdlog::drop_all();

    ASSERT_NO_THROW(Log::init());
    EXPECT_NE(Log::getEngineLogger(), nullptr);
    EXPECT_NE(Log::getModLogger(), nullptr);
}

TEST_F(LogTest, LogLevelTrace) {
    Log::init("", "trace");
    EXPECT_EQ(Log::getEngineLogger()->level(), spdlog::level::trace);
}

TEST_F(LogTest, LogLevelDebug) {
    Log::init("", "debug");
    EXPECT_EQ(Log::getEngineLogger()->level(), spdlog::level::debug);
}

TEST_F(LogTest, LogLevelInfo) {
    Log::init("", "info");
    EXPECT_EQ(Log::getEngineLogger()->level(), spdlog::level::info);
}

TEST_F(LogTest, LogLevelError) {
    Log::init("", "error");
    EXPECT_EQ(Log::getEngineLogger()->level(), spdlog::level::err);
}

TEST_F(LogTest, LogLevelCritical) {
    Log::init("", "critical");
    EXPECT_EQ(Log::getEngineLogger()->level(), spdlog::level::critical);
}

TEST_F(LogTest, FormatStringInEngineLogger) {
    Log::init();
    EXPECT_NO_THROW(Log::getEngineLogger()->info("Value: {}, Name: {}", 42, "test"));
}

TEST_F(LogTest, FormatStringInModLogger) {
    Log::init();
    EXPECT_NO_THROW(Log::getModLogger()->warn("Warning code: {}", 404));
}

TEST_F(LogTest, AllEngineMacroLevels) {
    Log::init("", "trace");
    EXPECT_NO_THROW(LOG_TRACE("trace message"));
    EXPECT_NO_THROW(LOG_DEBUG("debug message"));
    EXPECT_NO_THROW(LOG_INFO("info message"));
    EXPECT_NO_THROW(LOG_WARN("warn message"));
    EXPECT_NO_THROW(LOG_ERROR("error message"));
    EXPECT_NO_THROW(LOG_CRITICAL("critical message"));
}

TEST_F(LogTest, AllModMacroLevels) {
    Log::init("", "trace");
    EXPECT_NO_THROW(MOD_LOG_TRACE("trace message"));
    EXPECT_NO_THROW(MOD_LOG_DEBUG("debug message"));
    EXPECT_NO_THROW(MOD_LOG_INFO("info message"));
    EXPECT_NO_THROW(MOD_LOG_WARN("warn message"));
    EXPECT_NO_THROW(MOD_LOG_ERROR("error message"));
    EXPECT_NO_THROW(MOD_LOG_CRITICAL("critical message"));
}

TEST_F(LogTest, MacrosWithFormatArgs) {
    Log::init();
    EXPECT_NO_THROW(LOG_INFO("Player {} scored {}", "Alice", 100));
    EXPECT_NO_THROW(MOD_LOG_INFO("Mod {} loaded v{}", "mymod", "1.0"));
}

class LogFileTest : public ::testing::Test {
protected:
    std::string logPath;

    void SetUp() override {
        spdlog::drop_all();
        logPath = (std::filesystem::temp_directory_path() / "gloaming_test_log.txt").string();
        std::filesystem::remove(logPath);
    }

    void TearDown() override {
        spdlog::drop_all();
        std::filesystem::remove(logPath);
    }
};

TEST_F(LogFileTest, FileLogging) {
    Log::init(logPath, "debug");
    LOG_INFO("File log test message");

    // Flush to ensure the message is written
    Log::getEngineLogger()->flush();

    // Verify file exists (content verification would require reading the file)
    std::ifstream f(logPath);
    EXPECT_TRUE(f.good()) << "Log file should have been created";
}
