#include "mod/ModLoader.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace gloaming {

bool ModLoader::init(Engine& engine, const ModLoaderConfig& config) {
    m_engine = &engine;
    m_config = config;

    // Initialize Lua bindings
    if (!m_luaBindings.init(engine, m_contentRegistry, m_eventBus)) {
        LOG_ERROR("ModLoader: failed to initialize Lua bindings");
        return false;
    }

    // Load mod enabled/disabled state
    loadModConfig();

    LOG_INFO("ModLoader: initialized (mods directory: '{}')", m_config.modsDirectory);
    return true;
}

int ModLoader::discoverMods() {
    m_mods.clear();
    m_loadOrder.clear();

    if (!fs::exists(m_config.modsDirectory) || !fs::is_directory(m_config.modsDirectory)) {
        LOG_WARN("ModLoader: mods directory '{}' does not exist", m_config.modsDirectory);
        return 0;
    }

    int count = 0;
    for (const auto& entry : fs::directory_iterator(m_config.modsDirectory)) {
        if (!entry.is_directory()) continue;

        std::string modJsonPath = entry.path().string() + "/mod.json";
        if (!fs::exists(modJsonPath)) continue;

        auto manifest = ModManifest::fromFile(modJsonPath);
        if (!manifest) {
            LOG_WARN("ModLoader: failed to parse manifest in '{}'", entry.path().string());
            continue;
        }

        // Validate
        auto errors = manifest->validate();
        if (!errors.empty()) {
            for (const auto& err : errors) {
                LOG_WARN("ModLoader: manifest validation error in '{}': {}",
                         manifest->id, err);
            }
            // Non-fatal: still register the mod but mark the issues
        }

        LoadedMod mod;
        mod.manifest = std::move(*manifest);
        mod.state = m_disabledMods.count(mod.manifest.id) > 0
            ? ModState::Disabled
            : ModState::Discovered;

        LOG_INFO("ModLoader: discovered mod '{}' v{} in '{}'",
                 mod.manifest.id, mod.manifest.version.toString(),
                 mod.manifest.directory);

        m_mods[mod.manifest.id] = std::move(mod);
        ++count;
    }

    LOG_INFO("ModLoader: discovered {} mods", count);
    return count;
}

bool ModLoader::resolveDependencies() {
    // Collect all mod IDs that are enabled (not disabled, not failed)
    std::vector<std::string> enabledIds;
    for (const auto& [id, mod] : m_mods) {
        if (mod.state != ModState::Disabled) {
            enabledIds.push_back(id);
        }
    }

    // Check for missing dependencies (report all errors per mod, not just first)
    for (const auto& id : enabledIds) {
        const auto& mod = m_mods[id];
        std::vector<std::string> depErrors;
        for (const auto& dep : mod.manifest.dependencies) {
            auto it = m_mods.find(dep.id);
            if (it == m_mods.end()) {
                LOG_ERROR("ModLoader: mod '{}' requires '{}' which was not found",
                          id, dep.id);
                depErrors.push_back("Missing dependency: " + dep.id);
            } else if (it->second.state == ModState::Disabled) {
                LOG_ERROR("ModLoader: mod '{}' requires '{}' which is disabled",
                          id, dep.id);
                depErrors.push_back("Disabled dependency: " + dep.id);
            } else if (!dep.versionReq.satisfiedBy(it->second.manifest.version)) {
                LOG_ERROR("ModLoader: mod '{}' requires '{}' {} but found {}",
                          id, dep.id, dep.versionReq.toString(),
                          it->second.manifest.version.toString());
                depErrors.push_back("Version mismatch for " + dep.id);
            }
        }
        if (!depErrors.empty()) {
            m_mods[id].state = ModState::Failed;
            std::string combined;
            for (size_t i = 0; i < depErrors.size(); ++i) {
                if (i > 0) combined += "; ";
                combined += depErrors[i];
            }
            m_mods[id].errorMessage = combined;
        }
    }

    // Check for incompatible mods
    for (const auto& id : enabledIds) {
        const auto& mod = m_mods[id];
        if (mod.state == ModState::Failed) continue;

        for (const auto& incomp : mod.manifest.incompatible) {
            auto it = m_mods.find(incomp);
            if (it != m_mods.end() && it->second.state != ModState::Disabled
                && it->second.state != ModState::Failed) {
                LOG_ERROR("ModLoader: mod '{}' is incompatible with '{}'", id, incomp);
                m_mods[id].state = ModState::Failed;
                m_mods[id].errorMessage = "Incompatible with: " + incomp;
                break;
            }
        }
    }

    // Rebuild enabled list (exclude newly failed)
    enabledIds.clear();
    for (const auto& [id, mod] : m_mods) {
        if (mod.state == ModState::Discovered) {
            enabledIds.push_back(id);
        }
    }

    // Topological sort
    if (!topologicalSort(enabledIds, m_loadOrder)) {
        LOG_ERROR("ModLoader: dependency cycle detected");
        return false;
    }

    LOG_INFO("ModLoader: load order resolved ({} mods):", m_loadOrder.size());
    for (size_t i = 0; i < m_loadOrder.size(); ++i) {
        LOG_INFO("  {}: {}", i + 1, m_loadOrder[i]);
    }

    return true;
}

