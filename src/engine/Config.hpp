#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace gloaming {

class Config {
public:
    /// Load configuration from a JSON file. Returns false if the file
    /// cannot be read; missing keys are handled via default values.
    bool loadFromFile(const std::string& path);

    /// Load configuration from a JSON string (useful for testing).
    bool loadFromString(const std::string& jsonStr);

    std::string getString(const std::string& key, const std::string& defaultVal = "") const;
    int         getInt(const std::string& key, int defaultVal = 0) const;
    float       getFloat(const std::string& key, float defaultVal = 0.0f) const;
    bool        getBool(const std::string& key, bool defaultVal = false) const;

    /// Check if a key exists (supports dot-notation, e.g. "window.width").
    bool hasKey(const std::string& key) const;

    const nlohmann::json& raw() const { return m_data; }

private:
    /// Resolve a dot-separated key path into the nested JSON value.
    const nlohmann::json* resolve(const std::string& key) const;

    nlohmann::json m_data;
};

} // namespace gloaming
