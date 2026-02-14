#pragma once

#include <sol/sol.hpp>

namespace gloaming {

class Engine;

/// Registers Stage 19E Lua APIs: Configuration and Persistence.
///
/// Provides:
///   config.get_string(key, default?)   — read string from config
///   config.get_int(key, default?)      — read int from config
///   config.get_float(key, default?)    — read float from config
///   config.get_bool(key, default?)     — read bool from config
///   config.set_string(key, value)      — set string in config
///   config.set_int(key, value)         — set int in config
///   config.set_float(key, value)       — set float in config
///   config.set_bool(key, value)        — set bool in config
///   config.save_local()                — persist local config to config.local.json
void bindConfigPersistenceAPI(sol::state& lua, Engine& engine);

} // namespace gloaming
