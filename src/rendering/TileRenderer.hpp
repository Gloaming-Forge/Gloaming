#pragma once

#include "rendering/IRenderer.hpp"
#include "rendering/Texture.hpp"
#include "rendering/Camera.hpp"
#include <vector>
#include <cstdint>

namespace gloaming {

/// Tile data structure - represents a single tile in the world
struct Tile {
    uint16_t id = 0;           // Tile type ID (0 = air/empty)
    uint8_t variant = 0;       // Visual variant (for tile variety)
    uint8_t flags = 0;         // Tile flags (solid, etc.)

    bool isEmpty() const { return id == 0; }
    bool isSolid() const { return (flags & FLAG_SOLID) != 0; }

    static constexpr uint8_t FLAG_SOLID = 1 << 0;
    static constexpr uint8_t FLAG_PLATFORM = 1 << 1;
    static constexpr uint8_t FLAG_TRANSPARENT = 1 << 2;
};

/// Tile definition - defines visual properties of a tile type
struct TileDefinition {
    uint16_t id = 0;
    std::string name;
    Rect textureRegion;        // Region in the tileset texture
    int variantCount = 1;      // Number of visual variants
    bool solid = true;
    bool transparent = false;

    TileDefinition() = default;
    TileDefinition(uint16_t id, const std::string& name, const Rect& region,
                   bool solid = true, bool transparent = false)
        : id(id), name(name), textureRegion(region), solid(solid),
          transparent(transparent) {}
};

/// Configuration for tile rendering
struct TileRenderConfig {
    int tileSize = 16;              // Size of tiles in pixels
    int viewPaddingTiles = 2;       // Extra tiles to render outside view for smoother scrolling
};

/// Renders a grid of tiles efficiently with culling
class TileRenderer {
public:
    TileRenderer() = default;
    TileRenderer(IRenderer* renderer, const TileRenderConfig& config = {});

    /// Set the renderer
    void setRenderer(IRenderer* renderer) { m_renderer = renderer; }

    /// Set the camera for view culling
    void setCamera(const Camera* camera) { m_camera = camera; }

    /// Set the tileset texture
    void setTileset(const Texture* tileset) { m_tileset = tileset; }

    /// Set the tile size
    void setTileSize(int size) { m_config.tileSize = size; }

    /// Get the tile size
    int getTileSize() const { return m_config.tileSize; }

    /// Register a tile definition
    void registerTile(const TileDefinition& def);

    /// Get a tile definition by ID
    const TileDefinition* getTileDefinition(uint16_t id) const;

    /// Render a region of tiles
    /// @param tiles 2D array of tiles (row-major: tiles[y * width + x])
    /// @param width Width of the tile array
    /// @param height Height of the tile array
    /// @param worldOffsetX World X offset for this tile array
    /// @param worldOffsetY World Y offset for this tile array
    void render(const Tile* tiles, int width, int height,
                float worldOffsetX = 0.0f, float worldOffsetY = 0.0f);

    /// Render tiles with a simple callback for dynamic tile data
    /// @param getTile Function to get tile at (x, y) world coordinates
    /// @param minX Minimum X coordinate to check
    /// @param maxX Maximum X coordinate to check (exclusive)
    /// @param minY Minimum Y coordinate to check
    /// @param maxY Maximum Y coordinate to check (exclusive)
    template<typename GetTileFunc>
    void render(GetTileFunc getTile, int minX, int maxX, int minY, int maxY);

    /// Get visible tile range based on camera
    void getVisibleTileRange(int& minX, int& maxX, int& minY, int& maxY) const;

    /// Get statistics from last render
    size_t getTilesRendered() const { return m_tilesRendered; }
    size_t getTilesCulled() const { return m_tilesCulled; }

private:
    void renderTile(const Tile& tile, float worldX, float worldY);

    IRenderer* m_renderer = nullptr;
    const Camera* m_camera = nullptr;
    const Texture* m_tileset = nullptr;
    TileRenderConfig m_config;

    std::vector<TileDefinition> m_tileDefinitions;
    size_t m_tilesRendered = 0;
    size_t m_tilesCulled = 0;
};

// Template implementation
template<typename GetTileFunc>
void TileRenderer::render(GetTileFunc getTile, int minX, int maxX, int minY, int maxY) {
    if (!m_renderer || !m_tileset) return;

    m_tilesRendered = 0;
    m_tilesCulled = 0;

    // Get visible range based on camera if available
    int visMinX = minX, visMaxX = maxX, visMinY = minY, visMaxY = maxY;
    if (m_camera) {
        getVisibleTileRange(visMinX, visMaxX, visMinY, visMaxY);
        // Clamp to provided bounds
        visMinX = std::max(visMinX, minX);
        visMaxX = std::min(visMaxX, maxX);
        visMinY = std::max(visMinY, minY);
        visMaxY = std::min(visMaxY, maxY);
    }

    // Render visible tiles
    for (int y = visMinY; y < visMaxY; ++y) {
        for (int x = visMinX; x < visMaxX; ++x) {
            Tile tile = getTile(x, y);
            if (!tile.isEmpty()) {
                float worldX = static_cast<float>(x * m_config.tileSize);
                float worldY = static_cast<float>(y * m_config.tileSize);
                renderTile(tile, worldX, worldY);
                ++m_tilesRendered;
            }
        }
    }

    // Calculate culled tiles (total area minus visible area)
    int totalArea = (maxX - minX) * (maxY - minY);
    int visibleArea = (visMaxX - visMinX) * (visMaxY - visMinY);
    m_tilesCulled = static_cast<size_t>(totalArea - visibleArea);
}

} // namespace gloaming
