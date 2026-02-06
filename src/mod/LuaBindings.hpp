#pragma once

#include <sol/sol.hpp>

#include <string>
#include <memory>

namespace gloaming {

// Forward declarations
class Engine;
class ContentRegistry;
class EventBus;

/// Manages the Lua scripting environment for mods.
/// Each mod gets its own sandboxed Lua environment built from a shared state.
class LuaBindings {
public:
    LuaBindings() = default;
    ~LuaBindings() = default;

    // Non-copyable
    LuaBindings(const LuaBindings&) = delete;
    LuaBindings& operator=(const LuaBindings&) = delete;

    /// Initialize the Lua state with engine bindings.
    /// Must be called before any mod scripts are loaded.
    bool init(Engine& engine, ContentRegistry& registry, EventBus& eventBus);

    /// Shutdown and clean up the Lua state
    void shutdown();

    /// Create a sandboxed environment for a mod.
    /// The environment inherits read-only access to engine APIs but has its own globals.
    sol::environment createModEnvironment(const std::string& modId);

    /// Execute a Lua script file within a mod's environment.
    /// Returns true on success.
    bool executeFile(const std::string& path, sol::environment& env);

    /// Execute a Lua string within a mod's environment.
    bool executeString(const std::string& code, sol::environment& env,
                       const std::string& chunkName = "=string");

    /// Get the raw sol2 state (for advanced usage)
    sol::state& getState() { return m_lua; }

    /// Check if initialized
    bool isInitialized() const { return m_initialized; }

private:
    /// Set up the sandbox by removing dangerous functions
    void applySandbox();

    /// Bind the logging API (log.info, log.warn, etc.)
    void bindLogAPI();

    /// Bind the content loading API (content.loadTiles, etc.)
    void bindContentAPI();

    /// Bind the events API (events.on, events.emit)
    void bindEventsAPI();

    /// Bind the mods utility API (mods.isLoaded, etc.)
    void bindModsAPI();

    /// Bind the audio API (audio.registerSound, audio.playSound, etc.)
    void bindAudioAPI();

    /// Bind math/noise utilities
    void bindUtilAPI();

    sol::state m_lua;
    Engine* m_engine = nullptr;
    ContentRegistry* m_registry = nullptr;
    EventBus* m_eventBus = nullptr;
    bool m_initialized = false;
};

} // namespace gloaming
