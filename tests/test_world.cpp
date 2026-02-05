#include <gtest/gtest.h>
#include "world/Chunk.hpp"
#include "world/ChunkGenerator.hpp"
#include "world/ChunkManager.hpp"
#include "world/WorldFile.hpp"
#include "world/TileMap.hpp"
#include <filesystem>
#include <atomic>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

using namespace gloaming;

namespace {
    // Thread-safe counter for unique test directory names
    std::atomic<int> s_testDirCounter{0};

    std::string makeUniqueTestDir(const char* prefix) {
        return std::string(prefix) + "_" +
               std::to_string(getpid()) + "_" +
               std::to_string(s_testDirCounter.fetch_add(1));
    }
}

// ============================================================================
// Coordinate Conversion Tests
// ============================================================================

TEST(ChunkCoordTest, WorldToChunkPositive) {
    // Tiles 0-63 are in chunk 0
    EXPECT_EQ(worldToChunkCoord(0), 0);
    EXPECT_EQ(worldToChunkCoord(63), 0);
    // Tiles 64-127 are in chunk 1
    EXPECT_EQ(worldToChunkCoord(64), 1);
    EXPECT_EQ(worldToChunkCoord(127), 1);
    // Large values
    EXPECT_EQ(worldToChunkCoord(1000), 15);
}

TEST(ChunkCoordTest, WorldToChunkNegative) {
    // Tiles -64 to -1 are in chunk -1
    EXPECT_EQ(worldToChunkCoord(-1), -1);
    EXPECT_EQ(worldToChunkCoord(-64), -1);
    // Tiles -128 to -65 are in chunk -2
    EXPECT_EQ(worldToChunkCoord(-65), -2);
    EXPECT_EQ(worldToChunkCoord(-128), -2);
}

TEST(ChunkCoordTest, WorldToLocalPositive) {
    EXPECT_EQ(worldToLocalCoord(0), 0);
    EXPECT_EQ(worldToLocalCoord(63), 63);
    EXPECT_EQ(worldToLocalCoord(64), 0);
    EXPECT_EQ(worldToLocalCoord(65), 1);
    EXPECT_EQ(worldToLocalCoord(127), 63);
}

TEST(ChunkCoordTest, WorldToLocalNegative) {
    EXPECT_EQ(worldToLocalCoord(-1), 63);
    EXPECT_EQ(worldToLocalCoord(-64), 0);
    EXPECT_EQ(worldToLocalCoord(-65), 63);
    EXPECT_EQ(worldToLocalCoord(-128), 0);
}

TEST(ChunkCoordTest, ChunkToWorld) {
    EXPECT_EQ(chunkToWorldCoord(0), 0);
    EXPECT_EQ(chunkToWorldCoord(1), 64);
    EXPECT_EQ(chunkToWorldCoord(-1), -64);
    EXPECT_EQ(chunkToWorldCoord(10), 640);
}

TEST(ChunkCoordTest, RoundTripConversion) {
    // For any world coordinate, converting to chunk+local and back should give the same result
    for (int worldX = -200; worldX < 200; ++worldX) {
        ChunkCoord chunkX = worldToChunkCoord(worldX);
        int localX = worldToLocalCoord(worldX);
        int reconstructed = chunkToWorldCoord(chunkX) + localX;
        EXPECT_EQ(reconstructed, worldX) << "Failed for worldX=" << worldX;
    }
}

// ============================================================================
// ChunkPosition Tests
// ============================================================================

