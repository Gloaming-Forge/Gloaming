#include "world/WorldGenerator.hpp"
#include "world/BiomeSystem.hpp"
#include "world/OreDistribution.hpp"
#include "world/StructurePlacer.hpp"
#include "engine/Log.hpp"
#include <algorithm>

namespace gloaming {

WorldGenerator::WorldGenerator()
    : m_biomeSystem(std::make_unique<BiomeSystem>())
    , m_oreDistribution(std::make_unique<OreDistribution>())
    , m_structurePlacer(std::make_unique<StructurePlacer>()) {
}

WorldGenerator::~WorldGenerator() = default;

void WorldGenerator::init(uint64_t seed) {
    m_seed = seed;
    m_heightCache.clear();
    m_heightCacheChunkX = INT32_MAX;
    LOG_INFO("WorldGenerator: initialized with seed {}", seed);
}

void WorldGenerator::registerTerrainGenerator(const std::string& name,
                                                TerrainHeightCallback callback) {
    m_terrainGenerators[name] = std::move(callback);
    LOG_DEBUG("WorldGenerator: registered terrain generator '{}'", name);
}

void WorldGenerator::setActiveTerrainGenerator(const std::string& name) {
    if (!name.empty() && m_terrainGenerators.find(name) == m_terrainGenerators.end()) {
        LOG_WARN("WorldGenerator: terrain generator '{}' not found", name);
        return;
    }
    m_activeTerrainGenerator = name;
    m_heightCache.clear();
    m_heightCacheChunkX = INT32_MAX;
}

int WorldGenerator::getSurfaceHeight(int worldX) const {
    // Check cache for the chunk this X belongs to
    int chunkX = worldToChunkCoord(worldX);
    ensureHeightCache(chunkX);

    auto it = m_heightCache.find(worldX);
    if (it != m_heightCache.end()) {
        return it->second;
    }
    // Fallback (should not happen if cache is built correctly)
    return defaultSurfaceHeight(worldX);
}

void WorldGenerator::ensureHeightCache(int chunkX) const {
    if (chunkX == m_heightCacheChunkX) return;

    m_heightCache.clear();
    m_heightCacheChunkX = chunkX;

    int worldMinX = chunkToWorldCoord(chunkX);

    // Try the active Lua terrain generator
    if (!m_activeTerrainGenerator.empty()) {
        auto it = m_terrainGenerators.find(m_activeTerrainGenerator);
        if (it != m_terrainGenerators.end()) {
            auto heights = it->second(chunkX, m_seed);
            for (int x = 0; x < CHUNK_SIZE && x < static_cast<int>(heights.size()); ++x) {
                m_heightCache[worldMinX + x] = heights[x];
            }
            return;
        }
    }

    // Built-in default terrain generation
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        int worldX = worldMinX + x;
        m_heightCache[worldX] = defaultSurfaceHeight(worldX);
    }
}

int WorldGenerator::defaultSurfaceHeight(int worldX) const {
    // Get the biome at this position (if biomes are registered)
    const BiomeDef& biome = m_biomeSystem->getBiomeAt(worldX, m_seed);

    float heightNoise = Noise::fractalNoise1D(
        static_cast<float>(worldX) * m_config.terrainScale,
        m_seed, 4, 0.5f
    );

    float amplitude = m_config.terrainAmplitude * biome.heightScale;
    int baseHeight = m_config.surfaceLevel + static_cast<int>(biome.heightOffset);

    return baseHeight + static_cast<int>((heightNoise - 0.5f) * amplitude);
}

void WorldGenerator::registerPass(const std::string& name, int priority,
    std::function<void(Chunk& chunk, uint64_t seed, const WorldGenConfig& config)> callback) {

    GenerationPass pass;
    pass.name = name;
    pass.priority = priority;
    pass.generate = std::move(callback);
    m_customPasses.push_back(std::move(pass));

    // Keep sorted by priority
    std::sort(m_customPasses.begin(), m_customPasses.end(),
              [](const GenerationPass& a, const GenerationPass& b) {
                  return a.priority < b.priority;
              });

    LOG_DEBUG("WorldGenerator: registered custom pass '{}' (priority {})", name, priority);
}

void WorldGenerator::registerDecorator(const std::string& name,
                                         ChunkDecoratorCallback callback) {
    m_decorators.emplace_back(name, std::move(callback));
    LOG_DEBUG("WorldGenerator: registered decorator '{}'", name);
}

void WorldGenerator::generateChunk(Chunk& chunk) const {
    // Step 1: Generate terrain (surface heights + biome layers)
    generateTerrain(chunk);

    // Step 2: Carve caves
    if (m_config.generateCaves) {
        generateCaves(chunk);
    }

    // Step 3: Place ores
    if (m_config.generateOres) {
        generateOres(chunk);
    }

    // Step 4: Place structures
    if (m_config.generateStructures) {
        generateStructures(chunk);
    }

    // Step 5: Run custom passes
    runCustomPasses(chunk);

    // Mark as clean since it was just generated
    chunk.clearDirty(ChunkDirtyFlags::NeedsSave);
}

