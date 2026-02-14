#include "engine/Config.hpp"
#include "engine/Log.hpp"

#include <fstream>
#include <sstream>

namespace gloaming {

bool Config::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    try {
        m_data = nlohmann::json::parse(file);
    } catch (const nlohmann::json::parse_error&) {
        return false;
    }
    return true;
}

bool Config::loadFromString(const std::string& jsonStr) {
    try {
        m_data = nlohmann::json::parse(jsonStr);
    } catch (const nlohmann::json::parse_error&) {
        return false;
    }
    return true;
}

bool Config::mergeFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    nlohmann::json overlay;
    try {
        overlay = nlohmann::json::parse(file);
    } catch (const nlohmann::json::parse_error&) {
        return false;
    }

    // Merge into a copy so that m_data is unchanged if mergeJson throws.
    nlohmann::json merged = m_data;
    mergeJson(merged, overlay);
    m_data = std::move(merged);
    return true;
}

bool Config::saveToFile(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }

    file << m_data.dump(4) << '\n';
    return file.good();
}

bool Config::saveOverridesToFile(const std::string& path) const {
    if (m_dirtyKeys.empty()) {
        return false;
    }

    // Build a minimal JSON containing only the keys that were
    // modified at runtime, preserving their nested structure.
    nlohmann::json overrides = nlohmann::json::object();
    for (const auto& key : m_dirtyKeys) {
        const nlohmann::json* val = resolve(key);
        if (!val) continue;

        // Walk the dot-separated key path, creating intermediate objects
        nlohmann::json* cur = &overrides;
        std::istringstream stream(key);
        std::string segment;
        std::string prev;
        bool first = true;

        while (std::getline(stream, segment, '.')) {
            if (!first) {
                if (!cur->is_object()) *cur = nlohmann::json::object();
                cur = &(*cur)[prev];
            }
            prev = segment;
            first = false;
        }
        if (!cur->is_object()) *cur = nlohmann::json::object();
        (*cur)[prev] = *val;
    }

    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }

    file << overrides.dump(4) << '\n';
    return file.good();
}

// ---------------------------------------------------------------------------
// Key resolution
// ---------------------------------------------------------------------------

const nlohmann::json* Config::resolve(const std::string& key) const {
    const nlohmann::json* current = &m_data;
    std::istringstream stream(key);
    std::string segment;

    while (std::getline(stream, segment, '.')) {
        if (!current->is_object() || !current->contains(segment)) {
            return nullptr;
        }
        current = &(*current)[segment];
    }
    return current;
}

nlohmann::json& Config::resolveOrCreate(const std::string& key) {
    nlohmann::json* current = &m_data;
    std::istringstream stream(key);
    std::string segment;
    std::string nextSegment;

    // Read the first segment
    if (!std::getline(stream, segment, '.')) {
        // Empty key — return (and potentially create) a root "" key.
        return (*current)[""];
    }

    // Walk through all but the last segment, creating objects as needed
    while (std::getline(stream, nextSegment, '.')) {
        if (!current->is_object() && !current->is_null()) {
            LOG_WARN("Config: overwriting non-object value at '{}' with object", segment);
            *current = nlohmann::json::object();
        } else if (!current->is_object()) {
            *current = nlohmann::json::object();
        }
        current = &(*current)[segment];
        segment = nextSegment;
    }

    // Final segment — create or return the leaf
    if (!current->is_object() && !current->is_null()) {
        LOG_WARN("Config: overwriting non-object value with object while setting key '{}'", segment);
        *current = nlohmann::json::object();
    } else if (!current->is_object()) {
        *current = nlohmann::json::object();
    }
    return (*current)[segment];
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------

bool Config::hasKey(const std::string& key) const {
    return resolve(key) != nullptr;
}

std::string Config::getString(const std::string& key, const std::string& defaultVal) const {
    const auto* val = resolve(key);
    if (val && val->is_string()) {
        return val->get<std::string>();
    }
    return defaultVal;
}

int Config::getInt(const std::string& key, int defaultVal) const {
    const auto* val = resolve(key);
    if (val && val->is_number_integer()) {
        return val->get<int>();
    }
    return defaultVal;
}

float Config::getFloat(const std::string& key, float defaultVal) const {
    const auto* val = resolve(key);
    if (val && val->is_number()) {
        return val->get<float>();
    }
    return defaultVal;
}

bool Config::getBool(const std::string& key, bool defaultVal) const {
    const auto* val = resolve(key);
    if (val && val->is_boolean()) {
        return val->get<bool>();
    }
    return defaultVal;
}

// ---------------------------------------------------------------------------
// Setters
// ---------------------------------------------------------------------------

void Config::setString(const std::string& key, const std::string& value) {
    resolveOrCreate(key) = value;
    m_dirtyKeys.insert(key);
}

void Config::setInt(const std::string& key, int value) {
    resolveOrCreate(key) = value;
    m_dirtyKeys.insert(key);
}

void Config::setFloat(const std::string& key, float value) {
    resolveOrCreate(key) = value;
    m_dirtyKeys.insert(key);
}

void Config::setBool(const std::string& key, bool value) {
    resolveOrCreate(key) = value;
    m_dirtyKeys.insert(key);
}

// ---------------------------------------------------------------------------
// JSON merge
// ---------------------------------------------------------------------------

void Config::mergeJson(nlohmann::json& base, const nlohmann::json& overlay) {
    if (!overlay.is_object()) {
        base = overlay;
        return;
    }
    if (!base.is_object()) {
        base = nlohmann::json::object();
    }
    for (auto it = overlay.begin(); it != overlay.end(); ++it) {
        if (it->is_object() && base.contains(it.key()) && base[it.key()].is_object()) {
            mergeJson(base[it.key()], *it);
        } else {
            base[it.key()] = *it;
        }
    }
}

} // namespace gloaming
