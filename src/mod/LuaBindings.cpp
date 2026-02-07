#include "mod/LuaBindings.hpp"
#include "mod/ContentRegistry.hpp"
#include "mod/EventBus.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "audio/AudioSystem.hpp"

#include <fstream>
#include <sstream>
#include <cmath>
#include <filesystem>

namespace gloaming {

namespace fs = std::filesystem;

/// Validate that a relative path doesn't escape the base directory.
/// Uses canonical path resolution when possible to handle symlinks.
static bool isPathSafe(const std::string& baseDir, const std::string& relPath) {
    // Quick reject: absolute paths and obvious traversal
    if (relPath.empty()) return false;
    if (relPath[0] == '/' || relPath[0] == '\\') return false;
    if (relPath.find("..") != std::string::npos) return false;

    // Canonical path check: verify resolved path stays within base dir
    try {
        auto resolvedBase = fs::weakly_canonical(baseDir);
        auto resolvedFull = fs::weakly_canonical(baseDir + "/" + relPath);
        auto rel = fs::relative(resolvedFull, resolvedBase);
        auto relStr = rel.string();
        return !relStr.empty() && !relStr.starts_with("..");
    } catch (const fs::filesystem_error&) {
        return false;
    }
}

bool LuaBindings::init(Engine& engine, ContentRegistry& registry, EventBus& eventBus) {
    m_engine = &engine;
    m_registry = &registry;
    m_eventBus = &eventBus;

    // Open standard Lua libraries
    m_lua.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::table,
        sol::lib::coroutine,
        sol::lib::utf8
    );

    // Apply security sandbox
    applySandbox();

    // Bind engine APIs
    bindLogAPI();
    bindContentAPI();
    bindEventsAPI();
    bindModsAPI();
    bindAudioAPI();
    bindUtilAPI();

    m_initialized = true;
    LOG_INFO("LuaBindings: initialized successfully");
    return true;
}

void LuaBindings::shutdown() {
    // The sol::state destructor handles cleanup
    m_initialized = false;
    m_engine = nullptr;
    m_registry = nullptr;
    m_eventBus = nullptr;
    LOG_INFO("LuaBindings: shut down");
}

void LuaBindings::applySandbox() {
    // Remove dangerous global functions
    m_lua["os"] = sol::nil;
    m_lua["io"] = sol::nil;
    m_lua["debug"] = sol::nil;
    m_lua["package"] = sol::nil;
    m_lua["loadfile"] = sol::nil;
    m_lua["dofile"] = sol::nil;
    m_lua["load"] = sol::nil;
    m_lua["rawget"] = sol::nil;
    m_lua["rawset"] = sol::nil;
    m_lua["rawequal"] = sol::nil;
    m_lua["rawlen"] = sol::nil;
    m_lua["collectgarbage"] = sol::nil;

    // Remove string.dump (can be used to produce bytecode for sandbox escape)
    sol::table stringLib = m_lua["string"];
    if (stringLib.valid()) {
        stringLib["dump"] = sol::nil;
    }

    // We keep 'require' but it will be overridden per-mod to scope to the mod's directory
    m_lua["require"] = sol::nil;

    // Set instruction count limit to prevent infinite loops (10 million instructions)
    lua_sethook(m_lua.lua_state(), [](lua_State* L, lua_Debug*) {
        luaL_error(L, "instruction limit exceeded (possible infinite loop)");
    }, LUA_MASKCOUNT, 10000000);

    LOG_DEBUG("LuaBindings: sandbox applied");
}

void LuaBindings::bindLogAPI() {
    auto log = m_lua.create_named_table("log");
    log["info"] = [](const std::string& msg) {
        MOD_LOG_INFO("{}", msg);
    };
    log["warn"] = [](const std::string& msg) {
        MOD_LOG_WARN("{}", msg);
    };
    log["error"] = [](const std::string& msg) {
        MOD_LOG_ERROR("{}", msg);
    };
    log["debug"] = [](const std::string& msg) {
        MOD_LOG_DEBUG("{}", msg);
    };
    log["trace"] = [](const std::string& msg) {
        MOD_LOG_TRACE("{}", msg);
    };
}