ChunkGeneratorCallback WorldGenerator::asCallback() const {
    return [this](Chunk& chunk, uint64_t /*seed*/) {
        generateChunk(chunk);
    };
}

void WorldGenerator::generateTerrain(Chunk& chunk) const {
    int worldMinX = chunk.getWorldMinX();
    int worldMinY = chunk.getWorldMinY();

    constexpr uint16_t TILE_AIR = 0;

    for (int localX = 0; localX < CHUNK_SIZE; ++localX) {
        int worldX = worldMinX + localX;

        // Get biome at this column
        const BiomeDef& biome = m_biomeSystem->getBiomeAt(worldX, m_seed);

        // Get surface height
        int surfaceY = getSurfaceHeight(worldX);

        // Determine dirt depth using noise + biome settings
        float dirtNoise = Noise::smoothNoise1D(
            static_cast<float>(worldX) * 0.1f, m_seed + 1000, 1.0f
        );
        int dirtDepth = biome.dirtDepth +
                        static_cast<int>(dirtNoise * static_cast<float>(
                            m_config.dirtDepthMax - m_config.dirtDepthMin));

        for (int localY = 0; localY < CHUNK_SIZE; ++localY) {
            int worldY = worldMinY + localY;

            uint16_t tileId = TILE_AIR;
            uint8_t flags = 0;

            if (worldY > surfaceY) {
                // Underground
                int depth = worldY - surfaceY;
                if (depth <= dirtDepth) {
                    tileId = biome.subsurfaceTile;
                } else {
                    tileId = biome.stoneTile;
                }
            } else if (worldY == surfaceY) {
                // Surface
                tileId = biome.surfaceTile;
            }
            // Above surface = air (default)

            if (tileId != TILE_AIR) {
                flags = Tile::FLAG_SOLID;
            }

            chunk.setTileId(localX, localY, tileId, 0, flags);
        }
    }
}

void WorldGenerator::generateCaves(Chunk& chunk) const {
    int worldMinX = chunk.getWorldMinX();
    int worldMinY = chunk.getWorldMinY();

    for (int localX = 0; localX < CHUNK_SIZE; ++localX) {
        int worldX = worldMinX + localX;
        int surfaceY = getSurfaceHeight(worldX);

        // Get biome cave frequency modifier
        const BiomeDef& biome = m_biomeSystem->getBiomeAt(worldX, m_seed);

        for (int localY = 0; localY < CHUNK_SIZE; ++localY) {
            int worldY = worldMinY + localY;
            int depth = worldY - surfaceY;

            // Only generate caves below minimum depth
            if (depth < m_config.caveMinDepth) continue;

            // Skip air tiles
            Tile current = chunk.getTile(localX, localY);
            if (current.isEmpty()) continue;

            // Cave noise with biome frequency modifier
            float effectiveScale = m_config.caveScale * biome.caveFrequency;
            float caveNoise = Noise::fractalNoise2D(
                static_cast<float>(worldX) * effectiveScale,
                static_cast<float>(worldY) * effectiveScale,
                m_seed + 2000, 3, 0.5f
            );

            // Larger caves deeper underground (threshold decreases with depth)
            float depthFactor = static_cast<float>(depth) / 500.0f;
            float threshold = m_config.caveThreshold - depthFactor * 0.05f;
            threshold = std::max(threshold, 0.45f); // Cap how open caves can be

            if (caveNoise > threshold) {
                chunk.setTileId(localX, localY, 0, 0, 0); // Air
            }
        }
    }
}

void WorldGenerator::generateOres(Chunk& chunk) const {
    auto surfaceHeightAt = [this](int worldX) -> int {
        return getSurfaceHeight(worldX);
    };
    auto getBiomeAt = [this](int worldX) -> std::string {
        return m_biomeSystem->getBiomeAt(worldX, m_seed).id;
    };
    m_oreDistribution->generateOres(chunk, m_seed, surfaceHeightAt, getBiomeAt);
}

void WorldGenerator::generateStructures(Chunk& chunk) const {
    auto surfaceHeightAt = [this](int worldX) -> int {
        return getSurfaceHeight(worldX);
    };
    auto getBiomeAt = [this](int worldX) -> std::string {
        return m_biomeSystem->getBiomeAt(worldX, m_seed).id;
    };
    m_structurePlacer->placeStructures(chunk, m_seed, surfaceHeightAt, getBiomeAt);
}

void WorldGenerator::runCustomPasses(Chunk& chunk) const {
    // Run custom passes (already sorted by priority)
    for (const auto& pass : m_customPasses) {
        try {
            pass.generate(chunk, m_seed, m_config);
        } catch (const std::exception& e) {
            LOG_ERROR("WorldGenerator: custom pass '{}' failed: {}", pass.name, e.what());
        }
    }

    // Run decorators
    for (const auto& [name, decorator] : m_decorators) {
        try {
            decorator(chunk, m_seed);
        } catch (const std::exception& e) {
            LOG_ERROR("WorldGenerator: decorator '{}' failed: {}", name, e.what());
        }
    }
}

} // namespace gloaming
