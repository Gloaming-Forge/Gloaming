#pragma once

#include <sol/sol.hpp>

namespace gloaming {

class Engine;
class Profiler;
class ResourceManager;
class DiagnosticOverlay;

/// Registers Stage 18 Lua APIs: Profiler, Resource info, Diagnostics, Engine info.
///
/// Provides:
///   profiler.*     — Zone timing, frame stats, budget queries
///   resources.*    — Resource counts, memory usage, leak detection
///   diagnostics.*  — Overlay control (mode cycling)
///   engine.*       — Version info, target FPS, frame count, elapsed time
void bindPolishAPI(sol::state& lua, Engine& engine,
                   Profiler& profiler,
                   ResourceManager& resourceManager,
                   DiagnosticOverlay& diagnosticOverlay);

} // namespace gloaming
