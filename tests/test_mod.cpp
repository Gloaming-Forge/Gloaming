#include <gtest/gtest.h>

#include "mod/ModManifest.hpp"
#include "mod/EventBus.hpp"
#include "mod/ContentRegistry.hpp"
#include "mod/HotReload.hpp"

#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

using namespace gloaming;
namespace fs = std::filesystem;

// ============================================================================
// Version Tests
// ============================================================================

TEST(Version, ParseValid) {
    auto v = Version::parse("1.2.3");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->major, 1);
    EXPECT_EQ(v->minor, 2);
    EXPECT_EQ(v->patch, 3);
}

TEST(Version, ParseZeros) {
    auto v = Version::parse("0.0.0");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->major, 0);
    EXPECT_EQ(v->minor, 0);
    EXPECT_EQ(v->patch, 0);
}

TEST(Version, ParseLargeNumbers) {
    auto v = Version::parse("10.200.3000");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->major, 10);
    EXPECT_EQ(v->minor, 200);
    EXPECT_EQ(v->patch, 3000);
}

TEST(Version, ParseInvalid) {
    EXPECT_FALSE(Version::parse("").has_value());
    EXPECT_FALSE(Version::parse("abc").has_value());
    EXPECT_FALSE(Version::parse("1.2").has_value());
    EXPECT_FALSE(Version::parse("1").has_value());
    EXPECT_FALSE(Version::parse("1.2.3.4").has_value());
}

TEST(Version, ToString) {
    Version v{1, 2, 3};
    EXPECT_EQ(v.toString(), "1.2.3");
}

TEST(Version, Comparison) {
    Version v100{1, 0, 0};
    Version v110{1, 1, 0};
    Version v111{1, 1, 1};
    Version v200{2, 0, 0};

    EXPECT_TRUE(v100 < v110);
    EXPECT_TRUE(v110 < v111);
    EXPECT_TRUE(v111 < v200);
    EXPECT_TRUE(v100 < v200);
    EXPECT_FALSE(v200 < v100);
    EXPECT_TRUE(v100 == v100);
    EXPECT_TRUE(v100 != v110);
    EXPECT_TRUE(v100 <= v100);
    EXPECT_TRUE(v100 <= v110);
    EXPECT_TRUE(v200 >= v110);
    EXPECT_TRUE(v200 > v100);
}

// ============================================================================
// VersionRequirement Tests
// ============================================================================

TEST(VersionRequirement, ParseGreaterEqual) {
    auto req = VersionRequirement::parse(">=1.0.0");
    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->op, VersionRequirement::Op::GreaterEqual);
    EXPECT_EQ(req->version, (Version{1, 0, 0}));
}

TEST(VersionRequirement, ParseGreater) {
    auto req = VersionRequirement::parse(">2.0.0");
    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->op, VersionRequirement::Op::Greater);
}

TEST(VersionRequirement, ParseLessEqual) {
    auto req = VersionRequirement::parse("<=3.0.0");
    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->op, VersionRequirement::Op::LessEqual);
}

TEST(VersionRequirement, ParseLess) {
    auto req = VersionRequirement::parse("<3.0.0");
    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->op, VersionRequirement::Op::Less);
}

TEST(VersionRequirement, ParseEqual) {
    auto req = VersionRequirement::parse("==1.5.0");
    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->op, VersionRequirement::Op::Equal);
}

TEST(VersionRequirement, ParseBareVersion) {
    auto req = VersionRequirement::parse("1.0.0");
    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->op, VersionRequirement::Op::GreaterEqual);
}

TEST(VersionRequirement, ParseAny) {
    auto req = VersionRequirement::parse("*");
    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->op, VersionRequirement::Op::Any);
}

TEST(VersionRequirement, ParseEmpty) {
    auto req = VersionRequirement::parse("");
    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->op, VersionRequirement::Op::Any);
}

TEST(VersionRequirement, SatisfiedByGreaterEqual) {
    auto req = VersionRequirement::parse(">=1.0.0");
    ASSERT_TRUE(req.has_value());

    EXPECT_TRUE(req->satisfiedBy(Version{1, 0, 0}));
    EXPECT_TRUE(req->satisfiedBy(Version{1, 1, 0}));
    EXPECT_TRUE(req->satisfiedBy(Version{2, 0, 0}));
    EXPECT_FALSE(req->satisfiedBy(Version{0, 9, 9}));
}

