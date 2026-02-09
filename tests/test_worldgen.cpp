#include <gtest/gtest.h>
#include "world/WorldGenerator.hpp"
#include "world/BiomeSystem.hpp"
#include "world/OreDistribution.hpp"
#include "world/StructurePlacer.hpp"
#include "world/ChunkGenerator.hpp"

using namespace gloaming;

// ============================================================================
// BiomeSystem Tests
// ============================================================================

TEST(BiomeSystemTest, RegisterBiome) {
    BiomeSystem biomes;
    EXPECT_EQ(biomes.biomeCount(), 0);

    BiomeDef forest;
    forest.id = "forest";
    forest.name = "Forest";
    forest.temperatureMin = 0.3f;
    forest.temperatureMax = 0.7f;
    forest.humidityMin = 0.4f;
    forest.humidityMax = 0.8f;
    forest.surfaceTile = 1;
    forest.subsurfaceTile = 2;
    forest.stoneTile = 3;

    EXPECT_TRUE(biomes.registerBiome(forest));
    EXPECT_EQ(biomes.biomeCount(), 1);

    // Duplicate registration should fail
    EXPECT_FALSE(biomes.registerBiome(forest));
    EXPECT_EQ(biomes.biomeCount(), 1);
}

TEST(BiomeSystemTest, RegisterMultipleBiomes) {
    BiomeSystem biomes;

    BiomeDef desert;
    desert.id = "desert";
    desert.temperatureMin = 0.7f;
    desert.temperatureMax = 1.0f;
    desert.humidityMin = 0.0f;
    desert.humidityMax = 0.3f;

    BiomeDef tundra;
    tundra.id = "tundra";
    tundra.temperatureMin = 0.0f;
    tundra.temperatureMax = 0.2f;
    tundra.humidityMin = 0.0f;
    tundra.humidityMax = 0.5f;

    EXPECT_TRUE(biomes.registerBiome(desert));
    EXPECT_TRUE(biomes.registerBiome(tundra));
    EXPECT_EQ(biomes.biomeCount(), 2);

    auto ids = biomes.getBiomeIds();
    EXPECT_EQ(ids.size(), 2);
}

TEST(BiomeSystemTest, GetBiome) {
    BiomeSystem biomes;

    BiomeDef forest;
    forest.id = "forest";
    forest.name = "Forest";
    forest.surfaceTile = 10;
    biomes.registerBiome(forest);

    const BiomeDef* found = biomes.getBiome("forest");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "Forest");
    EXPECT_EQ(found->surfaceTile, 10);

    EXPECT_EQ(biomes.getBiome("nonexistent"), nullptr);
}

TEST(BiomeSystemTest, RemoveBiome) {
    BiomeSystem biomes;

    BiomeDef biome;
    biome.id = "test";
    biomes.registerBiome(biome);

    EXPECT_TRUE(biomes.removeBiome("test"));
    EXPECT_EQ(biomes.biomeCount(), 0);
    EXPECT_FALSE(biomes.removeBiome("test"));
}

TEST(BiomeSystemTest, ClearBiomes) {
    BiomeSystem biomes;

    BiomeDef a; a.id = "a";
    BiomeDef b; b.id = "b";
    biomes.registerBiome(a);
    biomes.registerBiome(b);
    EXPECT_EQ(biomes.biomeCount(), 2);

    biomes.clear();
    EXPECT_EQ(biomes.biomeCount(), 0);
}

TEST(BiomeSystemTest, EmptyIdRejected) {
    BiomeSystem biomes;
    BiomeDef bad;
    bad.id = "";
    EXPECT_FALSE(biomes.registerBiome(bad));
}

TEST(BiomeSystemTest, GetBiomeAtReturnsDefault) {
    BiomeSystem biomes;
    // No biomes registered - should return default
    const BiomeDef& biome = biomes.getBiomeAt(100, 42);
    // Default biome has empty ID
    EXPECT_TRUE(biome.id.empty());
}

TEST(BiomeSystemTest, GetBiomeAtWithBiomes) {
    BiomeSystem biomes;

    // Register a biome that covers the entire climate range
    BiomeDef plains;
    plains.id = "plains";
    plains.temperatureMin = 0.0f;
    plains.temperatureMax = 1.0f;
    plains.humidityMin = 0.0f;
    plains.humidityMax = 1.0f;
    biomes.registerBiome(plains);

    // Should always find the plains biome since it covers everything
    const BiomeDef& result = biomes.getBiomeAt(100, 42);
    EXPECT_EQ(result.id, "plains");
}

TEST(BiomeSystemTest, TemperatureHumidityDeterministic) {
    BiomeSystem biomes;

    float t1 = biomes.getTemperature(100, 42);
    float t2 = biomes.getTemperature(100, 42);
    EXPECT_EQ(t1, t2);

    // Different seed should give different values
    float t3 = biomes.getTemperature(100, 43);
    EXPECT_NE(t1, t3);

    // Values should be in [0, 1]
    for (int x = -200; x < 200; ++x) {
        float temp = biomes.getTemperature(x, 42);
        float humid = biomes.getHumidity(x, 42);
        EXPECT_GE(temp, 0.0f);
        EXPECT_LE(temp, 1.0f);
        EXPECT_GE(humid, 0.0f);
        EXPECT_LE(humid, 1.0f);
    }
}

