#pragma once

#include "rendering/TileRenderer.hpp"
#include "rendering/Camera.hpp"
#include "world/Chunk.hpp"

#include <array>
#include <unordered_map>
#include <functional>
#include <cmath>

namespace gloaming {

/// Named tile layers for multi-layer rendering.
/// Games can use any subset of these layers.
enum class TileLayerIndex : int {
    Background = 0,     // Behind everything (e.g., cave walls, sky tiles)
    Ground     = 1,     // Main walkable ground layer
    Decoration = 2,     // Decorations on top of ground (flowers, grass overlays)
    Foreground = 3,     // Above entities (tree canopy, overhanging cliffs)
    Count      = 4
};

/// A tile layer stores a parallel tile grid for a chunk.
/// Each chunk can have up to 4 layers of tiles.
struct TileLayerData {
    std::array<Tile, CHUNK_TILE_COUNT> tiles{};

    Tile getTile(int localX, int localY) const {
        if (localX < 0 || localX >= CHUNK_SIZE || localY < 0 || localY >= CHUNK_SIZE) {
            return Tile{};
        }
        return tiles[localY * CHUNK_SIZE + localX];
    }

    void setTile(int localX, int localY, const Tile& tile) {
        if (localX >= 0 && localX < CHUNK_SIZE && localY >= 0 && localY < CHUNK_SIZE) {
            tiles[localY * CHUNK_SIZE + localX] = tile;
        }
    }

    bool isEmpty() const {
        for (const auto& t : tiles) {
            if (!t.isEmpty()) return false;
        }
        return true;
    }
};

/// Manages multiple tile layers per chunk for multi-layer rendering.
/// This sits alongside the existing Chunk/TileMap system without modifying it.
/// The existing TileMap remains the "Ground" layer; this class adds optional
/// Background, Decoration, and Foreground layers.
///
/// LIMITATION: Extra tile layers are NOT serialized. They exist only in memory
/// for the current session. If a mod places decoration or foreground tiles, those
/// tiles will be lost when the world is saved and reloaded. Serialization support
/// for extra layers is planned alongside the WorldFile system.
class TileLayerManager {
public:
    using GetTileFunc = std::function<Tile(int worldX, int worldY, int layer)>;

    TileLayerManager() = default;

    /// Set the tile size (must match TileRenderer)
    void setTileSize(int size) { m_tileSize = size; }

    /// Set a tile in a specific layer at world coordinates
    void setTile(int worldX, int worldY, int layer, const Tile& tile) {
        if (layer < 0 || layer >= static_cast<int>(TileLayerIndex::Count)) return;

        ChunkPosition chunkPos{worldToChunkCoord(worldX), worldToChunkCoord(worldY)};
        int localX = worldToLocalCoord(worldX);
        int localY = worldToLocalCoord(worldY);

        auto& chunkLayers = m_layers[chunkPos];
        if (!chunkLayers[layer]) {
            chunkLayers[layer] = std::make_unique<TileLayerData>();
        }
        chunkLayers[layer]->setTile(localX, localY, tile);
    }

    /// Get a tile from a specific layer at world coordinates
    Tile getTile(int worldX, int worldY, int layer) const {
        if (layer < 0 || layer >= static_cast<int>(TileLayerIndex::Count)) return Tile{};

        ChunkPosition chunkPos{worldToChunkCoord(worldX), worldToChunkCoord(worldY)};
        auto it = m_layers.find(chunkPos);
        if (it == m_layers.end()) return Tile{};

        const auto& chunkLayers = it->second;
        if (!chunkLayers[layer]) return Tile{};

        int localX = worldToLocalCoord(worldX);
        int localY = worldToLocalCoord(worldY);
        return chunkLayers[layer]->getTile(localX, localY);
    }

    /// Render a specific layer using the TileRenderer.
    /// Call this at the appropriate point in the render pipeline:
    ///   - Background/Ground layers: before entities
    ///   - Decoration: before entities (same depth)
    ///   - Foreground: after entities
    void renderLayer(TileRenderer& renderer, const Camera& camera, int layer) const {
        if (layer < 0 || layer >= static_cast<int>(TileLayerIndex::Count)) return;

        int minX, maxX, minY, maxY;
        renderer.getVisibleTileRange(minX, maxX, minY, maxY);

        renderer.render(
            [this, layer](int x, int y) -> Tile {
                return getTile(x, y, layer);
            },
            minX, maxX, minY, maxY
        );
    }

    /// Clear all layers for a chunk
    void clearChunk(ChunkPosition pos) {
        m_layers.erase(pos);
    }

    /// Clear a specific layer for a chunk
    void clearChunkLayer(ChunkPosition pos, int layer) {
        auto it = m_layers.find(pos);
        if (it != m_layers.end() && layer >= 0 && layer < static_cast<int>(TileLayerIndex::Count)) {
            it->second[layer].reset();
        }
    }

    /// Check if a specific layer has any data for a chunk
    bool hasLayerData(ChunkPosition pos, int layer) const {
        auto it = m_layers.find(pos);
        if (it == m_layers.end()) return false;
        if (layer < 0 || layer >= static_cast<int>(TileLayerIndex::Count)) return false;
        return it->second[layer] != nullptr;
    }

private:
    using ChunkLayerArray = std::array<std::unique_ptr<TileLayerData>, static_cast<int>(TileLayerIndex::Count)>;
    std::unordered_map<ChunkPosition, ChunkLayerArray, ChunkPositionHash> m_layers;
    int m_tileSize = 16;
};

} // namespace gloaming
