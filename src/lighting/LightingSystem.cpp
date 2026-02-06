#include "lighting/LightingSystem.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"

#include <chrono>

namespace gloaming {

void LightingSystem::init(Registry& registry, Engine& engine) {
    System::init(registry, engine);

    m_tileMap = &engine.getTileMap();
    m_camera = &engine.getCamera();

    LOG_INFO("LightingSystem initialized");
}

void LightingSystem::setConfig(const LightingSystemConfig& config) {
    m_config = config;
    m_lightMap.setConfig(config.lightMap);
    m_dayNightCycle.setConfig(config.dayNight);
    m_needsRecalc = true;
}

void LightingSystem::update(float dt) {
    if (!m_config.enabled || !m_tileMap || !m_tileMap->isWorldLoaded()) return;

    // Update day/night cycle
    m_dayNightCycle.update(dt);
    m_stats.skyBrightness = m_dayNightCycle.getSkyBrightness();

    // Sync light map chunks with world chunks
    syncChunksWithWorld();

    // Periodic recalculation
    m_recalcTimer += dt;
    if (m_recalcTimer >= m_config.recalcInterval || m_needsRecalc) {
        m_recalcTimer = 0.0f;
        m_needsRecalc = false;

        collectLightSources();
        recalculate();
    }
}

void LightingSystem::syncChunksWithWorld() {
    if (!m_tileMap) return;

    auto& chunkMgr = m_tileMap->getChunkManager();
    auto loadedChunks = chunkMgr.getLoadedChunks();

    // Add light data for newly loaded chunks
    for (const Chunk* chunk : loadedChunks) {
        ChunkPosition pos = chunk->getPosition();
        if (!m_lightMap.hasChunk(pos)) {
            m_lightMap.addChunk(pos);
            m_needsRecalc = true;
        }
    }

    // Remove light data for unloaded chunks
    // Build set of loaded positions
    std::unordered_map<ChunkPosition, bool, ChunkPositionHash> loadedSet;
    for (const Chunk* chunk : loadedChunks) {
        loadedSet[chunk->getPosition()] = true;
    }

    // Find and remove chunks that are no longer loaded
    std::vector<ChunkPosition> toRemove;
    int minX, maxX, minY, maxY;
    m_lightMap.getWorldRange(minX, maxX, minY, maxY);

    // Iterate through light map chunks and check
    for (ChunkCoord cy = worldToChunkCoord(minY); cy <= worldToChunkCoord(maxY - 1); ++cy) {
        for (ChunkCoord cx = worldToChunkCoord(minX); cx <= worldToChunkCoord(maxX - 1); ++cx) {
            ChunkPosition pos(cx, cy);
            if (m_lightMap.hasChunk(pos) && loadedSet.find(pos) == loadedSet.end()) {
                toRemove.push_back(pos);
            }
        }
    }

    for (const auto& pos : toRemove) {
        m_lightMap.removeChunk(pos);
    }
}

void LightingSystem::collectLightSources() {
    m_lightSources.clear();

    if (!m_tileMap) return;

    int tileSize = m_tileMap->getTileSize();

    // Collect entity light sources
    getRegistry().each<Transform, LightSource>(
        [this, tileSize](Entity /*entity*/, const Transform& transform, const LightSource& light) {
            if (!light.enabled) return;

            // Convert world pixel position to tile coordinates
            Vec2 lightPos = transform.position + light.offset;
            int tileX = static_cast<int>(std::floor(lightPos.x / tileSize));
            int tileY = static_cast<int>(std::floor(lightPos.y / tileSize));

            // Scale color by effective intensity
            float intensity = light.getEffectiveIntensity();
            uint8_t r = static_cast<uint8_t>(light.color.r * intensity);
            uint8_t g = static_cast<uint8_t>(light.color.g * intensity);
            uint8_t b = static_cast<uint8_t>(light.color.b * intensity);

            // Radius determines max light level at source
            TileLight tileColor(r, g, b);
            m_lightSources.emplace_back(tileX, tileY, tileColor);
        }
    );

    // Collect tile-based light sources (e.g., torches, glowing ores)
    // Iterate visible area tiles looking for light-emitting tiles
    if (m_camera) {
        int visMinX, visMaxX, visMinY, visMaxY;
        m_tileMap->getVisibleTileRange(*m_camera, visMinX, visMaxX, visMinY, visMaxY);

        // Add padding for light propagation beyond visible area
        int pad = m_config.lightMap.maxLightRadius;
        visMinX -= pad;
        visMaxX += pad;
        visMinY -= pad;
        visMaxY += pad;

        // For now, tile light emissions are handled via the content registry.
        // When tiles with light_emission are placed, they should be registered as sources.
        // This will be done through the mod system's tile definitions.
        // Placeholder: the light source list from entities is the primary mechanism.
    }

    m_stats.pointLightCount = m_lightSources.size();
}

void LightingSystem::recalculate() {
    if (!m_tileMap || !m_tileMap->isWorldLoaded()) return;

    auto start = std::chrono::high_resolution_clock::now();

    TileLight skyColor = m_dayNightCycle.getSkyColor();

    // Create isSolid callback
    auto isSolid = [this](int wx, int wy) -> bool {
        return m_tileMap->isSolid(wx, wy);
    };

    // Create getSurfaceY callback - find first solid tile going downward
    auto getSurfaceY = [this](int wx) -> int {
        // Scan downward from top of loaded area to find surface
        int minX, maxX, minY, maxY;
        m_lightMap.getWorldRange(minX, maxX, minY, maxY);

        for (int y = minY; y < maxY; ++y) {
            if (m_tileMap->isSolid(wx, y)) {
                return y;
            }
        }
        return maxY; // No surface found
    };

    m_lightMap.recalculateAll(m_lightSources, isSolid, getSurfaceY, skyColor);

    auto end = std::chrono::high_resolution_clock::now();
    m_stats.lastRecalcTimeMs = std::chrono::duration<float, std::milli>(end - start).count();
}

void LightingSystem::renderLightOverlay(IRenderer* renderer, const Camera& camera) {
    if (!m_config.enabled || !renderer) return;

    int tileSize = m_tileMap ? m_tileMap->getTileSize() : 16;

    // Determine visible tile range
    Rect visArea = camera.getVisibleArea();
    int minTileX = static_cast<int>(std::floor(visArea.x / tileSize)) - 1;
    int maxTileX = static_cast<int>(std::ceil((visArea.x + visArea.width) / tileSize)) + 1;
    int minTileY = static_cast<int>(std::floor(visArea.y / tileSize)) - 1;
    int maxTileY = static_cast<int>(std::ceil((visArea.y + visArea.height) / tileSize)) + 1;

    size_t tilesLit = 0;

    for (int ty = minTileY; ty < maxTileY; ++ty) {
        for (int tx = minTileX; tx < maxTileX; ++tx) {
            if (m_config.lightMap.enableSmoothLighting) {
                renderSmoothTile(renderer, camera, tx, ty, tileSize);
            } else {
                renderFlatTile(renderer, camera, tx, ty, tileSize);
            }
            ++tilesLit;
        }
    }

    m_stats.tilesLit = tilesLit;
}

void LightingSystem::renderFlatTile(IRenderer* renderer, const Camera& camera,
                                     int tileX, int tileY, int tileSize) {
    TileLight light = m_lightMap.getLight(tileX, tileY);

    // If fully lit, no overlay needed
    if (light.r >= 255 && light.g >= 255 && light.b >= 255) return;

    // Calculate darkness: we draw a dark overlay where there is no light.
    // The overlay alpha = 255 - brightness
    uint8_t darkness = static_cast<uint8_t>(255 - light.maxChannel());
    if (darkness == 0) return;

    // World position of the tile
    float worldX = static_cast<float>(tileX * tileSize);
    float worldY = static_cast<float>(tileY * tileSize);

    // Convert to screen coordinates
    Vec2 screenPos = camera.worldToScreen({worldX, worldY});
    float zoom = camera.getZoom();
    float screenSize = tileSize * zoom;

    // Draw a semi-transparent black overlay to darken unlit areas
    // For colored light: tint the darkness appropriately
    // If light has color, reduce the overlay in that color channel
    uint8_t overlayR = static_cast<uint8_t>(255 - light.r);
    uint8_t overlayG = static_cast<uint8_t>(255 - light.g);
    uint8_t overlayB = static_cast<uint8_t>(255 - light.b);

    // Use the maximum darkness value as alpha for the darkest overlay
    // and blend colors to simulate colored light
    Color overlayColor(0, 0, 0, darkness);

    // For truly colored lighting: we overlay the inverse of the light color
    // This makes unlit areas black and lit areas show the light's tint
    if (light.r != light.g || light.g != light.b) {
        // Colored light: overlay each channel's inverse
        overlayColor = Color(0, 0, 0, static_cast<uint8_t>(
            std::max({overlayR, overlayG, overlayB})
        ));
    }

    renderer->drawRectangle(
        Rect(screenPos.x, screenPos.y, screenSize, screenSize),
        overlayColor
    );
}

void LightingSystem::renderSmoothTile(IRenderer* renderer, const Camera& camera,
                                       int tileX, int tileY, int tileSize) {
    // Get corner light values for bilinear interpolation
    TileLight topLeft     = m_lightMap.getCornerLight(tileX,     tileY);
    TileLight topRight    = m_lightMap.getCornerLight(tileX + 1, tileY);
    TileLight bottomLeft  = m_lightMap.getCornerLight(tileX,     tileY + 1);
    TileLight bottomRight = m_lightMap.getCornerLight(tileX + 1, tileY + 1);

    // Average for the tile
    int avgR = (topLeft.r + topRight.r + bottomLeft.r + bottomRight.r) / 4;
    int avgG = (topLeft.g + topRight.g + bottomLeft.g + bottomRight.g) / 4;
    int avgB = (topLeft.b + topRight.b + bottomLeft.b + bottomRight.b) / 4;

    // If fully lit, skip
    if (avgR >= 252 && avgG >= 252 && avgB >= 252) return;

    uint8_t darkness = static_cast<uint8_t>(255 - std::max({avgR, avgG, avgB}));
    if (darkness == 0) return;

    // World position of the tile
    float worldX = static_cast<float>(tileX * tileSize);
    float worldY = static_cast<float>(tileY * tileSize);

    Vec2 screenPos = camera.worldToScreen({worldX, worldY});
    float zoom = camera.getZoom();
    float screenSize = tileSize * zoom;

    // For smooth lighting, we subdivide each tile into 4 quadrants,
    // each using the interpolated corner value closest to it.
    float halfSize = screenSize * 0.5f;

    auto drawQuad = [&](float sx, float sy, const TileLight& light) {
        uint8_t d = static_cast<uint8_t>(255 - std::max({light.r, light.g, light.b}));
        if (d == 0) return;
        renderer->drawRectangle(Rect(sx, sy, halfSize, halfSize), Color(0, 0, 0, d));
    };

    // Top-left quadrant: average of topLeft and center
    TileLight tlQuad(
        static_cast<uint8_t>((topLeft.r + avgR) / 2),
        static_cast<uint8_t>((topLeft.g + avgG) / 2),
        static_cast<uint8_t>((topLeft.b + avgB) / 2)
    );

    // Top-right quadrant
    TileLight trQuad(
        static_cast<uint8_t>((topRight.r + avgR) / 2),
        static_cast<uint8_t>((topRight.g + avgG) / 2),
        static_cast<uint8_t>((topRight.b + avgB) / 2)
    );

    // Bottom-left quadrant
    TileLight blQuad(
        static_cast<uint8_t>((bottomLeft.r + avgR) / 2),
        static_cast<uint8_t>((bottomLeft.g + avgG) / 2),
        static_cast<uint8_t>((bottomLeft.b + avgB) / 2)
    );

    // Bottom-right quadrant
    TileLight brQuad(
        static_cast<uint8_t>((bottomRight.r + avgR) / 2),
        static_cast<uint8_t>((bottomRight.g + avgG) / 2),
        static_cast<uint8_t>((bottomRight.b + avgB) / 2)
    );

    drawQuad(screenPos.x,            screenPos.y,            tlQuad);
    drawQuad(screenPos.x + halfSize, screenPos.y,            trQuad);
    drawQuad(screenPos.x,            screenPos.y + halfSize, blQuad);
    drawQuad(screenPos.x + halfSize, screenPos.y + halfSize, brQuad);
}

} // namespace gloaming