void LuaBindings::bindContentAPI() {
    auto content = m_lua.create_named_table("content");

    // content.loadTiles(path) — loads tile definitions from JSON file
    content["loadTiles"] = [this](const std::string& path, sol::this_environment te) {
        sol::environment& env = te;
        std::string modId = env["_MOD_ID"].get_or<std::string>("");
        std::string modDir = env["_MOD_DIR"].get_or<std::string>("");
        if (!isPathSafe(modDir, path)) {
            MOD_LOG_ERROR("content.loadTiles: path traversal rejected '{}'", path);
            return false;
        }
        std::string fullPath = modDir + "/" + path;

        std::ifstream file(fullPath);
        if (!file.is_open()) {
            MOD_LOG_ERROR("content.loadTiles: cannot open '{}'", fullPath);
            return false;
        }
        try {
            nlohmann::json json;
            file >> json;
            return m_registry->loadTilesFromJson(json, modId, modDir);
        } catch (const nlohmann::json::parse_error& e) {
            MOD_LOG_ERROR("content.loadTiles: JSON error in '{}': {}", fullPath, e.what());
            return false;
        }
    };

    // content.loadItems(path)
    content["loadItems"] = [this](const std::string& path, sol::this_environment te) {
        sol::environment& env = te;
        std::string modId = env["_MOD_ID"].get_or<std::string>("");
        std::string modDir = env["_MOD_DIR"].get_or<std::string>("");
        if (!isPathSafe(modDir, path)) {
            MOD_LOG_ERROR("content.loadItems: path traversal rejected '{}'", path);
            return false;
        }
        std::string fullPath = modDir + "/" + path;

        std::ifstream file(fullPath);
        if (!file.is_open()) {
            MOD_LOG_ERROR("content.loadItems: cannot open '{}'", fullPath);
            return false;
        }
        try {
            nlohmann::json json;
            file >> json;
            return m_registry->loadItemsFromJson(json, modId, modDir);
        } catch (const nlohmann::json::parse_error& e) {
            MOD_LOG_ERROR("content.loadItems: JSON error in '{}': {}", fullPath, e.what());
            return false;
        }
    };

    // content.loadEnemies(path)
    content["loadEnemies"] = [this](const std::string& path, sol::this_environment te) {
        sol::environment& env = te;
        std::string modId = env["_MOD_ID"].get_or<std::string>("");
        std::string modDir = env["_MOD_DIR"].get_or<std::string>("");
        if (!isPathSafe(modDir, path)) {
            MOD_LOG_ERROR("content.loadEnemies: path traversal rejected '{}'", path);
            return false;
        }
        std::string fullPath = modDir + "/" + path;

        std::ifstream file(fullPath);
        if (!file.is_open()) {
            MOD_LOG_ERROR("content.loadEnemies: cannot open '{}'", fullPath);
            return false;
        }
        try {
            nlohmann::json json;
            file >> json;
            return m_registry->loadEnemiesFromJson(json, modId, modDir);
        } catch (const nlohmann::json::parse_error& e) {
            MOD_LOG_ERROR("content.loadEnemies: JSON error in '{}': {}", fullPath, e.what());
            return false;
        }
    };

    // content.loadRecipes(path)
    content["loadRecipes"] = [this](const std::string& path, sol::this_environment te) {
        sol::environment& env = te;
        std::string modId = env["_MOD_ID"].get_or<std::string>("");
        std::string modDir = env["_MOD_DIR"].get_or<std::string>("");
        if (!isPathSafe(modDir, path)) {
            MOD_LOG_ERROR("content.loadRecipes: path traversal rejected '{}'", path);
            return false;
        }
        std::string fullPath = modDir + "/" + path;

        std::ifstream file(fullPath);
        if (!file.is_open()) {
            MOD_LOG_ERROR("content.loadRecipes: cannot open '{}'", fullPath);
            return false;
        }
        try {
            nlohmann::json json;
            file >> json;
            return m_registry->loadRecipesFromJson(json, modId);
        } catch (const nlohmann::json::parse_error& e) {
            MOD_LOG_ERROR("content.loadRecipes: JSON error in '{}': {}", fullPath, e.what());
            return false;
        }
    };

    // content.getTile(id)
    content["getTile"] = [this](const std::string& id) -> sol::object {
        const auto* tile = m_registry->getTile(id);
        if (!tile) return sol::nil;
        // Return a table with tile properties
        sol::table t = m_lua.create_table();
        t["id"] = tile->qualifiedId;
        t["name"] = tile->name;
        t["solid"] = tile->solid;
        t["transparent"] = tile->transparent;
        t["hardness"] = tile->hardness;
        t["runtime_id"] = tile->runtimeId;
        return t;
    };

    // content.getItem(id)
    content["getItem"] = [this](const std::string& id) -> sol::object {
        const auto* item = m_registry->getItem(id);
        if (!item) return sol::nil;
        sol::table t = m_lua.create_table();
        t["id"] = item->qualifiedId;
        t["name"] = item->name;
        t["type"] = item->type;
        t["damage"] = item->damage;
        t["rarity"] = item->rarity;
        t["max_stack"] = item->maxStack;
        return t;
    };
}