TEST(BiomeSystemTest, BiomeSelectionByClimate) {
    BiomeSystem biomes;

    BiomeDef desert;
    desert.id = "desert";
    desert.temperatureMin = 0.7f;
    desert.temperatureMax = 1.0f;
    desert.humidityMin = 0.0f;
    desert.humidityMax = 0.3f;
    biomes.registerBiome(desert);

    BiomeDef tundra;
    tundra.id = "tundra";
    tundra.temperatureMin = 0.0f;
    tundra.temperatureMax = 0.3f;
    tundra.humidityMin = 0.0f;
    tundra.humidityMax = 0.5f;
    biomes.registerBiome(tundra);

    BiomeDef forest;
    forest.id = "forest";
    forest.temperatureMin = 0.3f;
    forest.temperatureMax = 0.7f;
    forest.humidityMin = 0.3f;
    forest.humidityMax = 0.8f;
    biomes.registerBiome(forest);

    // Search for positions that map to different biomes
    bool foundDesert = false, foundTundra = false, foundForest = false;
    for (int x = -5000; x < 5000; x += 10) {
        const BiomeDef& b = biomes.getBiomeAt(x, 42);
        if (b.id == "desert") foundDesert = true;
        else if (b.id == "tundra") foundTundra = true;
        else if (b.id == "forest") foundForest = true;
    }
    // With a wide enough range, we should find at least some biome diversity
    // (one or more biome types should be found)
    EXPECT_TRUE(foundDesert || foundTundra || foundForest);
}

// ============================================================================
// OreDistribution Tests
// ============================================================================

TEST(OreDistributionTest, RegisterOre) {
    OreDistribution ores;
    EXPECT_EQ(ores.oreCount(), 0);

    OreRule copper;
    copper.id = "copper";
    copper.tileId = 10;
    copper.minDepth = 5;
    copper.maxDepth = 100;
    copper.frequency = 0.2f;

    EXPECT_TRUE(ores.registerOre(copper));
    EXPECT_EQ(ores.oreCount(), 1);

    // Duplicate fails
    EXPECT_FALSE(ores.registerOre(copper));
}

TEST(OreDistributionTest, GetOre) {
    OreDistribution ores;

    OreRule iron;
    iron.id = "iron";
    iron.tileId = 11;
    iron.minDepth = 20;
    iron.maxDepth = 200;
    ores.registerOre(iron);

    const OreRule* found = ores.getOre("iron");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->tileId, 11);
    EXPECT_EQ(found->minDepth, 20);

    EXPECT_EQ(ores.getOre("nonexistent"), nullptr);
}

TEST(OreDistributionTest, RemoveOre) {
    OreDistribution ores;

    OreRule rule;
    rule.id = "test_ore";
    rule.tileId = 42;
    ores.registerOre(rule);

    EXPECT_TRUE(ores.removeOre("test_ore"));
    EXPECT_EQ(ores.oreCount(), 0);
    EXPECT_FALSE(ores.removeOre("test_ore"));
}

TEST(OreDistributionTest, ClearOres) {
    OreDistribution ores;

    OreRule a; a.id = "a"; a.tileId = 1;
    OreRule b; b.id = "b"; b.tileId = 2;
    ores.registerOre(a);
    ores.registerOre(b);

    ores.clear();
    EXPECT_EQ(ores.oreCount(), 0);
}

TEST(OreDistributionTest, EmptyIdRejected) {
    OreDistribution ores;
    OreRule bad;
    bad.id = "";
    EXPECT_FALSE(ores.registerOre(bad));
}

TEST(OreDistributionTest, GenerateOresInChunk) {
    OreDistribution ores;

    // Register a high-frequency ore that should definitely appear
    OreRule testOre;
    testOre.id = "test_ore";
    testOre.tileId = 50;
    testOre.minDepth = 0;
    testOre.maxDepth = 500;
    testOre.frequency = 0.9f;          // Very high frequency
    testOre.noiseThreshold = 0.1f;     // Very low threshold = more spawns
    testOre.replaceTiles = {3};        // Replace stone
    ores.registerOre(testOre);

    // Create a chunk filled with stone (tile 3) at depth
    ChunkPosition pos(0, 2);   // Below surface
    Chunk chunk(pos);
    Tile stone;
    stone.id = 3;
    stone.flags = Tile::FLAG_SOLID;
    chunk.fill(stone);

    // Surface is at y=100, chunk at y=2 means worldMinY=128 (depth=28+)
    auto surfaceAt = [](int) -> int { return 100; };
    ores.generateOres(chunk, 42, surfaceAt);

    // Count ore tiles
    int oreCount = 0;
    for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            if (chunk.getTile(x, y).id == 50) ++oreCount;
        }
    }

    // With high frequency and low threshold, there should be some ore
    EXPECT_GT(oreCount, 0);
    // But not all tiles should be ore
    EXPECT_LT(oreCount, CHUNK_TILE_COUNT);
}

