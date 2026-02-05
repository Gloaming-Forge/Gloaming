#include "engine/Config.hpp"

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

} // namespace gloaming
