#pragma once

#include <sol/sol.hpp>

namespace gloaming {

class Engine;
class SceneManager;
class TimerSystem;
class SaveSystem;

/// Registers all Stage 16 Lua APIs: Scene Management, Timer/Scheduler, Save/Load.
///
/// Provides:
///   scene.*  — Scene registration, transitions, scene stack, persistent entities
///   timer.*  — Delayed and repeating callbacks, entity-scoped, pause-aware
///   save.*   — Key-value persistence per mod
void bindSceneTimerSaveAPI(sol::state& lua, Engine& engine,
                           SceneManager& sceneManager,
                           TimerSystem& timerSystem,
                           SaveSystem& saveSystem);

} // namespace gloaming