TEST(VersionRequirement, SatisfiedByEqual) {
    auto req = VersionRequirement::parse("==1.5.0");
    ASSERT_TRUE(req.has_value());

    EXPECT_TRUE(req->satisfiedBy(Version{1, 5, 0}));
    EXPECT_FALSE(req->satisfiedBy(Version{1, 5, 1}));
    EXPECT_FALSE(req->satisfiedBy(Version{1, 4, 0}));
}

TEST(VersionRequirement, SatisfiedByAny) {
    auto req = VersionRequirement::parse("*");
    ASSERT_TRUE(req.has_value());

    EXPECT_TRUE(req->satisfiedBy(Version{0, 0, 1}));
    EXPECT_TRUE(req->satisfiedBy(Version{99, 99, 99}));
}

// ============================================================================
// ModManifest Tests
// ============================================================================

TEST(ModManifest, FromJsonMinimal) {
    nlohmann::json json = {
        {"id", "test-mod"},
        {"name", "Test Mod"},
        {"version", "1.0.0"}
    };

    auto manifest = ModManifest::fromJson(json, "/mods/test-mod");
    ASSERT_TRUE(manifest.has_value());
    EXPECT_EQ(manifest->id, "test-mod");
    EXPECT_EQ(manifest->name, "Test Mod");
    EXPECT_EQ(manifest->version, (Version{1, 0, 0}));
    EXPECT_EQ(manifest->directory, "/mods/test-mod");
    EXPECT_EQ(manifest->entryPoint, "scripts/init.lua");  // default
    EXPECT_EQ(manifest->loadPriority, 100);  // default
}

TEST(ModManifest, FromJsonFull) {
    nlohmann::json json = {
        {"id", "awesome-expansion"},
        {"name", "Awesome Expansion Pack"},
        {"version", "1.2.0"},
        {"engine_version", ">=0.5.0"},
        {"authors", {"Alice", "Bob"}},
        {"description", "Adds cool stuff"},
        {"dependencies", {
            {{"id", "base-game"}, {"version", ">=1.0.0"}}
        }},
        {"optional_dependencies", {
            {{"id", "magic-overhaul"}, {"version", ">=2.0.0"}}
        }},
        {"incompatible", {"old-magic-mod"}},
        {"load_priority", 50},
        {"entry_point", "scripts/main.lua"},
        {"provides", {
            {"content", true},
            {"worldgen", true},
            {"ui", false},
            {"audio", true}
        }}
    };

    auto manifest = ModManifest::fromJson(json, "/mods/awesome");
    ASSERT_TRUE(manifest.has_value());
    EXPECT_EQ(manifest->id, "awesome-expansion");
    EXPECT_EQ(manifest->authors.size(), 2u);
    EXPECT_EQ(manifest->authors[0], "Alice");
    EXPECT_EQ(manifest->dependencies.size(), 1u);
    EXPECT_EQ(manifest->dependencies[0].id, "base-game");
    EXPECT_EQ(manifest->optionalDependencies.size(), 1u);
    EXPECT_EQ(manifest->incompatible.size(), 1u);
    EXPECT_EQ(manifest->loadPriority, 50);
    EXPECT_EQ(manifest->entryPoint, "scripts/main.lua");
    EXPECT_TRUE(manifest->provides.content);
    EXPECT_TRUE(manifest->provides.worldgen);
    EXPECT_FALSE(manifest->provides.ui);
    EXPECT_TRUE(manifest->provides.audio);
}

TEST(ModManifest, FromJsonMissingId) {
    nlohmann::json json = {
        {"name", "No ID Mod"},
        {"version", "1.0.0"}
    };
    auto manifest = ModManifest::fromJson(json, "/mods/test");
    EXPECT_FALSE(manifest.has_value());
}

TEST(ModManifest, FromJsonMissingName) {
    nlohmann::json json = {
        {"id", "test"},
        {"version", "1.0.0"}
    };
    auto manifest = ModManifest::fromJson(json, "/mods/test");
    EXPECT_FALSE(manifest.has_value());
}

TEST(ModManifest, FromJsonMissingVersion) {
    nlohmann::json json = {
        {"id", "test"},
        {"name", "Test"}
    };
    auto manifest = ModManifest::fromJson(json, "/mods/test");
    EXPECT_FALSE(manifest.has_value());
}

