#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>
#include <cstdint>

namespace gloaming {

/// Maximum save file size per mod (1 MB)
constexpr size_t MAX_SAVE_FILE_SIZE = 1024 * 1024;

/// Maximum nesting depth for table values
constexpr int MAX_NESTING_DEPTH = 8;

/// Key-value persistence for mod-specific data.
///
/// Each mod gets its own namespace — mods can't overwrite each other's data.
/// Data is stored in-memory and flushed to disk on world save.
///
/// File format: One JSON file per mod at `worlds/<world>/moddata/<mod-id>.json`
/// Backup:      `.bak` copy of previous save for corruption recovery
///
/// Value types: string, number, boolean, table (nested up to 8 levels deep)
/// Size limit:  1 MB per mod save file
class SaveSystem {
public:
    SaveSystem() = default;

    /// Set the world path for save file storage.
    /// Must be called before any save/load operations.
    void setWorldPath(const std::string& worldPath);

    /// Get the current world path
    const std::string& getWorldPath() const { return m_worldPath; }

    // ========================================================================
    // Per-Mod Data Access
    // ========================================================================

    /// Set a value for a key in the specified mod's save data.
    /// @param modId The mod identifier (namespace)
    /// @param key The key to store
    /// @param value JSON-compatible value (string, number, boolean, table/object)
    /// @return true if set successfully, false if size limit would be exceeded
    bool set(const std::string& modId, const std::string& key, const nlohmann::json& value);

    /// Get a value for a key from the specified mod's save data.
    /// @param modId The mod identifier
    /// @param key The key to retrieve
    /// @param defaultValue Value returned if key doesn't exist
    /// @return The stored value, or defaultValue if not found
    nlohmann::json get(const std::string& modId, const std::string& key,
                       const nlohmann::json& defaultValue = nlohmann::json()) const;

    /// Delete a key from the specified mod's save data.
    /// @param modId The mod identifier
    /// @param key The key to remove
    /// @return true if the key existed and was removed
    bool remove(const std::string& modId, const std::string& key);

    /// Check if a key exists in the specified mod's save data.
    bool has(const std::string& modId, const std::string& key) const;

    /// Get all keys for a mod
    std::vector<std::string> keys(const std::string& modId) const;

    // ========================================================================
    // File Operations
    // ========================================================================

    /// Load all mod data from disk for the current world.
    /// Called when a world is loaded.
    /// @return Number of mod files loaded
    int loadAll();

    /// Load a single mod's data from disk.
    /// @param modId The mod identifier
    /// @return true if loaded successfully (or file doesn't exist yet)
    bool loadMod(const std::string& modId);

    /// Save all mod data to disk.
    /// Called on world save (manual or auto-save).
    /// @return Number of mod files saved
    int saveAll();

    /// Save a single mod's data to disk.
    /// @param modId The mod identifier
    /// @return true if saved successfully
    bool saveMod(const std::string& modId);

    /// Clear all in-memory data (called when world is closed)
    void clear();

    // ========================================================================
    // Statistics
    // ========================================================================

    /// Get the number of mods with save data
    size_t modCount() const { return m_modData.size(); }

    /// Get the number of keys stored for a mod
    size_t keyCount(const std::string& modId) const;

    /// Get the approximate serialized size of a mod's data in bytes
    size_t estimateSize(const std::string& modId) const;

    /// Check if any data has been modified since last save
    bool isDirty() const { return m_dirty; }

private:
    /// Get the file path for a mod's save file
    std::string getModFilePath(const std::string& modId) const;

    /// Get the backup file path for a mod's save file
    std::string getModBackupPath(const std::string& modId) const;

    /// Validate nesting depth of a JSON value
    static bool validateDepth(const nlohmann::json& value, int maxDepth);
    static bool validateDepthRecursive(const nlohmann::json& value, int currentDepth, int maxDepth);

    /// Attempt to load mod data from a backup file
    bool loadFromBackup(const std::string& modId);

    std::string m_worldPath;
    std::unordered_map<std::string, nlohmann::json> m_modData; // modId -> { key: value, ... }
    bool m_dirty = false;
};

} // namespace gloaming
