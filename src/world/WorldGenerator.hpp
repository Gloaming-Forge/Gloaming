#pragma once

#include "world/Chunk.hpp"
#include "world/ChunkGenerator.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace gloaming {

// Forward declarations
class BiomeSystem;
class OreDistribution;
class StructurePlacer;

/// Configuration for the world generator
struct WorldGenConfig {
    int surfaceLevel = 100;         // Default surface Y coordinate
    int seaLevel = 80;              // Sea level for ocean/lake generation
    float terrainScale = 0.02f;     // Horizontal noise frequency for terrain
    float terrainAmplitude = 40.0f; // Vertical amplitude of terrain variation
    int dirtDepthMin = 3;           // Minimum dirt layer depth
    int dirtDepthMax = 7;           // Maximum dirt layer depth
    float caveScale = 0.05f;        // Noise frequency for cave generation
    float caveThreshold = 0.65f;    // Noise value above which caves form
    int caveMinDepth = 10;          // Minimum depth below surface for caves
    bool generateCaves = true;
    bool generateOres = true;
    bool generateStructures = true;
};

/// A generation pass that runs on each chunk.
/// Priority controls execution order (lower = earlier).
struct GenerationPass {
    std::string name;
    int priority = 0;
    std::function<void(Chunk& chunk, uint64_t seed, const WorldGenConfig& config)> generate;
};

/// Custom terrain height callback from Lua.
/// Given (chunk_x, seed), returns a table of 64 heights.
using TerrainHeightCallback = std::function<std::vector<int>(int chunkX, uint64_t seed)>;

/// Custom chunk decorator callback from Lua.
/// Called after main terrain generation to add per-chunk details.
using ChunkDecoratorCallback = std::function<void(Chunk& chunk, uint64_t seed)>;

/// Coordinates the multi-pass world generation pipeline.
///
/// The generation pipeline for each chunk:
///   1. Determine biome for each column (via BiomeSystem)
///   2. Generate terrain heights (surface shape)
///   3. Fill terrain layers (grass/dirt/stone per biome)
///   4. Carve caves
///   5. Place ores
///   6. Place structures
///   7. Run mod-registered custom passes
///
/// All passes are deterministic given the same seed.
class WorldGenerator {
public:
    WorldGenerator();
    ~WorldGenerator();

    /// Initialize with a world seed
    void init(uint64_t seed);

    /// Get/set the world seed
    uint64_t getSeed() const { return m_seed; }
    void setSeed(uint64_t seed) {
        m_seed = seed;
        m_heightCache.clear();
        m_heightCacheChunkX = INT32_MAX;
    }

    /// Get/set configuration
    WorldGenConfig& getConfig() { return m_config; }
    const WorldGenConfig& getConfig() const { return m_config; }

    /// Get the subsystems
    BiomeSystem& getBiomeSystem() { return *m_biomeSystem; }
    const BiomeSystem& getBiomeSystem() const { return *m_biomeSystem; }

    OreDistribution& getOreDistribution() { return *m_oreDistribution; }
    const OreDistribution& getOreDistribution() const { return *m_oreDistribution; }

    StructurePlacer& getStructurePlacer() { return *m_structurePlacer; }
    const StructurePlacer& getStructurePlacer() const { return *m_structurePlacer; }

    // ========================================================================
    // Terrain Height Generation
    // ========================================================================

    /// Register a named terrain height generator (from Lua).
    /// The callback receives (chunkX, seed) and returns 64 heights.
    void registerTerrainGenerator(const std::string& name, TerrainHeightCallback callback);

    /// Set the active terrain generator by name.
    /// If not set or name not found, uses the built-in default.
    void setActiveTerrainGenerator(const std::string& name);

    /// Get the surface height at a world X coordinate.
    /// Uses the active terrain generator or the built-in default.
    int getSurfaceHeight(int worldX) const;

    // ========================================================================
    // Custom Generation Passes
    // ========================================================================

    /// Register a custom generation pass (from Lua).
    /// Passes are sorted by priority and run after built-in generation.
    void registerPass(const std::string& name, int priority,
                      std::function<void(Chunk& chunk, uint64_t seed, const WorldGenConfig& config)> callback);

    /// Register a chunk decorator (from Lua).
    /// Decorators run after all passes and are simpler (no config param).
    void registerDecorator(const std::string& name, ChunkDecoratorCallback callback);

    // ========================================================================
    // Main Generation
    // ========================================================================

    /// Generate a complete chunk. This is the main entry point called
    /// by ChunkManager when a new chunk needs to be created.
    void generateChunk(Chunk& chunk) const;

    /// Get a ChunkGeneratorCallback suitable for ChunkManager::setGeneratorCallback
    ChunkGeneratorCallback asCallback() const;

    // ========================================================================
    // Built-in Generation Steps (can be called individually for testing)
    // ========================================================================

    /// Fill terrain based on surface heights and biome layers
    void generateTerrain(Chunk& chunk) const;

    /// Carve caves using 2D noise
    void generateCaves(Chunk& chunk) const;

    /// Place ores using the OreDistribution rules
    void generateOres(Chunk& chunk) const;

    /// Place structures using the StructurePlacer
    void generateStructures(Chunk& chunk) const;

    /// Run all registered custom passes
    void runCustomPasses(Chunk& chunk) const;

private:
    /// Compute surface height for a world X coordinate using built-in noise
    int defaultSurfaceHeight(int worldX) const;

    /// Cache surface heights for a chunk column range
    mutable std::unordered_map<int, int> m_heightCache;
    mutable int m_heightCacheChunkX = INT32_MAX; // sentinel

    /// Invalidate height cache when chunk changes
    void ensureHeightCache(int chunkX) const;

    uint64_t m_seed = 12345;
    WorldGenConfig m_config;

    // Subsystems
    std::unique_ptr<BiomeSystem> m_biomeSystem;
    std::unique_ptr<OreDistribution> m_oreDistribution;
    std::unique_ptr<StructurePlacer> m_structurePlacer;

    // Mod-registered terrain generators
    std::unordered_map<std::string, TerrainHeightCallback> m_terrainGenerators;
    std::string m_activeTerrainGenerator;

    // Custom generation passes (sorted by priority)
    std::vector<GenerationPass> m_customPasses;

    // Chunk decorators
    std::vector<std::pair<std::string, ChunkDecoratorCallback>> m_decorators;
};

} // namespace gloaming