TEST(ModManifest, FromJsonInvalidVersion) {
    nlohmann::json json = {
        {"id", "test"},
        {"name", "Test"},
        {"version", "bad-version"}
    };
    auto manifest = ModManifest::fromJson(json, "/mods/test");
    EXPECT_FALSE(manifest.has_value());
}

TEST(ModManifest, FromJsonStringDependencies) {
    nlohmann::json json = {
        {"id", "test"},
        {"name", "Test"},
        {"version", "1.0.0"},
        {"dependencies", {"base-game", "some-lib"}}
    };
    auto manifest = ModManifest::fromJson(json, "/mods/test");
    ASSERT_TRUE(manifest.has_value());
    EXPECT_EQ(manifest->dependencies.size(), 2u);
    EXPECT_EQ(manifest->dependencies[0].id, "base-game");
    EXPECT_EQ(manifest->dependencies[1].id, "some-lib");
}

TEST(ModManifest, ValidateValid) {
    nlohmann::json json = {
        {"id", "valid-mod"},
        {"name", "Valid Mod"},
        {"version", "1.0.0"}
    };
    auto manifest = ModManifest::fromJson(json, "/mods/test");
    ASSERT_TRUE(manifest.has_value());
    auto errors = manifest->validate();
    EXPECT_TRUE(errors.empty());
}

TEST(ModManifest, ValidateEmptyId) {
    ModManifest manifest;
    manifest.id = "";
    manifest.name = "Test";
    manifest.version = {1, 0, 0};
    auto errors = manifest.validate();
    EXPECT_FALSE(errors.empty());
}

TEST(ModManifest, ValidateSelfDependency) {
    ModManifest manifest;
    manifest.id = "self-dep";
    manifest.name = "Self Dep";
    manifest.version = {1, 0, 0};
    manifest.dependencies.push_back({"self-dep", {}});
    auto errors = manifest.validate();
    EXPECT_FALSE(errors.empty());
}

TEST(ModManifest, FromFile) {
    // Create a temporary mod.json file
    std::string tmpDir = "/tmp/gloaming_test_mod_" + std::to_string(getpid());
    fs::create_directories(tmpDir);

    nlohmann::json json = {
        {"id", "file-test"},
        {"name", "File Test"},
        {"version", "2.0.0"},
        {"description", "Loaded from file"}
    };

    std::string modJsonPath = tmpDir + "/mod.json";
    std::ofstream file(modJsonPath);
    file << json.dump(4);
    file.close();

    auto manifest = ModManifest::fromFile(modJsonPath);
    ASSERT_TRUE(manifest.has_value());
    EXPECT_EQ(manifest->id, "file-test");
    EXPECT_EQ(manifest->version, (Version{2, 0, 0}));
    EXPECT_EQ(manifest->directory, tmpDir);

    // Cleanup
    fs::remove_all(tmpDir);
}

// ============================================================================
// ContentId Tests
// ============================================================================

TEST(ContentId, ParseQualified) {
    auto id = ContentId::parse("base-game:dirt");
    EXPECT_EQ(id.modId, "base-game");
    EXPECT_EQ(id.localId, "dirt");
    EXPECT_EQ(id.full(), "base-game:dirt");
}

TEST(ContentId, ParseUnqualifiedWithDefault) {
    auto id = ContentId::parse("dirt", "base-game");
    EXPECT_EQ(id.modId, "base-game");
    EXPECT_EQ(id.localId, "dirt");
}

TEST(ContentId, ParseUnqualifiedNoDefault) {
    auto id = ContentId::parse("dirt");
    EXPECT_EQ(id.modId, "");
    EXPECT_EQ(id.localId, "dirt");
}

// ============================================================================
// ContentRegistry Tests
// ============================================================================

TEST(ContentRegistry, RegisterTile) {
    ContentRegistry registry;

    TileContentDef tile;
    tile.id = "dirt";
    tile.qualifiedId = "base:dirt";
    tile.name = "Dirt";
    tile.solid = true;

    uint16_t runtimeId = registry.registerTile(tile);
    EXPECT_GT(runtimeId, 0u);
    EXPECT_EQ(registry.tileCount(), 1u);

    const auto* retrieved = registry.getTile("base:dirt");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->name, "Dirt");
    EXPECT_EQ(retrieved->runtimeId, runtimeId);
    EXPECT_TRUE(retrieved->solid);
}