TEST(OreDistributionTest, OreRespectsDepth) {
    OreDistribution ores;

    OreRule deepOre;
    deepOre.id = "deep_ore";
    deepOre.tileId = 60;
    deepOre.minDepth = 200;     // Only very deep
    deepOre.maxDepth = 500;
    deepOre.frequency = 1.0f;
    deepOre.noiseThreshold = 0.0f;
    deepOre.replaceTiles = {3};
    ores.registerOre(deepOre);

    // Create a chunk near the surface (chunk y=1, worldMinY=64)
    // With surface at 100, max depth here is about 27
    ChunkPosition pos(0, 1);
    Chunk chunk(pos);
    Tile stone; stone.id = 3; stone.flags = Tile::FLAG_SOLID;
    chunk.fill(stone);

    auto surfaceAt = [](int) -> int { return 100; };
    ores.generateOres(chunk, 42, surfaceAt);

    // Should not find any deep ore near the surface
    int oreCount = 0;
    for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            if (chunk.getTile(x, y).id == 60) ++oreCount;
        }
    }
    EXPECT_EQ(oreCount, 0);
}

TEST(OreDistributionTest, OreDoesNotReplaceAir) {
    OreDistribution ores;

    OreRule surfaceOre;
    surfaceOre.id = "surface_ore";
    surfaceOre.tileId = 70;
    surfaceOre.minDepth = 0;
    surfaceOre.maxDepth = 500;
    surfaceOre.frequency = 1.0f;
    surfaceOre.noiseThreshold = 0.0f;
    surfaceOre.replaceTiles = {3}; // Only replace stone
    ores.registerOre(surfaceOre);

    // Create a chunk that's mostly air
    ChunkPosition pos(0, 2);
    Chunk chunk(pos);
    // Only set a few stone tiles
    chunk.setTileId(10, 10, 3, 0, Tile::FLAG_SOLID);
    chunk.setTileId(20, 20, 3, 0, Tile::FLAG_SOLID);

    auto surfaceAt = [](int) -> int { return 100; };
    ores.generateOres(chunk, 42, surfaceAt);

    // Air tiles should remain air
    EXPECT_EQ(chunk.getTile(0, 0).id, 0);
    EXPECT_EQ(chunk.getTile(5, 5).id, 0);
}

// ============================================================================
// StructurePlacer Tests
// ============================================================================

TEST(StructurePlacerTest, RegisterStructure) {
    StructurePlacer placer;
    EXPECT_EQ(placer.structureCount(), 0);

    StructureTemplate tree;
    tree.id = "tree";
    tree.name = "Oak Tree";
    tree.width = 3;
    tree.height = 5;
    tree.chance = 0.05f;
    tree.spacing = 8;

    // Add trunk and leaves
    tree.tiles.push_back({0, 0, 10, 0, Tile::FLAG_SOLID, true}); // Trunk base
    tree.tiles.push_back({0, -1, 10, 0, Tile::FLAG_SOLID, true}); // Trunk
    tree.tiles.push_back({0, -2, 10, 0, Tile::FLAG_SOLID, true}); // Trunk
    tree.tiles.push_back({-1, -3, 11, 0, 0, true}); // Leaves
    tree.tiles.push_back({0, -3, 11, 0, 0, true});  // Leaves
    tree.tiles.push_back({1, -3, 11, 0, 0, true});  // Leaves
    tree.tiles.push_back({0, -4, 11, 0, 0, true});  // Top leaf

    EXPECT_TRUE(placer.registerStructure(tree));
    EXPECT_EQ(placer.structureCount(), 1);

    // Duplicate fails
    EXPECT_FALSE(placer.registerStructure(tree));
}

TEST(StructurePlacerTest, GetStructure) {
    StructurePlacer placer;

    StructureTemplate s;
    s.id = "house";
    s.name = "Small House";
    s.width = 10;
    s.height = 8;
    placer.registerStructure(s);

    const StructureTemplate* found = placer.getStructure("house");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "Small House");
    EXPECT_EQ(found->width, 10);

    EXPECT_EQ(placer.getStructure("nonexistent"), nullptr);
}

TEST(StructurePlacerTest, RemoveStructure) {
    StructurePlacer placer;

    StructureTemplate s;
    s.id = "test";
    placer.registerStructure(s);

    EXPECT_TRUE(placer.removeStructure("test"));
    EXPECT_EQ(placer.structureCount(), 0);
    EXPECT_FALSE(placer.removeStructure("test"));
}

TEST(StructurePlacerTest, ClearStructures) {
    StructurePlacer placer;

    StructureTemplate a; a.id = "a";
    StructureTemplate b; b.id = "b";
    placer.registerStructure(a);
    placer.registerStructure(b);

    placer.clear();
    EXPECT_EQ(placer.structureCount(), 0);
}

