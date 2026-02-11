#include "gameplay/SaveSystem.hpp"
#include "engine/Log.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace gloaming {

void SaveSystem::setWorldPath(const std::string& worldPath) {
    m_worldPath = worldPath;
}

bool SaveSystem::set(const std::string& modId, const std::string& key, const nlohmann::json& value) {
    // Validate nesting depth
    if (!validateDepth(value, MAX_NESTING_DEPTH)) {
        LOG_WARN("SaveSystem::set: value for key '{}' in mod '{}' exceeds max nesting depth ({})",
                 key, modId, MAX_NESTING_DEPTH);
        return false;
    }

    // Set the value in memory
    auto& data = m_modData[modId];
    data[key] = value;

    // Check size limit
    size_t size = estimateSize(modId);
    if (size > MAX_SAVE_FILE_SIZE) {
        // Revert the change
        data.erase(key);
        if (data.empty()) {
            m_modData.erase(modId);
        }
        LOG_WARN("SaveSystem::set: mod '{}' save data would exceed {} byte limit (attempted: {} bytes)",
                 modId, MAX_SAVE_FILE_SIZE, size);
        return false;
    }

    m_dirty = true;
    return true;
}

nlohmann::json SaveSystem::get(const std::string& modId, const std::string& key,
                                const nlohmann::json& defaultValue) const {
    auto modIt = m_modData.find(modId);
    if (modIt == m_modData.end()) return defaultValue;

    auto keyIt = modIt->second.find(key);
    if (keyIt == modIt->second.end()) return defaultValue;

    return *keyIt;
}

bool SaveSystem::remove(const std::string& modId, const std::string& key) {
    auto modIt = m_modData.find(modId);
    if (modIt == m_modData.end()) return false;

    auto erased = modIt->second.erase(key);
    if (erased > 0) {
        m_dirty = true;
        // Clean up empty mod entries
        if (modIt->second.empty()) {
            m_modData.erase(modIt);
        }
        return true;
    }
    return false;
}

bool SaveSystem::has(const std::string& modId, const std::string& key) const {
    auto modIt = m_modData.find(modId);
    if (modIt == m_modData.end()) return false;
    return modIt->second.contains(key);
}

std::vector<std::string> SaveSystem::keys(const std::string& modId) const {
    std::vector<std::string> result;
    auto modIt = m_modData.find(modId);
    if (modIt != m_modData.end() && modIt->second.is_object()) {
        for (auto it = modIt->second.begin(); it != modIt->second.end(); ++it) {
            result.push_back(it.key());
        }
    }
    return result;
}

int SaveSystem::loadAll() {
    if (m_worldPath.empty()) {
        LOG_WARN("SaveSystem::loadAll: no world path set");
        return 0;
    }

    std::string moddataDir = m_worldPath + "/moddata";
    if (!fs::exists(moddataDir)) {
        LOG_INFO("SaveSystem: no moddata directory at '{}', nothing to load", moddataDir);
        return 0;
    }

    int loaded = 0;
    try {
        for (const auto& entry : fs::directory_iterator(moddataDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string modId = entry.path().stem().string();
                // Skip backup files
                if (modId.size() > 4 && modId.substr(modId.size() - 4) == ".bak") continue;

                if (loadMod(modId)) {
                    ++loaded;
                }
            }
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("SaveSystem::loadAll: error scanning moddata directory: {}", ex.what());
    }

    m_dirty = false;
    LOG_INFO("SaveSystem: loaded {} mod save files", loaded);
    return loaded;
}

bool SaveSystem::loadMod(const std::string& modId) {
    std::string filePath = getModFilePath(modId);

    if (!fs::exists(filePath)) {
        // Not an error — mod just doesn't have saved data yet
        return true;
    }

    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            LOG_WARN("SaveSystem::loadMod: could not open '{}'", filePath);
            return false;
        }

        nlohmann::json data = nlohmann::json::parse(file);
        if (!data.is_object()) {
            LOG_WARN("SaveSystem::loadMod: '{}' is not a JSON object, trying backup", filePath);
            // Try backup
            return loadFromBackup(modId);
        }

        m_modData[modId] = std::move(data);
        LOG_INFO("SaveSystem: loaded save data for mod '{}'", modId);
        return true;

    } catch (const nlohmann::json::parse_error& ex) {
        LOG_WARN("SaveSystem::loadMod: parse error in '{}': {}, trying backup", filePath, ex.what());
        return loadFromBackup(modId);
    } catch (const std::exception& ex) {
        LOG_ERROR("SaveSystem::loadMod: error loading '{}': {}", filePath, ex.what());
        return false;
    }
}

