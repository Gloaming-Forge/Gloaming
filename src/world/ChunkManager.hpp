#pragma once

#include "world/Chunk.hpp"
#include "world/ChunkGenerator.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace gloaming {

/// Configuration for the chunk manager
struct ChunkManagerConfig {
    int loadRadiusChunks = 3;       // Chunks to keep loaded around center
    int unloadRadiusChunks = 5;     // Distance at which to unload chunks
    int maxLoadedChunks = 100;      // Maximum chunks to keep in memory
    bool autoSaveOnUnload = true;   // Save modified chunks when unloading
};

/// Statistics about chunk manager state
struct ChunkManagerStats {
    size_t loadedChunks = 0;
    size_t dirtyChunks = 0;
    size_t chunksGenerated = 0;
    size_t chunksLoaded = 0;
    size_t chunksSaved = 0;
    size_t chunksUnloaded = 0;
};

/// Callback types for chunk lifecycle events
using ChunkLoadedCallback = std::function<void(Chunk& chunk)>;
using ChunkUnloadingCallback = std::function<void(Chunk& chunk)>;
using ChunkSaveCallback = std::function<bool(const Chunk& chunk, const std::string& worldPath)>;
using ChunkLoadCallback = std::function<bool(Chunk& chunk, const std::string& worldPath)>;

/// Manages loading, unloading, and caching of world chunks
/// Provides the main interface for tile access in an infinite world
class ChunkManager {
public:
    ChunkManager() = default;
    explicit ChunkManager(const ChunkManagerConfig& config);

    /// Initialize the chunk manager
    void init(uint64_t worldSeed = 12345);

    /// Set the world save path (for loading/saving chunks)
    void setWorldPath(const std::string& path) { m_worldPath = path; }
    const std::string& getWorldPath() const { return m_worldPath; }

    /// Set the chunk generator
    void setGenerator(ChunkGenerator generator) { m_generator = std::move(generator); }
    ChunkGenerator& getGenerator() { return m_generator; }

    /// Update chunk loading/unloading based on center position (usually camera)
    /// @param centerWorldX World X coordinate to center chunk loading around
    /// @param centerWorldY World Y coordinate to center chunk loading around
    void update(float centerWorldX, float centerWorldY);

    /// Force update around a specific chunk position
    void updateAroundChunk(ChunkCoord chunkX, ChunkCoord chunkY);

    // ========================================================================
    // Tile Access
    // ========================================================================

    /// Get tile at world coordinates
    /// @return Tile at position, or empty tile if chunk not loaded
    Tile getTile(int worldX, int worldY) const;

    /// Set tile at world coordinates
    /// @return true if successful (chunk was loaded or created)
    bool setTile(int worldX, int worldY, const Tile& tile);

    /// Set tile with just ID at world coordinates
    bool setTileId(int worldX, int worldY, uint16_t id, uint8_t variant = 0, uint8_t flags = 0);

    /// Check if tile at world coordinates is solid
    bool isSolid(int worldX, int worldY) const;

    /// Check if a chunk at the given world coordinates is loaded
    bool isChunkLoaded(int worldX, int worldY) const;

    /// Check if a chunk at chunk coordinates is loaded
    bool isChunkLoadedAt(ChunkCoord chunkX, ChunkCoord chunkY) const;

    // ========================================================================
    // Chunk Access
    // ========================================================================

    /// Get chunk at world coordinates (may load/generate)
    /// @param load If true, will load/generate chunk if not present
    /// @return Pointer to chunk, or nullptr if not loaded and load=false
    Chunk* getChunkAt(int worldX, int worldY, bool load = true);
    const Chunk* getChunkAt(int worldX, int worldY, bool load = false) const;

    /// Get chunk at chunk coordinates
    Chunk* getChunk(ChunkCoord chunkX, ChunkCoord chunkY, bool load = true);
    const Chunk* getChunk(ChunkCoord chunkX, ChunkCoord chunkY, bool load = false) const;

