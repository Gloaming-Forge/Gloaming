#include <gtest/gtest.h>
#include "lighting/LightMap.hpp"
#include "lighting/DayNightCycle.hpp"
#include "lighting/LightingSystem.hpp"

using namespace gloaming;

// ============================================================================
// TileLight Tests
// ============================================================================

TEST(TileLightTest, DefaultConstruction) {
    TileLight light;
    EXPECT_EQ(light.r, 0);
    EXPECT_EQ(light.g, 0);
    EXPECT_EQ(light.b, 0);
    EXPECT_TRUE(light.isDark());
}

TEST(TileLightTest, ValueConstruction) {
    TileLight light(100, 200, 50);
    EXPECT_EQ(light.r, 100);
    EXPECT_EQ(light.g, 200);
    EXPECT_EQ(light.b, 50);
    EXPECT_FALSE(light.isDark());
}

TEST(TileLightTest, MaxChannel) {
    TileLight light(50, 200, 100);
    EXPECT_EQ(light.maxChannel(), 200);

    TileLight dark;
    EXPECT_EQ(dark.maxChannel(), 0);
}

TEST(TileLightTest, ComponentWiseMax) {
    TileLight a(100, 50, 200);
    TileLight b(50, 150, 100);
    TileLight result = TileLight::max(a, b);
    EXPECT_EQ(result.r, 100);
    EXPECT_EQ(result.g, 150);
    EXPECT_EQ(result.b, 200);
}

TEST(TileLightTest, Equality) {
    TileLight a(100, 200, 50);
    TileLight b(100, 200, 50);
    TileLight c(100, 200, 51);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

// ============================================================================
// ChunkLightData Tests
// ============================================================================

TEST(ChunkLightDataTest, DefaultIsAllDark) {
    ChunkLightData data;
    for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            EXPECT_TRUE(data.getLight(x, y).isDark());
        }
    }
}

TEST(ChunkLightDataTest, SetAndGet) {
    ChunkLightData data;
    TileLight light(255, 128, 64);
    data.setLight(10, 20, light);

    TileLight result = data.getLight(10, 20);
    EXPECT_EQ(result.r, 255);
    EXPECT_EQ(result.g, 128);
    EXPECT_EQ(result.b, 64);

    // Other tiles still dark
    EXPECT_TRUE(data.getLight(0, 0).isDark());
}

TEST(ChunkLightDataTest, OutOfBoundsReturnsDark) {
    ChunkLightData data;
    data.setLight(0, 0, TileLight(255, 255, 255));

    // Out of bounds reads return dark
    EXPECT_TRUE(data.getLight(-1, 0).isDark());
    EXPECT_TRUE(data.getLight(0, -1).isDark());
    EXPECT_TRUE(data.getLight(CHUNK_SIZE, 0).isDark());
    EXPECT_TRUE(data.getLight(0, CHUNK_SIZE).isDark());
}

TEST(ChunkLightDataTest, Clear) {
    ChunkLightData data;
    data.setLight(10, 10, TileLight(255, 255, 255));
    EXPECT_FALSE(data.getLight(10, 10).isDark());

    data.clear();
    EXPECT_TRUE(data.getLight(10, 10).isDark());
}

// ============================================================================
// LightMap Tests
// ============================================================================

TEST(LightMapTest, AddAndRemoveChunk) {
    LightMap map;
    ChunkPosition pos(0, 0);

    EXPECT_FALSE(map.hasChunk(pos));
    EXPECT_EQ(map.getChunkCount(), 0u);

    map.addChunk(pos);
    EXPECT_TRUE(map.hasChunk(pos));
    EXPECT_EQ(map.getChunkCount(), 1u);

    map.removeChunk(pos);
    EXPECT_FALSE(map.hasChunk(pos));
    EXPECT_EQ(map.getChunkCount(), 0u);
}

TEST(LightMapTest, SetAndGetLight) {
    LightMap map;
    map.addChunk(ChunkPosition(0, 0));

    TileLight light(200, 100, 50);
    map.setLight(10, 20, light);

    TileLight result = map.getLight(10, 20);
    EXPECT_EQ(result.r, 200);
    EXPECT_EQ(result.g, 100);
    EXPECT_EQ(result.b, 50);
}

TEST(LightMapTest, GetLightUnloadedChunkReturnsDark) {
    LightMap map;
    // No chunks loaded
    TileLight result = map.getLight(100, 200);
    EXPECT_TRUE(result.isDark());
}