TEST(StructurePlacerTest, PlaceAtDirect) {
    StructurePlacer placer;

    // Create a simple 2-tile structure
    StructureTemplate pillar;
    pillar.id = "pillar";
    pillar.tiles.push_back({0, 0, 20, 0, Tile::FLAG_SOLID, true});
    pillar.tiles.push_back({0, -1, 20, 0, Tile::FLAG_SOLID, true});
    placer.registerStructure(pillar);

    // Create a chunk and place the structure
    Chunk chunk(ChunkPosition(0, 0));
    // Place at world position (10, 30) which is local (10, 30)
    bool placed = placer.placeAt(chunk, pillar, 10, 30);
    EXPECT_TRUE(placed);

    // Check tiles were placed
    EXPECT_EQ(chunk.getTile(10, 30).id, 20);
    EXPECT_EQ(chunk.getTile(10, 29).id, 20); // y-1
}

TEST(StructurePlacerTest, PlaceAtOutOfBoundsPartial) {
    StructurePlacer placer;

    StructureTemplate wide;
    wide.id = "wide";
    // Tiles that extend outside the chunk
    wide.tiles.push_back({0, 0, 25, 0, Tile::FLAG_SOLID, true});
    wide.tiles.push_back({-1, 0, 25, 0, Tile::FLAG_SOLID, true}); // Will be outside at x=0
    placer.registerStructure(wide);

    Chunk chunk(ChunkPosition(0, 0));
    // Place at local (0, 30) - one tile will be at local (-1, 30) which is outside
    bool placed = placer.placeAt(chunk, wide, 0, 30);
    EXPECT_TRUE(placed); // At least one tile placed

    EXPECT_EQ(chunk.getTile(0, 30).id, 25);
    // The (-1, 30) tile should not be placed (out of bounds)
}

TEST(StructurePlacerTest, EmptyIdRejected) {
    StructurePlacer placer;
    StructureTemplate bad;
    bad.id = "";
    EXPECT_FALSE(placer.registerStructure(bad));
}

// ============================================================================
// WorldGenerator Tests
// ============================================================================

TEST(WorldGeneratorTest, Initialization) {
    WorldGenerator gen;
    gen.init(42);

    EXPECT_EQ(gen.getSeed(), 42);
}

TEST(WorldGeneratorTest, DefaultConfig) {
    WorldGenerator gen;
    gen.init(42);

    const auto& config = gen.getConfig();
    EXPECT_EQ(config.surfaceLevel, 100);
    EXPECT_TRUE(config.generateCaves);
    EXPECT_TRUE(config.generateOres);
    EXPECT_TRUE(config.generateStructures);
}

TEST(WorldGeneratorTest, ConfigModification) {
    WorldGenerator gen;
    gen.init(42);

    gen.getConfig().surfaceLevel = 200;
    gen.getConfig().generateCaves = false;

    EXPECT_EQ(gen.getConfig().surfaceLevel, 200);
    EXPECT_FALSE(gen.getConfig().generateCaves);
}

TEST(WorldGeneratorTest, SurfaceHeight) {
    WorldGenerator gen;
    gen.init(42);

    // Surface height should be deterministic
    int h1 = gen.getSurfaceHeight(100);
    int h2 = gen.getSurfaceHeight(100);
    EXPECT_EQ(h1, h2);

    // Different X should (generally) give different heights
    // With the default noise, heights vary
    bool varied = false;
    for (int x = 0; x < 100; ++x) {
        if (gen.getSurfaceHeight(x) != gen.getSurfaceHeight(0)) {
            varied = true;
            break;
        }
    }
    EXPECT_TRUE(varied);
}

TEST(WorldGeneratorTest, SurfaceHeightNearConfig) {
    WorldGenerator gen;
    gen.init(42);
    gen.getConfig().surfaceLevel = 100;
    gen.getConfig().terrainAmplitude = 40.0f;

    // Heights should be within surfaceLevel +/- amplitude
    for (int x = -100; x < 100; ++x) {
        int h = gen.getSurfaceHeight(x);
        EXPECT_GE(h, 100 - 40) << "x=" << x;
        EXPECT_LE(h, 100 + 40) << "x=" << x;
    }
}

TEST(WorldGeneratorTest, GenerateTerrainChunk) {
    WorldGenerator gen;
    gen.init(42);

    // Generate a chunk that should intersect the surface
    ChunkPosition pos(0, 1);  // worldMinY = 64, which is near default surface=100
    Chunk chunk(pos);

    gen.generateTerrain(chunk);

    // Should have some non-empty tiles (underground portion)
    EXPECT_FALSE(chunk.isEmpty());

    // Check that there are both air and solid tiles (surface is within this chunk)
    int airCount = 0, solidCount = 0;
    for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            Tile tile = chunk.getTile(x, y);
            if (tile.isEmpty()) airCount++;
            else solidCount++;
        }
    }
    // Both should be present (surface goes through this chunk)
    EXPECT_GT(airCount, 0);
    EXPECT_GT(solidCount, 0);
}