TEST(ChunkPositionTest, Equality) {
    ChunkPosition a(1, 2);
    ChunkPosition b(1, 2);
    ChunkPosition c(2, 2);

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(ChunkPositionTest, Ordering) {
    ChunkPosition a(0, 0);
    ChunkPosition b(1, 0);
    ChunkPosition c(0, 1);

    EXPECT_TRUE(a < b);  // Same y, smaller x
    EXPECT_TRUE(a < c);  // Smaller y
    EXPECT_TRUE(b < c);  // Smaller y
}

TEST(ChunkPositionTest, Hash) {
    ChunkPositionHash hasher;
    ChunkPosition a(1, 2);
    ChunkPosition b(1, 2);
    ChunkPosition c(2, 1);

    EXPECT_EQ(hasher(a), hasher(b));
    EXPECT_NE(hasher(a), hasher(c));  // Different positions should (usually) have different hashes
}

// ============================================================================
// Chunk Tests
// ============================================================================

TEST(ChunkTest, DefaultConstruction) {
    Chunk chunk;
    EXPECT_EQ(chunk.getPosition().x, 0);
    EXPECT_EQ(chunk.getPosition().y, 0);
    EXPECT_TRUE(chunk.isEmpty());
    EXPECT_FALSE(chunk.isDirty());
}

TEST(ChunkTest, PositionConstruction) {
    ChunkPosition pos(5, -3);
    Chunk chunk(pos);

    EXPECT_EQ(chunk.getPosition(), pos);
    EXPECT_EQ(chunk.getWorldMinX(), 320);   // 5 * 64
    EXPECT_EQ(chunk.getWorldMinY(), -192);  // -3 * 64
    EXPECT_EQ(chunk.getWorldMaxX(), 384);   // 5 * 64 + 64
    EXPECT_EQ(chunk.getWorldMaxY(), -128);  // -3 * 64 + 64
}

TEST(ChunkTest, SetAndGetTile) {
    Chunk chunk;

    Tile tile;
    tile.id = 1;
    tile.variant = 2;
    tile.flags = Tile::FLAG_SOLID;

    EXPECT_TRUE(chunk.setTile(10, 20, tile));

    Tile retrieved = chunk.getTile(10, 20);
    EXPECT_EQ(retrieved.id, 1);
    EXPECT_EQ(retrieved.variant, 2);
    EXPECT_EQ(retrieved.flags, Tile::FLAG_SOLID);
    EXPECT_TRUE(retrieved.isSolid());
}

TEST(ChunkTest, SetTileId) {
    Chunk chunk;

    EXPECT_TRUE(chunk.setTileId(5, 5, 42, 1, Tile::FLAG_PLATFORM));

    Tile tile = chunk.getTile(5, 5);
    EXPECT_EQ(tile.id, 42);
    EXPECT_EQ(tile.variant, 1);
    EXPECT_EQ(tile.flags, Tile::FLAG_PLATFORM);
}

TEST(ChunkTest, OutOfBoundsAccess) {
    Chunk chunk;

    // Out of bounds get returns empty tile
    EXPECT_TRUE(chunk.getTile(-1, 0).isEmpty());
    EXPECT_TRUE(chunk.getTile(64, 0).isEmpty());
    EXPECT_TRUE(chunk.getTile(0, -1).isEmpty());
    EXPECT_TRUE(chunk.getTile(0, 64).isEmpty());

    // Out of bounds set returns false
    EXPECT_FALSE(chunk.setTileId(-1, 0, 1));
    EXPECT_FALSE(chunk.setTileId(64, 0, 1));
}

TEST(ChunkTest, Fill) {
    Chunk chunk;

    Tile fillTile;
    fillTile.id = 5;
    fillTile.flags = Tile::FLAG_SOLID;

    chunk.fill(fillTile);

    EXPECT_FALSE(chunk.isEmpty());
    EXPECT_EQ(chunk.countNonEmptyTiles(), CHUNK_TILE_COUNT);
    EXPECT_EQ(chunk.getTile(0, 0).id, 5);
    EXPECT_EQ(chunk.getTile(63, 63).id, 5);
}

TEST(ChunkTest, Clear) {
    Chunk chunk;
    chunk.setTileId(10, 10, 1);
    chunk.setTileId(20, 20, 2);

    EXPECT_FALSE(chunk.isEmpty());

    chunk.clear();

    EXPECT_TRUE(chunk.isEmpty());
    EXPECT_EQ(chunk.countNonEmptyTiles(), 0);
}

TEST(ChunkTest, DirtyFlags) {
    Chunk chunk;

    EXPECT_FALSE(chunk.isDirty());
    EXPECT_FALSE(chunk.isDirty(ChunkDirtyFlags::TileData));
    EXPECT_FALSE(chunk.isDirty(ChunkDirtyFlags::NeedsSave));

    // Setting a tile marks it dirty
    chunk.setTileId(0, 0, 1);

    EXPECT_TRUE(chunk.isDirty());
    EXPECT_TRUE(chunk.isDirty(ChunkDirtyFlags::TileData));
    EXPECT_TRUE(chunk.isDirty(ChunkDirtyFlags::NeedsSave));

    // Clear specific flag
    chunk.clearDirty(ChunkDirtyFlags::TileData);
    EXPECT_TRUE(chunk.isDirty(ChunkDirtyFlags::NeedsSave));
    EXPECT_FALSE(chunk.isDirty(ChunkDirtyFlags::TileData));

    // Clear all
    chunk.clearDirty();
    EXPECT_FALSE(chunk.isDirty());
}

TEST(ChunkTest, IndexConversion) {
    // Test index calculation
    EXPECT_EQ(Chunk::localToIndex(0, 0), 0);
    EXPECT_EQ(Chunk::localToIndex(63, 0), 63);
    EXPECT_EQ(Chunk::localToIndex(0, 1), 64);
    EXPECT_EQ(Chunk::localToIndex(63, 63), CHUNK_TILE_COUNT - 1);

    // Test reverse conversion
    EXPECT_EQ(Chunk::indexToLocalX(0), 0);
    EXPECT_EQ(Chunk::indexToLocalY(0), 0);
    EXPECT_EQ(Chunk::indexToLocalX(65), 1);
    EXPECT_EQ(Chunk::indexToLocalY(65), 1);
}

// ============================================================================
// Noise Tests
// ============================================================================

TEST(NoiseTest, Range) {
    // Test that noise values are in [0, 1] range
    uint64_t seed = 12345;

    for (int x = -100; x < 100; ++x) {
        float val = Noise::noise1D(x, seed);
        EXPECT_GE(val, 0.0f);
        EXPECT_LE(val, 1.0f);
    }

    for (int x = -50; x < 50; ++x) {
        for (int y = -50; y < 50; ++y) {
            float val = Noise::noise2D(x, y, seed);
            EXPECT_GE(val, 0.0f);
            EXPECT_LE(val, 1.0f);
        }
    }
}

TEST(NoiseTest, Deterministic) {
    uint64_t seed = 42;

    // Same input should always give same output
    float val1 = Noise::noise2D(100, 200, seed);
    float val2 = Noise::noise2D(100, 200, seed);
    EXPECT_EQ(val1, val2);

    // Different seed should give different output
    float val3 = Noise::noise2D(100, 200, seed + 1);
    EXPECT_NE(val1, val3);
}

TEST(NoiseTest, SmoothNoiseRange) {
    uint64_t seed = 99;

    for (float x = -10.0f; x < 10.0f; x += 0.1f) {
        float val = Noise::smoothNoise1D(x, seed);
        EXPECT_GE(val, 0.0f);
        EXPECT_LE(val, 1.0f);
    }
}

TEST(NoiseTest, FractalNoiseRange) {
    uint64_t seed = 12345;

    for (float x = -10.0f; x < 10.0f; x += 0.5f) {
        float val = Noise::fractalNoise1D(x, seed);
        EXPECT_GE(val, 0.0f);
        EXPECT_LE(val, 1.0f);
    }
}

// ============================================================================
// ChunkGenerator Tests
// ============================================================================

TEST(ChunkGeneratorTest, DefaultGeneration) {
    ChunkGenerator generator(12345);
    ChunkPosition pos(0, 1);  // Below surface
    Chunk chunk(pos);

    generator.generate(chunk);

    // Should have generated some terrain
    EXPECT_FALSE(chunk.isEmpty());
}

TEST(ChunkGeneratorTest, DeterministicGeneration) {
    uint64_t seed = 42;
    ChunkGenerator generator1(seed);
    ChunkGenerator generator2(seed);

    ChunkPosition pos(5, 2);
    Chunk chunk1(pos);
    Chunk chunk2(pos);

    generator1.generate(chunk1);
    generator2.generate(chunk2);

    // Same seed + position should produce identical chunks
    for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            EXPECT_EQ(chunk1.getTile(x, y).id, chunk2.getTile(x, y).id)
                << "Mismatch at (" << x << ", " << y << ")";
        }
    }
}

