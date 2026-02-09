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
                                     const std::function<int(int worldX)>& surfaceHeightAt,
                                     const std::function<std::string(int worldX)>& getBiomeAt) const {
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

            // Check biome restriction per column
            if (!rule.biomes.empty() && getBiomeAt) {
                std::string biomeId = getBiomeAt(worldX);
                bool biomeMatch = false;
                for (const auto& b : rule.biomes) {
                    if (b == biomeId) { biomeMatch = true; break; }
                }
                if (!biomeMatch) continue;
            }

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

} // namespace gloaming
