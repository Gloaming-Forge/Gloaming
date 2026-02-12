#include "engine/ResourceManager.hpp"

#include <algorithm>

namespace gloaming {

void ResourceManager::track(const std::string& path, const std::string& type,
                             size_t sizeBytes, bool persistent) {
    auto it = m_entries.find(path);
    if (it != m_entries.end()) {
        // Update existing entry — subtract old size, add new
        m_totalBytes -= it->second.sizeBytes;
        it->second.type = type;
        it->second.sizeBytes = sizeBytes;
        it->second.persistent = persistent;
        m_totalBytes += sizeBytes;
    } else {
        m_entries[path] = ResourceEntry{path, type, sizeBytes, persistent};
        m_totalBytes += sizeBytes;
    }
}

void ResourceManager::untrack(const std::string& path) {
    auto it = m_entries.find(path);
    if (it != m_entries.end()) {
        m_totalBytes -= it->second.sizeBytes;
        m_entries.erase(it);
    }
}

bool ResourceManager::isTracked(const std::string& path) const {
    return m_entries.count(path) > 0;
}

const ResourceEntry* ResourceManager::getEntry(const std::string& path) const {
    auto it = m_entries.find(path);
    if (it != m_entries.end()) {
        return &it->second;
    }
    return nullptr;
}

ResourceStats ResourceManager::getStats() const {
    ResourceStats stats;
    stats.totalCount = m_entries.size();
    stats.totalBytes = m_totalBytes;

    for (const auto& [path, entry] : m_entries) {
        if (entry.type == "texture") stats.textureCount++;
        else if (entry.type == "sound") stats.soundCount++;
        else if (entry.type == "music") stats.musicCount++;
        else if (entry.type == "script") stats.scriptCount++;
        else if (entry.type == "data") stats.dataCount++;
    }

    return stats;
}

std::vector<const ResourceEntry*> ResourceManager::getEntriesByType(
    const std::string& type) const {
    std::vector<const ResourceEntry*> result;
    for (const auto& [path, entry] : m_entries) {
        if (entry.type == type) {
            result.push_back(&entry);
        }
    }
    return result;
}

size_t ResourceManager::clearTransient() {
    size_t removed = 0;
    auto it = m_entries.begin();
    while (it != m_entries.end()) {
        if (!it->second.persistent) {
            m_totalBytes -= it->second.sizeBytes;
            it = m_entries.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }
    return removed;
}

void ResourceManager::clear() {
    m_entries.clear();
    m_totalBytes = 0;
}

std::vector<std::string> ResourceManager::findLeaks(
    const std::vector<std::string>& aliveResources) const {

    // Build a set of alive paths for fast lookup
    std::unordered_map<std::string, bool> alive;
    for (const auto& path : aliveResources) {
        alive[path] = true;
    }

    std::vector<std::string> leaks;
    for (const auto& [path, entry] : m_entries) {
        if (alive.find(path) == alive.end()) {
            leaks.push_back(path);
        }
    }

    std::sort(leaks.begin(), leaks.end());
    return leaks;
}

} // namespace gloaming