TEST(LightMapTest, SetLightUnloadedChunkIgnored) {
    LightMap map;
    // Should not crash
    map.setLight(100, 200, TileLight(255, 255, 255));
    EXPECT_TRUE(map.getLight(100, 200).isDark());
}

TEST(LightMapTest, MultipleChunks) {
    LightMap map;
    map.addChunk(ChunkPosition(0, 0));
    map.addChunk(ChunkPosition(1, 0));
    map.addChunk(ChunkPosition(0, 1));

    // Set light in each chunk
    map.setLight(10, 10, TileLight(100, 0, 0));    // Chunk (0,0)
    map.setLight(70, 10, TileLight(0, 100, 0));    // Chunk (1,0)
    map.setLight(10, 70, TileLight(0, 0, 100));    // Chunk (0,1)

    EXPECT_EQ(map.getLight(10, 10).r, 100);
    EXPECT_EQ(map.getLight(70, 10).g, 100);
    EXPECT_EQ(map.getLight(10, 70).b, 100);
}

TEST(LightMapTest, ClearAll) {
    LightMap map;
    map.addChunk(ChunkPosition(0, 0));
    map.setLight(10, 10, TileLight(255, 255, 255));

    map.clearAll();
    EXPECT_TRUE(map.getLight(10, 10).isDark());
    // Chunks still exist
    EXPECT_TRUE(map.hasChunk(ChunkPosition(0, 0)));
}

TEST(LightMapTest, ClearChunk) {
    LightMap map;
    map.addChunk(ChunkPosition(0, 0));
    map.addChunk(ChunkPosition(1, 0));

    map.setLight(10, 10, TileLight(255, 255, 255));
    map.setLight(70, 10, TileLight(255, 255, 255));

    map.clearChunk(ChunkPosition(0, 0));
    EXPECT_TRUE(map.getLight(10, 10).isDark());
    EXPECT_FALSE(map.getLight(70, 10).isDark()); // Other chunk unaffected
}

TEST(LightMapTest, CornerLightInterpolation) {
    LightMap map;
    map.addChunk(ChunkPosition(0, 0));

    // Set 4 tiles around a corner to known values
    map.setLight(4, 4, TileLight(100, 100, 100));
    map.setLight(5, 4, TileLight(200, 200, 200));
    map.setLight(4, 5, TileLight(0, 0, 0));
    map.setLight(5, 5, TileLight(100, 100, 100));

    // Corner at (5, 5) is the average of tiles (4,4), (5,4), (4,5), (5,5)
    TileLight corner = map.getCornerLight(5, 5);
    EXPECT_EQ(corner.r, 100);  // (100+200+0+100)/4 = 100
    EXPECT_EQ(corner.g, 100);
    EXPECT_EQ(corner.b, 100);
}

TEST(LightMapTest, WorldRange) {
    LightMap map;
    map.addChunk(ChunkPosition(0, 0));
    map.addChunk(ChunkPosition(1, 0));
    map.addChunk(ChunkPosition(0, 1));

    int minX, maxX, minY, maxY;
    map.getWorldRange(minX, maxX, minY, maxY);

    EXPECT_EQ(minX, 0);
    EXPECT_EQ(maxX, 128);  // chunk(1,0) + CHUNK_SIZE
    EXPECT_EQ(minY, 0);
    EXPECT_EQ(maxY, 128);  // chunk(0,1) + CHUNK_SIZE
}

TEST(LightMapTest, NegativeCoordinateChunks) {
    LightMap map;
    map.addChunk(ChunkPosition(-1, -1));

    // Tile at world (-1, -1) should be in chunk (-1, -1), local (63, 63)
    map.setLight(-1, -1, TileLight(128, 64, 32));
    TileLight result = map.getLight(-1, -1);
    EXPECT_EQ(result.r, 128);
    EXPECT_EQ(result.g, 64);
    EXPECT_EQ(result.b, 32);
}

// ============================================================================
// Light Propagation Tests
// ============================================================================

