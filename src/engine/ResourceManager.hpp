#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstddef>

namespace gloaming {

/// Tracks a single loaded resource.
struct ResourceEntry {
    std::string path;
    std::string type;        // "texture", "sound", "music", "script", "data"
    size_t sizeBytes = 0;
    bool persistent  = false; // Survives hot-reload / scene transitions
};

/// Aggregate resource statistics.
struct ResourceStats {
    size_t textureCount  = 0;
    size_t soundCount    = 0;
    size_t musicCount    = 0;
    size_t scriptCount   = 0;
    size_t dataCount     = 0;
    size_t totalCount    = 0;
    size_t totalBytes    = 0;
};

/// Centralized resource tracker. Keeps a registry of all loaded assets
/// (textures, sounds, music, scripts, data files) so the engine can report
/// usage stats, detect leaks, and provide modders with diagnostic info.
///
/// This does NOT own the actual resource data — ownership stays with the
/// respective subsystem (TextureManager, AudioSystem, etc.). The
/// ResourceManager is a bookkeeping overlay.
class ResourceManager {
public:
    /// Register a loaded resource. If a resource with the same path already
    /// exists, the entry is updated (not duplicated).
    void track(const std::string& path, const std::string& type,
               size_t sizeBytes = 0, bool persistent = false);

    /// Remove a resource entry (called when unloaded).
    void untrack(const std::string& path);

    /// Check if a resource is tracked.
    bool isTracked(const std::string& path) const;

    /// Get the entry for a resource (returns nullptr if not found).
    const ResourceEntry* getEntry(const std::string& path) const;

    /// Get aggregate statistics.
    ResourceStats getStats() const;

    /// Get all entries of a given type (e.g. "texture").
    std::vector<const ResourceEntry*> getEntriesByType(const std::string& type) const;

    /// Get all tracked resource entries.
    const std::unordered_map<std::string, ResourceEntry>& getAllEntries() const {
        return m_entries;
    }

    /// Total number of tracked resources.
    size_t count() const { return m_entries.size(); }

    /// Total estimated memory usage across all tracked resources.
    size_t totalBytes() const { return m_totalBytes; }

    /// Remove all non-persistent entries (called on scene transition / hot-reload).
    size_t clearTransient();

    /// Remove all entries.
    void clear();

    /// Check for potential leaks: resources tracked but with zero references
    /// in their respective subsystems. Returns list of suspect paths.
    /// The caller must provide a set of "alive" paths from the actual managers.
    std::vector<std::string> findLeaks(
        const std::vector<std::string>& aliveResources) const;

private:
    std::unordered_map<std::string, ResourceEntry> m_entries;
    size_t m_totalBytes = 0;
};

} // namespace gloaming
