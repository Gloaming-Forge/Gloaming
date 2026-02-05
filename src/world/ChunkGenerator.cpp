#include "world/ChunkGenerator.hpp"

namespace gloaming {

// ============================================================================
// Noise Implementation
// ============================================================================

uint32_t Noise::hash(int x, uint64_t seed) {
    // Simple but effective hash combining seed and position
    uint64_t h = static_cast<uint64_t>(x) * 374761393ULL + seed;
    h = (h ^ (h >> 13)) * 1274126177ULL;
    h = h ^ (h >> 16);
    return static_cast<uint32_t>(h);
}

uint32_t Noise::hash2D(int x, int y, uint64_t seed) {
    // Combine x and y with different multipliers
    uint64_t h = static_cast<uint64_t>(x) * 374761393ULL +
                 static_cast<uint64_t>(y) * 668265263ULL + seed;
    h = (h ^ (h >> 13)) * 1274126177ULL;
    h = h ^ (h >> 16);
    return static_cast<uint32_t>(h);
}

float Noise::noise1D(int x, uint64_t seed) {
    uint32_t h = hash(x, seed);
    return static_cast<float>(h) / static_cast<float>(UINT32_MAX);
}

float Noise::noise2D(int x, int y, uint64_t seed) {
    uint32_t h = hash2D(x, y, seed);
    return static_cast<float>(h) / static_cast<float>(UINT32_MAX);
}

float Noise::smoothNoise1D(float x, uint64_t seed, float scale) {
    x *= scale;
    int x0 = static_cast<int>(std::floor(x));
    int x1 = x0 + 1;
    float fx = x - static_cast<float>(x0);

    float v0 = noise1D(x0, seed);
    float v1 = noise1D(x1, seed);

    return lerp(v0, v1, smoothStep(fx));
}

float Noise::smoothNoise2D(float x, float y, uint64_t seed, float scale) {
    x *= scale;
    y *= scale;

    int x0 = static_cast<int>(std::floor(x));
    int x1 = x0 + 1;
    int y0 = static_cast<int>(std::floor(y));
    int y1 = y0 + 1;

    float fx = x - static_cast<float>(x0);
    float fy = y - static_cast<float>(y0);

    // Get noise at corners
    float v00 = noise2D(x0, y0, seed);
    float v10 = noise2D(x1, y0, seed);
    float v01 = noise2D(x0, y1, seed);
    float v11 = noise2D(x1, y1, seed);

    // Interpolate
    float sx = smoothStep(fx);
    float sy = smoothStep(fy);

    float top = lerp(v00, v10, sx);
    float bottom = lerp(v01, v11, sx);

    return lerp(top, bottom, sy);
}

float Noise::fractalNoise1D(float x, uint64_t seed, int octaves, float persistence) {
    float total = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        total += smoothNoise1D(x * frequency, seed + static_cast<uint64_t>(i) * 1000, 1.0f) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }

    return total / maxValue;
}

float Noise::fractalNoise2D(float x, float y, uint64_t seed, int octaves, float persistence) {
    float total = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        total += smoothNoise2D(x * frequency, y * frequency,
                               seed + static_cast<uint64_t>(i) * 1000, 1.0f) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }

    return total / maxValue;
}

// ============================================================================
// ChunkGenerator Implementation
// ============================================================================

void ChunkGenerator::generate(Chunk& chunk) const {
    if (m_callback) {
        m_callback(chunk, m_seed);
    } else {
        defaultGenerator(chunk, m_seed);
    }
}

void ChunkGenerator::defaultGenerator(Chunk& chunk, uint64_t seed) {
    // Get chunk's world position
    int worldMinX = chunk.getWorldMinX();
    int worldMinY = chunk.getWorldMinY();

    // Tile IDs for default terrain (these would be registered by base-game mod)
    constexpr uint16_t TILE_AIR = 0;
    constexpr uint16_t TILE_GRASS = 1;
    constexpr uint16_t TILE_DIRT = 2;
    constexpr uint16_t TILE_STONE = 3;

    // Generate terrain for each column
    for (int localX = 0; localX < CHUNK_SIZE; ++localX) {
        int worldX = worldMinX + localX;

        // Generate surface height using fractal noise
        // Surface around y=100 with variations
        float heightNoise = Noise::fractalNoise1D(
            static_cast<float>(worldX) * 0.02f, seed, 4, 0.5f
        );
        int surfaceY = 100 + static_cast<int>((heightNoise - 0.5f) * 40.0f);

        // Dirt depth varies slightly
        float dirtNoise = Noise::smoothNoise1D(
            static_cast<float>(worldX) * 0.1f, seed + 1000, 1.0f
        );
        int dirtDepth = 3 + static_cast<int>(dirtNoise * 4.0f);

        for (int localY = 0; localY < CHUNK_SIZE; ++localY) {
            int worldY = worldMinY + localY;

            uint16_t tileId = TILE_AIR;
            uint8_t flags = 0;

            if (worldY > surfaceY) {
                // Underground
                if (worldY <= surfaceY + dirtDepth) {
                    // Dirt layer
                    tileId = TILE_DIRT;
                } else {
                    // Stone layer
                    tileId = TILE_STONE;

                    // Add some cave holes using 2D noise
                    float caveNoise = Noise::fractalNoise2D(
                        static_cast<float>(worldX) * 0.05f,
                        static_cast<float>(worldY) * 0.05f,
                        seed + 2000, 3, 0.5f
                    );
                    if (caveNoise > 0.65f) {
                        tileId = TILE_AIR;  // Cave
                    }
                }
            } else if (worldY == surfaceY) {
                // Surface - grass
                tileId = TILE_GRASS;
            }
            // Above surface is air (default)

            // Set solid flag for non-air tiles
            if (tileId != TILE_AIR) {
                flags = Tile::FLAG_SOLID;
            }

            chunk.setTileId(localX, localY, tileId, 0, flags);
        }
    }

    // Mark as clean since it was just generated (not user-modified)
    chunk.clearDirty(ChunkDirtyFlags::NeedsSave);
}

void ChunkGenerator::flatGenerator(Chunk& chunk, uint64_t seed, int surfaceY) {
    (void)seed;  // Not used for flat generation

    constexpr uint16_t TILE_AIR = 0;
    constexpr uint16_t TILE_GRASS = 1;
    constexpr uint16_t TILE_DIRT = 2;
    constexpr uint16_t TILE_STONE = 3;

    int worldMinY = chunk.getWorldMinY();

    for (int localX = 0; localX < CHUNK_SIZE; ++localX) {
        for (int localY = 0; localY < CHUNK_SIZE; ++localY) {
            int worldY = worldMinY + localY;

            uint16_t tileId = TILE_AIR;
            uint8_t flags = 0;

            if (worldY > surfaceY) {
                if (worldY <= surfaceY + 5) {
                    tileId = TILE_DIRT;
                } else {
                    tileId = TILE_STONE;
                }
            } else if (worldY == surfaceY) {
                tileId = TILE_GRASS;
            }

            if (tileId != TILE_AIR) {
                flags = Tile::FLAG_SOLID;
            }

            chunk.setTileId(localX, localY, tileId, 0, flags);
        }
    }

    chunk.clearDirty(ChunkDirtyFlags::NeedsSave);
}

void ChunkGenerator::emptyGenerator(Chunk& chunk, uint64_t seed) {
    (void)seed;
    chunk.clear();
    chunk.clearDirty(ChunkDirtyFlags::NeedsSave);
}

} // namespace gloaming