TEST(ChunkGeneratorTest, FlatGeneration) {
    Chunk chunk(ChunkPosition(0, 1));  // Position where surface=100 falls
    ChunkGenerator::flatGenerator(chunk, 0, 100);

    // Check surface tile at y=100
    // In chunk (0,1), worldY ranges from 64 to 127
    // Surface at y=100 means localY = 36
    Tile surfaceTile = chunk.getTile(0, 36);
    EXPECT_EQ(surfaceTile.id, 1);  // Grass

    // Above surface should be air
    Tile aboveTile = chunk.getTile(0, 35);
    EXPECT_EQ(aboveTile.id, 0);  // Air

    // Below surface should be dirt or stone
    Tile belowTile = chunk.getTile(0, 37);
    EXPECT_NE(belowTile.id, 0);  // Not air
}

TEST(ChunkGeneratorTest, EmptyGeneration) {
    Chunk chunk(ChunkPosition(0, 0));
    chunk.setTileId(0, 0, 1);  // Set a tile first

    ChunkGenerator::emptyGenerator(chunk, 0);

    EXPECT_TRUE(chunk.isEmpty());
}

TEST(ChunkGeneratorTest, CustomCallback) {
    ChunkGenerator generator(12345);

    // Set custom generator that fills with a specific tile
    generator.setGeneratorCallback([](Chunk& chunk, uint64_t seed) {
        (void)seed;
        Tile tile;
        tile.id = 99;
        chunk.fill(tile);
    });

    Chunk chunk(ChunkPosition(0, 0));
    generator.generate(chunk);

    EXPECT_EQ(chunk.getTile(0, 0).id, 99);
    EXPECT_EQ(chunk.getTile(63, 63).id, 99);
}

