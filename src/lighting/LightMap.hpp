#pragma once

#include "rendering/IRenderer.hpp"
#include "world/Chunk.hpp"
#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <queue>
#include <algorithm>
#include <functional>

namespace gloaming {

/// Per-tile light value with RGB channels
struct TileLight {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    constexpr TileLight() = default;
    constexpr TileLight(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}

    /// Max channel value for propagation comparisons
    uint8_t maxChannel() const { return std::max({r, g, b}); }

    /// Check if effectively dark
    bool isDark() const { return r == 0 && g == 0 && b == 0; }

    /// Component-wise max
    static TileLight max(const TileLight& a, const TileLight& b) {
        return {
            std::max(a.r, b.r),
            std::max(a.g, b.g),
            std::max(a.b, b.b)
        };
    }

    bool operator==(const TileLight& other) const {
        return r == other.r && g == other.g && b == other.b;
    }
    bool operator!=(const TileLight& other) const { return !(*this == other); }
};

/// Light data for a single chunk (64x64 tiles)
struct ChunkLightData {
    std::array<TileLight, CHUNK_TILE_COUNT> lights{};

    TileLight getLight(int localX, int localY) const {
        if (localX < 0 || localX >= CHUNK_SIZE || localY < 0 || localY >= CHUNK_SIZE)
            return {};
        return lights[localY * CHUNK_SIZE + localX];
    }

    void setLight(int localX, int localY, const TileLight& light) {
        if (localX >= 0 && localX < CHUNK_SIZE && localY >= 0 && localY < CHUNK_SIZE)
            lights[localY * CHUNK_SIZE + localX] = light;
    }

    void clear() { lights.fill(TileLight{}); }
};

/// Configuration for the lighting system
struct LightingConfig {
    int lightFalloff = 16;              // Light drops by this per tile
    int skylightFalloff = 10;           // Skylight drops by this per tile underground
    int maxLightRadius = 16;            // Max tiles a light can reach
    uint8_t maxLightLevel = 255;        // Maximum light channel value
    bool enableSkylight = true;         // Enable skylight from surface
    bool enableSmoothLighting = true;   // Enable corner interpolation
};

/// Represents a light source at a specific tile position
struct TileLightSource {
    int worldX = 0;
    int worldY = 0;
    TileLight color;

    TileLightSource() = default;
    TileLightSource(int x, int y, const TileLight& c) : worldX(x), worldY(y), color(c) {}
};

/// BFS propagation node
struct LightNode {
    int worldX;
    int worldY;
    TileLight light;
};

/// Manages per-tile light values across loaded chunks.
/// Uses BFS flood-fill for light propagation.
class LightMap {
public:
    LightMap() = default;
    explicit LightMap(const LightingConfig& config) : m_config(config) {}

    /// Set configuration
    void setConfig(const LightingConfig& config) { m_config = config; }
    const LightingConfig& getConfig() const { return m_config; }

    /// Create light data for a chunk
    void addChunk(const ChunkPosition& pos);

    /// Remove light data for a chunk
    void removeChunk(const ChunkPosition& pos);

    /// Check if a chunk has light data
    bool hasChunk(const ChunkPosition& pos) const;

    /// Get light at world tile coordinates
    TileLight getLight(int worldX, int worldY) const;

    /// Set light at world tile coordinates (directly, without propagation)
    void setLight(int worldX, int worldY, const TileLight& light);

    /// Get the interpolated light at a corner position (for smooth rendering).
    /// Returns the average of the 4 tiles sharing that corner.
    /// @param tileX, tileY identify the corner as the top-left corner of tile (tileX, tileY)
    TileLight getCornerLight(int tileX, int tileY) const;

    /// Clear all light in all chunks
    void clearAll();

    /// Clear light in a specific chunk
    void clearChunk(const ChunkPosition& pos);

    // ========================================================================
    // Light Propagation
    // ========================================================================

    /// Propagate light from a single source using BFS flood-fill.
    /// @param source The light source to propagate from
    /// @param isSolid Callback: returns true if a tile blocks light
    void propagateLight(const TileLightSource& source,
                        const std::function<bool(int, int)>& isSolid);

    /// Propagate skylight downward from the surface.
    /// @param minWorldX, maxWorldX Horizontal range to compute skylight
    /// @param getSurfaceY Callback: returns the Y coordinate of the first solid tile in column x
    /// @param isSolid Callback: returns true if a tile blocks light
    /// @param skyColor The ambient sky light color
    void propagateSkylight(int minWorldX, int maxWorldX,
                           const std::function<int(int)>& getSurfaceY,
                           const std::function<bool(int, int)>& isSolid,
                           const TileLight& skyColor);

    /// Recalculate all lighting for loaded chunks.
    /// @param lightSources All active point light sources
    /// @param isSolid Callback: returns true if a tile blocks light
    /// @param getSurfaceY Callback: returns Y of first solid tile in column x
    /// @param skyColor Current ambient sky color
    void recalculateAll(const std::vector<TileLightSource>& lightSources,
                        const std::function<bool(int, int)>& isSolid,
                        const std::function<int(int)>& getSurfaceY,
                        const TileLight& skyColor);

    // ========================================================================
    // Queries
    // ========================================================================

    /// Get number of chunks with light data
    size_t getChunkCount() const { return m_chunks.size(); }

    /// Get light data for a chunk (const)
    const ChunkLightData* getChunkData(const ChunkPosition& pos) const;

    /// Get light data for a chunk (mutable)
    ChunkLightData* getChunkData(const ChunkPosition& pos);

    /// Get the world tile range covered by loaded light chunks
    void getWorldRange(int& minX, int& maxX, int& minY, int& maxY) const;

private:
    LightingConfig m_config;
    std::unordered_map<ChunkPosition, ChunkLightData, ChunkPositionHash> m_chunks;
};

} // namespace gloaming