int ModLoader::loadMods() {
    int loaded = 0;
    for (const auto& modId : m_loadOrder) {
        if (loadMod(modId)) {
            ++loaded;
        }
    }

    // Update the mods.isLoaded API
    updateModsAPI();

    LOG_INFO("ModLoader: loaded {}/{} mods", loaded, m_loadOrder.size());
    return loaded;
}

void ModLoader::postInitMods() {
    for (const auto& modId : m_loadOrder) {
        auto it = m_mods.find(modId);
        if (it == m_mods.end() || it->second.state != ModState::Loaded) continue;

        auto& mod = it->second;
        if (mod.modTable.valid()) {
            sol::optional<sol::function> postInit = mod.modTable["postInit"];
            if (postInit) {
                try {
                    auto result = (*postInit)();
                    if (!result.valid()) {
                        sol::error err = result;
                        MOD_LOG_WARN("[{}] postInit error: {}", modId, err.what());
                        mod.state = ModState::Failed;
                        mod.errorMessage = "postInit error: " + std::string(err.what());
                        continue;
                    } else {
                        MOD_LOG_DEBUG("[{}] postInit completed", modId);
                    }
                } catch (const std::exception& e) {
                    MOD_LOG_WARN("[{}] postInit exception: {}", modId, e.what());
                    mod.state = ModState::Failed;
                    mod.errorMessage = "postInit exception: " + std::string(e.what());
                    continue;
                }
            }
        }
        mod.state = ModState::PostInit;
    }
    LOG_INFO("ModLoader: postInit completed for all mods");
}

void ModLoader::shutdown() {
    // Shutdown in reverse load order
    for (auto it = m_loadOrder.rbegin(); it != m_loadOrder.rend(); ++it) {
        auto modIt = m_mods.find(*it);
        if (modIt == m_mods.end()) continue;

        auto& mod = modIt->second;
        if (mod.state == ModState::Loaded || mod.state == ModState::PostInit) {
            if (mod.modTable.valid()) {
                sol::optional<sol::function> shutdownFn = mod.modTable["shutdown"];
                if (shutdownFn) {
                    try {
                        auto result = (*shutdownFn)();
                        if (!result.valid()) {
                            sol::error err = result;
                            MOD_LOG_WARN("[{}] shutdown error: {}", *it, err.what());
                        }
                    } catch (const std::exception& e) {
                        MOD_LOG_WARN("[{}] shutdown exception: {}", *it, e.what());
                    }
                }
            }
        }
    }

    // Clear mods first (releases sol::table/sol::environment references)
    // before shutting down the Lua state that owns them
    m_mods.clear();
    m_loadOrder.clear();
    m_contentRegistry.clear();
    m_eventBus.clear();
    m_luaBindings.shutdown();

    LOG_INFO("ModLoader: shut down");
}