// ============================================================================
// ChunkManager Tests
// ============================================================================

TEST(ChunkManagerTest, LoadChunk) {
    ChunkManager manager;
    manager.init(12345);

    Chunk& chunk = manager.loadChunk(0, 0);

    EXPECT_TRUE(manager.isChunkLoadedAt(0, 0));
    EXPECT_EQ(chunk.getPosition().x, 0);
    EXPECT_EQ(chunk.getPosition().y, 0);
}

TEST(ChunkManagerTest, GetTile) {
    ChunkManager manager;
    manager.init(12345);

    // Force load chunk at origin
    manager.loadChunk(0, 0);

    // Get a tile - this should work since chunk is loaded
    Tile tile = manager.getTile(10, 20);
    // Tile ID depends on generation, just check it doesn't crash
    (void)tile;

    // Try getting a tile from unloaded chunk
    // This will load the chunk due to default behavior
    Tile tile2 = manager.getTile(1000, 1000);
    (void)tile2;
}

TEST(ChunkManagerTest, SetTile) {
    ChunkManager manager;
    manager.init(12345);

    // Set a tile
    EXPECT_TRUE(manager.setTileId(50, 50, 42, 1, Tile::FLAG_SOLID));

    // Verify it was set
    Tile tile = manager.getTile(50, 50);
    EXPECT_EQ(tile.id, 42);
    EXPECT_TRUE(tile.isSolid());
}

TEST(ChunkManagerTest, UnloadChunk) {
    ChunkManager manager;
    manager.init(12345);

    manager.loadChunk(0, 0);
    EXPECT_TRUE(manager.isChunkLoadedAt(0, 0));
    EXPECT_EQ(manager.getLoadedChunkCount(), 1);

    manager.unloadChunk(0, 0, false);
    EXPECT_FALSE(manager.isChunkLoadedAt(0, 0));
    EXPECT_EQ(manager.getLoadedChunkCount(), 0);
}

TEST(ChunkManagerTest, UpdateLoadsChunks) {
    ChunkManagerConfig config;
    config.loadRadiusChunks = 2;
    config.unloadRadiusChunks = 4;

    ChunkManager manager(config);
    manager.init(12345);

    // Update centered at origin
    manager.update(0.0f, 0.0f);

    // Should have loaded chunks in a 5x5 area (radius 2)
    // That's (2*2+1)^2 = 25 chunks
    EXPECT_EQ(manager.getLoadedChunkCount(), 25);

    // Check specific chunks are loaded
    EXPECT_TRUE(manager.isChunkLoadedAt(0, 0));
    EXPECT_TRUE(manager.isChunkLoadedAt(2, 2));
    EXPECT_TRUE(manager.isChunkLoadedAt(-2, -2));
}

TEST(ChunkManagerTest, UpdateUnloadsDistantChunks) {
    ChunkManagerConfig config;
    config.loadRadiusChunks = 1;
    config.unloadRadiusChunks = 2;

    ChunkManager manager(config);
    manager.init(12345);

    // Load at origin
    manager.update(0.0f, 0.0f);
    size_t initialCount = manager.getLoadedChunkCount();
    EXPECT_GT(initialCount, 0);

    // Move far away (in world coordinates, not chunk coordinates)
    float farX = 10.0f * CHUNK_SIZE * 16.0f;  // 10 chunks away
    manager.update(farX, 0.0f);

    // Old chunks should be unloaded, new ones loaded
    EXPECT_FALSE(manager.isChunkLoadedAt(0, 0));
}