TEST(LightPropagationTest, SingleSourceSpreads) {
    LightingConfig cfg;
    cfg.lightFalloff = 50;   // Large falloff for simple testing
    cfg.maxLightRadius = 16;

    LightMap map(cfg);
    map.addChunk(ChunkPosition(0, 0));

    auto isSolid = [](int, int) { return false; };

    TileLightSource source(32, 32, TileLight(255, 255, 255));
    map.propagateLight(source, isSolid);

    // Source tile should be fully lit
    TileLight at_source = map.getLight(32, 32);
    EXPECT_EQ(at_source.r, 255);
    EXPECT_EQ(at_source.g, 255);
    EXPECT_EQ(at_source.b, 255);

    // Adjacent tile should be dimmer by falloff
    TileLight neighbor = map.getLight(33, 32);
    EXPECT_EQ(neighbor.r, 205);  // 255 - 50

    // 2 tiles away should be dimmer
    TileLight far = map.getLight(34, 32);
    EXPECT_EQ(far.r, 155);  // 255 - 100

    // 5 tiles away: 255 - 250 = 5
    TileLight five_away = map.getLight(37, 32);
    EXPECT_EQ(five_away.r, 5);  // 255 - 250

    // 6 tiles away: fully attenuated
    TileLight very_far = map.getLight(38, 32);
    EXPECT_EQ(very_far.r, 0);  // 255 - 300 = 0 (clamped)
}

TEST(LightPropagationTest, SolidBlocksLight) {
    LightingConfig cfg;
    cfg.lightFalloff = 30;
    cfg.maxLightRadius = 16;

    LightMap map(cfg);
    map.addChunk(ChunkPosition(0, 0));

    // Wall at tile (33, 32) - right next to source
    auto isSolid = [](int x, int y) { return x == 33 && y == 32; };

    TileLightSource source(32, 32, TileLight(255, 0, 0));
    map.propagateLight(source, isSolid);

    // Source lit
    EXPECT_EQ(map.getLight(32, 32).r, 255);

    // Neighbor in open direction still lit
    EXPECT_GT(map.getLight(31, 32).r, 0);

    // Wall tile gets much less light (3x falloff for solid)
    TileLight wallLight = map.getLight(33, 32);
    EXPECT_EQ(wallLight.r, 165);  // 255 - 90 (30 * 3)
}

TEST(LightPropagationTest, ColoredLightPropagation) {
    LightingConfig cfg;
    cfg.lightFalloff = 50;

    LightMap map(cfg);
    map.addChunk(ChunkPosition(0, 0));

    auto isSolid = [](int, int) { return false; };

    // Red light source
    TileLightSource source(32, 32, TileLight(255, 0, 0));
    map.propagateLight(source, isSolid);

    // Red channel propagates
    EXPECT_EQ(map.getLight(33, 32).r, 205);
    // Green and blue stay at 0 (no source)
    EXPECT_EQ(map.getLight(33, 32).g, 0);
    EXPECT_EQ(map.getLight(33, 32).b, 0);
}

TEST(LightPropagationTest, MultipleLightSourcesCombine) {
    LightingConfig cfg;
    cfg.lightFalloff = 50;

    LightMap map(cfg);
    map.addChunk(ChunkPosition(0, 0));

    auto isSolid = [](int, int) { return false; };

    // Red light on left
    map.propagateLight(TileLightSource(30, 32, TileLight(255, 0, 0)), isSolid);
    // Blue light on right
    map.propagateLight(TileLightSource(34, 32, TileLight(0, 0, 255)), isSolid);

    // Center tile should have both red and blue
    TileLight center = map.getLight(32, 32);
    EXPECT_GT(center.r, 0);
    EXPECT_GT(center.b, 0);
}

TEST(LightPropagationTest, DarkSourceDoesNothing) {
    LightMap map;
    map.addChunk(ChunkPosition(0, 0));

    auto isSolid = [](int, int) { return false; };

    TileLightSource source(32, 32, TileLight(0, 0, 0));
    map.propagateLight(source, isSolid);

    // Everything should remain dark
    EXPECT_TRUE(map.getLight(32, 32).isDark());
    EXPECT_TRUE(map.getLight(33, 32).isDark());
}

// ============================================================================
// Skylight Tests
// ============================================================================

TEST(SkylightTest, AboveSurfaceFullyLit) {
    LightingConfig cfg;
    cfg.enableSkylight = true;
    cfg.skylightFalloff = 20;
    cfg.maxLightRadius = 16;

    LightMap map(cfg);
    map.addChunk(ChunkPosition(0, 0));

    // Surface at y=32
    auto getSurfaceY = [](int) { return 32; };
    auto isSolid = [](int, int y) { return y >= 32; };

    TileLight skyColor(200, 200, 255);
    map.propagateSkylight(0, CHUNK_SIZE, getSurfaceY, isSolid, skyColor);

    // Above surface should be lit
    TileLight above = map.getLight(10, 20);
    EXPECT_EQ(above.r, 200);
    EXPECT_EQ(above.g, 200);
    EXPECT_EQ(above.b, 255);
}

