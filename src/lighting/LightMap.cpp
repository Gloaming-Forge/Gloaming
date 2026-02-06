#include "lighting/LightMap.hpp"
#include <cmath>
#include <functional>

namespace gloaming {

void LightMap::addChunk(const ChunkPosition& pos) {
    if (m_chunks.find(pos) == m_chunks.end()) {
        m_chunks[pos] = ChunkLightData{};
    }
}

void LightMap::removeChunk(const ChunkPosition& pos) {
    m_chunks.erase(pos);
}

bool LightMap::hasChunk(const ChunkPosition& pos) const {
    return m_chunks.find(pos) != m_chunks.end();
}

TileLight LightMap::getLight(int worldX, int worldY) const {
    ChunkPosition cpos(worldToChunkCoord(worldX), worldToChunkCoord(worldY));
    auto it = m_chunks.find(cpos);
    if (it == m_chunks.end()) return {};

    int lx = worldToLocalCoord(worldX);
    int ly = worldToLocalCoord(worldY);
    return it->second.getLight(lx, ly);
}

void LightMap::setLight(int worldX, int worldY, const TileLight& light) {
    ChunkPosition cpos(worldToChunkCoord(worldX), worldToChunkCoord(worldY));
    auto it = m_chunks.find(cpos);
    if (it == m_chunks.end()) return;

    int lx = worldToLocalCoord(worldX);
    int ly = worldToLocalCoord(worldY);
    it->second.setLight(lx, ly, light);
}

TileLight LightMap::getCornerLight(int tileX, int tileY) const {
    // A corner at (tileX, tileY) is shared by the 4 tiles:
    // (tileX-1, tileY-1), (tileX, tileY-1), (tileX-1, tileY), (tileX, tileY)
    TileLight tl = getLight(tileX - 1, tileY - 1);
    TileLight tr = getLight(tileX,     tileY - 1);
    TileLight bl = getLight(tileX - 1, tileY);
    TileLight br = getLight(tileX,     tileY);

    return TileLight(
        static_cast<uint8_t>((static_cast<int>(tl.r) + tr.r + bl.r + br.r) / 4),
        static_cast<uint8_t>((static_cast<int>(tl.g) + tr.g + bl.g + br.g) / 4),
        static_cast<uint8_t>((static_cast<int>(tl.b) + tr.b + bl.b + br.b) / 4)
    );
}

void LightMap::clearAll() {
    for (auto& [pos, data] : m_chunks) {
        data.clear();
    }
}

void LightMap::clearChunk(const ChunkPosition& pos) {
    auto it = m_chunks.find(pos);
    if (it != m_chunks.end()) {
        it->second.clear();
    }
}

void LightMap::propagateLight(const TileLightSource& source,
                               const std::function<bool(int, int)>& isSolid) {
    if (source.color.isDark()) return;

    // BFS flood-fill from the source position
    std::queue<LightNode> queue;
    queue.push({source.worldX, source.worldY, source.color});

    // Set the source tile to the full light value
    TileLight existing = getLight(source.worldX, source.worldY);
    TileLight merged = TileLight::max(existing, source.color);
    setLight(source.worldX, source.worldY, merged);

    while (!queue.empty()) {
        LightNode node = queue.front();
        queue.pop();

        // Try all 4 neighbors
        static constexpr int dx[] = {-1, 1, 0, 0};
        static constexpr int dy[] = {0, 0, -1, 1};

        for (int i = 0; i < 4; ++i) {
            int nx = node.worldX + dx[i];
            int ny = node.worldY + dy[i];

            // Skip tiles in unloaded chunks
            ChunkPosition cpos(worldToChunkCoord(nx), worldToChunkCoord(ny));
            if (m_chunks.find(cpos) == m_chunks.end()) continue;

            // Calculate attenuated light
            int falloff = m_config.lightFalloff;
            // Solid tiles attenuate much more
            if (isSolid(nx, ny)) {
                falloff = falloff * 3;
            }

            int newR = static_cast<int>(node.light.r) - falloff;
            int newG = static_cast<int>(node.light.g) - falloff;
            int newB = static_cast<int>(node.light.b) - falloff;

            // Clamp to zero
            if (newR <= 0 && newG <= 0 && newB <= 0) continue;
            newR = std::max(0, newR);
            newG = std::max(0, newG);
            newB = std::max(0, newB);

            TileLight newLight(static_cast<uint8_t>(newR),
                               static_cast<uint8_t>(newG),
                               static_cast<uint8_t>(newB));

            // Only propagate if this light is brighter than what's there
            TileLight current = getLight(nx, ny);
            if (newLight.r > current.r || newLight.g > current.g || newLight.b > current.b) {
                TileLight final_light = TileLight::max(current, newLight);
                setLight(nx, ny, final_light);
                queue.push({nx, ny, newLight});
            }
        }
    }
}

void LightMap::propagateSkylight(int minWorldX, int maxWorldX,
                                  const std::function<int(int)>& getSurfaceY,
                                  const std::function<bool(int, int)>& isSolid,
                                  const TileLight& skyColor) {
    if (!m_config.enableSkylight) return;

    // Determine loaded Y range for above-surface fill
    int worldMinY = 0, worldMaxY = 0;
    {
        int rx1, rx2, ry1, ry2;
        getWorldRange(rx1, rx2, ry1, ry2);
        worldMinY = ry1;
        worldMaxY = ry2;
    }

    // For each column, find the surface and let light penetrate downward.
    // Uses direct setLight/getLight (O(1) per tile) instead of iterating all chunks.
    for (int col = minWorldX; col < maxWorldX; ++col) {
        int surfaceY = getSurfaceY(col);

        // Set all tiles above the surface to full sky light
        for (int wy = worldMinY; wy < surfaceY && wy < worldMaxY; ++wy) {
            TileLight current = getLight(col, wy);
            setLight(col, wy, TileLight::max(current, skyColor));
        }

        // Propagate skylight downward from surface, dimming through solid tiles
        TileLight currentLight = skyColor;
        int maxDepth = surfaceY + m_config.maxLightRadius * 2;
        for (int wy = surfaceY; wy < maxDepth; ++wy) {
            ChunkPosition cpos(worldToChunkCoord(col), worldToChunkCoord(wy));
            auto it = m_chunks.find(cpos);
            if (it == m_chunks.end()) continue;

            int lx = worldToLocalCoord(col);
            int ly = worldToLocalCoord(wy);

            TileLight existing = it->second.getLight(lx, ly);
            it->second.setLight(lx, ly, TileLight::max(existing, currentLight));

            int falloff = isSolid(col, wy)
                ? m_config.skylightFalloff * 2
                : m_config.skylightFalloff;
            int newR = std::max(0, static_cast<int>(currentLight.r) - falloff);
            int newG = std::max(0, static_cast<int>(currentLight.g) - falloff);
            int newB = std::max(0, static_cast<int>(currentLight.b) - falloff);
            currentLight = TileLight(static_cast<uint8_t>(newR),
                                     static_cast<uint8_t>(newG),
                                     static_cast<uint8_t>(newB));

            if (currentLight.isDark()) break;
        }
    }

    // Propagate skylight sideways into caves using BFS from boundary tiles
    // (lit tiles near the surface that are adjacent to darker tiles).
    std::vector<TileLightSource> boundarySources;
    static constexpr int bdx[] = {-1, 1, 0, 0};
    static constexpr int bdy[] = {0, 0, -1, 1};
    for (int col = minWorldX; col < maxWorldX; ++col) {
        int surfaceY = getSurfaceY(col);
        for (int wy = surfaceY - 1; wy < surfaceY + m_config.maxLightRadius; ++wy) {
            TileLight light = getLight(col, wy);
            if (light.isDark()) continue;

            for (int d = 0; d < 4; ++d) {
                TileLight neighborLight = getLight(col + bdx[d], wy + bdy[d]);
                if (light.r > neighborLight.r + m_config.lightFalloff ||
                    light.g > neighborLight.g + m_config.lightFalloff ||
                    light.b > neighborLight.b + m_config.lightFalloff) {
                    boundarySources.emplace_back(col, wy, light);
                    break;
                }
            }
        }
    }

    for (const auto& src : boundarySources) {
        propagateLight(src, isSolid);
    }
}

void LightMap::recalculateAll(const std::vector<TileLightSource>& lightSources,
                               const std::function<bool(int, int)>& isSolid,
                               const std::function<int(int)>& getSurfaceY,
                               const TileLight& skyColor) {
    // Clear all light data
    clearAll();

    // Determine world range from loaded chunks
    int minX, maxX, minY, maxY;
    getWorldRange(minX, maxX, minY, maxY);

    if (m_chunks.empty()) return;

    // Step 1: Propagate skylight
    if (m_config.enableSkylight) {
        propagateSkylight(minX, maxX, getSurfaceY, isSolid, skyColor);
    }

    // Step 2: Propagate all point light sources
    for (const auto& source : lightSources) {
        propagateLight(source, isSolid);
    }
}

const ChunkLightData* LightMap::getChunkData(const ChunkPosition& pos) const {
    auto it = m_chunks.find(pos);
    return it != m_chunks.end() ? &it->second : nullptr;
}

ChunkLightData* LightMap::getChunkData(const ChunkPosition& pos) {
    auto it = m_chunks.find(pos);
    return it != m_chunks.end() ? &it->second : nullptr;
}

void LightMap::getWorldRange(int& minX, int& maxX, int& minY, int& maxY) const {
    if (m_chunks.empty()) {
        minX = maxX = minY = maxY = 0;
        return;
    }

    bool first = true;
    ChunkCoord cMinX = 0, cMaxX = 0, cMinY = 0, cMaxY = 0;
    for (const auto& [pos, data] : m_chunks) {
        if (first) {
            cMinX = cMaxX = pos.x;
            cMinY = cMaxY = pos.y;
            first = false;
        } else {
            cMinX = std::min(cMinX, pos.x);
            cMaxX = std::max(cMaxX, pos.x);
            cMinY = std::min(cMinY, pos.y);
            cMaxY = std::max(cMaxY, pos.y);
        }
    }

    minX = chunkToWorldCoord(cMinX);
    maxX = chunkToWorldCoord(cMaxX) + CHUNK_SIZE;
    minY = chunkToWorldCoord(cMinY);
    maxY = chunkToWorldCoord(cMaxY) + CHUNK_SIZE;
}

} // namespace gloaming