TEST(ChunkManagerTest, GetChunksInRange) {
    ChunkManager manager;
    manager.init(12345);

    // Load some chunks
    manager.loadChunk(0, 0);
    manager.loadChunk(1, 0);
    manager.loadChunk(0, 1);
    manager.loadChunk(1, 1);

    // Get chunks in range that includes all of them
    auto chunks = manager.getChunksInRange(0, 127, 0, 127);
    EXPECT_EQ(chunks.size(), 4);
}

TEST(ChunkManagerTest, GetDirtyChunks) {
    ChunkManager manager;
    manager.init(12345);

    manager.loadChunk(0, 0);
    manager.loadChunk(1, 0);

    // Initially no dirty chunks (generation clears dirty flag)
    auto dirty = manager.getDirtyChunks();
    EXPECT_EQ(dirty.size(), 0);

    // Modify a tile
    manager.setTileId(10, 10, 99);

    dirty = manager.getDirtyChunks();
    EXPECT_EQ(dirty.size(), 1);
}

TEST(ChunkManagerTest, Statistics) {
    ChunkManager manager;
    manager.init(12345);

    manager.resetStats();
    const auto& stats = manager.getStats();

    EXPECT_EQ(stats.chunksGenerated, 0);

    manager.loadChunk(0, 0);
    manager.loadChunk(1, 1);

    EXPECT_EQ(stats.chunksGenerated, 2);
    EXPECT_EQ(stats.loadedChunks, 2);

    manager.unloadChunk(0, 0, false);
    EXPECT_EQ(stats.chunksUnloaded, 1);
}

// ============================================================================
// WorldFile Tests
// ============================================================================

class WorldFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDir = makeUniqueTestDir("test_world");
        std::filesystem::remove_all(testDir);
    }

    void TearDown() override {
        std::filesystem::remove_all(testDir);
    }

    std::string testDir;
};

TEST_F(WorldFileTest, CreateAndCheckExists) {
    WorldFile worldFile(testDir);

    EXPECT_FALSE(worldFile.worldExists());

    WorldMetadata meta;
    meta.name = "Test World";
    meta.seed = 42;

    FileResult result = worldFile.createWorld(meta);
    EXPECT_EQ(result, FileResult::Success);
    EXPECT_TRUE(worldFile.worldExists());
}

TEST_F(WorldFileTest, SaveAndLoadMetadata) {
    WorldFile worldFile(testDir);

    WorldMetadata meta;
    meta.name = "My Test World";
    meta.seed = 123456;
    meta.spawnX = 100.5f;
    meta.spawnY = 200.5f;
    meta.tilesPlaced = 42;

    EXPECT_EQ(worldFile.createWorld(meta), FileResult::Success);

    // Load metadata back
    WorldMetadata loaded;
    EXPECT_EQ(worldFile.loadMetadata(loaded), FileResult::Success);

    EXPECT_EQ(loaded.name, "My Test World");
    EXPECT_EQ(loaded.seed, 123456);
    EXPECT_FLOAT_EQ(loaded.spawnX, 100.5f);
    EXPECT_FLOAT_EQ(loaded.spawnY, 200.5f);
    EXPECT_EQ(loaded.tilesPlaced, 42);
}

TEST_F(WorldFileTest, SaveAndLoadChunk) {
    WorldFile worldFile(testDir);

    WorldMetadata meta;
    meta.name = "Chunk Test";
    EXPECT_EQ(worldFile.createWorld(meta), FileResult::Success);

    // Create and populate a chunk
    ChunkPosition pos(5, -3);
    Chunk chunk(pos);
    chunk.setTileId(10, 20, 42, 1, Tile::FLAG_SOLID);
    chunk.setTileId(30, 40, 99, 2, Tile::FLAG_PLATFORM);

    // Save it
    EXPECT_EQ(worldFile.saveChunk(chunk), FileResult::Success);
    EXPECT_TRUE(worldFile.chunkExists(5, -3));

    // Load it back
    Chunk loadedChunk(pos);
    EXPECT_EQ(worldFile.loadChunk(loadedChunk), FileResult::Success);

    // Verify tiles
    Tile tile1 = loadedChunk.getTile(10, 20);
    EXPECT_EQ(tile1.id, 42);
    EXPECT_EQ(tile1.variant, 1);
    EXPECT_TRUE(tile1.isSolid());

    Tile tile2 = loadedChunk.getTile(30, 40);
    EXPECT_EQ(tile2.id, 99);
    EXPECT_EQ(tile2.variant, 2);
}