    /// Get chunk by position
    Chunk* getChunk(const ChunkPosition& pos, bool load = true);
    const Chunk* getChunk(const ChunkPosition& pos, bool load = false) const;

    /// Force load/generate a chunk at the given chunk coordinates
    Chunk& loadChunk(ChunkCoord chunkX, ChunkCoord chunkY);

    /// Unload a chunk at the given chunk coordinates
    /// @param save If true, saves the chunk if it has modifications
    /// @return true if chunk was unloaded
    bool unloadChunk(ChunkCoord chunkX, ChunkCoord chunkY, bool save = true);

    /// Unload all chunks
    void unloadAllChunks(bool save = true);

    // ========================================================================
    // Saving/Loading
    // ========================================================================

    /// Save a specific chunk
    bool saveChunk(ChunkCoord chunkX, ChunkCoord chunkY);

    /// Save all modified chunks
    void saveAllDirtyChunks();

    /// Set custom save/load callbacks (for world file integration)
    void setSaveCallback(ChunkSaveCallback callback) { m_saveCallback = std::move(callback); }
    void setLoadCallback(ChunkLoadCallback callback) { m_loadCallback = std::move(callback); }

    // ========================================================================
    // Events and Callbacks
    // ========================================================================

    /// Set callback called when a chunk is loaded or generated
    void setOnChunkLoaded(ChunkLoadedCallback callback) {
        m_onChunkLoaded = std::move(callback);
    }

    /// Set callback called just before a chunk is unloaded
    void setOnChunkUnloading(ChunkUnloadingCallback callback) {
        m_onChunkUnloading = std::move(callback);
    }

    // ========================================================================
    // Queries and Utilities
    // ========================================================================

    /// Get all loaded chunks
    std::vector<Chunk*> getLoadedChunks();
    std::vector<const Chunk*> getLoadedChunks() const;

    /// Get all dirty (modified) chunks
    std::vector<Chunk*> getDirtyChunks();

    /// Get chunks within a world coordinate range
    std::vector<Chunk*> getChunksInRange(int minWorldX, int maxWorldX,
                                          int minWorldY, int maxWorldY);

    /// Get the number of loaded chunks
    size_t getLoadedChunkCount() const { return m_chunks.size(); }

    /// Get statistics
    const ChunkManagerStats& getStats() const { return m_stats; }

    /// Reset statistics
    void resetStats() { m_stats = ChunkManagerStats{}; }

    /// Get/set configuration
    const ChunkManagerConfig& getConfig() const { return m_config; }
    void setConfig(const ChunkManagerConfig& config) { m_config = config; }

    /// Convert world coordinates to chunk coordinates
    static ChunkPosition worldToChunkPosition(int worldX, int worldY) {
        return ChunkPosition(worldToChunkCoord(worldX), worldToChunkCoord(worldY));
    }

    /// Mark a chunk as dirty
    void markChunkDirty(ChunkCoord chunkX, ChunkCoord chunkY,
                        ChunkDirtyFlags flags = ChunkDirtyFlags::All);

private:
    /// Get or create a chunk
    Chunk& getOrCreateChunk(const ChunkPosition& pos);

    /// Try to load a chunk from storage
    bool tryLoadFromStorage(Chunk& chunk);

    /// Generate a new chunk
    void generateChunk(Chunk& chunk);

    ChunkManagerConfig m_config;
    std::string m_worldPath;
    ChunkGenerator m_generator;

    // Chunk storage: position -> chunk
    std::unordered_map<ChunkPosition, std::unique_ptr<Chunk>, ChunkPositionHash> m_chunks;

    // Current center position for chunk loading
    ChunkCoord m_centerChunkX = 0;
    ChunkCoord m_centerChunkY = 0;

    // Callbacks
    ChunkLoadedCallback m_onChunkLoaded;
    ChunkUnloadingCallback m_onChunkUnloading;
    ChunkSaveCallback m_saveCallback;
    ChunkLoadCallback m_loadCallback;

    // Statistics
    mutable ChunkManagerStats m_stats;
};

} // namespace gloaming
