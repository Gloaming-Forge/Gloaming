#pragma once

#include <sol/sol.hpp>

namespace gloaming {

class Engine;

/// Registers all Stage 19C Lua APIs: Seamlessness (suspend/resume, graceful exit).
///
/// Provides:
///   engine.is_suspended()    — check if engine is in suspended state
///   engine.request_exit()    — request graceful shutdown
///   engine.on_suspend(fn)    — register suspend callback (shorthand for event.on)
///   engine.on_resume(fn)     — register resume callback (shorthand for event.on)
void bindSeamlessnessAPI(sol::state& lua, Engine& engine);

} // namespace gloaming
