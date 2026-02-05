#include "mod/ModManifest.hpp"
#include "engine/Log.hpp"

#include <fstream>
#include <sstream>
#include <regex>

namespace gloaming {

// ---------------------------------------------------------------------------
// Version
// ---------------------------------------------------------------------------

std::optional<Version> Version::parse(const std::string& str) {
    // Match "X.Y.Z" where X, Y, Z are non-negative integers
    std::regex re(R"((\d+)\.(\d+)\.(\d+))");
    std::smatch match;
    if (!std::regex_match(str, match, re)) {
        return std::nullopt;
    }
    return Version{
        std::stoi(match[1].str()),
        std::stoi(match[2].str()),
        std::stoi(match[3].str())
    };
}

std::string Version::toString() const {
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

// ---------------------------------------------------------------------------
// VersionRequirement
// ---------------------------------------------------------------------------

std::optional<VersionRequirement> VersionRequirement::parse(const std::string& str) {
    if (str.empty() || str == "*") {
        return VersionRequirement{Op::Any, {}};
    }

    VersionRequirement req;

    // Determine operator
    std::string versionStr = str;
    if (str.starts_with(">=")) {
        req.op = Op::GreaterEqual;
        versionStr = str.substr(2);
    } else if (str.starts_with(">")) {
        req.op = Op::Greater;
        versionStr = str.substr(1);
    } else if (str.starts_with("<=")) {
        req.op = Op::LessEqual;
        versionStr = str.substr(2);
    } else if (str.starts_with("<")) {
        req.op = Op::Less;
        versionStr = str.substr(1);
    } else if (str.starts_with("==")) {
        req.op = Op::Equal;
        versionStr = str.substr(2);
    } else {
        req.op = Op::GreaterEqual;  // Default: treat bare version as >=
    }

    auto ver = Version::parse(versionStr);
    if (!ver) return std::nullopt;
    req.version = *ver;

    return req;
}

bool VersionRequirement::satisfiedBy(const Version& ver) const {
    switch (op) {
        case Op::Any:          return true;
        case Op::Equal:        return ver == version;
        case Op::GreaterEqual: return ver >= version;
        case Op::Greater:      return ver > version;
        case Op::LessEqual:    return ver <= version;
        case Op::Less:         return ver < version;
    }
    return false;
}

std::string VersionRequirement::toString() const {
    switch (op) {
        case Op::Any:          return "*";
        case Op::Equal:        return "==" + version.toString();
        case Op::GreaterEqual: return ">=" + version.toString();
        case Op::Greater:      return ">" + version.toString();
        case Op::LessEqual:    return "<=" + version.toString();
        case Op::Less:         return "<" + version.toString();
    }
    return "?";
}

// ---------------------------------------------------------------------------
// ModManifest
// ---------------------------------------------------------------------------

static ModDependency parseDependency(const nlohmann::json& json) {
    ModDependency dep;
    if (json.is_string()) {
        dep.id = json.get<std::string>();
        dep.versionReq = {VersionRequirement::Op::Any, {}};
    } else if (json.is_object()) {
        dep.id = json.value("id", "");
        std::string verStr = json.value("version", "*");
        auto req = VersionRequirement::parse(verStr);
        dep.versionReq = req.value_or(VersionRequirement{VersionRequirement::Op::Any, {}});
    }
    return dep;
}

std::optional<ModManifest> ModManifest::fromJson(const nlohmann::json& json,
                                                  const std::string& modDir) {
    ModManifest manifest;
    manifest.directory = modDir;

    // Required fields
    if (!json.contains("id") || !json["id"].is_string()) {
        LOG_ERROR("ModManifest: missing required field 'id'");
        return std::nullopt;
    }
    manifest.id = json["id"].get<std::string>();

    if (!json.contains("name") || !json["name"].is_string()) {
        LOG_ERROR("ModManifest: '{}' missing required field 'name'", manifest.id);
        return std::nullopt;
    }
    manifest.name = json["name"].get<std::string>();

    if (!json.contains("version") || !json["version"].is_string()) {
        LOG_ERROR("ModManifest: '{}' missing required field 'version'", manifest.id);
        return std::nullopt;
    }
    auto ver = Version::parse(json["version"].get<std::string>());
    if (!ver) {
        LOG_ERROR("ModManifest: '{}' has invalid version format", manifest.id);
        return std::nullopt;
    }
    manifest.version = *ver;

    // Optional: engine_version
    if (json.contains("engine_version") && json["engine_version"].is_string()) {
        auto req = VersionRequirement::parse(json["engine_version"].get<std::string>());
        if (req) {
            manifest.engineVersionReq = *req;
        }
    }

    // Optional: authors
    if (json.contains("authors") && json["authors"].is_array()) {
        for (const auto& author : json["authors"]) {
            if (author.is_string()) {
                manifest.authors.push_back(author.get<std::string>());
            }
        }
    }

    // Optional: description
    manifest.description = json.value("description", "");

    // Optional: dependencies
    if (json.contains("dependencies") && json["dependencies"].is_array()) {
        for (const auto& dep : json["dependencies"]) {
            manifest.dependencies.push_back(parseDependency(dep));
        }
    }

    // Optional: optional_dependencies
    if (json.contains("optional_dependencies") && json["optional_dependencies"].is_array()) {
        for (const auto& dep : json["optional_dependencies"]) {
            manifest.optionalDependencies.push_back(parseDependency(dep));
        }
    }

    // Optional: incompatible
    if (json.contains("incompatible") && json["incompatible"].is_array()) {
        for (const auto& inc : json["incompatible"]) {
            if (inc.is_string()) {
                manifest.incompatible.push_back(inc.get<std::string>());
            }
        }
    }

    // Optional: load_priority
    manifest.loadPriority = json.value("load_priority", 100);

    // Optional: entry_point
    manifest.entryPoint = json.value("entry_point", "scripts/init.lua");

    // Optional: provides
    if (json.contains("provides") && json["provides"].is_object()) {
        const auto& prov = json["provides"];
        manifest.provides.content = prov.value("content", false);
        manifest.provides.worldgen = prov.value("worldgen", false);
        manifest.provides.ui = prov.value("ui", false);
        manifest.provides.audio = prov.value("audio", false);
    }

    return manifest;
}

std::optional<ModManifest> ModManifest::fromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("ModManifest: cannot open '{}'", path);
        return std::nullopt;
    }