void LuaBindings::bindEventsAPI() {
    auto events = m_lua.create_named_table("events");

    // events.on(eventName, handler) — subscribe to an event
    events["on"] = [this](const std::string& eventName, sol::function handler,
                          sol::optional<int> priority) -> uint64_t {
        int prio = priority.value_or(0);
        return m_eventBus->on(eventName, [handler](const EventData& data) -> bool {
            // Convert EventData to a Lua table for the handler
            try {
                sol::protected_function_result result = handler(std::cref(data));
                if (!result.valid()) {
                    sol::error err = result;
                    MOD_LOG_ERROR("Event handler error: {}", err.what());
                    return false;
                }
                // If handler returns true, cancel the event
                if (result.get_type() == sol::type::boolean) {
                    return result.get<bool>();
                }
            } catch (const std::exception& e) {
                MOD_LOG_ERROR("Event handler exception: {}", e.what());
            }
            return false;
        }, prio);
    };

    // events.off(handlerId) — unsubscribe
    events["off"] = [this](uint64_t id) {
        m_eventBus->off(id);
    };

    // events.emit(eventName, data_table) — emit an event
    events["emit"] = [this](const std::string& eventName, sol::optional<sol::table> dataTable) {
        EventData data;
        if (dataTable) {
            // Convert Lua table to EventData
            dataTable->for_each([&data](const sol::object& key, const sol::object& value) {
                if (!key.is<std::string>()) return;
                std::string k = key.as<std::string>();
                if (value.is<std::string>()) {
                    data.setString(k, value.as<std::string>());
                } else if (value.is<bool>()) {
                    data.setBool(k, value.as<bool>());
                } else if (value.is<int>()) {
                    data.setInt(k, value.as<int>());
                } else if (value.is<float>() || value.is<double>()) {
                    data.setFloat(k, value.as<float>());
                }
            });
        }
        return m_eventBus->emit(eventName, data);
    };
}

