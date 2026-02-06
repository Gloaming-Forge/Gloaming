#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace gloaming {

/// Semantic version for mods and engine compatibility
struct Version {
    int major = 0;
    int minor = 0;
    int patch = 0;

    constexpr Version() = default;
    constexpr Version(int maj, int min, int pat) : major(maj), minor(min), patch(pat) {}

    /// Parse from string like "1.2.3"
    static std::optional<Version> parse(const std::string& str);

    /// Convert to string
    std::string toString() const;

    // Comparison operators
    bool operator==(const Version& other) const {
        return major == other.major && minor == other.minor && patch == other.patch;
    }
    bool operator!=(const Version& other) const { return !(*this == other); }
    bool operator<(const Version& other) const {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return patch < other.patch;
    }
    bool operator>(const Version& other) const { return other < *this; }
    bool operator<=(const Version& other) const { return !(other < *this); }
    bool operator>=(const Version& other) const { return !(*this < other); }
};

/// Version requirement like ">=1.0.0"
struct VersionRequirement {
    enum class Op { Equal, GreaterEqual, Greater, LessEqual, Less, Any };

    Op op = Op::Any;
    Version version;

    /// Parse from string like ">=1.0.0" or "1.0.0" (exact) or "*" (any)
    static std::optional<VersionRequirement> parse(const std::string& str);

    /// Check if a version satisfies this requirement
    bool satisfiedBy(const Version& ver) const;

    std::string toString() const;
};

/// Dependency specification
struct ModDependency {
    std::string id;
    VersionRequirement versionReq;
};

/// What content types a mod provides
struct ModProvides {
    bool content = false;
    bool worldgen = false;
    bool ui = false;
    bool audio = false;
};

/// Parsed mod.json manifest
struct ModManifest {
    // Required fields
    std::string id;
    std::string name;
    Version version;

    // Optional fields
    VersionRequirement engineVersionReq;    // Engine version requirement
    std::vector<std::string> authors;
    std::string description;
    std::vector<ModDependency> dependencies;
    std::vector<ModDependency> optionalDependencies;
    std::vector<std::string> incompatible;
    int loadPriority = 100;
    std::string entryPoint = "scripts/init.lua";
    ModProvides provides;

    // Derived at load time
    std::string directory;                  // Filesystem path to mod root

    /// Load from a JSON object
    static std::optional<ModManifest> fromJson(const nlohmann::json& json,
                                               const std::string& modDir);

    /// Load from a file path
    static std::optional<ModManifest> fromFile(const std::string& path);

    /// Validate the manifest, returns list of errors (empty = valid)
    std::vector<std::string> validate() const;
};

} // namespace gloaming
