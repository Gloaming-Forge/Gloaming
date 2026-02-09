#include "world/StructurePlacer.hpp"
#include "world/ChunkGenerator.hpp"
#include "engine/Log.hpp"
#include <algorithm>

namespace gloaming {

bool StructurePlacer::registerStructure(const StructureTemplate& structure) {
    if (structure.id.empty()) {
        LOG_WARN("StructurePlacer: cannot register structure with empty ID");
        return false;
    }
    if (m_structures.count(structure.id)) {
        LOG_WARN("StructurePlacer: structure '{}' already registered", structure.id);
        return false;
    }
    m_structures[structure.id] = structure;
    LOG_DEBUG("StructurePlacer: registered structure '{}' ({}x{}, chance={})",
              structure.id, structure.width, structure.height, structure.chance);
    return true;
}

bool StructurePlacer::removeStructure(const std::string& id) {
    return m_structures.erase(id) > 0;
}

const StructureTemplate* StructurePlacer::getStructure(const std::string& id) const {
    auto it = m_structures.find(id);
    return (it != m_structures.end()) ? &it->second : nullptr;
}

std::vector<std::string> StructurePlacer::getStructureIds() const {
    std::vector<std::string> ids;
    ids.reserve(m_structures.size());
    for (const auto& [id, _] : m_structures) {
        ids.push_back(id);
    }
    return ids;
}

void StructurePlacer::clear() {
    m_structures.clear();
}

uint32_t StructurePlacer::placementHash(int worldX, int worldY, uint64_t seed,
                                          const std::string& structureId) {
    // Combine position, seed, and structure ID into a deterministic hash
    uint64_t h = static_cast<uint64_t>(worldX) * 374761393ULL +
                 static_cast<uint64_t>(worldY) * 668265263ULL +
                 seed;
    // Mix in structure ID
    for (char c : structureId) {
        h = h * 31 + static_cast<uint64_t>(c);
    }
    h = (h ^ (h >> 13)) * 1274126177ULL;
    h = h ^ (h >> 16);
    return static_cast<uint32_t>(h);
}

bool StructurePlacer::isValidPlacement(const StructureTemplate& structure,
                                         int worldX, int worldY, int surfaceHeight,
                                         const std::string& biomeId,
                                         const Chunk& chunk) const {
    // Check biome restriction
    if (!structure.biomes.empty()) {
        bool biomeMatch = false;
        for (const auto& b : structure.biomes) {
            if (b == biomeId) { biomeMatch = true; break; }
        }
        if (!biomeMatch) return false;
    }

    int depth = worldY - surfaceHeight;

    switch (structure.placement) {
        case StructurePlacement::Surface:
            // Must be at or near surface level
            if (std::abs(depth) > 2) return false;
            break;
        case StructurePlacement::Underground:
            if (depth < structure.minDepth || depth > structure.maxDepth) return false;
            break;
        case StructurePlacement::Ceiling:
            // Ceiling structures must be underground and need air below + solid above
            if (depth < structure.minDepth || depth > structure.maxDepth) return false;
            break;
        case StructurePlacement::Anywhere:
            break;
    }

    // Check needsGround: the tile directly below the origin must be solid
    int chunkMinX = chunk.getWorldMinX();
    int chunkMinY = chunk.getWorldMinY();
    int localX = worldX - chunkMinX;
    int localYBelow = (worldY + 1) - chunkMinY;
    int localYAbove = (worldY - 1) - chunkMinY;
    int localY = worldY - chunkMinY;

    if (structure.needsGround) {
        if (Chunk::isValidLocalCoord(localX, localYBelow)) {
            Tile below = chunk.getTile(localX, localYBelow);
            if (below.isEmpty()) return false;
        }
    }

    // Check needsAir: the origin tile itself should be air (structure placed into open space)
    if (structure.needsAir) {
        if (Chunk::isValidLocalCoord(localX, localY)) {
            Tile at = chunk.getTile(localX, localY);
            if (!at.isEmpty()) return false;
        }
    }

    // Ceiling-specific: needs solid tile above origin
    if (structure.placement == StructurePlacement::Ceiling) {
        if (Chunk::isValidLocalCoord(localX, localYAbove)) {
            Tile above = chunk.getTile(localX, localYAbove);
            if (above.isEmpty()) return false; // Need solid ceiling
        }
    }

    return true;
}

void StructurePlacer::placeStructures(Chunk& chunk, uint64_t seed,
                                        const std::function<int(int worldX)>& surfaceHeightAt,
                                        const std::function<std::string(int worldX)>& getBiomeAt) const {
    int worldMinX = chunk.getWorldMinX();
    int worldMinY = chunk.getWorldMinY();

    for (const auto& [_, structure] : m_structures) {
        // Only check positions with spacing intervals for efficiency
        int step = std::max(1, structure.spacing);

        // Align to grid based on spacing to ensure determinism across chunks
        int startX = worldMinX - (((worldMinX % step) + step) % step);

        for (int worldX = startX; worldX < worldMinX + CHUNK_SIZE; worldX += step) {
            int surfaceY = surfaceHeightAt(worldX);

            // Determine Y position based on placement type
            int placeY = surfaceY;
            if (structure.placement == StructurePlacement::Surface) {
                placeY = surfaceY; // Place at surface
            } else if (structure.placement == StructurePlacement::Underground ||
                       structure.placement == StructurePlacement::Ceiling) {
                // Use noise to determine underground Y
                float depthNoise = Noise::noise2D(worldX, 0, seed + 80000);
                int depthRange = structure.maxDepth - structure.minDepth;
                placeY = surfaceY + structure.minDepth +
                         static_cast<int>(depthNoise * static_cast<float>(depthRange));
            }

            // Check if this position wants a structure (deterministic)
            uint32_t hash = placementHash(worldX, placeY, seed, structure.id);
            float chance = static_cast<float>(hash & 0xFFFF) / 65535.0f;
            if (chance > structure.chance) continue;

            // Check biome
            std::string biomeId = getBiomeAt(worldX);

            // Validate placement (includes needsGround/needsAir/ceiling checks)
            if (!isValidPlacement(structure, worldX, placeY, surfaceY, biomeId, chunk)) continue;

            // Place the structure
            placeAt(chunk, structure, worldX, placeY);
        }
    }
}

bool StructurePlacer::placeAt(Chunk& chunk, const StructureTemplate& structure,
                                int worldX, int worldY) const {
    int chunkMinX = chunk.getWorldMinX();
    int chunkMinY = chunk.getWorldMinY();
    bool placed = false;

    for (const auto& tile : structure.tiles) {
        int tileWorldX = worldX + tile.offsetX;
        int tileWorldY = worldY + tile.offsetY;

        // Check if this tile falls within the current chunk
        int localX = tileWorldX - chunkMinX;
        int localY = tileWorldY - chunkMinY;

        if (!Chunk::isValidLocalCoord(localX, localY)) continue;

        // Check overwrite rules
        if (!tile.overwriteAir) {
            Tile existing = chunk.getTile(localX, localY);
            if (existing.isEmpty()) continue;
        }

        chunk.setTileId(localX, localY, tile.tileId, tile.variant, tile.flags);
        placed = true;
    }

    return placed;
}

} // namespace gloaming
