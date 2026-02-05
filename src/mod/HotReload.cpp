#include "mod/HotReload.hpp"
#include "engine/Log.hpp"

namespace fs = std::filesystem;

namespace gloaming {

void HotReload::watchMod(const std::string& modId, const std::string& directory) {
    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        LOG_WARN("HotReload: directory '{}' does not exist for mod '{}'", directory, modId);
        return;
    }

    WatchedMod watch;
    watch.modId = modId;
    watch.directory = directory;
    watch.fileTimestamps = scanDirectory(directory);

    m_watchedMods[modId] = std::move(watch);
    LOG_DEBUG("HotReload: watching mod '{}' ({} files)", modId,
             m_watchedMods[modId].fileTimestamps.size());
}

void HotReload::unwatchMod(const std::string& modId) {
    m_watchedMods.erase(modId);
}

void HotReload::unwatchAll() {
    m_watchedMods.clear();
}

bool HotReload::poll() {
    auto now = Clock::now();
    auto elapsed = std::chrono::duration<float>(now - m_lastPollTime).count();
    if (elapsed < m_pollIntervalSeconds) {
        return false;
    }
    m_lastPollTime = now;

    bool anyChanges = false;

    for (auto& [modId, watch] : m_watchedMods) {
        auto currentState = scanDirectory(watch.directory);
        auto changed = detectChanges(watch.fileTimestamps, currentState);

        if (!changed.empty()) {
            LOG_INFO("HotReload: {} file(s) changed in mod '{}'", changed.size(), modId);
            for (const auto& file : changed) {
                LOG_DEBUG("HotReload:   changed: {}", file);
            }

            if (m_callback) {
                m_callback(modId, changed);
            }

            // Update stored timestamps
            watch.fileTimestamps = std::move(currentState);
            anyChanges = true;
        }
    }

    return anyChanges;
}

std::unordered_map<std::string, HotReload::FileTime>
HotReload::scanDirectory(const std::string& dir) {
    std::unordered_map<std::string, FileTime> timestamps;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;

            // Only watch relevant file types
            std::string ext = entry.path().extension().string();
            if (ext == ".lua" || ext == ".json" || ext == ".png" || ext == ".ogg"
                || ext == ".wav" || ext == ".frag" || ext == ".vert") {
                timestamps[entry.path().string()] = entry.last_write_time();
            }
        }
    } catch (const fs::filesystem_error& e) {
        LOG_WARN("HotReload: error scanning directory '{}': {}", dir, e.what());
    }

    return timestamps;
}

std::vector<std::string> HotReload::detectChanges(
        const std::unordered_map<std::string, FileTime>& oldState,
        const std::unordered_map<std::string, FileTime>& newState) {
    std::vector<std::string> changed;

    // Check for modified or new files
    for (const auto& [path, newTime] : newState) {
        auto it = oldState.find(path);
        if (it == oldState.end()) {
            // New file
            changed.push_back(path);
        } else if (it->second != newTime) {
            // Modified file
            changed.push_back(path);
        }
    }

    // Check for deleted files
    for (const auto& [path, _] : oldState) {
        if (newState.count(path) == 0) {
            changed.push_back(path);
        }
    }

    return changed;
}

} // namespace gloaming