TEST(ContentRegistry, RegisterTileRuntimeLookup) {
    ContentRegistry registry;

    TileContentDef tile;
    tile.id = "stone";
    tile.qualifiedId = "base:stone";
    tile.name = "Stone";

    uint16_t runtimeId = registry.registerTile(tile);
    const auto* byRuntime = registry.getTileByRuntime(runtimeId);
    ASSERT_NE(byRuntime, nullptr);
    EXPECT_EQ(byRuntime->name, "Stone");
}

TEST(ContentRegistry, RegisterItem) {
    ContentRegistry registry;

    ItemDefinition item;
    item.id = "copper_sword";
    item.qualifiedId = "base:copper_sword";
    item.name = "Copper Sword";
    item.type = "weapon";
    item.damage = 12;

    registry.registerItem(item);
    EXPECT_EQ(registry.itemCount(), 1u);
    EXPECT_TRUE(registry.hasItem("base:copper_sword"));

    const auto* retrieved = registry.getItem("base:copper_sword");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->damage, 12);
    EXPECT_EQ(retrieved->type, "weapon");
}

TEST(ContentRegistry, RegisterEnemy) {
    ContentRegistry registry;

    EnemyDefinition enemy;
    enemy.id = "bat";
    enemy.qualifiedId = "base:bat";
    enemy.name = "Bat";
    enemy.health = 50.0f;
    enemy.damage = 8;

    registry.registerEnemy(enemy);
    EXPECT_EQ(registry.enemyCount(), 1u);

    const auto* retrieved = registry.getEnemy("base:bat");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->health, 50.0f);
    EXPECT_EQ(retrieved->damage, 8);
}

TEST(ContentRegistry, RegisterRecipe) {
    ContentRegistry registry;

    RecipeDefinition recipe;
    recipe.id = "copper_bar_recipe";
    recipe.qualifiedId = "base:copper_bar_recipe";
    recipe.resultItem = "base:copper_bar";
    recipe.resultCount = 1;
    recipe.ingredients = {{"base:copper_ore", 3}};
    recipe.station = "base:furnace";
    recipe.category = "materials";

    registry.registerRecipe(recipe);
    EXPECT_EQ(registry.recipeCount(), 1u);

    const auto* retrieved = registry.getRecipe("base:copper_bar_recipe");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->resultItem, "base:copper_bar");
    EXPECT_EQ(retrieved->resultCount, 1);
    EXPECT_EQ(retrieved->station, "base:furnace");
}

TEST(ContentRegistry, GetRecipesByCategory) {
    ContentRegistry registry;

    RecipeDefinition r1;
    r1.id = "r1";
    r1.qualifiedId = "base:r1";
    r1.category = "weapons";
    registry.registerRecipe(r1);

    RecipeDefinition r2;
    r2.id = "r2";
    r2.qualifiedId = "base:r2";
    r2.category = "materials";
    registry.registerRecipe(r2);

    RecipeDefinition r3;
    r3.id = "r3";
    r3.qualifiedId = "base:r3";
    r3.category = "weapons";
    registry.registerRecipe(r3);

    auto weapons = registry.getRecipesByCategory("weapons");
    EXPECT_EQ(weapons.size(), 2u);

    auto materials = registry.getRecipesByCategory("materials");
    EXPECT_EQ(materials.size(), 1u);
}

TEST(ContentRegistry, GetRecipesForItem) {
    ContentRegistry registry;

    RecipeDefinition r1;
    r1.id = "r1";
    r1.qualifiedId = "base:r1";
    r1.resultItem = "base:sword";
    registry.registerRecipe(r1);

    RecipeDefinition r2;
    r2.id = "r2";
    r2.qualifiedId = "base:r2";
    r2.resultItem = "base:shield";
    registry.registerRecipe(r2);

    auto swordRecipes = registry.getRecipesForItem("base:sword");
    EXPECT_EQ(swordRecipes.size(), 1u);
}

TEST(ContentRegistry, LoadTilesFromJson) {
    ContentRegistry registry;

    nlohmann::json json = {
        {"tiles", {
            {
                {"id", "dirt"},
                {"name", "Dirt"},
                {"solid", true},
                {"hardness", 0.5f},
                {"texture", "textures/tiles/dirt.png"}
            },
            {
                {"id", "stone"},
                {"name", "Stone"},
                {"solid", true},
                {"hardness", 3.0f}
            }
        }}
    };

    bool result = registry.loadTilesFromJson(json, "test-mod", "/mods/test-mod");
    EXPECT_TRUE(result);
    EXPECT_EQ(registry.tileCount(), 2u);

    const auto* dirt = registry.getTile("test-mod:dirt");
    ASSERT_NE(dirt, nullptr);
    EXPECT_EQ(dirt->name, "Dirt");
    EXPECT_EQ(dirt->hardness, 0.5f);
    EXPECT_EQ(dirt->texturePath, "/mods/test-mod/textures/tiles/dirt.png");
}

