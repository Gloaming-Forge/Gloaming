#include "world/OreDistribution.hpp"
#include "world/ChunkGenerator.hpp"
#include "engine/Log.hpp"
#include <algorithm>

namespace gloaming {

bool OreDistribution::registerOre(const OreRule& rule) {
    if (rule.id.empty()) {
        LOG_WARN("OreDistribution: cannot register ore with empty ID");
        return false;
    }
    if (m_oreIndex.count(rule.id)) {
        LOG_WARN("OreDistribution: ore '{}' already registered", rule.id);
        return false;
    }

    m_oreIndex[rule.id] = m_ores.size();
    m_ores.push_back(rule);

    // Sort by maxDepth descending so deeper ores are placed first
    // (avoids shallow ores overwriting rare deep ores)
    std::sort(m_ores.begin(), m_ores.end(),
              [](const OreRule& a, const OreRule& b) {
                  return a.maxDepth > b.maxDepth;
              });

    // Rebuild index after sort
    m_oreIndex.clear();
    for (size_t i = 0; i < m_ores.size(); ++i) {
        m_oreIndex[m_ores[i].id] = i;
    }

    LOG_DEBUG("OreDistribution: registered ore '{}' (depth {}-{}, frequency {})",
              rule.id, rule.minDepth, rule.maxDepth, rule.frequency);
    return true;
}

bool OreDistribution::removeOre(const std::string& id) {
    auto it = m_oreIndex.find(id);
    if (it == m_oreIndex.end()) return false;

    size_t idx = it->second;
    m_ores.erase(m_ores.begin() + static_cast<ptrdiff_t>(idx));
    m_oreIndex.erase(it);

    // Rebuild index
    m_oreIndex.clear();
    for (size_t i = 0; i < m_ores.size(); ++i) {
        m_oreIndex[m_ores[i].id] = i;
    }
    return true;
}

const OreRule* OreDistribution::getOre(const std::string& id) const {
    auto it = m_oreIndex.find(id);
    if (it == m_oreIndex.end()) return nullptr;
    return &m_ores[it->second];
}

std::vector<std::string> OreDistribution::getOreIds() const {
    std::vector<std::string> ids;
    ids.reserve(m_ores.size());
    for (const auto& ore : m_ores) {
        ids.push_back(ore.id);
    }
    return ids;
}

void OreDistribution::clear() {
    m_ores.clear();
    m_oreIndex.clear();
}

bool OreDistribution::canReplace(uint16_t tileId, const OreRule& rule) {
    if (rule.replaceTiles.empty()) return tileId != 0; // Replace any non-air
    for (uint16_t rt : rule.replaceTiles) {
        if (tileId == rt) return true;
    }
    return false;
}

void OreDistribution::generateOres(Chunk& chunk, uint64_t seed,
                                     const std::function<int(int worldX)>& surfaceHeightAt) const {
    int worldMinX = chunk.getWorldMinX();
    int worldMinY = chunk.getWorldMinY();

    for (const auto& rule : m_ores) {
        // Use a unique seed offset per ore type to avoid correlation
        uint64_t oreSeed = seed + static_cast<uint64_t>(
            Noise::hash2D_public(
                static_cast<int>(rule.id.size()),
                static_cast<int>(rule.id[0]),
                seed
            )
        );

        for (int localX = 0; localX < CHUNK_SIZE; ++localX) {
            int worldX = worldMinX + localX;
            int surfaceY = surfaceHeightAt(worldX);

            for (int localY = 0; localY < CHUNK_SIZE; ++localY) {
                int worldY = worldMinY + localY;
                int depth = worldY - surfaceY;

                // Check depth range
                if (depth < rule.minDepth || depth > rule.maxDepth) continue;

                // Check if current tile can be replaced
                Tile current = chunk.getTile(localX, localY);
                if (!canReplace(current.id, rule)) continue;

                // Noise-based check for ore clusters
                float oreNoise = Noise::fractalNoise2D(
                    static_cast<float>(worldX) * rule.noiseScale,
                    static_cast<float>(worldY) * rule.noiseScale,
                    oreSeed, 2, 0.5f
                );

                if (oreNoise < rule.noiseThreshold) continue;

                // Frequency check using deterministic hash
                float freqNoise = Noise::noise2D(worldX, worldY, oreSeed + 10000);
                if (freqNoise > rule.frequency) continue;

                // Place the ore tile
                chunk.setTileId(localX, localY, rule.tileId, 0, Tile::FLAG_SOLID);
            }
        }
    }
}

void OreDistribution::placeVein(Chunk& chunk, int localX, int localY,
                                  const OreRule& rule, uint64_t seed) const {
    // Determine vein size
    int range = rule.veinSizeMax - rule.veinSizeMin;
    int veinSize = rule.veinSizeMin;
    if (range > 0) {
        float sizeNoise = Noise::noise2D(localX, localY, seed + 20000);
        veinSize += static_cast<int>(sizeNoise * static_cast<float>(range));
    }

    // Place tiles in a roughly circular vein pattern
    int placed = 0;
    int cx = localX;
    int cy = localY;

    for (int i = 0; i < veinSize && placed < veinSize; ++i) {
        if (Chunk::isValidLocalCoord(cx, cy)) {
            Tile current = chunk.getTile(cx, cy);
            if (canReplace(current.id, rule)) {
                chunk.setTileId(cx, cy, rule.tileId, 0, Tile::FLAG_SOLID);
                ++placed;
            }
        }

        // Random walk for vein shape
        float dirNoise = Noise::noise2D(cx + i, cy + i, seed + 30000 + static_cast<uint64_t>(i));
        int dir = static_cast<int>(dirNoise * 4.0f);
        switch (dir) {
            case 0: cx++; break;
            case 1: cx--; break;
            case 2: cy++; break;
            case 3: cy--; break;
        }
    }
}

} // namespace gloaming