    nlohmann::json json;
    try {
        file >> json;
    } catch (const nlohmann::json::parse_error& e) {
        LOG_ERROR("ModManifest: JSON parse error in '{}': {}", path, e.what());
        return std::nullopt;
    }

    // Derive directory from file path
    std::string dir = path;
    auto pos = dir.find_last_of("/\\");
    if (pos != std::string::npos) {
        dir = dir.substr(0, pos);
    }

    return fromJson(json, dir);
}

std::vector<std::string> ModManifest::validate() const {
    std::vector<std::string> errors;

    if (id.empty()) {
        errors.push_back("Mod id is empty");
    }
    // Validate ID format: lowercase alphanumeric + hyphens
    for (char c : id) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_') {
            errors.push_back("Mod id '" + id + "' contains invalid character '" + c + "'");
            break;
        }
    }
    if (name.empty()) {
        errors.push_back("Mod name is empty");
    }
    if (version == Version{0, 0, 0}) {
        errors.push_back("Mod version is 0.0.0");
    }
    if (entryPoint.empty()) {
        errors.push_back("Entry point is empty");
    }
    for (const auto& dep : dependencies) {
        if (dep.id.empty()) {
            errors.push_back("Dependency has empty id");
        }
        if (dep.id == id) {
            errors.push_back("Mod depends on itself");
        }
    }
    return errors;
}

} // namespace gloaming