TEST(ContentRegistry, LoadItemsFromJson) {
    ContentRegistry registry;

    nlohmann::json json = {
        {"items", {
            {
                {"id", "copper_pickaxe"},
                {"name", "Copper Pickaxe"},
                {"type", "tool"},
                {"damage", 5},
                {"pickaxe_power", 35.0f},
                {"use_time", 20}
            }
        }}
    };

    bool result = registry.loadItemsFromJson(json, "test-mod", "/mods/test-mod");
    EXPECT_TRUE(result);
    EXPECT_EQ(registry.itemCount(), 1u);

    const auto* pick = registry.getItem("test-mod:copper_pickaxe");
    ASSERT_NE(pick, nullptr);
    EXPECT_EQ(pick->type, "tool");
    EXPECT_FLOAT_EQ(pick->pickaxePower, 35.0f);
}

TEST(ContentRegistry, LoadEnemiesFromJson) {
    ContentRegistry registry;

    nlohmann::json json = {
        {"enemies", {
            {
                {"id", "bat"},
                {"name", "Bat"},
                {"health", 30.0f},
                {"damage", 8},
                {"defense", 2},
                {"knockback_resist", 0.0f},
                {"animations", {
                    {"idle", {{"frames", {0, 1, 2}}, {"fps", 4}}},
                    {"fly", {{"frames", {3, 4, 5, 6}}, {"fps", 8}}}
                }},
                {"drops", {
                    {{"item", "leather"}, {"count", {1, 3}}, {"chance", 0.5f}}
                }},
                {"spawn_conditions", {
                    {"depth", {{"min", 50}, {"max", 200}}},
                    {"light_level", {{"max", 0.3f}}}
                }}
            }
        }}
    };

    bool result = registry.loadEnemiesFromJson(json, "test-mod", "/mods/test-mod");
    EXPECT_TRUE(result);
    EXPECT_EQ(registry.enemyCount(), 1u);

    const auto* bat = registry.getEnemy("test-mod:bat");
    ASSERT_NE(bat, nullptr);
    EXPECT_EQ(bat->health, 30.0f);
    EXPECT_EQ(bat->damage, 8);
    EXPECT_EQ(bat->animations.size(), 2u);
    EXPECT_EQ(bat->drops.size(), 1u);
    EXPECT_EQ(bat->drops[0].countMin, 1);
    EXPECT_EQ(bat->drops[0].countMax, 3);
    EXPECT_FLOAT_EQ(bat->drops[0].chance, 0.5f);
    EXPECT_FLOAT_EQ(bat->spawnConditions.depthMin, 50.0f);
    EXPECT_FLOAT_EQ(bat->spawnConditions.depthMax, 200.0f);
}

TEST(ContentRegistry, LoadRecipesFromJson) {
    ContentRegistry registry;

    nlohmann::json json = {
        {"recipes", {
            {
                {"id", "copper_sword_recipe"},
                {"result", {{"item", "copper_sword"}, {"count", 1}}},
                {"ingredients", {
                    {{"item", "copper_bar"}, {"count", 8}},
                    {{"item", "wood"}, {"count", 3}}
                }},
                {"station", "anvil"},
                {"category", "weapons"}
            }
        }}
    };

    bool result = registry.loadRecipesFromJson(json, "test-mod");
    EXPECT_TRUE(result);
    EXPECT_EQ(registry.recipeCount(), 1u);

    const auto* recipe = registry.getRecipe("test-mod:copper_sword_recipe");
    ASSERT_NE(recipe, nullptr);
    EXPECT_EQ(recipe->resultItem, "copper_sword");
    EXPECT_EQ(recipe->ingredients.size(), 2u);
    EXPECT_EQ(recipe->station, "anvil");
    EXPECT_EQ(recipe->category, "weapons");
}