TEST(SkylightTest, BelowSurfaceDimmer) {
    LightingConfig cfg;
    cfg.enableSkylight = true;
    cfg.skylightFalloff = 20;
    cfg.maxLightRadius = 16;
    cfg.lightFalloff = 20;

    LightMap map(cfg);
    map.addChunk(ChunkPosition(0, 0));

    auto getSurfaceY = [](int) { return 32; };
    auto isSolid = [](int, int y) { return y >= 32; };

    TileLight skyColor(200, 200, 200);
    map.propagateSkylight(0, CHUNK_SIZE, getSurfaceY, isSolid, skyColor);

    // At surface - lit (solid tile gets 2x falloff)
    TileLight atSurface = map.getLight(10, 32);
    EXPECT_GT(atSurface.maxChannel(), 0);

    // Below surface should be dimmer
    TileLight below1 = map.getLight(10, 33);
    TileLight below2 = map.getLight(10, 34);
    EXPECT_LE(below1.maxChannel(), atSurface.maxChannel());
    EXPECT_LE(below2.maxChannel(), below1.maxChannel());
}

TEST(SkylightTest, DisabledSkylightDoesNothing) {
    LightingConfig cfg;
    cfg.enableSkylight = false;

    LightMap map(cfg);
    map.addChunk(ChunkPosition(0, 0));

    auto getSurfaceY = [](int) { return 32; };
    auto isSolid = [](int, int) { return false; };

    map.propagateSkylight(0, CHUNK_SIZE, getSurfaceY, isSolid, TileLight(255, 255, 255));

    // Everything should remain dark
    EXPECT_TRUE(map.getLight(10, 10).isDark());
}

// ============================================================================
// Full Recalculation Tests
// ============================================================================

TEST(RecalcTest, EmptyWorldStaysDark) {
    LightMap map;
    map.addChunk(ChunkPosition(0, 0));

    auto isSolid = [](int, int) { return false; };
    auto getSurfaceY = [](int) { return 64; }; // Below all loaded tiles (no surface)

    std::vector<TileLightSource> sources;
    map.recalculateAll(sources, isSolid, getSurfaceY, TileLight(0, 0, 0));

    // With no sky and no sources, everything dark
    EXPECT_TRUE(map.getLight(32, 32).isDark());
}

TEST(RecalcTest, SourceAndSkyCombinable) {
    LightingConfig cfg;
    cfg.enableSkylight = true;
    cfg.skylightFalloff = 10;
    cfg.lightFalloff = 16;
    cfg.maxLightRadius = 16;

    LightMap map(cfg);
    map.addChunk(ChunkPosition(0, 0));

    auto isSolid = [](int, int y) { return y >= 32; };
    auto getSurfaceY = [](int) { return 32; };

    // A torch underground
    std::vector<TileLightSource> sources;
    sources.emplace_back(10, 40, TileLight(255, 200, 100));

    map.recalculateAll(sources, isSolid, getSurfaceY, TileLight(200, 200, 255));

    // Above surface: sky lit
    EXPECT_GT(map.getLight(10, 20).maxChannel(), 0);

    // At torch: brightly lit
    TileLight atTorch = map.getLight(10, 40);
    EXPECT_GT(atTorch.r, 200);
}

// ============================================================================
// DayNightCycle Tests
// ============================================================================

TEST(DayNightTest, DefaultStartsAtZero) {
    DayNightCycle cycle;
    EXPECT_FLOAT_EQ(cycle.getTime(), 0.0f);
    EXPECT_FLOAT_EQ(cycle.getNormalizedTime(), 0.0f);
    EXPECT_EQ(cycle.getDayCount(), 0);
}

TEST(DayNightTest, TimeAdvances) {
    DayNightConfig cfg;
    cfg.dayDurationSeconds = 100.0f;
    DayNightCycle cycle(cfg);

    cycle.update(10.0f);
    EXPECT_NEAR(cycle.getNormalizedTime(), 0.1f, 0.001f);
    EXPECT_NEAR(cycle.getTime(), 10.0f, 0.001f);
}

TEST(DayNightTest, DayRollover) {
    DayNightConfig cfg;
    cfg.dayDurationSeconds = 100.0f;
    DayNightCycle cycle(cfg);

    cycle.update(105.0f);
    EXPECT_EQ(cycle.getDayCount(), 1);
    EXPECT_NEAR(cycle.getTime(), 5.0f, 0.001f);
}