bool ModLoader::loadMod(const std::string& modId) {
    auto it = m_mods.find(modId);
    if (it == m_mods.end()) {
        LOG_ERROR("ModLoader: mod '{}' not found", modId);
        return false;
    }

    auto& mod = it->second;
    mod.state = ModState::Loading;

    // Create Lua environment for this mod
    mod.luaEnv = m_luaBindings.createModEnvironment(modId);
    mod.luaEnv["_MOD_DIR"] = mod.manifest.directory;

    // Execute entry point
    std::string entryPath = mod.manifest.directory + "/" + mod.manifest.entryPoint;
    if (!fs::exists(entryPath)) {
        MOD_LOG_WARN("[{}] entry point '{}' not found, skipping script execution",
                     modId, entryPath);
        mod.state = ModState::Loaded;
        return true;
    }

    // Load the entry point file
    auto result = m_luaBindings.getState().load_file(entryPath);
    if (!result.valid()) {
        sol::error err = result;
        MOD_LOG_ERROR("[{}] failed to load entry point: {}", modId, err.what());
        mod.state = ModState::Failed;
        mod.errorMessage = "Script load error: " + std::string(err.what());
        return false;
    }

    sol::protected_function chunk = result;
    sol::set_environment(mod.luaEnv, chunk);

    auto execResult = chunk();
    if (!execResult.valid()) {
        sol::error err = execResult;
        MOD_LOG_ERROR("[{}] entry point execution error: {}", modId, err.what());
        mod.state = ModState::Failed;
        mod.errorMessage = "Script execution error: " + std::string(err.what());
        return false;
    }

    // The entry point should return a mod table
    if (execResult.get_type() == sol::type::table) {
        mod.modTable = execResult;

        // Call init() if present
        sol::optional<sol::function> initFn = mod.modTable["init"];
        if (initFn) {
            auto initResult = (*initFn)();
            if (!initResult.valid()) {
                sol::error err = initResult;
                MOD_LOG_ERROR("[{}] init() error: {}", modId, err.what());
                mod.state = ModState::Failed;
                mod.errorMessage = "init() error: " + std::string(err.what());
                return false;
            }
        }
    }

    mod.state = ModState::Loaded;
    MOD_LOG_INFO("[{}] loaded successfully (v{})", modId, mod.manifest.version.toString());
    return true;
}

bool ModLoader::isModLoaded(const std::string& modId) const {
    auto it = m_mods.find(modId);
    if (it == m_mods.end()) return false;
    return it->second.state == ModState::Loaded || it->second.state == ModState::PostInit;
}

const LoadedMod* ModLoader::getMod(const std::string& modId) const {
    auto it = m_mods.find(modId);
    return it != m_mods.end() ? &it->second : nullptr;
}

std::vector<std::string> ModLoader::getDiscoveredModIds() const {
    std::vector<std::string> ids;
    ids.reserve(m_mods.size());
    for (const auto& [id, _] : m_mods) {
        ids.push_back(id);
    }
    return ids;
}

void ModLoader::setModEnabled(const std::string& modId, bool enabled) {
    if (enabled) {
        m_disabledMods.erase(modId);
    } else {
        m_disabledMods.insert(modId);
    }
    saveModConfig();
}

bool ModLoader::isModEnabled(const std::string& modId) const {
    return m_disabledMods.count(modId) == 0;
}

size_t ModLoader::loadedCount() const {
    size_t count = 0;
    for (const auto& [_, mod] : m_mods) {
        if (mod.state == ModState::Loaded || mod.state == ModState::PostInit) {
            ++count;
        }
    }
    return count;
}

size_t ModLoader::failedCount() const {
    size_t count = 0;
    for (const auto& [_, mod] : m_mods) {
        if (mod.state == ModState::Failed) {
            ++count;
        }
    }
    return count;
}

// ---------------------------------------------------------------------------
// Dependency Resolution
// ---------------------------------------------------------------------------

