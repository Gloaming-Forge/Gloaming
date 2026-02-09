#pragma once

#include "world/Chunk.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace gloaming {

/// Defines how an ore type is distributed underground.
struct OreRule {
    std::string id;             // Unique identifier (e.g. "copper_ore")
    uint16_t tileId = 0;        // Tile ID to place

    // Depth range (world Y coordinates below surface)
    int minDepth = 0;           // Minimum depth below surface
    int maxDepth = 1000;        // Maximum depth below surface

    // Vein properties
    int veinSizeMin = 3;        // Minimum tiles per vein
    int veinSizeMax = 8;        // Maximum tiles per vein

    // Frequency: attempts per chunk column
    float frequency = 0.1f;     // Probability of a vein attempt per column

    // Noise-based filtering
    float noiseScale = 0.1f;    // Noise frequency for ore distribution
    float noiseThreshold = 0.7f; // Noise value above which ore spawns

    // Only replaces these tile IDs (typically stone)
    std::vector<uint16_t> replaceTiles = {3}; // Default: stone

    // Biome restrictions (empty = all biomes)
    std::vector<std::string> biomes;
};

/// Manages ore placement rules and generates ore veins in chunks.
///
/// Ore placement uses a combination of:
///   - Depth-based probability
///   - 2D noise thresholding (for natural-looking clusters)
///   - Random vein generation (connected groups of ore tiles)
class OreDistribution {
public:
    OreDistribution() = default;

    /// Register an ore distribution rule.
    /// Returns false if an ore with this ID already exists.
    bool registerOre(const OreRule& rule);

    /// Remove an ore rule by ID.
    bool removeOre(const std::string& id);

    /// Get an ore rule by ID.
    const OreRule* getOre(const std::string& id) const;

    /// Get all registered ore IDs.
    std::vector<std::string> getOreIds() const;

    /// Get the number of registered ores.
    size_t oreCount() const { return m_ores.size(); }

    /// Clear all registered ores.
    void clear();

    /// Generate ores in a chunk.
    /// @param chunk The chunk to place ores in
    /// @param seed The world seed
    /// @param surfaceHeightAt Function that returns surface height for a world X
    void generateOres(Chunk& chunk, uint64_t seed,
                      const std::function<int(int worldX)>& surfaceHeightAt) const;

private:
    /// Place a single ore vein starting at a position within a chunk.
    /// @param chunk The chunk to modify
    /// @param localX Starting local X coordinate
    /// @param localY Starting local Y coordinate
    /// @param rule The ore rule to apply
    /// @param seed Seed for deterministic vein shape
    void placeVein(Chunk& chunk, int localX, int localY,
                   const OreRule& rule, uint64_t seed) const;

    /// Check if a tile ID is in the replace list for a rule
    static bool canReplace(uint16_t tileId, const OreRule& rule);

    // Ordered by priority (deeper ores processed first to avoid overwriting)
    std::vector<OreRule> m_ores;
    std::unordered_map<std::string, size_t> m_oreIndex; // id -> index in m_ores
};

} // namespace gloaming
