#pragma once

#include <string>
#include <set>
#include <nlohmann/json.hpp>

namespace gloaming {

class Config {
public:
    /// Load configuration from a JSON file. Returns false if the file
    /// cannot be read; missing keys are handled via default values.
    bool loadFromFile(const std::string& path);

    /// Load configuration from a JSON string (useful for testing).
    bool loadFromString(const std::string& jsonStr);

    /// Merge another JSON file on top of the current configuration.
    /// Existing keys are overwritten by the overlay; keys not present
    /// in the overlay are preserved.  Returns false if the file cannot
    /// be read or parsed (the current config is unchanged on failure).
    bool mergeFromFile(const std::string& path);

    /// Save the current configuration to a JSON file.
    /// Returns false if the file cannot be written.
    bool saveToFile(const std::string& path) const;

    /// Save only the keys that were modified at runtime (via setters)
    /// to a JSON file.  This avoids dumping the entire merged config,
    /// which would mask future base config changes.  Returns false if
    /// the file cannot be written or no keys were modified.
    bool saveOverridesToFile(const std::string& path) const;

    // --- Getters (read with dot-notation key paths) ---

    std::string getString(const std::string& key, const std::string& defaultVal = "") const;
    int         getInt(const std::string& key, int defaultVal = 0) const;
    float       getFloat(const std::string& key, float defaultVal = 0.0f) const;
    bool        getBool(const std::string& key, bool defaultVal = false) const;

    // --- Setters (write with dot-notation key paths) ---

    void setString(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setFloat(const std::string& key, float value);
    void setBool(const std::string& key, bool value);

    /// Check if a key exists (supports dot-notation, e.g. "window.width").
    bool hasKey(const std::string& key) const;

    /// Return the set of keys modified at runtime via setters.
    const std::set<std::string>& dirtyKeys() const { return m_dirtyKeys; }

    const nlohmann::json& raw() const { return m_data; }

private:
    /// Resolve a dot-separated key path into the nested JSON value.
    const nlohmann::json* resolve(const std::string& key) const;

    /// Resolve a dot-separated key path for writing, creating
    /// intermediate objects as needed.  Returns a reference to the
    /// leaf value (which may be null/newly created).
    nlohmann::json& resolveOrCreate(const std::string& key);

    /// Recursively merge `overlay` into `base`.  Object keys in `overlay`
    /// overwrite or extend `base`; non-object values replace outright.
    static void mergeJson(nlohmann::json& base, const nlohmann::json& overlay);

    nlohmann::json m_data;
    std::set<std::string> m_dirtyKeys;  ///< Keys modified at runtime via setters
};

} // namespace gloaming
