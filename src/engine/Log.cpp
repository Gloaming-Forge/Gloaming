#include "engine/Log.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace gloaming {

std::shared_ptr<spdlog::logger> Log::s_engineLogger;
std::shared_ptr<spdlog::logger> Log::s_modLogger;

void Log::init(const std::string& logFile, const std::string& level) {
    std::vector<spdlog::sink_ptr> sinks;

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_pattern("[%H:%M:%S] [%n] [%^%l%$] %v");
    sinks.push_back(consoleSink);

    if (!logFile.empty()) {
        auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile, true);
        fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");
        sinks.push_back(fileSink);
    }

    s_engineLogger = std::make_shared<spdlog::logger>("ENGINE", sinks.begin(), sinks.end());
    s_modLogger = std::make_shared<spdlog::logger>("MOD", sinks.begin(), sinks.end());

    // Set log level
    auto spdLevel = spdlog::level::debug;
    if (level == "trace")    spdLevel = spdlog::level::trace;
    else if (level == "debug")    spdLevel = spdlog::level::debug;
    else if (level == "info")     spdLevel = spdlog::level::info;
    else if (level == "warn")     spdLevel = spdlog::level::warn;
    else if (level == "error")    spdLevel = spdlog::level::err;
    else if (level == "critical") spdLevel = spdlog::level::critical;

    s_engineLogger->set_level(spdLevel);
    s_modLogger->set_level(spdLevel);

    spdlog::register_logger(s_engineLogger);
    spdlog::register_logger(s_modLogger);
}

void Log::shutdown() {
    spdlog::shutdown();
}

std::shared_ptr<spdlog::logger>& Log::getEngineLogger() {
    return s_engineLogger;
}

std::shared_ptr<spdlog::logger>& Log::getModLogger() {
    return s_modLogger;
}

} // namespace gloaming
