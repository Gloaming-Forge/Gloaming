#include "engine/ConfigPersistenceLuaBindings.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"

namespace gloaming {

void bindConfigPersistenceAPI(sol::state& lua, Engine& engine) {
    sol::table configApi = lua["config"].valid()
        ? lua["config"].get<sol::table>()
        : lua.create_named_table("config");

    // config.get_string(key, default?) -> string
    configApi["get_string"] = [&engine](const std::string& key,
                                         sol::optional<std::string> def) -> std::string {
        return engine.getConfig().getString(key, def.value_or(""));
    };

    // config.get_int(key, default?) -> int
    configApi["get_int"] = [&engine](const std::string& key,
                                      sol::optional<int> def) -> int {
        return engine.getConfig().getInt(key, def.value_or(0));
    };

    // config.get_float(key, default?) -> float
    configApi["get_float"] = [&engine](const std::string& key,
                                        sol::optional<float> def) -> float {
        return engine.getConfig().getFloat(key, def.value_or(0.0f));
    };

    // config.get_bool(key, default?) -> bool
    configApi["get_bool"] = [&engine](const std::string& key,
                                       sol::optional<bool> def) -> bool {
        return engine.getConfig().getBool(key, def.value_or(false));
    };

    // config.set_string(key, value)
    configApi["set_string"] = [&engine](const std::string& key,
                                         const std::string& value) {
        engine.getConfig().setString(key, value);
    };

    // config.set_int(key, value)
    configApi["set_int"] = [&engine](const std::string& key, int value) {
        engine.getConfig().setInt(key, value);
    };

    // config.set_float(key, value)
    configApi["set_float"] = [&engine](const std::string& key, float value) {
        engine.getConfig().setFloat(key, value);
    };

    // config.set_bool(key, value)
    configApi["set_bool"] = [&engine](const std::string& key, bool value) {
        engine.getConfig().setBool(key, value);
    };

    // config.save_local() -> bool
    // Persists the current config to config.local.json for device-specific settings.
    configApi["save_local"] = [&engine]() -> bool {
        bool ok = engine.getConfig().saveToFile("config.local.json");
        if (ok) {
            LOG_INFO("Local config saved to config.local.json");
        } else {
            LOG_WARN("Failed to save local config to config.local.json");
        }
        return ok;
    };
}

} // namespace gloaming
