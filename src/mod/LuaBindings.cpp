#include "mod/LuaBindings.hpp"
#include "mod/ContentRegistry.hpp"
#include "mod/EventBus.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "audio/AudioSystem.hpp"
#include "ui/UISystem.hpp"
#include "ui/UIWidgets.hpp"

#include <algorithm>
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
    bindUIAPI();
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

    // events.on(eventName, handler) — subscribe to an event.
    // The "update" event is special: handlers are called directly each frame
    // with dt (float) as the argument instead of going through the EventBus.
    events["on"] = [this](const std::string& eventName, sol::function handler,
                          sol::optional<int> priority) -> uint64_t {
        if (eventName == "update") {
            uint64_t id = m_nextUpdateId++;
            m_updateCallbacks.push_back({id, sol::protected_function(handler)});
            return id;
        }

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
        // Check update callbacks first
        auto it = std::remove_if(m_updateCallbacks.begin(), m_updateCallbacks.end(),
            [id](const UpdateHandler& h) { return h.id == id; });
        if (it != m_updateCallbacks.end()) {
            m_updateCallbacks.erase(it, m_updateCallbacks.end());
            return;
        }
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

/// Helper: parse hex color string to Color, returns false on invalid input
static bool parseHexColor(const std::string& hex, Color& out) {
    if (hex.size() < 7 || hex[0] != '#') return false;
    try {
        unsigned int r = std::stoul(hex.substr(1, 2), nullptr, 16);
        unsigned int g = std::stoul(hex.substr(3, 2), nullptr, 16);
        unsigned int b = std::stoul(hex.substr(5, 2), nullptr, 16);
        unsigned int a = (hex.size() >= 9) ? std::stoul(hex.substr(7, 2), nullptr, 16) : 255;
        out = Color(r, g, b, a);
        return true;
    } catch (const std::exception&) {
        MOD_LOG_WARN("Invalid hex color: '{}'", hex);
        return false;
    }
}

/// Helper: parse a UIStyle from a Lua table
static UIStyle parseStyle(const sol::table& t) {
    UIStyle style;

    // Width/height
    sol::optional<sol::object> wObj = t["width"];
    if (wObj) {
        if (wObj->is<float>() || wObj->is<int>()) {
            style.width = UIDimension::Fixed(wObj->as<float>());
        } else if (wObj->is<std::string>()) {
            std::string ws = wObj->as<std::string>();
            if (ws == "auto") style.width = UIDimension::Auto();
            else if (ws == "grow") style.width = UIDimension::Grow();
            else if (!ws.empty() && ws.back() == '%') {
                try { style.width = UIDimension::Percent(std::stof(ws)); }
                catch (const std::exception&) { MOD_LOG_WARN("Invalid width percent: '{}'", ws); }
            }
        }
    }

    sol::optional<sol::object> hObj = t["height"];
    if (hObj) {
        if (hObj->is<float>() || hObj->is<int>()) {
            style.height = UIDimension::Fixed(hObj->as<float>());
        } else if (hObj->is<std::string>()) {
            std::string hs = hObj->as<std::string>();
            if (hs == "auto") style.height = UIDimension::Auto();
            else if (hs == "grow") style.height = UIDimension::Grow();
            else if (!hs.empty() && hs.back() == '%') {
                try { style.height = UIDimension::Percent(std::stof(hs)); }
                catch (const std::exception&) { MOD_LOG_WARN("Invalid height percent: '{}'", hs); }
            }
        }
    }

    style.minWidth = t.get_or("min_width", 0.0f);
    style.minHeight = t.get_or("min_height", 0.0f);
    style.maxWidth = t.get_or("max_width", 0.0f);
    style.maxHeight = t.get_or("max_height", 0.0f);

    // Flex layout
    sol::optional<std::string> dir = t.get<sol::optional<std::string>>("flex_direction");
    if (dir) {
        if (*dir == "row") style.flexDirection = FlexDirection::Row;
        else if (*dir == "column") style.flexDirection = FlexDirection::Column;
    }

    sol::optional<std::string> jc = t.get<sol::optional<std::string>>("justify_content");
    if (jc) {
        if (*jc == "start") style.justifyContent = JustifyContent::Start;
        else if (*jc == "center") style.justifyContent = JustifyContent::Center;
        else if (*jc == "end") style.justifyContent = JustifyContent::End;
        else if (*jc == "space_between") style.justifyContent = JustifyContent::SpaceBetween;
        else if (*jc == "space_around") style.justifyContent = JustifyContent::SpaceAround;
    }

    sol::optional<std::string> ai = t.get<sol::optional<std::string>>("align_items");
    if (ai) {
        if (*ai == "start") style.alignItems = AlignItems::Start;
        else if (*ai == "center") style.alignItems = AlignItems::Center;
        else if (*ai == "end") style.alignItems = AlignItems::End;
        else if (*ai == "stretch") style.alignItems = AlignItems::Stretch;
    }

    style.gap = t.get_or("gap", 0.0f);

    // Padding - can be a number (all sides) or a table
    sol::optional<sol::object> padObj = t["padding"];
    if (padObj) {
        if (padObj->is<float>() || padObj->is<int>()) {
            style.padding = UIEdges(padObj->as<float>());
        } else if (padObj->is<sol::table>()) {
            sol::table p = padObj->as<sol::table>();
            style.padding.top = p.get_or("top", 0.0f);
            style.padding.right = p.get_or("right", 0.0f);
            style.padding.bottom = p.get_or("bottom", 0.0f);
            style.padding.left = p.get_or("left", 0.0f);
        }
    }

    // Margin - same pattern
    sol::optional<sol::object> marObj = t["margin"];
    if (marObj) {
        if (marObj->is<float>() || marObj->is<int>()) {
            style.margin = UIEdges(marObj->as<float>());
        } else if (marObj->is<sol::table>()) {
            sol::table m = marObj->as<sol::table>();
            style.margin.top = m.get_or("top", 0.0f);
            style.margin.right = m.get_or("right", 0.0f);
            style.margin.bottom = m.get_or("bottom", 0.0f);
            style.margin.left = m.get_or("left", 0.0f);
        }
    }

    // Background color as hex string or table {r, g, b, a}
    sol::optional<sol::object> bgObj = t["background"];
    if (bgObj) {
        if (bgObj->is<std::string>()) {
            parseHexColor(bgObj->as<std::string>(), style.backgroundColor);
        } else if (bgObj->is<sol::table>()) {
            sol::table c = bgObj->as<sol::table>();
            style.backgroundColor = Color(
                c.get_or(1, 0),
                c.get_or(2, 0),
                c.get_or(3, 0),
                c.get_or(4, 255)
            );
        }
    }

    // Border
    sol::optional<sol::table> borderObj = t.get<sol::optional<sol::table>>("border");
    if (borderObj) {
        style.border.width = borderObj->get_or("width", 0.0f);
        sol::optional<std::string> bc = borderObj->get<sol::optional<std::string>>("color");
        if (bc) {
            parseHexColor(*bc, style.border.color);
        }
    }

    // Text
    sol::optional<int> fsObj = t.get<sol::optional<int>>("font_size");
    if (fsObj) style.fontSize = *fsObj;
    sol::optional<sol::object> tcObj = t["text_color"];
    if (tcObj) {
        if (tcObj->is<std::string>()) {
            parseHexColor(tcObj->as<std::string>(), style.textColor);
        } else if (tcObj->is<sol::table>()) {
            sol::table c = tcObj->as<sol::table>();
            style.textColor = Color(
                c.get_or(1, 255),
                c.get_or(2, 255),
                c.get_or(3, 255),
                c.get_or(4, 255)
            );
        }
    }

    sol::optional<std::string> ta = t.get<sol::optional<std::string>>("text_align");
    if (ta) {
        if (*ta == "left") style.textAlign = TextAlign::Left;
        else if (*ta == "center") style.textAlign = TextAlign::Center;
        else if (*ta == "right") style.textAlign = TextAlign::Right;
    }

    sol::optional<bool> visObj = t.get<sol::optional<bool>>("visible");
    if (visObj) style.visible = *visObj;
    sol::optional<bool> ohObj = t.get<sol::optional<bool>>("overflow_hidden");
    if (ohObj) style.overflowHidden = *ohObj;

    return style;
}

/// Helper: apply a Lua style table to an element by merging only fields present
/// in the table. This preserves constructor defaults (e.g. UIButton colors).
static void applyStyleTable(UIElement* element, const sol::optional<sol::table>& styleTable) {
    if (!styleTable) return;
    UIStyle parsed = parseStyle(*styleTable);
    UIStyle& existing = element->getStyle();
    // Only overwrite fields that were explicitly set in the table
    if ((*styleTable)["width"].valid()) existing.width = parsed.width;
    if ((*styleTable)["height"].valid()) existing.height = parsed.height;
    if ((*styleTable)["min_width"].valid()) existing.minWidth = parsed.minWidth;
    if ((*styleTable)["min_height"].valid()) existing.minHeight = parsed.minHeight;
    if ((*styleTable)["max_width"].valid()) existing.maxWidth = parsed.maxWidth;
    if ((*styleTable)["max_height"].valid()) existing.maxHeight = parsed.maxHeight;
    if ((*styleTable)["flex_direction"].valid()) existing.flexDirection = parsed.flexDirection;
    if ((*styleTable)["justify_content"].valid()) existing.justifyContent = parsed.justifyContent;
    if ((*styleTable)["align_items"].valid()) existing.alignItems = parsed.alignItems;
    if ((*styleTable)["gap"].valid()) existing.gap = parsed.gap;
    if ((*styleTable)["padding"].valid()) existing.padding = parsed.padding;
    if ((*styleTable)["margin"].valid()) existing.margin = parsed.margin;
    if ((*styleTable)["background"].valid()) existing.backgroundColor = parsed.backgroundColor;
    if ((*styleTable)["border"].valid()) existing.border = parsed.border;
    if ((*styleTable)["font_size"].valid()) existing.fontSize = parsed.fontSize;
    if ((*styleTable)["text_color"].valid()) existing.textColor = parsed.textColor;
    if ((*styleTable)["text_align"].valid()) existing.textAlign = parsed.textAlign;
    if ((*styleTable)["visible"].valid()) existing.visible = parsed.visible;
    if ((*styleTable)["overflow_hidden"].valid()) existing.overflowHidden = parsed.overflowHidden;
}

/// Helper: add children from a Lua table/array to a UIElement (indexed iteration)
static void addLuaChildren(std::shared_ptr<UIElement>& parent, const sol::optional<sol::table>& childrenTable) {
    if (!childrenTable) return;
    size_t len = childrenTable->size();
    for (size_t i = 1; i <= len; ++i) {
        sol::object value = (*childrenTable)[i];
        if (value.is<std::shared_ptr<UIElement>>()) {
            parent->addChild(value.as<std::shared_ptr<UIElement>>());
        }
    }
}

void LuaBindings::bindUIAPI() {
    auto ui = m_lua.create_named_table("ui");

    // --- Element creation functions ---

    // ui.Box({ id = "foo", style = {...} }, { children... })
    ui["Box"] = [this](sol::optional<sol::table> props,
                        sol::optional<sol::table> children) -> std::shared_ptr<UIElement> {
        std::string id;
        sol::optional<sol::table> styleTable;
        if (props) {
            id = props->get_or<std::string>("id", "");
            styleTable = props->get<sol::optional<sol::table>>("style");
        }
        auto box = std::make_shared<UIBox>(id);
        applyStyleTable(box.get(), styleTable);
        auto elem = std::static_pointer_cast<UIElement>(box);
        addLuaChildren(elem, children);
        return elem;
    };

    // ui.Text({ id = "label", text = "Hello", style = {...} })
    ui["Text"] = [this](sol::optional<sol::table> props) -> std::shared_ptr<UIElement> {
        std::string id, text;
        sol::optional<sol::table> styleTable;
        if (props) {
            id = props->get_or<std::string>("id", "");
            text = props->get_or<std::string>("text", "");
            styleTable = props->get<sol::optional<sol::table>>("style");
        }
        auto textElem = std::make_shared<UIText>(id, text);
        applyStyleTable(textElem.get(), styleTable);
        return std::static_pointer_cast<UIElement>(textElem);
    };

    // ui.Image({ id = "icon", style = {...} })
    ui["Image"] = [this](sol::optional<sol::table> props) -> std::shared_ptr<UIElement> {
        std::string id;
        sol::optional<sol::table> styleTable;
        if (props) {
            id = props->get_or<std::string>("id", "");
            styleTable = props->get<sol::optional<sol::table>>("style");
        }
        auto img = std::make_shared<UIImage>(id);
        applyStyleTable(img.get(), styleTable);
        return std::static_pointer_cast<UIElement>(img);
    };

    // ui.Button({ id = "btn", label = "Click Me", style = {...}, on_click = function() ... end })
    ui["Button"] = [this](sol::optional<sol::table> props) -> std::shared_ptr<UIElement> {
        std::string id, label;
        sol::optional<sol::table> styleTable;
        sol::optional<sol::function> onClick;
        if (props) {
            id = props->get_or<std::string>("id", "");
            label = props->get_or<std::string>("label", "");
            styleTable = props->get<sol::optional<sol::table>>("style");
            onClick = props->get<sol::optional<sol::function>>("on_click");

            // Hover/press colors
            sol::optional<sol::table> hcObj = props->get<sol::optional<sol::table>>("hover_color");
            sol::optional<sol::table> pcObj = props->get<sol::optional<sol::table>>("press_color");

            auto btn = std::make_shared<UIButton>(id, label);
            applyStyleTable(btn.get(), styleTable);
            if (onClick) {
                sol::function fn = *onClick;
                btn->setOnClick([fn]() {
                    try {
                        sol::protected_function_result result = fn();
                        if (!result.valid()) {
                            sol::error err = result;
                            MOD_LOG_ERROR("Button on_click error: {}", err.what());
                        }
                    } catch (const std::exception& e) {
                        MOD_LOG_ERROR("Button on_click exception: {}", e.what());
                    }
                });
            }
            if (hcObj) {
                btn->setHoverColor(Color(
                    hcObj->get_or(1, 80), hcObj->get_or(2, 80),
                    hcObj->get_or(3, 110), hcObj->get_or(4, 255)));
            }
            if (pcObj) {
                btn->setPressColor(Color(
                    pcObj->get_or(1, 40), pcObj->get_or(2, 40),
                    pcObj->get_or(3, 60), pcObj->get_or(4, 255)));
            }
            return std::static_pointer_cast<UIElement>(btn);
        }
        return std::static_pointer_cast<UIElement>(std::make_shared<UIButton>(id, label));
    };

    // ui.Slider({ id = "vol", min = 0, max = 100, value = 50, on_change = function(v) end, style = {...} })
    ui["Slider"] = [this](sol::optional<sol::table> props) -> std::shared_ptr<UIElement> {
        std::string id;
        float minVal = 0.0f, maxVal = 1.0f, value = 0.0f;
        sol::optional<sol::table> styleTable;
        sol::optional<sol::function> onChange;

        if (props) {
            id = props->get_or<std::string>("id", "");
            minVal = props->get_or("min", 0.0f);
            maxVal = props->get_or("max", 1.0f);
            value = props->get_or("value", 0.0f);
            styleTable = props->get<sol::optional<sol::table>>("style");
            onChange = props->get<sol::optional<sol::function>>("on_change");
        }

        auto slider = std::make_shared<UISlider>(id);
        slider->setRange(minVal, maxVal);
        slider->setValue(value);
        applyStyleTable(slider.get(), styleTable);

        if (onChange) {
            sol::function fn = *onChange;
            slider->setOnChange([fn](float v) {
                try {
                    sol::protected_function_result result = fn(v);
                    if (!result.valid()) {
                        sol::error err = result;
                        MOD_LOG_ERROR("Slider on_change error: {}", err.what());
                    }
                } catch (const std::exception& e) {
                    MOD_LOG_ERROR("Slider on_change exception: {}", e.what());
                }
            });
        }

        return std::static_pointer_cast<UIElement>(slider);
    };

    // ui.Grid({ id = "grid", columns = 10, cell_width = 48, cell_height = 48, style = {...} }, { children })
    ui["Grid"] = [this](sol::optional<sol::table> props,
                         sol::optional<sol::table> children) -> std::shared_ptr<UIElement> {
        std::string id;
        int columns = 1;
        float cellW = 0.0f, cellH = 0.0f;
        sol::optional<sol::table> styleTable;

        if (props) {
            id = props->get_or<std::string>("id", "");
            columns = props->get_or("columns", 1);
            cellW = props->get_or("cell_width", 0.0f);
            cellH = props->get_or("cell_height", 0.0f);
            styleTable = props->get<sol::optional<sol::table>>("style");
        }

        auto grid = std::make_shared<UIGrid>(id, columns);
        grid->setCellSize(cellW, cellH);
        applyStyleTable(grid.get(), styleTable);
        auto elem = std::static_pointer_cast<UIElement>(grid);
        addLuaChildren(elem, children);
        return elem;
    };

    // ui.ScrollPanel({ id = "scroll", style = {...} }, { children })
    ui["ScrollPanel"] = [this](sol::optional<sol::table> props,
                                sol::optional<sol::table> children) -> std::shared_ptr<UIElement> {
        std::string id;
        sol::optional<sol::table> styleTable;
        if (props) {
            id = props->get_or<std::string>("id", "");
            styleTable = props->get<sol::optional<sol::table>>("style");
        }
        auto scroll = std::make_shared<UIScrollPanel>(id);
        applyStyleTable(scroll.get(), styleTable);
        auto elem = std::static_pointer_cast<UIElement>(scroll);
        addLuaChildren(elem, children);
        return elem;
    };

    // --- Screen management ---

    // ui.register(name, builderFunction)
    // The builder function is called to create the UI tree.
    // For static UIs, it's called once. For dynamic UIs, it's called every frame.
    ui["register"] = [this](const std::string& name, sol::function builder) {
        UISystem* uiSys = m_engine->getUISystem();
        if (!uiSys) {
            MOD_LOG_ERROR("ui.register: UI system not available");
            return;
        }

        // Call the builder once to create the initial tree
        try {
            sol::protected_function_result result = builder();
            if (result.valid() && result.get_type() != sol::type::nil) {
                auto root = result.get<std::shared_ptr<UIElement>>();
                uiSys->registerScreen(name, root);
                MOD_LOG_INFO("UI screen '{}' registered", name);
            } else if (!result.valid()) {
                sol::error err = result;
                MOD_LOG_ERROR("ui.register '{}': builder error: {}", name, err.what());
            }
        } catch (const std::exception& e) {
            MOD_LOG_ERROR("ui.register '{}': exception: {}", name, e.what());
        }
    };

    // ui.registerDynamic(name, builderFunction)
    // Builder is called every frame to rebuild the UI tree.
    ui["registerDynamic"] = [this](const std::string& name, sol::function builder) {
        UISystem* uiSys = m_engine->getUISystem();
        if (!uiSys) {
            MOD_LOG_ERROR("ui.registerDynamic: UI system not available");
            return;
        }

        uiSys->registerDynamicScreen(name, [builder]() -> std::shared_ptr<UIElement> {
            try {
                sol::protected_function_result result = builder();
                if (result.valid() && result.get_type() != sol::type::nil) {
                    return result.get<std::shared_ptr<UIElement>>();
                }
                if (!result.valid()) {
                    sol::error err = result;
                    MOD_LOG_ERROR("UI dynamic builder error: {}", err.what());
                }
            } catch (const std::exception& e) {
                MOD_LOG_ERROR("UI dynamic builder exception: {}", e.what());
            }
            return nullptr;
        });
        MOD_LOG_INFO("UI dynamic screen '{}' registered", name);
    };

    // ui.show(name) / ui.hide(name)
    ui["show"] = [this](const std::string& name) {
        UISystem* uiSys = m_engine->getUISystem();
        if (uiSys) uiSys->showScreen(name);
    };

    ui["hide"] = [this](const std::string& name) {
        UISystem* uiSys = m_engine->getUISystem();
        if (uiSys) uiSys->hideScreen(name);
    };

    // ui.isVisible(name)
    ui["isVisible"] = [this](const std::string& name) -> bool {
        UISystem* uiSys = m_engine->getUISystem();
        return uiSys && uiSys->isScreenVisible(name);
    };

    // ui.remove(name)
    ui["remove"] = [this](const std::string& name) {
        UISystem* uiSys = m_engine->getUISystem();
        if (uiSys) uiSys->removeScreen(name);
    };

    // ui.setBlocking(name, blocking)
    ui["setBlocking"] = [this](const std::string& name, bool blocking) {
        UISystem* uiSys = m_engine->getUISystem();
        if (uiSys) uiSys->setScreenBlocking(name, blocking);
    };

    // ui.setZOrder(name, zOrder)
    ui["setZOrder"] = [this](const std::string& name, int zOrder) {
        UISystem* uiSys = m_engine->getUISystem();
        if (uiSys) uiSys->setScreenZOrder(name, zOrder);
    };

    // ui.markDirty(name) - mark a dynamic screen for rebuild next frame
    ui["markDirty"] = [this](const std::string& name) {
        UISystem* uiSys = m_engine->getUISystem();
        if (uiSys) uiSys->markScreenDirty(name);
    };

    // ui.findById(id) — find an element across all visible screens
    // Returns nil if not found (we can't return a raw pointer to Lua safely,
    // but we can modify properties via the ID)
    ui["setVisible"] = [this](const std::string& elementId, bool visible) {
        UISystem* uiSys = m_engine->getUISystem();
        if (!uiSys) return;
        UIElement* elem = uiSys->findById(elementId);
        if (elem) elem->getStyle().visible = visible;
    };

    // ui.setText(elementId, text)
    ui["setText"] = [this](const std::string& elementId, const std::string& text) {
        UISystem* uiSys = m_engine->getUISystem();
        if (!uiSys) return;
        UIElement* elem = uiSys->findById(elementId);
        if (elem && elem->getType() == UIElementType::Text) {
            static_cast<UIText*>(elem)->setText(text);
        } else if (elem && elem->getType() == UIElementType::Button) {
            static_cast<UIButton*>(elem)->setLabel(text);
        }
    };

    // ui.setSliderValue(elementId, value)
    ui["setSliderValue"] = [this](const std::string& elementId, float value) {
        UISystem* uiSys = m_engine->getUISystem();
        if (!uiSys) return;
        UIElement* elem = uiSys->findById(elementId);
        if (elem && elem->getType() == UIElementType::Slider) {
            static_cast<UISlider*>(elem)->setValue(value);
        }
    };

    // ui.getSliderValue(elementId) -> float
    ui["getSliderValue"] = [this](const std::string& elementId) -> float {
        UISystem* uiSys = m_engine->getUISystem();
        if (!uiSys) return 0.0f;
        UIElement* elem = uiSys->findById(elementId);
        if (elem && elem->getType() == UIElementType::Slider) {
            return static_cast<UISlider*>(elem)->getValue();
        }
        return 0.0f;
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

void LuaBindings::tickUpdate(float dt) {
    for (auto& handler : m_updateCallbacks) {
        try {
            sol::protected_function_result result = handler.callback(dt);
            if (!result.valid()) {
                sol::error err = result;
                MOD_LOG_ERROR("Update handler error: {}", err.what());
            }
        } catch (const std::exception& e) {
            MOD_LOG_ERROR("Update handler exception: {}", e.what());
        }
    }
}

} // namespace gloaming