TEST_F(WorldFileTest, GetSavedChunkPositions) {
    WorldFile worldFile(testDir);

    WorldMetadata meta;
    EXPECT_EQ(worldFile.createWorld(meta), FileResult::Success);

    // Save some chunks
    Chunk c1(ChunkPosition(0, 0));
    Chunk c2(ChunkPosition(1, 2));
    Chunk c3(ChunkPosition(-5, 10));

    worldFile.saveChunk(c1);
    worldFile.saveChunk(c2);
    worldFile.saveChunk(c3);

    auto positions = worldFile.getSavedChunkPositions();
    EXPECT_EQ(positions.size(), 3);
}

TEST_F(WorldFileTest, DeleteChunk) {
    WorldFile worldFile(testDir);

    WorldMetadata meta;
    worldFile.createWorld(meta);

    Chunk chunk(ChunkPosition(1, 1));
    worldFile.saveChunk(chunk);

    EXPECT_TRUE(worldFile.chunkExists(1, 1));
    EXPECT_TRUE(worldFile.deleteChunk(1, 1));
    EXPECT_FALSE(worldFile.chunkExists(1, 1));
}

TEST_F(WorldFileTest, DeleteWorld) {
    WorldFile worldFile(testDir);

    WorldMetadata meta;
    worldFile.createWorld(meta);
    EXPECT_TRUE(worldFile.worldExists());

    EXPECT_EQ(worldFile.deleteWorld(), FileResult::Success);
    EXPECT_FALSE(worldFile.worldExists());
}

TEST_F(WorldFileTest, LoadNonExistent) {
    WorldFile worldFile(testDir);

    WorldMetadata meta;
    EXPECT_EQ(worldFile.loadMetadata(meta), FileResult::FileNotFound);

    Chunk chunk(ChunkPosition(0, 0));
    EXPECT_EQ(worldFile.loadChunk(chunk), FileResult::FileNotFound);
}

// ============================================================================
// TileMap Tests
// ============================================================================

class TileMapTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDir = makeUniqueTestDir("test_tilemap");
        std::filesystem::remove_all(testDir);
    }

    void TearDown() override {
        std::filesystem::remove_all(testDir);
    }

    std::string testDir;
};

TEST_F(TileMapTest, CreateWorld) {
    TileMap tileMap;

    EXPECT_FALSE(tileMap.isWorldLoaded());

    bool created = tileMap.createWorld(testDir, "Test World", 12345);
    EXPECT_TRUE(created);
    EXPECT_TRUE(tileMap.isWorldLoaded());
    EXPECT_EQ(tileMap.getSeed(), 12345);
}

TEST_F(TileMapTest, LoadWorld) {
    // First create a world
    {
        TileMap tileMap;
        tileMap.createWorld(testDir, "Test World", 42);
        tileMap.setSpawnPoint(100.0f, 200.0f);
        tileMap.saveWorld();
        tileMap.closeWorld();
    }

    // Then load it
    {
        TileMap tileMap;
        EXPECT_TRUE(tileMap.loadWorld(testDir));
        EXPECT_EQ(tileMap.getSeed(), 42);

        Vec2 spawn = tileMap.getSpawnPoint();
        EXPECT_FLOAT_EQ(spawn.x, 100.0f);
        EXPECT_FLOAT_EQ(spawn.y, 200.0f);
    }
}

TEST_F(TileMapTest, SetAndGetTile) {
    TileMap tileMap;
    tileMap.createWorld(testDir, "Test", 0);

    EXPECT_TRUE(tileMap.setTileId(50, 50, 42));

    Tile tile = tileMap.getTile(50, 50);
    EXPECT_EQ(tile.id, 42);
}

TEST_F(TileMapTest, TilePersistence) {
    // Create world and set some tiles
    {
        TileMap tileMap;
        tileMap.createWorld(testDir, "Test", 0);
        tileMap.setTileId(100, 100, 77);
        tileMap.setTileId(-50, -50, 88);
        tileMap.saveWorld();
        tileMap.closeWorld();
    }

    // Load and verify
    {
        TileMap tileMap;
        tileMap.loadWorld(testDir);

        // Need to update to load chunks
        tileMap.update(100.0f, 100.0f);
        EXPECT_EQ(tileMap.getTile(100, 100).id, 77);

        tileMap.update(-50.0f, -50.0f);
        EXPECT_EQ(tileMap.getTile(-50, -50).id, 88);
    }
}