TEST(WorldGeneratorTest, GenerateTerrainDeterministic) {
    WorldGenerator gen1;
    gen1.init(42);
    WorldGenerator gen2;
    gen2.init(42);

    ChunkPosition pos(5, 2);
    Chunk chunk1(pos);
    Chunk chunk2(pos);

    gen1.generateTerrain(chunk1);
    gen2.generateTerrain(chunk2);

    for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            EXPECT_EQ(chunk1.getTile(x, y).id, chunk2.getTile(x, y).id)
                << "Mismatch at (" << x << ", " << y << ")";
        }
    }
}

TEST(WorldGeneratorTest, GenerateCaves) {
    WorldGenerator gen;
    gen.init(42);
    gen.getConfig().caveMinDepth = 5;
    gen.getConfig().caveThreshold = 0.5f; // Lower threshold = more caves

    // Create a deep chunk filled with stone
    ChunkPosition pos(0, 5);  // worldMinY = 320, well below surface
    Chunk chunk(pos);
    Tile stone;
    stone.id = 3;
    stone.flags = Tile::FLAG_SOLID;
    chunk.fill(stone);

    gen.generateCaves(chunk);

    // Should have some air tiles (caves carved)
    int airCount = 0;
    for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            if (chunk.getTile(x, y).isEmpty()) airCount++;
        }
    }
    // Deep underground with moderate threshold should carve some caves
    EXPECT_GT(airCount, 0);
}

TEST(WorldGeneratorTest, CavesDisabled) {
    WorldGenerator gen;
    gen.init(42);
    gen.getConfig().generateCaves = false;

    ChunkPosition pos(0, 5);
    Chunk chunk(pos);
    Tile stone;
    stone.id = 3;
    stone.flags = Tile::FLAG_SOLID;
    chunk.fill(stone);

    gen.generateChunk(chunk);

    // With caves disabled, the deep chunk should still have terrain but
    // all tiles at this depth should be stone (no carving)
    // Note: terrain gen may produce different tiles at surface level,
    // but deep underground should be all stone
    bool allNonEmpty = true;
    for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            if (chunk.getTile(x, y).isEmpty()) {
                allNonEmpty = false;
                break;
            }
        }
        if (!allNonEmpty) break;
    }
    // At depth, with no caves, terrain should be fully solid
    EXPECT_TRUE(allNonEmpty);
}

TEST(WorldGeneratorTest, FullGeneration) {
    WorldGenerator gen;
    gen.init(42);

    ChunkPosition pos(0, 1);
    Chunk chunk(pos);

    gen.generateChunk(chunk);

    // Should have generated content
    EXPECT_FALSE(chunk.isEmpty());

    // The chunk should not be marked as needing save (just generated)
    EXPECT_FALSE(chunk.isDirty(ChunkDirtyFlags::NeedsSave));
}

TEST(WorldGeneratorTest, AsCallback) {
    WorldGenerator gen;
    gen.init(42);

    auto callback = gen.asCallback();

    ChunkPosition pos(0, 1);
    Chunk chunk(pos);
    callback(chunk, gen.getSeed());

    EXPECT_FALSE(chunk.isEmpty());
}

TEST(WorldGeneratorTest, RegisterCustomTerrainGenerator) {
    WorldGenerator gen;
    gen.init(42);

    // Register a flat terrain generator
    gen.registerTerrainGenerator("flat", [](int chunkX, uint64_t seed) -> std::vector<int> {
        (void)chunkX; (void)seed;
        std::vector<int> heights(CHUNK_SIZE, 50);  // Flat at y=50
        return heights;
    });

    gen.setActiveTerrainGenerator("flat");

    // All surface heights should be 50
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        EXPECT_EQ(gen.getSurfaceHeight(x), 50);
    }
}

TEST(WorldGeneratorTest, CustomPassesRun) {
    WorldGenerator gen;
    gen.init(42);
    gen.getConfig().generateCaves = false;
    gen.getConfig().generateOres = false;
    gen.getConfig().generateStructures = false;

    bool passRan = false;
    gen.registerPass("test_pass", 0,
        [&passRan](Chunk& chunk, uint64_t seed, const WorldGenConfig& config) {
            (void)chunk; (void)seed; (void)config;
            passRan = true;
        });

    ChunkPosition pos(0, 0);
    Chunk chunk(pos);
    gen.generateChunk(chunk);

    EXPECT_TRUE(passRan);
}

TEST(WorldGeneratorTest, DecoratorRuns) {
    WorldGenerator gen;
    gen.init(42);

    bool decoratorRan = false;
    gen.registerDecorator("test_decorator",
        [&decoratorRan](Chunk& chunk, uint64_t seed) {
            (void)chunk; (void)seed;
            decoratorRan = true;
        });

    ChunkPosition pos(0, 0);
    Chunk chunk(pos);
    gen.generateChunk(chunk);

    EXPECT_TRUE(decoratorRan);
}

