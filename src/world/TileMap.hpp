#pragma once

#include "world/ChunkManager.hpp"
#include "world/WorldFile.hpp"
#include "rendering/Camera.hpp"
#include <string>
#include <functional>

namespace gloaming {

// Forward declaration
class TileRenderer;

/// Configuration for the TileMap
struct TileMapConfig {
    ChunkManagerConfig chunkManager;
    int tileSize = 16;              // Size of each tile in pixels
    bool autoSave = true;           // Auto-save when world is closed
};

/// Main interface for the infinite tile world
/// Combines ChunkManager, WorldFile, and rendering support
class TileMap {
public:
    TileMap() = default;
    explicit TileMap(const TileMapConfig& config);

    /// Create a new world
    /// @param worldPath Path to the world directory
    /// @param worldName Display name for the world
    /// @param seed World generation seed
    /// @return true if world was created successfully
    [[nodiscard]] bool createWorld(const std::string& worldPath, const std::string& worldName, uint64_t seed);

    /// Load an existing world
    /// @param worldPath Path to the world directory
    /// @return true if world was loaded successfully
    [[nodiscard]] bool loadWorld(const std::string& worldPath);

    /// Save the current world
    /// @return true if save was successful
    [[nodiscard]] bool saveWorld();

    /// Close the current world (saves if autoSave is enabled)
    void closeWorld();

    /// Check if a world is currently loaded
    bool isWorldLoaded() const { return m_worldLoaded; }

    // ========================================================================
    // World Update
    // ========================================================================

    /// Update chunk loading based on camera position
    /// Should be called each frame
    void update(const Camera& camera);

    /// Update chunk loading around a specific world position
    void update(float worldX, float worldY);

    // ========================================================================
    // Tile Access
    // ========================================================================

    /// Get tile at world coordinates
    Tile getTile(int worldX, int worldY) const;

    /// Set tile at world coordinates
    bool setTile(int worldX, int worldY, const Tile& tile);

    /// Set tile with ID at world coordinates
    bool setTileId(int worldX, int worldY, uint16_t id, uint8_t variant = 0, uint8_t flags = 0);

    /// Check if tile at world coordinates is solid
    bool isSolid(int worldX, int worldY) const;

    /// Check if a position is empty (air)
    bool isEmpty(int worldX, int worldY) const;

    /// Convert world pixel position to tile coordinates
    void worldToTile(float worldX, float worldY, int& tileX, int& tileY) const;

    /// Convert tile coordinates to world pixel position (top-left of tile)
    void tileToWorld(int tileX, int tileY, float& worldX, float& worldY) const;

    // ========================================================================
    // Rendering Support
    // ========================================================================

    /// Render tiles using the TileRenderer
    /// Uses camera-based culling automatically
    void render(TileRenderer& renderer, const Camera& camera) const;

    /// Get a tile callback suitable for TileRenderer::render()
    /// Usage: tileRenderer.render(tileMap.getTileCallback(), minX, maxX, minY, maxY);
    std::function<Tile(int, int)> getTileCallback() const;

    /// Get visible tile range based on camera
    void getVisibleTileRange(const Camera& camera, int& minX, int& maxX, int& minY, int& maxY) const;

    // ========================================================================
    // World Properties
    // ========================================================================

    /// Get the world metadata
    const WorldMetadata& getMetadata() const { return m_worldFile.getMetadata(); }

    /// Get the world seed
    uint64_t getSeed() const;

    /// Get/set spawn point
    Vec2 getSpawnPoint() const;
    void setSpawnPoint(float x, float y);

    /// Update play time (called from engine)
    void addPlayTime(uint64_t seconds);

    /// Track tile placed (for stats)
    void trackTilePlaced() { ++m_metadata.tilesPlaced; }

    /// Track tile mined (for stats)
    void trackTileMined() { ++m_metadata.tilesMined; }

    // ========================================================================
    // Chunk Access
    // ========================================================================

    /// Get the chunk manager for advanced operations
    ChunkManager& getChunkManager() { return m_chunkManager; }
    const ChunkManager& getChunkManager() const { return m_chunkManager; }

    /// Get the world file for advanced operations
    WorldFile& getWorldFile() { return m_worldFile; }
    const WorldFile& getWorldFile() const { return m_worldFile; }

    /// Set a custom chunk generator callback
    void setGeneratorCallback(ChunkGeneratorCallback callback);

    // ========================================================================
    // Configuration
    // ========================================================================

    /// Get/set configuration
    const TileMapConfig& getConfig() const { return m_config; }
    void setConfig(const TileMapConfig& config);

    /// Get tile size in pixels
    int getTileSize() const { return m_config.tileSize; }

    // ========================================================================
    // Statistics
    // ========================================================================

    /// Get chunk manager statistics
    const ChunkManagerStats& getStats() const { return m_chunkManager.getStats(); }

    /// Get number of loaded chunks
    size_t getLoadedChunkCount() const { return m_chunkManager.getLoadedChunkCount(); }

private:
    /// Set up chunk manager save/load callbacks
    void setupChunkCallbacks();

    TileMapConfig m_config;
    ChunkManager m_chunkManager;
    WorldFile m_worldFile;
    WorldMetadata m_metadata;

    bool m_worldLoaded = false;
};

} // namespace gloaming