TEST_F(TileMapTest, CoordinateConversion) {
    TileMapConfig config;
    config.tileSize = 16;
    TileMap tileMap(config);

    int tileX, tileY;
    tileMap.worldToTile(32.0f, 48.0f, tileX, tileY);
    EXPECT_EQ(tileX, 2);
    EXPECT_EQ(tileY, 3);

    float worldX, worldY;
    tileMap.tileToWorld(2, 3, worldX, worldY);
    EXPECT_FLOAT_EQ(worldX, 32.0f);
    EXPECT_FLOAT_EQ(worldY, 48.0f);
}

TEST_F(TileMapTest, NegativeCoordinateConversion) {
    TileMapConfig config;
    config.tileSize = 16;
    TileMap tileMap(config);

    int tileX, tileY;
    tileMap.worldToTile(-32.0f, -48.0f, tileX, tileY);
    EXPECT_EQ(tileX, -2);
    EXPECT_EQ(tileY, -3);
}

TEST_F(TileMapTest, IsSolid) {
    TileMap tileMap;
    tileMap.createWorld(testDir, "Test", 0);

    // Set a solid tile
    tileMap.setTileId(10, 10, 1, 0, Tile::FLAG_SOLID);
    EXPECT_TRUE(tileMap.isSolid(10, 10));

    // Set a non-solid tile
    tileMap.setTileId(20, 20, 1, 0, 0);
    EXPECT_FALSE(tileMap.isSolid(20, 20));
}

TEST_F(TileMapTest, IsEmpty) {
    TileMap tileMap;
    tileMap.createWorld(testDir, "Test", 0);

    // Clear a spot
    tileMap.setTileId(10, 10, 0);  // Air
    EXPECT_TRUE(tileMap.isEmpty(10, 10));

    // Set a tile
    tileMap.setTileId(10, 10, 1);
    EXPECT_FALSE(tileMap.isEmpty(10, 10));
}

TEST_F(TileMapTest, GetTileCallback) {
    TileMap tileMap;
    tileMap.createWorld(testDir, "Test", 0);
    tileMap.setTileId(5, 5, 42);

    auto callback = tileMap.getTileCallback();
    Tile tile = callback(5, 5);
    EXPECT_EQ(tile.id, 42);
}

TEST_F(TileMapTest, CustomGenerator) {
    TileMap tileMap;
    tileMap.createWorld(testDir, "Test", 0);

    // Set custom generator
    tileMap.setGeneratorCallback([](Chunk& chunk, uint64_t seed) {
        (void)seed;
        // Fill with a specific pattern
        for (int y = 0; y < CHUNK_SIZE; ++y) {
            for (int x = 0; x < CHUNK_SIZE; ++x) {
                if ((x + y) % 2 == 0) {
                    chunk.setTileId(x, y, 1);
                }
            }
        }
    });

    // Load a new chunk (will use custom generator)
    tileMap.update(1000.0f, 1000.0f);

    // Check pattern (converting world coords to chunk coords)
    // At world position (1000, 1000) with tile size 16:
    // Tile (62, 62) should be in a checkerboard pattern
    int tileX = 62;
    int tileY = 62;
    if ((tileX + tileY) % 2 == 0) {
        EXPECT_EQ(tileMap.getTile(tileX, tileY).id, 1);
    } else {
        EXPECT_TRUE(tileMap.getTile(tileX, tileY).isEmpty());
    }
}

TEST_F(TileMapTest, CloseWithoutLoad) {
    TileMap tileMap;
    // Should not crash
    tileMap.closeWorld();
    EXPECT_FALSE(tileMap.isWorldLoaded());
}

TEST_F(TileMapTest, OperationsWithoutWorld) {
    TileMap tileMap;

    // Operations without loaded world should return safe defaults
    EXPECT_TRUE(tileMap.getTile(0, 0).isEmpty());
    EXPECT_FALSE(tileMap.setTileId(0, 0, 1));
    EXPECT_FALSE(tileMap.isSolid(0, 0));
    EXPECT_TRUE(tileMap.isEmpty(0, 0));
}