TEST(DayNightTest, MultipleDays) {
    DayNightConfig cfg;
    cfg.dayDurationSeconds = 100.0f;
    DayNightCycle cycle(cfg);

    cycle.update(350.0f);
    EXPECT_EQ(cycle.getDayCount(), 3);
    EXPECT_NEAR(cycle.getTime(), 50.0f, 0.001f);
}

TEST(DayNightTest, TimeOfDayPhases) {
    DayNightConfig cfg;
    cfg.dayDurationSeconds = 100.0f;
    cfg.dawnStart = 0.2f;
    cfg.dayStart = 0.3f;
    cfg.duskStart = 0.7f;
    cfg.nightStart = 0.8f;
    DayNightCycle cycle(cfg);

    // Night at start (0.0)
    EXPECT_EQ(cycle.getTimeOfDay(), TimeOfDay::Night);
    EXPECT_TRUE(cycle.isNight());

    // Dawn at 0.25
    cycle.setNormalizedTime(0.25f);
    EXPECT_EQ(cycle.getTimeOfDay(), TimeOfDay::Dawn);

    // Day at 0.5
    cycle.setNormalizedTime(0.5f);
    EXPECT_EQ(cycle.getTimeOfDay(), TimeOfDay::Day);
    EXPECT_TRUE(cycle.isDay());

    // Dusk at 0.75
    cycle.setNormalizedTime(0.75f);
    EXPECT_EQ(cycle.getTimeOfDay(), TimeOfDay::Dusk);

    // Night at 0.9
    cycle.setNormalizedTime(0.9f);
    EXPECT_EQ(cycle.getTimeOfDay(), TimeOfDay::Night);
}

TEST(DayNightTest, SkyColorAtDay) {
    DayNightConfig cfg;
    cfg.dayDurationSeconds = 100.0f;
    cfg.dayColor = TileLight(255, 255, 240);
    DayNightCycle cycle(cfg);

    cycle.setNormalizedTime(0.5f); // Middle of day
    TileLight sky = cycle.getSkyColor();
    EXPECT_EQ(sky.r, cfg.dayColor.r);
    EXPECT_EQ(sky.g, cfg.dayColor.g);
    EXPECT_EQ(sky.b, cfg.dayColor.b);
}

TEST(DayNightTest, SkyColorAtNight) {
    DayNightConfig cfg;
    cfg.dayDurationSeconds = 100.0f;
    cfg.nightColor = TileLight(20, 20, 50);
    DayNightCycle cycle(cfg);

    cycle.setNormalizedTime(0.0f); // Start of cycle = night
    TileLight sky = cycle.getSkyColor();
    EXPECT_EQ(sky.r, cfg.nightColor.r);
    EXPECT_EQ(sky.g, cfg.nightColor.g);
    EXPECT_EQ(sky.b, cfg.nightColor.b);
}

TEST(DayNightTest, SkyBrightness) {
    DayNightConfig cfg;
    cfg.dayDurationSeconds = 100.0f;
    cfg.dayColor = TileLight(255, 255, 240);
    cfg.nightColor = TileLight(20, 20, 50);
    DayNightCycle cycle(cfg);

    // Day: brightness should be near 1.0
    cycle.setNormalizedTime(0.5f);
    EXPECT_GT(cycle.getSkyBrightness(), 0.9f);

    // Night: brightness should be low
    cycle.setNormalizedTime(0.0f);
    EXPECT_LT(cycle.getSkyBrightness(), 0.25f);
}

TEST(DayNightTest, SetNormalizedTime) {
    DayNightConfig cfg;
    cfg.dayDurationSeconds = 200.0f;
    DayNightCycle cycle(cfg);

    cycle.setNormalizedTime(0.5f);
    EXPECT_NEAR(cycle.getTime(), 100.0f, 0.1f);
    EXPECT_NEAR(cycle.getNormalizedTime(), 0.5f, 0.001f);
}

TEST(DayNightTest, SetTime) {
    DayNightConfig cfg;
    cfg.dayDurationSeconds = 200.0f;
    DayNightCycle cycle(cfg);

    cycle.setTime(150.0f);
    EXPECT_NEAR(cycle.getTime(), 150.0f, 0.1f);

    // Wraps around
    cycle.setTime(250.0f);
    EXPECT_NEAR(cycle.getTime(), 50.0f, 0.1f);
}

