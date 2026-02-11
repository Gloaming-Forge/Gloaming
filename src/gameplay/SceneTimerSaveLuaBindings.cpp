#include "gameplay/SceneTimerSaveLuaBindings.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "gameplay/SceneManager.hpp"
#include "gameplay/TimerSystem.hpp"
#include "gameplay/SaveSystem.hpp"

namespace gloaming {

/// Helper: convert a sol::object to nlohmann::json for the save system
static nlohmann::json solToJson(const sol::object& obj, int depth = 0) {
    if (depth > MAX_NESTING_DEPTH) {
        LOG_WARN("save: value exceeds max nesting depth");
        return nullptr;
    }

    switch (obj.get_type()) {
        case sol::type::nil:
        case sol::type::none:
            return nullptr;
        case sol::type::boolean:
            return obj.as<bool>();
        case sol::type::number: {
            // Check if it's an integer or float
            double d = obj.as<double>();
            if (d == static_cast<double>(static_cast<int64_t>(d)) &&
                d >= -9007199254740992.0 && d <= 9007199254740992.0) {
                return static_cast<int64_t>(d);
            }
            return d;
        }
        case sol::type::string:
            return obj.as<std::string>();
        case sol::type::table: {
            sol::table tbl = obj.as<sol::table>();
            // Determine if this is an array or object by checking keys
            bool isArray = true;
            size_t maxIdx = 0;
            size_t count = 0;
            tbl.for_each([&](const sol::object& key, const sol::object& /*val*/) {
                ++count;
                if (key.get_type() != sol::type::number) {
                    isArray = false;
                } else {
                    double k = key.as<double>();
                    if (k != static_cast<double>(static_cast<size_t>(k)) || k < 1) {
                        isArray = false;
                    } else {
                        maxIdx = std::max(maxIdx, static_cast<size_t>(k));
                    }
                }
            });
            // Also check for sequential integer keys starting at 1
            if (isArray && maxIdx == count && count > 0) {
                nlohmann::json arr = nlohmann::json::array();
                for (size_t i = 1; i <= maxIdx; ++i) {
                    arr.push_back(solToJson(tbl[i], depth + 1));
                }
                return arr;
            } else {
                nlohmann::json object = nlohmann::json::object();
                tbl.for_each([&](const sol::object& key, const sol::object& val) {
                    std::string keyStr;
                    if (key.get_type() == sol::type::string) {
                        keyStr = key.as<std::string>();
                    } else if (key.get_type() == sol::type::number) {
                        keyStr = std::to_string(key.as<int>());
                    } else {
                        return; // skip non-string/number keys
                    }
                    object[keyStr] = solToJson(val, depth + 1);
                });
                return object;
            }
        }
        default:
            return nullptr;
    }
}

/// Helper: convert nlohmann::json to sol::object for Lua return values
static sol::object jsonToSol(sol::state& lua, const nlohmann::json& j) {
    if (j.is_null())    return sol::nil;
    if (j.is_boolean()) return sol::make_object(lua, j.get<bool>());
    if (j.is_number_integer()) return sol::make_object(lua, j.get<int64_t>());
    if (j.is_number_float())   return sol::make_object(lua, j.get<double>());
    if (j.is_string())  return sol::make_object(lua, j.get<std::string>());
    if (j.is_array()) {
        sol::table tbl = lua.create_table();
        for (size_t i = 0; i < j.size(); ++i) {
            tbl[static_cast<int>(i + 1)] = jsonToSol(lua, j[i]);
        }
        return tbl;
    }
    if (j.is_object()) {
        sol::table tbl = lua.create_table();
        for (auto it = j.begin(); it != j.end(); ++it) {
            tbl[it.key()] = jsonToSol(lua, it.value());
        }
        return tbl;
    }
    return sol::nil;
}


void bindSceneTimerSaveAPI(sol::state& lua, Engine& engine,
                           SceneManager& sceneManager,
                           TimerSystem& timerSystem,
                           SaveSystem& saveSystem) {

    // =========================================================================
    // scene API — scene/level management
    // =========================================================================
    auto sceneApi = lua.create_named_table("scene");

    // scene.register(name, { tiles = "...", size = { width, height },
    //                        camera = { mode, x, y, zoom },
    //                        on_enter = fn, on_exit = fn })
    sceneApi["register"] = [&sceneManager](const std::string& name, sol::table opts) {
        SceneDefinition def;
        def.tilesPath = opts.get_or<std::string>("tiles", "");
        def.isOverlay = opts.get_or("overlay", false);

        // Size
        sol::optional<sol::table> sizeTable = opts.get<sol::optional<sol::table>>("size");
        if (sizeTable) {
            def.width = sizeTable->get_or("width", 0);
            def.height = sizeTable->get_or("height", 0);
        }

        // Camera config
        sol::optional<sol::table> cameraTable = opts.get<sol::optional<sol::table>>("camera");
        if (cameraTable) {
            def.camera.configured = true;
            def.camera.mode = cameraTable->get_or<std::string>("mode", "free_follow");
            def.camera.x = cameraTable->get_or("x", 0.0f);
            def.camera.y = cameraTable->get_or("y", 0.0f);
            def.camera.zoom = cameraTable->get_or("zoom", 1.0f);
        }

        // Callbacks
        sol::optional<sol::function> onEnter = opts.get<sol::optional<sol::function>>("on_enter");
        if (onEnter) {
            sol::function fn = *onEnter;
            def.onEnter = [fn]() {
                try { fn(); }
                catch (const std::exception& ex) {
                    MOD_LOG_ERROR("scene on_enter error: {}", ex.what());
                }
            };
        }

        sol::optional<sol::function> onExit = opts.get<sol::optional<sol::function>>("on_exit");
        if (onExit) {
            sol::function fn = *onExit;
            def.onExit = [fn]() {
                try { fn(); }
                catch (const std::exception& ex) {
                    MOD_LOG_ERROR("scene on_exit error: {}", ex.what());
                }
            };
        }

        sceneManager.registerScene(name, std::move(def));
    };

    // scene.go_to(name, { transition = "fade", duration = 0.5 })
    sceneApi["go_to"] = [&sceneManager](const std::string& name, sol::optional<sol::table> opts) -> bool {
        TransitionType transition = TransitionType::Instant;
        float duration = 0.0f;

        if (opts) {
            std::string transStr = opts->get_or<std::string>("transition", "instant");
            transition = parseTransitionType(transStr);
            duration = opts->get_or("duration", 0.0f);
        }

        return sceneManager.goTo(name, transition, duration);
    };

    // scene.push(name) — overlay scene on stack
    sceneApi["push"] = [&sceneManager](const std::string& name) -> bool {
        return sceneManager.push(name);
    };

    // scene.pop() — remove top overlay scene
    sceneApi["pop"] = [&sceneManager]() -> bool {
        return sceneManager.pop();
    };

    // scene.current() -> string
    sceneApi["current"] = [&sceneManager, &lua]() -> sol::object {
        const auto& name = sceneManager.currentScene();
        if (name.empty()) return sol::nil;
        return sol::make_object(lua, name);
    };

    // scene.set_persistent(entityId) — mark entity as surviving scene transitions
    sceneApi["set_persistent"] = [&sceneManager](uint32_t entityId) {
        sceneManager.setPersistent(static_cast<Entity>(entityId));
    };

    // scene.is_persistent(entityId) -> bool
    sceneApi["is_persistent"] = [&sceneManager](uint32_t entityId) -> bool {
        return sceneManager.isPersistent(static_cast<Entity>(entityId));
    };

    // scene.clear_persistent(entityId)
    sceneApi["clear_persistent"] = [&sceneManager](uint32_t entityId) {
        sceneManager.clearPersistent(static_cast<Entity>(entityId));
    };

    // scene.mark_local(entityId) — mark entity as scene-local (destroyed on scene exit)
    sceneApi["mark_local"] = [&engine, &sceneManager](uint32_t entityId) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) return;
        if (!registry.has<SceneLocalEntity>(entity)) {
            registry.add<SceneLocalEntity>(entity, sceneManager.currentScene());
        }
    };

    // scene.is_transitioning() -> bool
    sceneApi["is_transitioning"] = [&sceneManager]() -> bool {
        return sceneManager.isTransitioning();
    };

    // scene.has(name) -> bool
    sceneApi["has"] = [&sceneManager](const std::string& name) -> bool {
        return sceneManager.hasScene(name);
    };

    // =========================================================================
    // timer API — delayed and repeating callbacks
    // =========================================================================
    auto timerApi = lua.create_named_table("timer");

    // timer.after(seconds, callback) -> timerId
    timerApi["after"] = [&timerSystem](float seconds, sol::function callback) -> uint32_t {
        sol::function fn = callback;
        return timerSystem.after(seconds, [fn]() {
            try { fn(); }
            catch (const std::exception& ex) {
                MOD_LOG_ERROR("timer.after callback error: {}", ex.what());
            }
        });
    };

    // timer.every(seconds, callback) -> timerId
    timerApi["every"] = [&timerSystem](float seconds, sol::function callback) -> uint32_t {
        sol::function fn = callback;
        return timerSystem.every(seconds, [fn]() {
            try { fn(); }
            catch (const std::exception& ex) {
                MOD_LOG_ERROR("timer.every callback error: {}", ex.what());
            }
        });
    };

    // timer.cancel(timerId) -> bool
    timerApi["cancel"] = [&timerSystem](uint32_t id) -> bool {
        return timerSystem.cancel(id);
    };

    // timer.after_for(entityId, seconds, callback) -> timerId
    timerApi["after_for"] = [&timerSystem](uint32_t entityId, float seconds,
                                           sol::function callback) -> uint32_t {
        Entity entity = static_cast<Entity>(entityId);
        sol::function fn = callback;
        return timerSystem.afterFor(entity, seconds, [fn]() {
            try { fn(); }
            catch (const std::exception& ex) {
                MOD_LOG_ERROR("timer.after_for callback error: {}", ex.what());
            }
        });
    };

    // timer.every_for(entityId, seconds, callback) -> timerId
    timerApi["every_for"] = [&timerSystem](uint32_t entityId, float seconds,
                                           sol::function callback) -> uint32_t {
        Entity entity = static_cast<Entity>(entityId);
        sol::function fn = callback;
        return timerSystem.everyFor(entity, seconds, [fn]() {
            try { fn(); }
            catch (const std::exception& ex) {
                MOD_LOG_ERROR("timer.every_for callback error: {}", ex.what());
            }
        });
    };

    // timer.active_count() -> int
    timerApi["active_count"] = [&timerSystem]() -> int {
        return static_cast<int>(timerSystem.activeCount());
    };

    // timer.pause(timerId) -> bool
    timerApi["pause"] = [&timerSystem](uint32_t id) -> bool {
        return timerSystem.setPaused(id, true);
    };

    // timer.resume(timerId) -> bool
    timerApi["resume"] = [&timerSystem](uint32_t id) -> bool {
        return timerSystem.setPaused(id, false);
    };

    // =========================================================================
    // save API — key-value persistence per mod
    // =========================================================================
    auto saveApi = lua.create_named_table("save");

    // save.set(key, value) -> bool
    // The mod ID is automatically extracted from the calling mod's environment.
    saveApi["set"] = [&saveSystem](const std::string& key, sol::object value,
                                    sol::this_environment te) -> bool {
        sol::environment& env = te;
        std::string modId = env["_MOD_ID"].get_or<std::string>("");
        if (modId.empty()) {
            MOD_LOG_WARN("save.set: could not determine mod ID");
            return false;
        }

        nlohmann::json jsonVal = solToJson(value);
        return saveSystem.set(modId, key, jsonVal);
    };

    // save.get(key, default) -> value
    saveApi["get"] = [&saveSystem, &lua](const std::string& key, sol::optional<sol::object> defaultVal,
                                         sol::this_environment te) -> sol::object {
        sol::environment& env = te;
        std::string modId = env["_MOD_ID"].get_or<std::string>("");
        if (modId.empty()) {
            MOD_LOG_WARN("save.get: could not determine mod ID");
            if (defaultVal) return *defaultVal;
            return sol::nil;
        }

        nlohmann::json defaultJson;
        if (defaultVal) {
            defaultJson = solToJson(*defaultVal);
        }

        nlohmann::json result = saveSystem.get(modId, key, defaultJson);

        // If result is same as default (key not found), return the original Lua default
        if (result == defaultJson && !saveSystem.has(modId, key)) {
            if (defaultVal) return *defaultVal;
            return sol::nil;
        }

        return jsonToSol(lua, result);
    };

    // save.delete(key) -> bool
    saveApi["delete"] = [&saveSystem](const std::string& key,
                                       sol::this_environment te) -> bool {
        sol::environment& env = te;
        std::string modId = env["_MOD_ID"].get_or<std::string>("");
        if (modId.empty()) return false;
        return saveSystem.remove(modId, key);
    };

    // save.has(key) -> bool
    saveApi["has"] = [&saveSystem](const std::string& key,
                                    sol::this_environment te) -> bool {
        sol::environment& env = te;
        std::string modId = env["_MOD_ID"].get_or<std::string>("");
        if (modId.empty()) return false;
        return saveSystem.has(modId, key);
    };

    // save.keys() -> table
    saveApi["keys"] = [&saveSystem, &lua](sol::this_environment te) -> sol::object {
        sol::environment& env = te;
        std::string modId = env["_MOD_ID"].get_or<std::string>("");
        if (modId.empty()) return sol::nil;

        auto keyList = saveSystem.keys(modId);
        sol::table result = lua.create_table();
        for (size_t i = 0; i < keyList.size(); ++i) {
            result[static_cast<int>(i + 1)] = keyList[i];
        }
        return result;
    };
}

} // namespace gloaming