TEST(WorldGeneratorTest, CustomPassPriority) {
    WorldGenerator gen;
    gen.init(42);
    gen.getConfig().generateCaves = false;
    gen.getConfig().generateOres = false;
    gen.getConfig().generateStructures = false;

    std::vector<std::string> order;

    gen.registerPass("third", 30,
        [&order](Chunk&, uint64_t, const WorldGenConfig&) {
            order.push_back("third");
        });
    gen.registerPass("first", 10,
        [&order](Chunk&, uint64_t, const WorldGenConfig&) {
            order.push_back("first");
        });
    gen.registerPass("second", 20,
        [&order](Chunk&, uint64_t, const WorldGenConfig&) {
            order.push_back("second");
        });

    ChunkPosition pos(0, 0);
    Chunk chunk(pos);
    gen.generateChunk(chunk);

    ASSERT_EQ(order.size(), 3);
    EXPECT_EQ(order[0], "first");
    EXPECT_EQ(order[1], "second");
    EXPECT_EQ(order[2], "third");
}

TEST(WorldGeneratorTest, BiomeAffectsTerrain) {
    WorldGenerator gen;
    gen.init(42);

    // Register a high-altitude biome
    BiomeDef mountain;
    mountain.id = "mountain";
    mountain.temperatureMin = 0.0f;
    mountain.temperatureMax = 1.0f;
    mountain.humidityMin = 0.0f;
    mountain.humidityMax = 1.0f;
    mountain.heightOffset = 50.0f;
    mountain.heightScale = 2.0f;
    mountain.surfaceTile = 5;
    mountain.subsurfaceTile = 6;
    mountain.stoneTile = 7;
    gen.getBiomeSystem().registerBiome(mountain);

    // Get surface heights - they should be elevated
    int h = gen.getSurfaceHeight(0);
    // With heightOffset=50 and surfaceLevel=100, base should be around 150
    EXPECT_GT(h, gen.getConfig().surfaceLevel);
}

TEST(WorldGeneratorTest, OreIntegration) {
    WorldGenerator gen;
    gen.init(42);
    gen.getConfig().generateCaves = false;
    gen.getConfig().generateStructures = false;

    OreRule abundant;
    abundant.id = "abundant_ore";
    abundant.tileId = 99;
    abundant.minDepth = 0;
    abundant.maxDepth = 500;
    abundant.frequency = 0.8f;
    abundant.noiseThreshold = 0.1f;
    abundant.replaceTiles = {3}; // Replace stone
    gen.getOreDistribution().registerOre(abundant);

    // Generate a deep chunk
    ChunkPosition pos(0, 5);
    Chunk chunk(pos);
    gen.generateChunk(chunk);

    // Count the ore tiles
    int oreCount = 0;
    for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            if (chunk.getTile(x, y).id == 99) oreCount++;
        }
    }
    EXPECT_GT(oreCount, 0);
}

TEST(WorldGeneratorTest, StructureIntegration) {
    WorldGenerator gen;
    gen.init(42);
    gen.getConfig().generateCaves = false;
    gen.getConfig().generateOres = false;

    // Register a very common structure (high chance, small spacing)
    StructureTemplate marker;
    marker.id = "marker";
    marker.placement = StructurePlacement::Surface;
    marker.chance = 1.0f;   // Always place
    marker.spacing = 1;     // Every position
    marker.needsAir = false;  // Allow placing on solid surface tiles
    marker.needsGround = false;
    marker.tiles.push_back({0, 0, 88, 0, Tile::FLAG_SOLID, true});
    gen.getStructurePlacer().registerStructure(marker);

    // Generate a chunk near the surface
    ChunkPosition pos(0, 1);
    Chunk chunk(pos);
    gen.generateChunk(chunk);

    // Check if any marker tiles were placed
    int markerCount = 0;
    for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            if (chunk.getTile(x, y).id == 88) markerCount++;
        }
    }
    // With chance=1.0 and spacing=1, some markers should be placed at surface level
    EXPECT_GT(markerCount, 0);
}

TEST(WorldGeneratorTest, DifferentSeedsDifferentWorlds) {
    WorldGenerator gen1;
    gen1.init(42);
    WorldGenerator gen2;
    gen2.init(999);

    ChunkPosition pos(3, 2);
    Chunk chunk1(pos);
    Chunk chunk2(pos);

    gen1.generateChunk(chunk1);
    gen2.generateChunk(chunk2);

    // Different seeds should produce different terrain
    bool different = false;
    for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            if (chunk1.getTile(x, y).id != chunk2.getTile(x, y).id) {
                different = true;
                break;
            }
        }
        if (different) break;
    }
    EXPECT_TRUE(different);
}

TEST(WorldGeneratorTest, SeedChange) {
    WorldGenerator gen;
    gen.init(42);
    EXPECT_EQ(gen.getSeed(), 42);

    gen.setSeed(100);
    EXPECT_EQ(gen.getSeed(), 100);
}