TEST(DayNightTest, SkyColorTransitions) {
    DayNightConfig cfg;
    cfg.dayDurationSeconds = 100.0f;
    cfg.dawnStart = 0.2f;
    cfg.dayStart = 0.3f;
    cfg.nightColor = TileLight(20, 20, 50);
    cfg.dawnColor = TileLight(200, 150, 100);
    cfg.dayColor = TileLight(255, 255, 240);
    DayNightCycle cycle(cfg);

    // During dawn transition, sky color should be between night and day
    cycle.setNormalizedTime(0.25f);
    TileLight dawnSky = cycle.getSkyColor();
    EXPECT_GT(dawnSky.r, cfg.nightColor.r);
    EXPECT_LT(dawnSky.r, cfg.dayColor.r);
}

// ============================================================================
// LightingConfig Tests
// ============================================================================

TEST(LightingConfigTest, Defaults) {
    LightingConfig cfg;
    EXPECT_EQ(cfg.lightFalloff, 16);
    EXPECT_EQ(cfg.skylightFalloff, 10);
    EXPECT_EQ(cfg.maxLightRadius, 16);
    EXPECT_EQ(cfg.maxLightLevel, 255);
    EXPECT_TRUE(cfg.enableSkylight);
    EXPECT_TRUE(cfg.enableSmoothLighting);
}

TEST(LightingSystemConfigTest, Defaults) {
    LightingSystemConfig cfg;
    EXPECT_TRUE(cfg.enabled);
    EXPECT_FLOAT_EQ(cfg.recalcInterval, 0.1f);
    EXPECT_EQ(cfg.visiblePaddingTiles, 4);
}

// ============================================================================
// TileLightSource Tests
// ============================================================================

TEST(TileLightSourceTest, Construction) {
    TileLightSource src(10, 20, TileLight(255, 128, 64));
    EXPECT_EQ(src.worldX, 10);
    EXPECT_EQ(src.worldY, 20);
    EXPECT_EQ(src.color.r, 255);
    EXPECT_EQ(src.color.g, 128);
    EXPECT_EQ(src.color.b, 64);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(LightMapEdgeTest, PropagateAcrossChunkBoundary) {
    LightingConfig cfg;
    cfg.lightFalloff = 30;

    LightMap map(cfg);
    map.addChunk(ChunkPosition(0, 0));
    map.addChunk(ChunkPosition(1, 0));

    auto isSolid = [](int, int) { return false; };

    // Place light near right edge of chunk 0
    TileLightSource source(62, 10, TileLight(255, 255, 255));
    map.propagateLight(source, isSolid);

    // Light should propagate into chunk 1
    TileLight inChunk1 = map.getLight(64, 10);  // First tile in chunk 1
    EXPECT_GT(inChunk1.r, 0);
}

TEST(LightMapEdgeTest, EmptyMapGetWorldRange) {
    LightMap map;
    int minX, maxX, minY, maxY;
    map.getWorldRange(minX, maxX, minY, maxY);
    EXPECT_EQ(minX, 0);
    EXPECT_EQ(maxX, 0);
    EXPECT_EQ(minY, 0);
    EXPECT_EQ(maxY, 0);
}

TEST(LightMapEdgeTest, GetChunkData) {
    LightMap map;
    ChunkPosition pos(0, 0);

    EXPECT_EQ(map.getChunkData(pos), nullptr);

    map.addChunk(pos);
    EXPECT_NE(map.getChunkData(pos), nullptr);

    const LightMap& constMap = map;
    EXPECT_NE(constMap.getChunkData(pos), nullptr);
}

TEST(LightMapEdgeTest, DuplicateAddChunk) {
    LightMap map;
    ChunkPosition pos(0, 0);

    map.addChunk(pos);
    map.setLight(10, 10, TileLight(100, 100, 100));

    // Adding again should not clear existing data
    map.addChunk(pos);
    EXPECT_EQ(map.getLight(10, 10).r, 100);
    EXPECT_EQ(map.getChunkCount(), 1u);
}

TEST(LightMapEdgeTest, RemoveNonexistentChunk) {
    LightMap map;
    // Should not crash
    map.removeChunk(ChunkPosition(99, 99));
    EXPECT_EQ(map.getChunkCount(), 0u);
}

// ============================================================================
// DayNightCycle Edge Cases
// ============================================================================

TEST(DayNightEdgeTest, ZeroDuration) {
    DayNightConfig cfg;
    cfg.dayDurationSeconds = 0.0f;
    DayNightCycle cycle(cfg);

    EXPECT_FLOAT_EQ(cycle.getNormalizedTime(), 0.0f);
    // Should not crash on update
    cycle.update(1.0f);
}
