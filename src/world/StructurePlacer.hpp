#pragma once

#include "world/Chunk.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace gloaming {

/// A single tile placement within a structure template.
struct StructureTile {
    int offsetX = 0;            // Offset from structure origin
    int offsetY = 0;            // Offset from structure origin
    uint16_t tileId = 0;        // Tile to place
    uint8_t variant = 0;
    uint8_t flags = 0;
    bool overwriteAir = true;   // If false, only overwrites non-air tiles
};

/// Defines where a structure can be placed.
enum class StructurePlacement {
    Surface,                    // On the surface (origin at surface level)
    Underground,                // Below the surface
    Ceiling,                    // Hanging from cave ceilings
    Anywhere                    // No placement restriction
};

/// A template for a multi-tile structure (trees, houses, dungeons, etc.)
struct StructureTemplate {
    std::string id;             // Unique identifier
    std::string name;           // Display name

    // Tile data
    std::vector<StructureTile> tiles;
    int width = 0;              // Bounding width
    int height = 0;             // Bounding height

    // Placement rules
    StructurePlacement placement = StructurePlacement::Surface;
    float chance = 0.01f;       // Probability per valid position
    int spacing = 10;           // Minimum tiles between instances
    int minDepth = 0;           // Minimum depth below surface (for underground)
    int maxDepth = 1000;        // Maximum depth below surface (for underground)

    // Biome restrictions (empty = all biomes)
    std::vector<std::string> biomes;

    // Whether the structure needs solid ground under its origin
    bool needsGround = true;

    // Whether the structure needs open air above its origin
    bool needsAir = true;
};

/// Manages structure templates and places them during world generation.
///
/// Structures are deterministically placed based on the world seed.
/// Each structure type has placement rules (frequency, spacing, biome).
/// Structures that span chunk boundaries are handled by checking if the
/// origin falls within the current chunk.
class StructurePlacer {
public:
    StructurePlacer() = default;

    /// Register a structure template.
    /// Returns false if a structure with this ID already exists.
    bool registerStructure(const StructureTemplate& structure);

    /// Remove a structure by ID.
    bool removeStructure(const std::string& id);

    /// Get a structure template by ID.
    const StructureTemplate* getStructure(const std::string& id) const;

    /// Get all registered structure IDs.
    std::vector<std::string> getStructureIds() const;

    /// Get the number of registered structures.
    size_t structureCount() const { return m_structures.size(); }

    /// Clear all registered structures.
    void clear();

    /// Place structures in a chunk.
    /// @param chunk The chunk to place structures in
    /// @param seed The world seed
    /// @param surfaceHeightAt Function that returns surface height for a world X
    /// @param getBiomeAt Function that returns biome ID for a world X
    void placeStructures(Chunk& chunk, uint64_t seed,
                         const std::function<int(int worldX)>& surfaceHeightAt,
                         const std::function<const std::string&(int worldX)>& getBiomeAt) const;

    /// Place a specific structure at a world position in a chunk.
    /// Returns true if at least one tile was placed (origin must be in chunk).
    bool placeAt(Chunk& chunk, const StructureTemplate& structure,
                 int worldX, int worldY) const;

private:
    /// Check if a position is valid for a given structure
    bool isValidPlacement(const StructureTemplate& structure,
                          int worldX, int worldY, int surfaceHeight,
                          const std::string& biomeId,
                          const Chunk& chunk) const;

    /// Deterministic hash for structure placement decisions
    static uint32_t placementHash(int worldX, int worldY, uint64_t seed,
                                   const std::string& structureId);

    std::unordered_map<std::string, StructureTemplate> m_structures;
};

} // namespace gloaming