TEST(WorldGeneratorTest, SetSeedInvalidatesCache) {
    WorldGenerator gen;
    gen.init(42);

    int h1 = gen.getSurfaceHeight(100);

    // Change seed -- cached heights must be invalidated
    gen.setSeed(999);

    int h2 = gen.getSurfaceHeight(100);

    // Different seeds should produce different heights (with very high probability)
    EXPECT_NE(h1, h2);
}

TEST(WorldGeneratorTest, SubsystemAccess) {
    WorldGenerator gen;
    gen.init(42);

    // Should be able to access subsystems
    BiomeSystem& biomes = gen.getBiomeSystem();
    OreDistribution& ores = gen.getOreDistribution();
    StructurePlacer& structures = gen.getStructurePlacer();

    EXPECT_EQ(biomes.biomeCount(), 0);
    EXPECT_EQ(ores.oreCount(), 0);
    EXPECT_EQ(structures.structureCount(), 0);
}

// ============================================================================
// Integration Test: Full Pipeline
// ============================================================================

TEST(WorldGeneratorTest, FullPipelineIntegration) {
    WorldGenerator gen;
    gen.init(12345);

    // Register biomes
    BiomeDef plains;
    plains.id = "plains";
    plains.temperatureMin = 0.3f;
    plains.temperatureMax = 0.7f;
    plains.humidityMin = 0.2f;
    plains.humidityMax = 0.8f;
    plains.surfaceTile = 1;     // Grass
    plains.subsurfaceTile = 2;  // Dirt
    plains.stoneTile = 3;       // Stone
    plains.dirtDepth = 5;
    gen.getBiomeSystem().registerBiome(plains);

    BiomeDef desert;
    desert.id = "desert";
    desert.temperatureMin = 0.7f;
    desert.temperatureMax = 1.0f;
    desert.humidityMin = 0.0f;
    desert.humidityMax = 0.3f;
    desert.surfaceTile = 4;     // Sand
    desert.subsurfaceTile = 4;  // Sand
    desert.stoneTile = 5;       // Sandstone
    desert.dirtDepth = 10;
    gen.getBiomeSystem().registerBiome(desert);

    // Register ores
    OreRule copper;
    copper.id = "copper";
    copper.tileId = 10;
    copper.minDepth = 5;
    copper.maxDepth = 100;
    copper.frequency = 0.3f;
    copper.noiseThreshold = 0.5f;
    copper.replaceTiles = {3, 5};  // Replace stone and sandstone
    gen.getOreDistribution().registerOre(copper);

    // Register a simple structure
    StructureTemplate bush;
    bush.id = "bush";
    bush.placement = StructurePlacement::Surface;
    bush.chance = 0.3f;
    bush.spacing = 5;
    bush.tiles.push_back({0, -1, 15, 0, 0, true}); // Leaves above ground
    gen.getStructurePlacer().registerStructure(bush);

    // Generate several chunks to test the full pipeline
    for (int cx = -2; cx <= 2; ++cx) {
        for (int cy = 0; cy <= 4; ++cy) {
            Chunk chunk(ChunkPosition(cx, cy));
            gen.generateChunk(chunk);

            // Underground chunks should not be empty
            if (cy >= 2) {
                EXPECT_FALSE(chunk.isEmpty())
                    << "Underground chunk (" << cx << ", " << cy << ") should not be empty";
            }
        }
    }
}

// ============================================================================
// Ore Biome Filtering Tests
// ============================================================================

TEST(OreDistributionTest, OreBiomeFiltering) {
    OreDistribution ores;

    // Ore restricted to "desert" biome only
    OreRule desertOre;
    desertOre.id = "desert_ore";
    desertOre.tileId = 80;
    desertOre.minDepth = 0;
    desertOre.maxDepth = 500;
    desertOre.frequency = 1.0f;
    desertOre.noiseThreshold = 0.0f;
    desertOre.replaceTiles = {3};
    desertOre.biomes = {"desert"};
    ores.registerOre(desertOre);

    // Create a stone chunk at depth
    ChunkPosition pos(0, 3);
    Chunk chunk(pos);
    Tile stone; stone.id = 3; stone.flags = Tile::FLAG_SOLID;
    chunk.fill(stone);

    auto surfaceAt = [](int) -> int { return 100; };

    // All columns are "forest" biome -- ore should NOT appear
    auto forestBiome = [](int) -> std::string { return "forest"; };
    ores.generateOres(chunk, 42, surfaceAt, forestBiome);

    int oreCount = 0;
    for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            if (chunk.getTile(x, y).id == 80) oreCount++;
        }
    }
    EXPECT_EQ(oreCount, 0) << "Desert ore should not appear in forest biome";

    // Reset chunk and try with desert biome
    chunk.fill(stone);
    auto desertBiome = [](int) -> std::string { return "desert"; };
    ores.generateOres(chunk, 42, surfaceAt, desertBiome);

    oreCount = 0;
    for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            if (chunk.getTile(x, y).id == 80) oreCount++;
        }
    }
    EXPECT_GT(oreCount, 0) << "Desert ore should appear in desert biome";
}