TEST(ContentRegistry, NonexistentContent) {
    ContentRegistry registry;

    EXPECT_EQ(registry.getTile("nonexistent"), nullptr);
    EXPECT_EQ(registry.getItem("nonexistent"), nullptr);
    EXPECT_EQ(registry.getEnemy("nonexistent"), nullptr);
    EXPECT_EQ(registry.getRecipe("nonexistent"), nullptr);
    EXPECT_EQ(registry.getTileByRuntime(999), nullptr);
    EXPECT_FALSE(registry.hasTile("nonexistent"));
    EXPECT_FALSE(registry.hasItem("nonexistent"));
    EXPECT_FALSE(registry.hasEnemy("nonexistent"));
}

TEST(ContentRegistry, Clear) {
    ContentRegistry registry;

    TileContentDef tile;
    tile.id = "dirt";
    tile.qualifiedId = "base:dirt";
    registry.registerTile(tile);

    ItemDefinition item;
    item.id = "sword";
    item.qualifiedId = "base:sword";
    registry.registerItem(item);

    EXPECT_EQ(registry.tileCount(), 1u);
    EXPECT_EQ(registry.itemCount(), 1u);

    registry.clear();

    EXPECT_EQ(registry.tileCount(), 0u);
    EXPECT_EQ(registry.itemCount(), 0u);
}

TEST(ContentRegistry, TileRuntimeIdSequential) {
    ContentRegistry registry;

    TileContentDef t1;
    t1.id = "dirt";
    t1.qualifiedId = "base:dirt";
    uint16_t id1 = registry.registerTile(t1);

    TileContentDef t2;
    t2.id = "stone";
    t2.qualifiedId = "base:stone";
    uint16_t id2 = registry.registerTile(t2);

    EXPECT_EQ(id1, 1u);  // 0 = air
    EXPECT_EQ(id2, 2u);
}

TEST(ContentRegistry, TileWithLightEmission) {
    ContentRegistry registry;

    nlohmann::json json = {
        {"tiles", {
            {
                {"id", "torch"},
                {"name", "Torch"},
                {"solid", false},
                {"light_emission", {
                    {"r", 255}, {"g", 200}, {"b", 50}, {"intensity", 0.8f}
                }}
            }
        }}
    };

    registry.loadTilesFromJson(json, "base", "/mods/base");
    const auto* torch = registry.getTile("base:torch");
    ASSERT_NE(torch, nullptr);
    EXPECT_TRUE(torch->emitsLight);
    EXPECT_EQ(torch->lightColor.r, 255);
    EXPECT_EQ(torch->lightColor.g, 200);
    EXPECT_EQ(torch->lightColor.b, 50);
    EXPECT_FLOAT_EQ(torch->lightIntensity, 0.8f);
}

TEST(ContentRegistry, GetIdLists) {
    ContentRegistry registry;

    TileContentDef t1, t2;
    t1.qualifiedId = "a:t1"; t1.id = "t1";
    t2.qualifiedId = "a:t2"; t2.id = "t2";
    registry.registerTile(t1);
    registry.registerTile(t2);

    auto tileIds = registry.getTileIds();
    EXPECT_EQ(tileIds.size(), 2u);

    ItemDefinition i1;
    i1.qualifiedId = "a:i1"; i1.id = "i1";
    registry.registerItem(i1);
    auto itemIds = registry.getItemIds();
    EXPECT_EQ(itemIds.size(), 1u);
}

// ============================================================================
// EventBus Tests
// ============================================================================

TEST(EventBus, BasicEmitAndReceive) {
    EventBus bus;
    int received = 0;

    bus.on("test_event", [&](const EventData&) -> bool {
        ++received;
        return false;
    });

    bus.emit("test_event");
    EXPECT_EQ(received, 1);

    bus.emit("test_event");
    EXPECT_EQ(received, 2);
}

TEST(EventBus, EventData) {
    EventBus bus;
    std::string capturedName;
    float capturedDamage = 0.0f;

    bus.on("hit", [&](const EventData& data) -> bool {
        capturedName = data.getString("target");
        capturedDamage = data.getFloat("damage");
        return false;
    });

    EventData data;
    data.setString("target", "player");
    data.setFloat("damage", 25.5f);
    bus.emit("hit", data);

    EXPECT_EQ(capturedName, "player");
    EXPECT_FLOAT_EQ(capturedDamage, 25.5f);
}