int SaveSystem::saveAll() {
    if (m_worldPath.empty()) {
        LOG_WARN("SaveSystem::saveAll: no world path set");
        return 0;
    }

    int saved = 0;
    for (const auto& [modId, data] : m_modData) {
        if (saveMod(modId)) {
            ++saved;
        }
    }

    m_dirty = false;
    LOG_INFO("SaveSystem: saved {} mod save files", saved);
    return saved;
}

bool SaveSystem::saveMod(const std::string& modId) {
    auto modIt = m_modData.find(modId);
    if (modIt == m_modData.end() || modIt->second.empty()) {
        return true; // Nothing to save
    }

    // Ensure moddata directory exists
    std::string moddataDir = m_worldPath + "/moddata";
    try {
        fs::create_directories(moddataDir);
    } catch (const std::exception& ex) {
        LOG_ERROR("SaveSystem::saveMod: failed to create directory '{}': {}", moddataDir, ex.what());
        return false;
    }

    std::string filePath = getModFilePath(modId);
    std::string backupPath = getModBackupPath(modId);

    // Create backup of existing file
    if (fs::exists(filePath)) {
        try {
            fs::copy_file(filePath, backupPath, fs::copy_options::overwrite_existing);
        } catch (const std::exception& ex) {
            LOG_WARN("SaveSystem::saveMod: failed to create backup for '{}': {}", modId, ex.what());
            // Continue saving anyway
        }
    }

    // Write new file
    try {
        std::ofstream file(filePath);
        if (!file.is_open()) {
            LOG_ERROR("SaveSystem::saveMod: could not open '{}' for writing", filePath);
            return false;
        }
        file << modIt->second.dump(2);
        file.close();

        if (file.fail()) {
            LOG_ERROR("SaveSystem::saveMod: write error for '{}'", filePath);
            return false;
        }

        return true;
    } catch (const std::exception& ex) {
        LOG_ERROR("SaveSystem::saveMod: error saving '{}': {}", filePath, ex.what());
        return false;
    }
}

void SaveSystem::clear() {
    m_modData.clear();
    m_dirty = false;
}

size_t SaveSystem::keyCount(const std::string& modId) const {
    auto modIt = m_modData.find(modId);
    if (modIt == m_modData.end()) return 0;
    return modIt->second.size();
}

size_t SaveSystem::estimateSize(const std::string& modId) const {
    auto modIt = m_modData.find(modId);
    if (modIt == m_modData.end()) return 0;
    return modIt->second.dump().size();
}

std::string SaveSystem::getModFilePath(const std::string& modId) const {
    return m_worldPath + "/moddata/" + modId + ".json";
}

std::string SaveSystem::getModBackupPath(const std::string& modId) const {
    return m_worldPath + "/moddata/" + modId + ".json.bak";
}

bool SaveSystem::validateDepth(const nlohmann::json& value, int maxDepth) {
    return validateDepthRecursive(value, 0, maxDepth);
}

bool SaveSystem::validateDepthRecursive(const nlohmann::json& value, int currentDepth, int maxDepth) {
    if (currentDepth > maxDepth) return false;

    if (value.is_object()) {
        for (auto it = value.begin(); it != value.end(); ++it) {
            if (!validateDepthRecursive(it.value(), currentDepth + 1, maxDepth)) {
                return false;
            }
        }
    } else if (value.is_array()) {
        for (const auto& elem : value) {
            if (!validateDepthRecursive(elem, currentDepth + 1, maxDepth)) {
                return false;
            }
        }
    }

    return true;
}

// Private helper for loading from backup
bool SaveSystem::loadFromBackup(const std::string& modId) {
    std::string backupPath = getModBackupPath(modId);
    if (!fs::exists(backupPath)) {
        LOG_WARN("SaveSystem: no backup found for mod '{}'", modId);
        return false;
    }

    try {
        std::ifstream file(backupPath);
        if (!file.is_open()) {
            LOG_WARN("SaveSystem: could not open backup for mod '{}'", modId);
            return false;
        }

        nlohmann::json data = nlohmann::json::parse(file);
        if (!data.is_object()) {
            LOG_ERROR("SaveSystem: backup for mod '{}' is also invalid", modId);
            return false;
        }

        m_modData[modId] = std::move(data);
        LOG_WARN("SaveSystem: loaded mod '{}' from backup (primary file was corrupted)", modId);
        return true;

    } catch (const std::exception& ex) {
        LOG_ERROR("SaveSystem: backup for mod '{}' is also corrupted: {}", modId, ex.what());
        return false;
    }
}

} // namespace gloaming