TEST(OreDistributionTest, OreNoBiomeRestriction) {
    OreDistribution ores;

    // Ore with empty biomes list (no restriction)
    OreRule anyOre;
    anyOre.id = "any_ore";
    anyOre.tileId = 81;
    anyOre.minDepth = 0;
    anyOre.maxDepth = 500;
    anyOre.frequency = 0.9f;
    anyOre.noiseThreshold = 0.1f;
    anyOre.replaceTiles = {3};
    anyOre.biomes = {}; // No restriction
    ores.registerOre(anyOre);

    ChunkPosition pos(0, 3);
    Chunk chunk(pos);
    Tile stone; stone.id = 3; stone.flags = Tile::FLAG_SOLID;
    chunk.fill(stone);

    auto surfaceAt = [](int) -> int { return 100; };
    auto anyBiome = [](int) -> std::string { return "tundra"; };
    ores.generateOres(chunk, 42, surfaceAt, anyBiome);

    int oreCount = 0;
    for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            if (chunk.getTile(x, y).id == 81) oreCount++;
        }
    }
    EXPECT_GT(oreCount, 0) << "Unrestricted ore should appear in any biome";
}

// ============================================================================
// StructurePlacer needsGround/needsAir Tests
// ============================================================================

TEST(StructurePlacerTest, NeedsGroundEnforced) {
    StructurePlacer placer;

    // Structure that needs ground
    StructureTemplate post;
    post.id = "post";
    post.placement = StructurePlacement::Surface;
    post.chance = 1.0f;
    post.spacing = 1;
    post.needsGround = true;
    post.needsAir = false;
    post.tiles.push_back({0, 0, 30, 0, Tile::FLAG_SOLID, true});
    placer.registerStructure(post);

    // Create a chunk with some ground and some air
    Chunk chunk(ChunkPosition(0, 0));
    // Set a surface row at y=32: solid at y=33, air at y=32
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        chunk.setTileId(x, 33, 3, 0, Tile::FLAG_SOLID); // Ground
        // y=32 left as air (surface)
    }

    auto surfaceAt = [](int) -> int { return 32; };
    auto biomeAt = [](int) -> std::string { return "plains"; };
    placer.placeStructures(chunk, 42, surfaceAt, biomeAt);

    // Posts should be placed at y=32 where there's ground below (y=33)
    bool anyPlaced = false;
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        if (chunk.getTile(x, 32).id == 30) {
            anyPlaced = true;
            // Verify ground exists below
            EXPECT_NE(chunk.getTile(x, 33).id, 0)
                << "Structure at x=" << x << " was placed without ground below";
        }
    }
    // Should have placed at least some (chance=1.0)
    EXPECT_TRUE(anyPlaced);
}

TEST(StructurePlacerTest, NeedsAirEnforced) {
    StructurePlacer placer;

    // Structure that needs air at its origin
    StructureTemplate lantern;
    lantern.id = "lantern";
    lantern.placement = StructurePlacement::Surface;
    lantern.chance = 1.0f;
    lantern.spacing = 1;
    lantern.needsGround = false;
    lantern.needsAir = true;
    lantern.tiles.push_back({0, 0, 40, 0, 0, true});
    placer.registerStructure(lantern);

    // Create a fully solid chunk (no air anywhere)
    Chunk chunk(ChunkPosition(0, 0));
    Tile stone; stone.id = 3; stone.flags = Tile::FLAG_SOLID;
    chunk.fill(stone);

    auto surfaceAt = [](int) -> int { return 32; };
    auto biomeAt = [](int) -> std::string { return "plains"; };
    placer.placeStructures(chunk, 42, surfaceAt, biomeAt);

    // No lanterns should be placed because origin is solid (not air)
    int lanternCount = 0;
    for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            if (chunk.getTile(x, y).id == 40) lanternCount++;
        }
    }
    EXPECT_EQ(lanternCount, 0) << "Structures should not be placed where origin is solid";
}

// ============================================================================
// Negative Coordinate Tests
// ============================================================================

TEST(WorldGeneratorTest, NegativeChunkCoordinates) {
    WorldGenerator gen;
    gen.init(42);

    // Generate chunks at negative coordinates
    ChunkPosition negPos(-3, -2);
    Chunk chunk(negPos);
    gen.generateChunk(chunk);

    // Should generate content (above-surface chunks may be empty,
    // but we can verify it ran without crashing)
    // At negative Y chunks (above surface), terrain should be air
    // Just verify no crashes
    SUCCEED();
}

TEST(WorldGeneratorTest, NegativeWorldXSurfaceHeight) {
    WorldGenerator gen;
    gen.init(42);

    // Surface heights at negative X should be deterministic and within range
    for (int x = -200; x < 0; ++x) {
        int h = gen.getSurfaceHeight(x);
        EXPECT_GE(h, 100 - 40) << "x=" << x;
        EXPECT_LE(h, 100 + 40) << "x=" << x;
    }

    // Deterministic
    int h1 = gen.getSurfaceHeight(-100);
    int h2 = gen.getSurfaceHeight(-100);
    EXPECT_EQ(h1, h2);
}
