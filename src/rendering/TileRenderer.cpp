#include "rendering/TileRenderer.hpp"
#include "engine/Log.hpp"
#include <algorithm>
#include <cmath>

namespace gloaming {

TileRenderer::TileRenderer(IRenderer* renderer, const TileRenderConfig& config)
    : m_renderer(renderer), m_config(config) {
    // Reserve space for common tile count
    m_tileDefinitions.reserve(256);
}

void TileRenderer::registerTile(const TileDefinition& def) {
    // Expand vector if needed
    if (def.id >= m_tileDefinitions.size()) {
        m_tileDefinitions.resize(def.id + 1);
    }
    m_tileDefinitions[def.id] = def;
    LOG_DEBUG("TileRenderer: Registered tile '{}' with ID {}", def.name, def.id);
}

const TileDefinition* TileRenderer::getTileDefinition(uint16_t id) const {
    if (id < m_tileDefinitions.size() && m_tileDefinitions[id].id == id) {
        return &m_tileDefinitions[id];
    }
    return nullptr;
}

void TileRenderer::render(const Tile* tiles, int width, int height,
                          float worldOffsetX, float worldOffsetY) {
    if (!m_renderer || !m_tileset || !tiles) return;

    m_tilesRendered = 0;
    m_tilesCulled = 0;

    // Calculate visible range
    int visMinX = 0, visMaxX = width, visMinY = 0, visMaxY = height;

    if (m_camera) {
        Rect visible = m_camera->getVisibleArea();

        // Convert to tile coordinates
        int tileSize = m_config.tileSize;
        int padding = m_config.viewPaddingTiles;

        visMinX = static_cast<int>(std::floor((visible.x - worldOffsetX) / tileSize)) - padding;
        visMinY = static_cast<int>(std::floor((visible.y - worldOffsetY) / tileSize)) - padding;
        visMaxX = static_cast<int>(std::ceil((visible.x + visible.width - worldOffsetX) / tileSize)) + padding;
        visMaxY = static_cast<int>(std::ceil((visible.y + visible.height - worldOffsetY) / tileSize)) + padding;

        // Clamp to array bounds
        visMinX = std::max(0, visMinX);
        visMinY = std::max(0, visMinY);
        visMaxX = std::min(width, visMaxX);
        visMaxY = std::min(height, visMaxY);
    }

    // Render visible tiles
    for (int y = visMinY; y < visMaxY; ++y) {
        for (int x = visMinX; x < visMaxX; ++x) {
            const Tile& tile = tiles[y * width + x];
            if (!tile.isEmpty()) {
                float worldX = worldOffsetX + x * m_config.tileSize;
                float worldY = worldOffsetY + y * m_config.tileSize;
                renderTile(tile, worldX, worldY);
                ++m_tilesRendered;
            }
        }
    }

    // Calculate culled count
    int totalArea = width * height;
    int visibleArea = (visMaxX - visMinX) * (visMaxY - visMinY);
    m_tilesCulled = static_cast<size_t>(totalArea - visibleArea);
}

void TileRenderer::getVisibleTileRange(int& minX, int& maxX, int& minY, int& maxY) const {
    if (!m_camera) {
        // No camera, can't determine visible range
        return;
    }

    Rect visible = m_camera->getVisibleArea();
    int tileSize = m_config.tileSize;
    int padding = m_config.viewPaddingTiles;

    minX = static_cast<int>(std::floor(visible.x / tileSize)) - padding;
    minY = static_cast<int>(std::floor(visible.y / tileSize)) - padding;
    maxX = static_cast<int>(std::ceil((visible.x + visible.width) / tileSize)) + padding;
    maxY = static_cast<int>(std::ceil((visible.y + visible.height) / tileSize)) + padding;
}

void TileRenderer::renderTile(const Tile& tile, float worldX, float worldY) {
    const TileDefinition* def = getTileDefinition(tile.id);
    if (!def) {
        return;  // Unknown tile type
    }

    // Calculate source rectangle (handle variants)
    Rect source = def->textureRegion;
    if (def->variantCount > 1 && tile.variant < def->variantCount) {
        // Variants are assumed to be laid out horizontally
        source.x += tile.variant * source.width;
    }

    // Calculate destination
    float tileSize = static_cast<float>(m_config.tileSize);
    Rect dest;

    if (m_camera) {
        Vec2 screenPos = m_camera->worldToScreen({worldX + tileSize * 0.5f,
                                                   worldY + tileSize * 0.5f});
        float zoom = m_camera->getZoom();
        float size = tileSize * zoom;
        dest = Rect(screenPos.x - size * 0.5f, screenPos.y - size * 0.5f, size, size);
    } else {
        dest = Rect(worldX, worldY, tileSize, tileSize);
    }

    m_renderer->drawTextureRegion(m_tileset, source, dest);
}

} // namespace gloaming
