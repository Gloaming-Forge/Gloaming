#pragma once

#include <memory>
#include <string>
#include <spdlog/spdlog.h>

namespace gloaming {

class Log {
public:
    static void init(const std::string& logFile = "", const std::string& level = "debug");
    static void shutdown();

    static std::shared_ptr<spdlog::logger>& getEngineLogger();
    static std::shared_ptr<spdlog::logger>& getModLogger();

private:
    static std::shared_ptr<spdlog::logger> s_engineLogger;
    static std::shared_ptr<spdlog::logger> s_modLogger;
};

} // namespace gloaming

// Engine logging macros
#define LOG_TRACE(...)    ::gloaming::Log::getEngineLogger()->trace(__VA_ARGS__)
#define LOG_DEBUG(...)    ::gloaming::Log::getEngineLogger()->debug(__VA_ARGS__)
#define LOG_INFO(...)     ::gloaming::Log::getEngineLogger()->info(__VA_ARGS__)
#define LOG_WARN(...)     ::gloaming::Log::getEngineLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...)    ::gloaming::Log::getEngineLogger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::gloaming::Log::getEngineLogger()->critical(__VA_ARGS__)

// Mod logging macros
#define MOD_LOG_TRACE(...)    ::gloaming::Log::getModLogger()->trace(__VA_ARGS__)
#define MOD_LOG_DEBUG(...)    ::gloaming::Log::getModLogger()->debug(__VA_ARGS__)
#define MOD_LOG_INFO(...)     ::gloaming::Log::getModLogger()->info(__VA_ARGS__)
#define MOD_LOG_WARN(...)     ::gloaming::Log::getModLogger()->warn(__VA_ARGS__)
#define MOD_LOG_ERROR(...)    ::gloaming::Log::getModLogger()->error(__VA_ARGS__)
#define MOD_LOG_CRITICAL(...) ::gloaming::Log::getModLogger()->critical(__VA_ARGS__)