bool ModLoader::topologicalSort(const std::vector<std::string>& modIds,
                                 std::vector<std::string>& sorted) const {
    sorted.clear();
    std::unordered_set<std::string> visiting;
    std::unordered_set<std::string> visited;

    // Check for cycles first
    for (const auto& id : modIds) {
        if (visited.count(id) == 0) {
            if (hasCycle(id, visiting, visited)) {
                return false;
            }
        }
    }

    // Now do the actual sort
    visited.clear();
    visiting.clear();

    std::function<void(const std::string&)> visit = [&](const std::string& id) {
        if (visited.count(id) > 0) return;
        visited.insert(id);

        auto it = m_mods.find(id);
        if (it != m_mods.end()) {
            for (const auto& dep : it->second.manifest.dependencies) {
                if (std::find(modIds.begin(), modIds.end(), dep.id) != modIds.end()) {
                    visit(dep.id);
                }
            }
            // Also consider optional dependencies if they're available
            for (const auto& dep : it->second.manifest.optionalDependencies) {
                if (std::find(modIds.begin(), modIds.end(), dep.id) != modIds.end()) {
                    visit(dep.id);
                }
            }
        }

        sorted.push_back(id);
    };

    // Sort by load_priority first, then dependency order
    auto prioritySorted = modIds;
    std::sort(prioritySorted.begin(), prioritySorted.end(),
        [this](const std::string& a, const std::string& b) {
            auto itA = m_mods.find(a);
            auto itB = m_mods.find(b);
            int prioA = itA != m_mods.end() ? itA->second.manifest.loadPriority : 100;
            int prioB = itB != m_mods.end() ? itB->second.manifest.loadPriority : 100;
            return prioA < prioB;
        });

    for (const auto& id : prioritySorted) {
        visit(id);
    }

    return true;
}

bool ModLoader::hasCycle(const std::string& modId,
                          std::unordered_set<std::string>& visiting,
                          std::unordered_set<std::string>& visited) const {
    if (visiting.count(modId) > 0) return true;   // Cycle detected
    if (visited.count(modId) > 0) return false;    // Already processed

    visiting.insert(modId);

    auto it = m_mods.find(modId);
    if (it != m_mods.end()) {
        for (const auto& dep : it->second.manifest.dependencies) {
            if (m_mods.count(dep.id) > 0) {
                if (hasCycle(dep.id, visiting, visited)) {
                    LOG_ERROR("ModLoader: dependency cycle: {} -> {}", modId, dep.id);
                    return true;
                }
            }
        }
    }

    visiting.erase(modId);
    visited.insert(modId);
    return false;
}

// ---------------------------------------------------------------------------
// Config persistence
// ---------------------------------------------------------------------------

void ModLoader::loadModConfig() {
    if (m_config.configFile.empty()) return;
    if (!fs::exists(m_config.configFile)) return;

    try {
        std::ifstream file(m_config.configFile);
        if (!file.is_open()) return;

        nlohmann::json json;
        file >> json;

        if (json.contains("disabled") && json["disabled"].is_array()) {
            for (const auto& id : json["disabled"]) {
                if (id.is_string()) {
                    m_disabledMods.insert(id.get<std::string>());
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_WARN("ModLoader: failed to load mod config: {}", e.what());
    }
}

void ModLoader::saveModConfig() const {
    if (m_config.configFile.empty()) return;

    try {
        // Ensure directory exists
        fs::path configPath(m_config.configFile);
        if (configPath.has_parent_path()) {
            fs::create_directories(configPath.parent_path());
        }

        nlohmann::json json;
        json["disabled"] = nlohmann::json::array();
        for (const auto& id : m_disabledMods) {
            json["disabled"].push_back(id);
        }

        std::ofstream file(m_config.configFile);
        if (file.is_open()) {
            file << json.dump(4) << std::endl;
        }
    } catch (const std::exception& e) {
        LOG_WARN("ModLoader: failed to save mod config: {}", e.what());
    }
}

void ModLoader::updateModsAPI() {
    // Update the mods.isLoaded function to reflect actual state
    auto& lua = m_luaBindings.getState();
    lua["mods"]["isLoaded"] = [this](const std::string& modId) -> bool {
        return isModLoaded(modId);
    };

    // Also provide list of loaded mod IDs
    sol::table loadedList = lua.create_table();
    int idx = 1;
    for (const auto& id : m_loadOrder) {
        if (isModLoaded(id)) {
            loadedList[idx++] = id;
        }
    }
    lua["mods"]["loaded"] = loadedList;
}

} // namespace gloaming