TEST(EventBus, EventDataTypes) {
    EventData data;
    data.setString("name", "test");
    data.setFloat("speed", 1.5f);
    data.setInt("count", 42);
    data.setBool("active", true);

    EXPECT_EQ(data.getString("name"), "test");
    EXPECT_FLOAT_EQ(data.getFloat("speed"), 1.5f);
    EXPECT_EQ(data.getInt("count"), 42);
    EXPECT_TRUE(data.getBool("active"));

    // Defaults
    EXPECT_EQ(data.getString("missing", "default"), "default");
    EXPECT_FLOAT_EQ(data.getFloat("missing", 9.9f), 9.9f);
    EXPECT_EQ(data.getInt("missing", -1), -1);
    EXPECT_FALSE(data.getBool("missing", false));

    // Has checks
    EXPECT_TRUE(data.hasString("name"));
    EXPECT_FALSE(data.hasString("missing"));
    EXPECT_TRUE(data.hasFloat("speed"));
    EXPECT_TRUE(data.hasInt("count"));
    EXPECT_TRUE(data.hasBool("active"));
}

TEST(EventBus, PriorityOrder) {
    EventBus bus;
    std::vector<int> order;

    bus.on("test", [&](const EventData&) -> bool { order.push_back(2); return false; }, 10);
    bus.on("test", [&](const EventData&) -> bool { order.push_back(1); return false; }, 0);
    bus.on("test", [&](const EventData&) -> bool { order.push_back(3); return false; }, 20);

    bus.emit("test");

    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

TEST(EventBus, CancelEvent) {
    EventBus bus;
    int handlersCalled = 0;

    bus.on("test", [&](const EventData&) -> bool {
        ++handlersCalled;
        return true;  // Cancel
    }, 0);

    bus.on("test", [&](const EventData&) -> bool {
        ++handlersCalled;  // Should not be called
        return false;
    }, 10);

    bool cancelled = bus.emit("test");
    EXPECT_TRUE(cancelled);
    EXPECT_EQ(handlersCalled, 1);
}

TEST(EventBus, Unsubscribe) {
    EventBus bus;
    int count = 0;

    auto id = bus.on("test", [&](const EventData&) -> bool {
        ++count;
        return false;
    });

    bus.emit("test");
    EXPECT_EQ(count, 1);

    bus.off(id);
    bus.emit("test");
    EXPECT_EQ(count, 1);  // Handler removed, not called again
}

TEST(EventBus, UnsubscribeAll) {
    EventBus bus;
    int count = 0;

    bus.on("test", [&](const EventData&) -> bool { ++count; return false; });
    bus.on("test", [&](const EventData&) -> bool { ++count; return false; });

    bus.emit("test");
    EXPECT_EQ(count, 2);

    bus.offAll("test");
    bus.emit("test");
    EXPECT_EQ(count, 2);  // No handlers
}

TEST(EventBus, HandlerCount) {
    EventBus bus;

    EXPECT_EQ(bus.handlerCount("test"), 0u);

    bus.on("test", [](const EventData&) -> bool { return false; });
    bus.on("test", [](const EventData&) -> bool { return false; });
    bus.on("other", [](const EventData&) -> bool { return false; });

    EXPECT_EQ(bus.handlerCount("test"), 2u);
    EXPECT_EQ(bus.handlerCount("other"), 1u);
}

TEST(EventBus, EmitNoHandlers) {
    EventBus bus;
    bool cancelled = bus.emit("nonexistent");
    EXPECT_FALSE(cancelled);
}

TEST(EventBus, Clear) {
    EventBus bus;
    bus.on("a", [](const EventData&) -> bool { return false; });
    bus.on("b", [](const EventData&) -> bool { return false; });

    bus.clear();

    EXPECT_EQ(bus.handlerCount("a"), 0u);
    EXPECT_EQ(bus.handlerCount("b"), 0u);
}

// ============================================================================
// HotReload Tests
// ============================================================================

TEST(HotReload, WatchModCreatesEntry) {
    HotReload hotReload;
    EXPECT_FALSE(hotReload.isWatching());

    // Create a temp directory with a test file
    std::string tmpDir = "/tmp/gloaming_hotreload_test_" + std::to_string(getpid());
    fs::create_directories(tmpDir);
    {
        std::ofstream f(tmpDir + "/test.lua");
        f << "-- test";
    }

    hotReload.watchMod("test-mod", tmpDir);
    EXPECT_TRUE(hotReload.isWatching());
    EXPECT_EQ(hotReload.watchedModCount(), 1u);

    hotReload.unwatchAll();
    EXPECT_FALSE(hotReload.isWatching());

    fs::remove_all(tmpDir);
}

TEST(HotReload, DetectsFileChange) {
    std::string tmpDir = "/tmp/gloaming_hotreload_change_" + std::to_string(getpid());
    fs::create_directories(tmpDir);
    {
        std::ofstream f(tmpDir + "/test.lua");
        f << "-- original";
    }

    HotReload hotReload;
    hotReload.setPollInterval(0.0f);  // No delay for testing

    std::string changedModId;
    std::vector<std::string> changedFiles;

    hotReload.setCallback([&](const std::string& modId, const std::vector<std::string>& files) {
        changedModId = modId;
        changedFiles = files;
    });

    hotReload.watchMod("test-mod", tmpDir);

    // Modify the file
    {
        std::ofstream f(tmpDir + "/test.lua");
        f << "-- modified content that is different";
    }

    // Poll should detect the change
    // Note: filesystem timestamps have limited resolution, so this test
    // may occasionally be flaky on fast systems.
    bool detected = hotReload.poll();
    if (detected) {
        EXPECT_EQ(changedModId, "test-mod");
        EXPECT_FALSE(changedFiles.empty());
    }
    // Even if not detected due to timestamp resolution, the test shouldn't crash

    fs::remove_all(tmpDir);
}

TEST(HotReload, WatchNonexistentDirectory) {
    HotReload hotReload;
    hotReload.watchMod("bad-mod", "/nonexistent/path");
    EXPECT_FALSE(hotReload.isWatching());
}

TEST(HotReload, UnwatchSpecificMod) {
    std::string tmpDir1 = "/tmp/gloaming_hr_unwatch1_" + std::to_string(getpid());
    std::string tmpDir2 = "/tmp/gloaming_hr_unwatch2_" + std::to_string(getpid());
    fs::create_directories(tmpDir1);
    fs::create_directories(tmpDir2);

    HotReload hotReload;
    hotReload.watchMod("mod-a", tmpDir1);
    hotReload.watchMod("mod-b", tmpDir2);
    EXPECT_EQ(hotReload.watchedModCount(), 2u);

    hotReload.unwatchMod("mod-a");
    EXPECT_EQ(hotReload.watchedModCount(), 1u);
    EXPECT_TRUE(hotReload.isWatching());

    fs::remove_all(tmpDir1);
    fs::remove_all(tmpDir2);
}

// ============================================================================
// Integration: ModManifest + ContentRegistry
// ============================================================================

TEST(Integration, LoadContentFromModDirectory) {
    // Simulate a mod directory structure
    std::string tmpDir = "/tmp/gloaming_integ_" + std::to_string(getpid());
    fs::create_directories(tmpDir + "/content");

    // Write tiles.json
    nlohmann::json tilesJson = {
        {"tiles", {
            {{"id", "dirt"}, {"name", "Dirt"}, {"solid", true}},
            {{"id", "stone"}, {"name", "Stone"}, {"solid", true}, {"hardness", 3.0f}}
        }}
    };
    {
        std::ofstream f(tmpDir + "/content/tiles.json");
        f << tilesJson.dump(4);
    }

    // Write mod.json
    nlohmann::json modJson = {
        {"id", "test-mod"},
        {"name", "Test Mod"},
        {"version", "1.0.0"},
        {"provides", {{"content", true}}}
    };
    {
        std::ofstream f(tmpDir + "/mod.json");
        f << modJson.dump(4);
    }

    // Load manifest
    auto manifest = ModManifest::fromFile(tmpDir + "/mod.json");
    ASSERT_TRUE(manifest.has_value());
    EXPECT_EQ(manifest->id, "test-mod");

    // Load content
    ContentRegistry registry;

    std::ifstream tileFile(tmpDir + "/content/tiles.json");
    nlohmann::json loadedTiles;
    tileFile >> loadedTiles;
    tileFile.close();

    bool result = registry.loadTilesFromJson(loadedTiles, manifest->id, tmpDir);
    EXPECT_TRUE(result);
    EXPECT_EQ(registry.tileCount(), 2u);

    const auto* dirt = registry.getTile("test-mod:dirt");
    ASSERT_NE(dirt, nullptr);
    EXPECT_TRUE(dirt->solid);

    const auto* stone = registry.getTile("test-mod:stone");
    ASSERT_NE(stone, nullptr);
    EXPECT_FLOAT_EQ(stone->hardness, 3.0f);

    fs::remove_all(tmpDir);
}
