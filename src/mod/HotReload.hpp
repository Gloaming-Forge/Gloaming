#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>
#include <chrono>

namespace gloaming {

/// Tracks file modifications for hot-reloading mod content in debug builds.
/// Uses polling (portable across platforms, no inotify/FSEvents dependency).
class HotReload {
public:
    using ReloadCallback = std::function<void(const std::string& modId,
                                             const std::vector<std::string>& changedFiles)>;

    HotReload() = default;

    /// Start watching a mod directory.
    void watchMod(const std::string& modId, const std::string& directory);

    /// Stop watching a mod.
    void unwatchMod(const std::string& modId);

    /// Stop watching all mods.
    void unwatchAll();

    /// Set the callback for when changes are detected.
    void setCallback(ReloadCallback callback) { m_callback = std::move(callback); }

    /// Poll for changes. Call this periodically (e.g. once per second).
    /// Returns true if any changes were detected and the callback was invoked.
    bool poll();

    /// Set polling interval (minimum time between checks).
    void setPollInterval(float seconds) { m_pollIntervalSeconds = seconds; }

    /// Check if any mods are being watched.
    bool isWatching() const { return !m_watchedMods.empty(); }

    /// Get number of watched mods.
    size_t watchedModCount() const { return m_watchedMods.size(); }

private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using FileTime = std::filesystem::file_time_type;

    struct WatchedMod {
        std::string modId;
        std::string directory;
        std::unordered_map<std::string, FileTime> fileTimestamps;
    };

    /// Scan a directory and record all file timestamps.
    static std::unordered_map<std::string, FileTime> scanDirectory(const std::string& dir);

    /// Compare current state with stored state. Returns list of changed/new files.
    static std::vector<std::string> detectChanges(
        const std::unordered_map<std::string, FileTime>& oldState,
        const std::unordered_map<std::string, FileTime>& newState);

    std::unordered_map<std::string, WatchedMod> m_watchedMods;
    ReloadCallback m_callback;
    float m_pollIntervalSeconds = 1.0f;
    TimePoint m_lastPollTime = Clock::now();
};

} // namespace gloaming