void LuaBindings::bindAudioAPI() {
    auto audio = m_lua.create_named_table("audio");

    // audio.registerSound(id, path, options)
    // options = { volume = 0.8, pitch_variance = 0.1, cooldown = 0.1 }
    audio["registerSound"] = [this](const std::string& id, const std::string& path,
                                     sol::optional<sol::table> options,
                                     sol::this_environment te) {
        sol::environment& env = te;
        std::string modDir = env["_MOD_DIR"].get_or<std::string>("");

        // Validate and resolve path relative to mod directory
        if (!modDir.empty() && !isPathSafe(modDir, path)) {
            MOD_LOG_WARN("audio.registerSound: path '{}' escapes mod directory", path);
            return;
        }
        std::string fullPath = modDir.empty() ? path : (modDir + "/" + path);

        float volume = 1.0f;
        float pitchVariance = 0.0f;
        float cooldown = 0.0f;

        if (options) {
            volume = options->get_or("volume", 1.0f);
            pitchVariance = options->get_or("pitch_variance", 0.0f);
            cooldown = options->get_or("cooldown", 0.0f);
        }

        AudioSystem* audioSys = m_engine->getAudioSystem();
        if (audioSys) {
            audioSys->registerSound(id, fullPath, volume, pitchVariance, cooldown);
        }
    };

    // audio.playSound(id [, position])
    // position = { x = ..., y = ... } (optional, for positional audio)
    audio["playSound"] = [this](const std::string& id,
                                 sol::optional<sol::table> position) -> uint32_t {
        AudioSystem* audioSys = m_engine->getAudioSystem();
        if (!audioSys) return 0;

        if (position) {
            float x = position->get_or("x", 0.0f);
            float y = position->get_or("y", 0.0f);
            return audioSys->playSoundAt(id, x, y);
        }
        return audioSys->playSound(id);
    };

    // audio.stopSound(handle)
    audio["stopSound"] = [this](uint32_t handle) {
        AudioSystem* audioSys = m_engine->getAudioSystem();
        if (audioSys) audioSys->stopSound(handle);
    };

    // audio.stopAllSounds()
    audio["stopAllSounds"] = [this]() {
        AudioSystem* audioSys = m_engine->getAudioSystem();
        if (audioSys) audioSys->stopAllSounds();
    };

    // audio.playMusic(path [, options])
    // options = { fade_in = 2.0, loop = true }
    audio["playMusic"] = [this](const std::string& path, sol::optional<sol::table> options,
                                 sol::this_environment te) {
        sol::environment& env = te;
        std::string modDir = env["_MOD_DIR"].get_or<std::string>("");

        // Validate path stays within mod directory
        if (!modDir.empty() && !isPathSafe(modDir, path)) {
            MOD_LOG_WARN("audio.playMusic: path '{}' escapes mod directory", path);
            return;
        }
        std::string fullPath = modDir.empty() ? path : (modDir + "/" + path);

        float fadeIn = 0.0f;
        bool loop = true;
        if (options) {
            fadeIn = options->get_or("fade_in", 0.0f);
            loop = options->get_or("loop", true);
        }

        AudioSystem* audioSys = m_engine->getAudioSystem();
        if (audioSys) audioSys->playMusic(fullPath, fadeIn, loop);
    };

    // audio.stopMusic([options])
    // options = { fade_out = 2.0 }
    audio["stopMusic"] = [this](sol::optional<sol::table> options) {
        float fadeOut = 0.0f;
        if (options) {
            fadeOut = options->get_or("fade_out", 0.0f);
        }
        AudioSystem* audioSys = m_engine->getAudioSystem();
        if (audioSys) audioSys->stopMusic(fadeOut);
    };

    // audio.setVolume(channel, volume)
    // channel = "master", "sfx", "music", "ambient"
    audio["setVolume"] = [this](const std::string& channel, float volume) {
        AudioSystem* audioSys = m_engine->getAudioSystem();
        if (!audioSys) return;
        if (channel == "master") audioSys->setMasterVolume(volume);
        else if (channel == "sfx") audioSys->setSfxVolume(volume);
        else if (channel == "music") audioSys->setMusicVolume(volume);
        else if (channel == "ambient") audioSys->setAmbientVolume(volume);
        else MOD_LOG_WARN("audio.setVolume: unknown channel '{}'", channel);
    };

    // audio.getVolume(channel)
    audio["getVolume"] = [this](const std::string& channel) -> float {
        AudioSystem* audioSys = m_engine->getAudioSystem();
        if (!audioSys) return 0.0f;
        if (channel == "master") return audioSys->getMasterVolume();
        if (channel == "sfx") return audioSys->getSfxVolume();
        if (channel == "music") return audioSys->getMusicVolume();
        if (channel == "ambient") return audioSys->getAmbientVolume();
        MOD_LOG_WARN("audio.getVolume: unknown channel '{}'", channel);
        return 0.0f;
    };

    // audio.bindEvent(eventName, soundId)
    // Simple binding: when the event fires, play the sound
    audio["bindEvent"] = [this](const std::string& eventName, const std::string& soundId) {
        AudioSystem* audioSys = m_engine->getAudioSystem();
        if (audioSys) audioSys->bindSoundToEvent(eventName, soundId);
    };

    // audio.unbindEvent(eventName)
    audio["unbindEvent"] = [this](const std::string& eventName) {
        AudioSystem* audioSys = m_engine->getAudioSystem();
        if (audioSys) audioSys->unbindEvent(eventName);
    };

    // audio.isMusicPlaying()
    audio["isMusicPlaying"] = [this]() -> bool {
        AudioSystem* audioSys = m_engine->getAudioSystem();
        return audioSys && audioSys->isMusicPlaying();
    };

    // audio.getCurrentMusic()
    audio["getCurrentMusic"] = [this]() -> std::string {
        AudioSystem* audioSys = m_engine->getAudioSystem();
        return audioSys ? audioSys->getCurrentMusic() : "";
    };
}

