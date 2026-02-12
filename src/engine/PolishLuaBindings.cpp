#include "engine/PolishLuaBindings.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "engine/Profiler.hpp"
#include "engine/ResourceManager.hpp"
#include "engine/DiagnosticOverlay.hpp"

namespace gloaming {

void bindPolishAPI(sol::state& lua, Engine& engine,
                   Profiler& profiler,
                   ResourceManager& resourceManager,
                   DiagnosticOverlay& diagnosticOverlay) {

    // =========================================================================
    // profiler API — performance profiling
    // =========================================================================
    auto profilerApi = lua.create_named_table("profiler");

    // profiler.begin_zone(name)
    profilerApi["begin_zone"] = [&profiler](const std::string& name) {
        profiler.beginZone(name);
    };

    // profiler.end_zone(name)
    profilerApi["end_zone"] = [&profiler](const std::string& name) {
        profiler.endZone(name);
    };

    // profiler.zone_stats(name) -> { last_ms, avg_ms, min_ms, max_ms, samples }
    profilerApi["zone_stats"] = [&profiler, &lua](const std::string& name) -> sol::table {
        auto stats = profiler.getZoneStats(name);
        sol::table result = lua.create_table();
        result["name"]     = stats.name;
        result["last_ms"]  = stats.lastTimeMs;
        result["avg_ms"]   = stats.avgTimeMs;
        result["min_ms"]   = stats.minTimeMs;
        result["max_ms"]   = stats.maxTimeMs;
        result["samples"]  = stats.sampleCount;
        return result;
    };

    // profiler.frame_time() -> number (ms)
    profilerApi["frame_time"] = [&profiler]() -> double {
        return profiler.frameTimeMs();
    };

    // profiler.avg_frame_time() -> number (ms)
    profilerApi["avg_frame_time"] = [&profiler]() -> double {
        return profiler.avgFrameTimeMs();
    };

    // profiler.fps() -> number
    profilerApi["fps"] = [&profiler]() -> double {
        double avg = profiler.avgFrameTimeMs();
        return avg > 0.0 ? 1000.0 / avg : 0.0;
    };

    // profiler.budget_usage() -> number (0.0 to 1.0+)
    profilerApi["budget_usage"] = [&profiler]() -> double {
        return profiler.frameBudgetUsage();
    };

    // profiler.set_target_fps(fps)
    profilerApi["set_target_fps"] = [&profiler](int fps) {
        profiler.setTargetFPS(fps);
    };

    // profiler.set_enabled(bool)
    profilerApi["set_enabled"] = [&profiler](bool enabled) {
        profiler.setEnabled(enabled);
    };

    // profiler.is_enabled() -> bool
    profilerApi["is_enabled"] = [&profiler]() -> bool {
        return profiler.isEnabled();
    };

    // profiler.reset()
    profilerApi["reset"] = [&profiler]() {
        profiler.reset();
    };

    // =========================================================================
    // resources API — resource tracking
    // =========================================================================
    auto resourcesApi = lua.create_named_table("resources");

    // resources.track(path, type, size_bytes)
    resourcesApi["track"] = [&resourceManager](const std::string& path,
                                                const std::string& type,
                                                sol::optional<int> sizeBytes) {
        resourceManager.track(path, type,
                              sizeBytes ? static_cast<size_t>(*sizeBytes) : 0);
    };

    // resources.untrack(path)
    resourcesApi["untrack"] = [&resourceManager](const std::string& path) {
        resourceManager.untrack(path);
    };

    // resources.is_tracked(path) -> bool
    resourcesApi["is_tracked"] = [&resourceManager](const std::string& path) -> bool {
        return resourceManager.isTracked(path);
    };

    // resources.stats() -> { textures, sounds, music, scripts, data, total, bytes }
    resourcesApi["stats"] = [&resourceManager, &lua]() -> sol::table {
        auto stats = resourceManager.getStats();
        sol::table result = lua.create_table();
        result["textures"] = stats.textureCount;
        result["sounds"]   = stats.soundCount;
        result["music"]    = stats.musicCount;
        result["scripts"]  = stats.scriptCount;
        result["data"]     = stats.dataCount;
        result["total"]    = stats.totalCount;
        result["bytes"]    = stats.totalBytes;
        return result;
    };

    // resources.count() -> number
    resourcesApi["count"] = [&resourceManager]() -> int {
        return static_cast<int>(resourceManager.count());
    };

    // =========================================================================
    // diagnostics API — overlay control
    // =========================================================================
    auto diagApi = lua.create_named_table("diagnostics");

    // diagnostics.cycle()
    diagApi["cycle"] = [&diagnosticOverlay]() {
        diagnosticOverlay.cycle();
    };

    // diagnostics.set_mode(mode) — "off", "minimal", "full"
    diagApi["set_mode"] = [&diagnosticOverlay](const std::string& mode) {
        if (mode == "off") diagnosticOverlay.setMode(DiagnosticMode::Off);
        else if (mode == "minimal") diagnosticOverlay.setMode(DiagnosticMode::Minimal);
        else if (mode == "full") diagnosticOverlay.setMode(DiagnosticMode::Full);
    };

    // diagnostics.get_mode() -> string
    diagApi["get_mode"] = [&diagnosticOverlay]() -> std::string {
        switch (diagnosticOverlay.getMode()) {
            case DiagnosticMode::Off:     return "off";
            case DiagnosticMode::Minimal: return "minimal";
            case DiagnosticMode::Full:    return "full";
        }
        return "off";
    };

    // diagnostics.is_visible() -> bool
    diagApi["is_visible"] = [&diagnosticOverlay]() -> bool {
        return diagnosticOverlay.isVisible();
    };

    // =========================================================================
    // engine API — engine information and control
    // =========================================================================
    auto engineApi = lua.create_named_table("engine");

    // engine.version() -> string
    engineApi["version"] = []() -> std::string {
        return "0.5.0";
    };

    // engine.frame_count() -> number
    engineApi["frame_count"] = [&engine]() -> double {
        return static_cast<double>(engine.getTime().frameCount());
    };

    // engine.elapsed_time() -> number (seconds)
    engineApi["elapsed_time"] = [&engine]() -> double {
        return engine.getTime().elapsedTime();
    };

    // engine.delta_time() -> number (seconds)
    engineApi["delta_time"] = [&engine]() -> double {
        return engine.getTime().deltaTime();
    };

    // engine.screen_width() -> number
    engineApi["screen_width"] = [&engine]() -> int {
        return engine.getRenderer()->getScreenWidth();
    };

    // engine.screen_height() -> number
    engineApi["screen_height"] = [&engine]() -> int {
        return engine.getRenderer()->getScreenHeight();
    };

    // engine.entity_count() -> number
    engineApi["entity_count"] = [&engine]() -> int {
        return static_cast<int>(engine.getRegistry().alive());
    };

    // engine.log(level, message)
    engineApi["log"] = [](const std::string& level, const std::string& message) {
        if (level == "info")       MOD_LOG_INFO("{}", message);
        else if (level == "warn")  MOD_LOG_WARN("{}", message);
        else if (level == "error") MOD_LOG_ERROR("{}", message);
        else if (level == "debug") MOD_LOG_DEBUG("{}", message);
        else                       MOD_LOG_INFO("{}", message);
    };
}

} // namespace gloaming
