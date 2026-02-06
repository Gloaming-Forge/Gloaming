#pragma once

#include "mod/ModManifest.hpp"
#include "mod/ContentRegistry.hpp"
#include "mod/LuaBindings.hpp"
#include "mod/EventBus.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace gloaming {

// Forward declarations
class Engine;

/// State of a loaded mod
enum class ModState {
    Discovered,     // Manifest parsed, not yet loaded
    Loading,        // Currently loading
    Loaded,         // Successfully loaded (init called)
    PostInit,       // postInit called
    Failed,         // Failed to load
    Disabled        // Present but disabled by user
};

/// Runtime information about a loaded mod
struct LoadedMod {
    ModManifest manifest;
    ModState state = ModState::Discovered;
    sol::environment luaEnv;
    sol::table modTable;  // The table returned by init.lua
    std::string errorMessage;
};

/// Configuration for the mod loader
struct ModLoaderConfig {
    std::string modsDirectory = "mods";
    std::string installDirectory = "mods/install";
    std::string configFile = "config/mods.json";
    bool enableHotReload = false;
};

/// The mod loader orchestrates mod discovery, dependency resolution, and loading
class ModLoader {
public:
    ModLoader() = default;
    ~ModLoader() = default;

    // Non-copyable
    ModLoader(const ModLoader&) = delete;
    ModLoader& operator=(const ModLoader&) = delete;

    /// Initialize the mod loader
    bool init(Engine& engine, const ModLoaderConfig& config = {});

    /// Discover all mods in the mods directory
    /// Returns the number of mods discovered
    int discoverMods();

    /// Resolve dependencies and determine load order.
    /// Returns false if there are unresolvable dependency issues.
    bool resolveDependencies();

    /// Load all discovered and enabled mods in dependency order.
    /// Returns the number of mods successfully loaded.
    int loadMods();

    /// Call postInit on all loaded mods (after all mods are loaded)
    void postInitMods();

    /// Shutdown all mods
    void shutdown();

    /// Get the content registry
    ContentRegistry& getContentRegistry() { return m_contentRegistry; }
    const ContentRegistry& getContentRegistry() const { return m_contentRegistry; }

    /// Get the event bus
    EventBus& getEventBus() { return m_eventBus; }
    const EventBus& getEventBus() const { return m_eventBus; }

    /// Get the Lua bindings
    LuaBindings& getLuaBindings() { return m_luaBindings; }

    /// Check if a mod is loaded
    bool isModLoaded(const std::string& modId) const;

    /// Get a loaded mod by ID
    const LoadedMod* getMod(const std::string& modId) const;

    /// Get all loaded mod IDs in load order
    const std::vector<std::string>& getLoadOrder() const { return m_loadOrder; }

    /// Get all discovered mod IDs
    std::vector<std::string> getDiscoveredModIds() const;

    /// Enable/disable a mod (takes effect on next load)
    void setModEnabled(const std::string& modId, bool enabled);
    bool isModEnabled(const std::string& modId) const;

    /// Get the mods directory path
    const std::string& getModsDirectory() const { return m_config.modsDirectory; }

    /// Get stats
    size_t discoveredCount() const { return m_mods.size(); }
    size_t loadedCount() const;
    size_t failedCount() const;

private:
    /// Load a single mod (called in dependency order)
    bool loadMod(const std::string& modId);

    /// Topological sort for dependency resolution
    bool topologicalSort(const std::vector<std::string>& modIds,
                         std::vector<std::string>& sorted) const;

    /// Check for dependency cycles using DFS
    bool hasCycle(const std::string& modId,
                  std::unordered_set<std::string>& visiting,
                  std::unordered_set<std::string>& visited) const;

    /// Load enabled/disabled state from config
    void loadModConfig();

    /// Save enabled/disabled state to config
    void saveModConfig() const;

    /// Update the mods.isLoaded binding to reflect actual state
    void updateModsAPI();

    Engine* m_engine = nullptr;
    ModLoaderConfig m_config;

    // Core subsystems
    ContentRegistry m_contentRegistry;
    EventBus m_eventBus;
    LuaBindings m_luaBindings;

    // Mod data
    std::unordered_map<std::string, LoadedMod> m_mods;
    std::vector<std::string> m_loadOrder;

    // Enabled/disabled state
    std::unordered_set<std::string> m_disabledMods;
};

} // namespace gloaming