void LuaBindings::bindModsAPI() {
    auto mods = m_lua.create_named_table("mods");

    // mods.isLoaded(modId) — stub returning false until ModLoader replaces it
    // after all mods finish loading (via updateModsAPI). During init(), this will
    // always return false; use postInit() for cross-mod availability checks.
    mods["isLoaded"] = [](const std::string&) -> bool {
        return false;
    };
}

void LuaBindings::bindUtilAPI() {
    // Noise functions for world generation
    auto noise = m_lua.create_named_table("noise");

    // Simple Perlin-like noise using a hash function
    noise["perlin"] = [](float x, float seed) -> float {
        // Simple value noise
        auto hash = [](int n) -> float {
            n = (n << 13) ^ n;
            return 1.0f - static_cast<float>((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
        };
        float intSeed = seed;
        int ix = static_cast<int>(std::floor(x + intSeed));
        float fx = (x + intSeed) - std::floor(x + intSeed);
        float t = fx * fx * (3.0f - 2.0f * fx);  // Smoothstep
        return hash(ix) * (1.0f - t) + hash(ix + 1) * t;
    };

    noise["perlin2d"] = [](float x, float y, float seed) -> float {
        auto hash2d = [](int x, int y) -> float {
            int n = x + y * 57;
            n = (n << 13) ^ n;
            return 1.0f - static_cast<float>((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
        };
        float sx = x + seed;
        float sy = y + seed * 1.7f;
        int ix = static_cast<int>(std::floor(sx));
        int iy = static_cast<int>(std::floor(sy));
        float fx = sx - std::floor(sx);
        float fy = sy - std::floor(sy);
        float tx = fx * fx * (3.0f - 2.0f * fx);
        float ty = fy * fy * (3.0f - 2.0f * fy);

        float v00 = hash2d(ix, iy);
        float v10 = hash2d(ix + 1, iy);
        float v01 = hash2d(ix, iy + 1);
        float v11 = hash2d(ix + 1, iy + 1);

        float i0 = v00 * (1.0f - tx) + v10 * tx;
        float i1 = v01 * (1.0f - tx) + v11 * tx;
        return i0 * (1.0f - ty) + i1 * ty;
    };

    // Random utilities
    auto random = m_lua.create_named_table("random");
    random["create"] = [this](float seed) -> sol::table {
        sol::table rng = m_lua.create_table();
        // Store seed in the table
        rng["_seed"] = static_cast<int>(seed);
        rng["int"] = [](sol::table self, int minVal, int maxVal) -> int {
            int s = self["_seed"].get<int>();
            s = (s * 1103515245 + 12345) & 0x7fffffff;
            self["_seed"] = s;
            if (minVal >= maxVal) return minVal;
            return minVal + (s % (maxVal - minVal + 1));
        };
        rng["float"] = [](sol::table self, sol::optional<float> minVal, sol::optional<float> maxVal) -> float {
            int s = self["_seed"].get<int>();
            s = (s * 1103515245 + 12345) & 0x7fffffff;
            self["_seed"] = s;
            float normalized = static_cast<float>(s) / 2147483647.0f;
            float lo = minVal.value_or(0.0f);
            float hi = maxVal.value_or(1.0f);
            return lo + normalized * (hi - lo);
        };
        return rng;
    };

    // Vector utility
    auto vector = m_lua.create_named_table("vector");
    vector["normalize"] = [this](sol::table v) -> sol::table {
        float x = v.get_or("x", 0.0f);
        float y = v.get_or("y", 0.0f);
        float len = std::sqrt(x * x + y * y);
        sol::table result = m_lua.create_table();
        if (len > 0.0f) {
            result["x"] = x / len;
            result["y"] = y / len;
        } else {
            result["x"] = 0.0f;
            result["y"] = 0.0f;
        }
        return result;
    };
    vector["distance"] = [](sol::table a, sol::table b) -> float {
        float dx = b.get_or("x", 0.0f) - a.get_or("x", 0.0f);
        float dy = b.get_or("y", 0.0f) - a.get_or("y", 0.0f);
        return std::sqrt(dx * dx + dy * dy);
    };
    vector["length"] = [](sol::table v) -> float {
        float x = v.get_or("x", 0.0f);
        float y = v.get_or("y", 0.0f);
        return std::sqrt(x * x + y * y);
    };
}

sol::environment LuaBindings::createModEnvironment(const std::string& modId) {
    // Create an environment that inherits from the global table (read-only)
    sol::environment env(m_lua, sol::create, m_lua.globals());

    // Set mod-specific metadata
    env["_MOD_ID"] = modId;
    env["_MOD_DIR"] = "";  // Will be set by ModLoader before execution

    // Override 'require' to scope to the mod's directory
    env["require"] = [this, modId](const std::string& moduleName, sol::this_environment te) -> sol::object {
        sol::environment& callerEnv = te;
        std::string modDir = callerEnv["_MOD_DIR"].get_or<std::string>("");

        // Convert module name to path (dots to slashes)
        std::string relPath = moduleName;
        for (char& c : relPath) {
            if (c == '.') c = '/';
        }
        if (!isPathSafe(modDir, relPath)) {
            MOD_LOG_ERROR("[{}] require '{}': path traversal rejected", modId, moduleName);
            return sol::nil;
        }
        std::string path = modDir + "/" + relPath + ".lua";

        // Check if already loaded in this mod's environment
        std::string cacheKey = "_loaded_" + moduleName;
        sol::object cached = callerEnv[cacheKey];
        if (cached.valid() && cached != sol::nil) {
            return cached;
        }

        // Load and execute the file in the mod's environment
        auto result = m_lua.load_file(path);
        if (!result.valid()) {
            sol::error err = result;
            MOD_LOG_ERROR("[{}] require '{}': {}", modId, moduleName, err.what());
            return sol::nil;
        }

        sol::protected_function chunk = result;
        sol::set_environment(callerEnv, chunk);
        auto execResult = chunk();
        if (!execResult.valid()) {
            sol::error err = execResult;
            MOD_LOG_ERROR("[{}] require '{}' execution error: {}", modId, moduleName, err.what());
            return sol::nil;
        }

        // Cache the result
        sol::object returnVal = execResult;
        callerEnv[cacheKey] = returnVal;
        return returnVal;
    };

    return env;
}

bool LuaBindings::executeFile(const std::string& path, sol::environment& env) {
    auto result = m_lua.load_file(path);
    if (!result.valid()) {
        sol::error err = result;
        MOD_LOG_ERROR("LuaBindings: failed to load '{}': {}", path, err.what());
        return false;
    }

    sol::protected_function chunk = result;
    sol::set_environment(env, chunk);

    auto execResult = chunk();
    if (!execResult.valid()) {
        sol::error err = execResult;
        MOD_LOG_ERROR("LuaBindings: error executing '{}': {}", path, err.what());
        return false;
    }

    return true;
}

bool LuaBindings::executeString(const std::string& code, sol::environment& env,
                                 const std::string& chunkName) {
    auto result = m_lua.load(code, chunkName);
    if (!result.valid()) {
        sol::error err = result;
        MOD_LOG_ERROR("LuaBindings: failed to load string '{}': {}", chunkName, err.what());
        return false;
    }

    sol::protected_function chunk = result;
    sol::set_environment(env, chunk);

    auto execResult = chunk();
    if (!execResult.valid()) {
        sol::error err = execResult;
        MOD_LOG_ERROR("LuaBindings: error executing string '{}': {}", chunkName, err.what());
        return false;
    }

    return true;
}

} // namespace gloaming
