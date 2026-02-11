#include <gtest/gtest.h>

#include "gameplay/SceneManager.hpp"
#include "gameplay/TimerSystem.hpp"
#include "gameplay/SaveSystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"

#include <filesystem>
#include <fstream>
#include <cmath>

using namespace gloaming;

// =============================================================================
// SceneManager — Scene Registration Tests
// =============================================================================

TEST(SceneManagerTest, InitialState) {
    SceneManager mgr;
    EXPECT_TRUE(mgr.currentScene().empty());
    EXPECT_EQ(mgr.sceneCount(), 0u);
    EXPECT_EQ(mgr.stackDepth(), 0u);
    EXPECT_FALSE(mgr.isTransitioning());
    EXPECT_FALSE(mgr.isPausedByOverlay());
}

TEST(SceneManagerTest, RegisterScene) {
    SceneManager mgr;
    SceneDefinition def;
    def.width = 40;
    def.height = 30;
    mgr.registerScene("overworld", std::move(def));

    EXPECT_TRUE(mgr.hasScene("overworld"));
    EXPECT_FALSE(mgr.hasScene("dungeon"));
    EXPECT_EQ(mgr.sceneCount(), 1u);
}

TEST(SceneManagerTest, RegisterMultipleScenes) {
    SceneManager mgr;

    SceneDefinition def1;
    def1.width = 40;
    def1.height = 30;
    mgr.registerScene("overworld", std::move(def1));

    SceneDefinition def2;
    def2.width = 20;
    def2.height = 15;
    mgr.registerScene("house_1", std::move(def2));

    SceneDefinition def3;
    def3.isOverlay = true;
    mgr.registerScene("pause_menu", std::move(def3));

    EXPECT_EQ(mgr.sceneCount(), 3u);
    EXPECT_TRUE(mgr.hasScene("overworld"));
    EXPECT_TRUE(mgr.hasScene("house_1"));
    EXPECT_TRUE(mgr.hasScene("pause_menu"));
}

// =============================================================================
// SceneManager — Transition Type Parsing
// =============================================================================

TEST(SceneTransitionTest, ParseTransitionType) {
    EXPECT_EQ(parseTransitionType("instant"), TransitionType::Instant);
    EXPECT_EQ(parseTransitionType("fade"), TransitionType::Fade);
    EXPECT_EQ(parseTransitionType("slide_left"), TransitionType::SlideLeft);
    EXPECT_EQ(parseTransitionType("slide_right"), TransitionType::SlideRight);
    EXPECT_EQ(parseTransitionType("slide_up"), TransitionType::SlideUp);
    EXPECT_EQ(parseTransitionType("slide_down"), TransitionType::SlideDown);
    EXPECT_EQ(parseTransitionType("unknown"), TransitionType::Instant);
    EXPECT_EQ(parseTransitionType(""), TransitionType::Instant);
}

TEST(SceneTransitionTest, TransitionTypeToString) {
    EXPECT_STREQ(transitionTypeToString(TransitionType::Instant), "instant");
    EXPECT_STREQ(transitionTypeToString(TransitionType::Fade), "fade");
    EXPECT_STREQ(transitionTypeToString(TransitionType::SlideLeft), "slide_left");
    EXPECT_STREQ(transitionTypeToString(TransitionType::SlideRight), "slide_right");
    EXPECT_STREQ(transitionTypeToString(TransitionType::SlideUp), "slide_up");
    EXPECT_STREQ(transitionTypeToString(TransitionType::SlideDown), "slide_down");
}

// =============================================================================
// SceneManager — Transition State
// =============================================================================

TEST(TransitionStateTest, DefaultState) {
    TransitionState state;
    EXPECT_FALSE(state.active);
    EXPECT_EQ(state.type, TransitionType::Instant);
    EXPECT_FLOAT_EQ(state.duration, 0.0f);
    EXPECT_FLOAT_EQ(state.elapsed, 0.0f);
    EXPECT_TRUE(state.targetScene.empty());
    EXPECT_FALSE(state.halfwayReached);
}

TEST(TransitionStateTest, Progress) {
    TransitionState state;
    state.duration = 1.0f;

    state.elapsed = 0.0f;
    EXPECT_FLOAT_EQ(state.getProgress(), 0.0f);

    state.elapsed = 0.5f;
    EXPECT_FLOAT_EQ(state.getProgress(), 0.5f);

    state.elapsed = 1.0f;
    EXPECT_FLOAT_EQ(state.getProgress(), 1.0f);

    // Clamped at 1.0
    state.elapsed = 2.0f;
    EXPECT_FLOAT_EQ(state.getProgress(), 1.0f);
}

TEST(TransitionStateTest, IsComplete) {
    TransitionState state;
    state.duration = 1.0f;

    state.elapsed = 0.5f;
    EXPECT_FALSE(state.isComplete());

    state.elapsed = 1.0f;
    EXPECT_TRUE(state.isComplete());

    state.elapsed = 1.5f;
    EXPECT_TRUE(state.isComplete());
}

TEST(TransitionStateTest, ZeroDuration) {
    TransitionState state;
    state.duration = 0.0f;
    state.elapsed = 0.0f;
    EXPECT_FLOAT_EQ(state.getProgress(), 1.0f); // division by zero protection
    EXPECT_TRUE(state.isComplete());
}

// =============================================================================
// SceneManager — Scene Camera Config
// =============================================================================

TEST(SceneCameraConfigTest, DefaultValues) {
    SceneCameraConfig config;
    EXPECT_EQ(config.mode, "free_follow");
    EXPECT_FLOAT_EQ(config.x, 0.0f);
    EXPECT_FLOAT_EQ(config.y, 0.0f);
    EXPECT_FLOAT_EQ(config.zoom, 1.0f);
    EXPECT_FALSE(config.configured);
}

// =============================================================================
// SceneManager — PersistentEntity / SceneLocalEntity Components
// =============================================================================

TEST(SceneComponentsTest, PersistentEntityTag) {
    Registry registry;
    Entity e = registry.create();
    EXPECT_FALSE(registry.has<PersistentEntity>(e));

    registry.raw().emplace<PersistentEntity>(e);
    EXPECT_TRUE(registry.has<PersistentEntity>(e));
}

TEST(SceneComponentsTest, SceneLocalEntityTag) {
    Registry registry;
    Entity e = registry.create();

    registry.add<SceneLocalEntity>(e, "overworld");
    EXPECT_TRUE(registry.has<SceneLocalEntity>(e));
    EXPECT_EQ(registry.get<SceneLocalEntity>(e).sceneName, "overworld");
}

TEST(SceneComponentsTest, SceneLocalDefault) {
    SceneLocalEntity sle;
    EXPECT_TRUE(sle.sceneName.empty());

    SceneLocalEntity sle2("house_1");
    EXPECT_EQ(sle2.sceneName, "house_1");
}

// =============================================================================
// SceneManager — GoTo without Engine (basic validation)
// =============================================================================

TEST(SceneManagerTest, GoToWithoutInit) {
    SceneManager mgr;
    SceneDefinition def;
    mgr.registerScene("test", std::move(def));

    // Should fail gracefully without engine
    EXPECT_FALSE(mgr.goTo("test"));
}

TEST(SceneManagerTest, GoToUnregisteredScene) {
    SceneManager mgr;
    // Even without engine init, this should log a warning
    EXPECT_FALSE(mgr.goTo("nonexistent"));
}

// =============================================================================
// TimerSystem — One-Shot Timers
// =============================================================================

TEST(TimerSystemTest, AfterBasic) {
    TimerSystem ts;
    Registry registry;
    bool fired = false;

    TimerId id = ts.after(1.0f, [&fired]() { fired = true; });
    EXPECT_NE(id, InvalidTimerId);
    EXPECT_EQ(ts.activeCount(), 1u);

    // Not yet fired
    ts.update(0.5f, registry);
    EXPECT_FALSE(fired);
    EXPECT_EQ(ts.activeCount(), 1u);

    // Should fire now
    ts.update(0.6f, registry);
    EXPECT_TRUE(fired);
    EXPECT_EQ(ts.activeCount(), 0u); // removed after firing
}

TEST(TimerSystemTest, AfterExactTiming) {
    TimerSystem ts;
    Registry registry;
    bool fired = false;

    ts.after(1.0f, [&fired]() { fired = true; });

    ts.update(1.0f, registry);
    EXPECT_TRUE(fired);
    EXPECT_EQ(ts.activeCount(), 0u);
}

TEST(TimerSystemTest, AfterZeroDelay) {
    TimerSystem ts;
    Registry registry;
    bool fired = false;

    ts.after(0.0f, [&fired]() { fired = true; });

    ts.update(0.001f, registry);
    EXPECT_TRUE(fired);
}

TEST(TimerSystemTest, MultipleAfterTimers) {
    TimerSystem ts;
    Registry registry;
    int fireCount = 0;

    ts.after(0.5f, [&fireCount]() { fireCount++; });
    ts.after(1.0f, [&fireCount]() { fireCount++; });
    ts.after(1.5f, [&fireCount]() { fireCount++; });
    EXPECT_EQ(ts.activeCount(), 3u);

    ts.update(0.6f, registry);
    EXPECT_EQ(fireCount, 1);
    EXPECT_EQ(ts.activeCount(), 2u);

    ts.update(0.5f, registry);
    EXPECT_EQ(fireCount, 2);
    EXPECT_EQ(ts.activeCount(), 1u);

    ts.update(0.5f, registry);
    EXPECT_EQ(fireCount, 3);
    EXPECT_EQ(ts.activeCount(), 0u);
}

// =============================================================================
// TimerSystem — Repeating Timers
// =============================================================================

TEST(TimerSystemTest, EveryBasic) {
    TimerSystem ts;
    Registry registry;
    int fireCount = 0;

    TimerId id = ts.every(0.5f, [&fireCount]() { fireCount++; });
    EXPECT_NE(id, InvalidTimerId);

    ts.update(0.3f, registry);
    EXPECT_EQ(fireCount, 0);

    ts.update(0.3f, registry); // total = 0.6s, should fire once
    EXPECT_EQ(fireCount, 1);

    ts.update(0.5f, registry); // total = 1.1s, should fire again
    EXPECT_EQ(fireCount, 2);

    ts.update(0.5f, registry); // total = 1.6s
    EXPECT_EQ(fireCount, 3);

    // Timer is still active
    EXPECT_EQ(ts.activeCount(), 1u);
}

TEST(TimerSystemTest, EveryZeroInterval) {
    TimerSystem ts;
    Registry registry;

    // Should reject zero interval
    TimerId id = ts.every(0.0f, []() {});
    EXPECT_EQ(id, InvalidTimerId);
    EXPECT_EQ(ts.activeCount(), 0u);
}

TEST(TimerSystemTest, EveryNegativeInterval) {
    TimerSystem ts;
    Registry registry;

    TimerId id = ts.every(-1.0f, []() {});
    EXPECT_EQ(id, InvalidTimerId);
}

// =============================================================================
// TimerSystem — Cancellation
// =============================================================================

TEST(TimerSystemTest, CancelOneShot) {
    TimerSystem ts;
    Registry registry;
    bool fired = false;

    TimerId id = ts.after(1.0f, [&fired]() { fired = true; });
    EXPECT_TRUE(ts.cancel(id));

    ts.update(2.0f, registry);
    EXPECT_FALSE(fired);
    EXPECT_EQ(ts.activeCount(), 0u);
}

TEST(TimerSystemTest, CancelRepeating) {
    TimerSystem ts;
    Registry registry;
    int fireCount = 0;

    TimerId id = ts.every(0.5f, [&fireCount]() { fireCount++; });

    ts.update(0.6f, registry); // fires once
    EXPECT_EQ(fireCount, 1);

    EXPECT_TRUE(ts.cancel(id));

    ts.update(1.0f, registry); // should not fire again
    EXPECT_EQ(fireCount, 1);
}

TEST(TimerSystemTest, CancelInvalid) {
    TimerSystem ts;
    EXPECT_FALSE(ts.cancel(InvalidTimerId));
    EXPECT_FALSE(ts.cancel(999));
}

TEST(TimerSystemTest, DoubleCancelReturnsFalse) {
    TimerSystem ts;
    TimerId id = ts.after(1.0f, []() {});
    EXPECT_TRUE(ts.cancel(id));
    EXPECT_FALSE(ts.cancel(id)); // already cancelled
}

// =============================================================================
// TimerSystem — Entity-Scoped Timers
// =============================================================================

TEST(TimerSystemTest, AfterForAutoCancel) {
    TimerSystem ts;
    Registry registry;
    bool fired = false;

    Entity e = registry.create();
    ts.afterFor(e, 1.0f, [&fired]() { fired = true; });
    EXPECT_EQ(ts.activeCount(), 1u);

    // Destroy the entity
    registry.destroy(e);

    // Timer should auto-cancel
    ts.update(2.0f, registry);
    EXPECT_FALSE(fired);
    EXPECT_EQ(ts.activeCount(), 0u);
}

TEST(TimerSystemTest, EveryForAutoCancel) {
    TimerSystem ts;
    Registry registry;
    int fireCount = 0;

    Entity e = registry.create();
    ts.everyFor(e, 0.5f, [&fireCount]() { fireCount++; });

    ts.update(0.6f, registry); // fires once
    EXPECT_EQ(fireCount, 1);

    registry.destroy(e);

    ts.update(1.0f, registry); // should not fire
    EXPECT_EQ(fireCount, 1);
    EXPECT_EQ(ts.activeCount(), 0u);
}

TEST(TimerSystemTest, AfterForFiresIfEntityAlive) {
    TimerSystem ts;
    Registry registry;
    bool fired = false;

    Entity e = registry.create();
    ts.afterFor(e, 0.5f, [&fired]() { fired = true; });

    ts.update(0.6f, registry);
    EXPECT_TRUE(fired);
}

TEST(TimerSystemTest, CancelAllForEntity) {
    TimerSystem ts;
    Registry registry;

    Entity e1 = registry.create();
    Entity e2 = registry.create();

    ts.afterFor(e1, 1.0f, []() {});
    ts.afterFor(e1, 2.0f, []() {});
    ts.afterFor(e2, 1.0f, []() {});

    EXPECT_EQ(ts.activeCount(), 3u);
    int cancelled = ts.cancelAllForEntity(e1);
    EXPECT_EQ(cancelled, 2);

    // Need to update to clean up cancelled timers
    ts.update(0.0f, registry);
    EXPECT_EQ(ts.activeCount(), 1u);
}

// =============================================================================
// TimerSystem — Pause Behavior
// =============================================================================

TEST(TimerSystemTest, GamePausedStopsTimers) {
    TimerSystem ts;
    Registry registry;
    bool fired = false;

    ts.after(0.5f, [&fired]() { fired = true; });

    // Update with game paused
    ts.update(1.0f, registry, true);
    EXPECT_FALSE(fired);
    EXPECT_EQ(ts.activeCount(), 1u);

    // Resume
    ts.update(0.6f, registry, false);
    EXPECT_TRUE(fired);
}

TEST(TimerSystemTest, IndividualTimerPause) {
    TimerSystem ts;
    Registry registry;
    bool fired = false;

    TimerId id = ts.after(0.5f, [&fired]() { fired = true; });
    ts.setPaused(id, true);

    ts.update(1.0f, registry);
    EXPECT_FALSE(fired);

    ts.setPaused(id, false);
    ts.update(0.6f, registry);
    EXPECT_TRUE(fired);
}

// =============================================================================
// TimerSystem — Clear
// =============================================================================

TEST(TimerSystemTest, ClearRemovesAll) {
    TimerSystem ts;

    ts.after(1.0f, []() {});
    ts.every(0.5f, []() {});
    ts.after(2.0f, []() {});
    EXPECT_EQ(ts.activeCount(), 3u);

    ts.clear();
    EXPECT_EQ(ts.activeCount(), 0u);
}

// =============================================================================
// TimerSystem — ID Allocation
// =============================================================================

TEST(TimerSystemTest, UniqueIds) {
    TimerSystem ts;

    TimerId id1 = ts.after(1.0f, []() {});
    TimerId id2 = ts.after(1.0f, []() {});
    TimerId id3 = ts.every(0.5f, []() {});

    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id1, id3);
    EXPECT_NE(id1, InvalidTimerId);
}

// =============================================================================
// SaveSystem — Basic Set/Get/Delete
// =============================================================================

TEST(SaveSystemTest, SetAndGet) {
    SaveSystem ss;

    EXPECT_TRUE(ss.set("test_mod", "key1", "hello"));
    EXPECT_EQ(ss.get("test_mod", "key1"), "hello");
}

TEST(SaveSystemTest, GetWithDefault) {
    SaveSystem ss;

    auto result = ss.get("test_mod", "missing_key", 42);
    EXPECT_EQ(result, 42);
}

TEST(SaveSystemTest, GetNonExistentMod) {
    SaveSystem ss;

    auto result = ss.get("nonexistent", "key", "default");
    EXPECT_EQ(result, "default");
}

TEST(SaveSystemTest, SetNumber) {
    SaveSystem ss;

    EXPECT_TRUE(ss.set("mod", "level", 5));
    EXPECT_EQ(ss.get("mod", "level"), 5);
}

TEST(SaveSystemTest, SetBoolean) {
    SaveSystem ss;

    EXPECT_TRUE(ss.set("mod", "quest_done", true));
    EXPECT_EQ(ss.get("mod", "quest_done"), true);
}

TEST(SaveSystemTest, SetFloat) {
    SaveSystem ss;

    EXPECT_TRUE(ss.set("mod", "score", 3.14));
    EXPECT_DOUBLE_EQ(ss.get("mod", "score").get<double>(), 3.14);
}

TEST(SaveSystemTest, SetTable) {
    SaveSystem ss;

    nlohmann::json inventory = nlohmann::json::array();
    inventory.push_back({{"id", "sword"}, {"count", 1}});
    inventory.push_back({{"id", "torch"}, {"count", 15}});

    EXPECT_TRUE(ss.set("mod", "inventory", inventory));

    auto result = ss.get("mod", "inventory");
    ASSERT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0]["id"], "sword");
    EXPECT_EQ(result[1]["count"], 15);
}

TEST(SaveSystemTest, DeleteKey) {
    SaveSystem ss;

    ss.set("mod", "temp", "value");
    EXPECT_TRUE(ss.has("mod", "temp"));

    EXPECT_TRUE(ss.remove("mod", "temp"));
    EXPECT_FALSE(ss.has("mod", "temp"));
}

TEST(SaveSystemTest, DeleteNonExistent) {
    SaveSystem ss;
    EXPECT_FALSE(ss.remove("mod", "nonexistent"));
}

TEST(SaveSystemTest, HasKey) {
    SaveSystem ss;

    EXPECT_FALSE(ss.has("mod", "key"));
    ss.set("mod", "key", 42);
    EXPECT_TRUE(ss.has("mod", "key"));
}

TEST(SaveSystemTest, OverwriteValue) {
    SaveSystem ss;

    ss.set("mod", "key", "first");
    EXPECT_EQ(ss.get("mod", "key"), "first");

    ss.set("mod", "key", "second");
    EXPECT_EQ(ss.get("mod", "key"), "second");
}

// =============================================================================
// SaveSystem — Keys Listing
// =============================================================================

TEST(SaveSystemTest, KeysList) {
    SaveSystem ss;

    ss.set("mod", "a", 1);
    ss.set("mod", "b", 2);
    ss.set("mod", "c", 3);

    auto keyList = ss.keys("mod");
    EXPECT_EQ(keyList.size(), 3u);

    // Keys may be in any order, so check set membership
    std::set<std::string> keySet(keyList.begin(), keyList.end());
    EXPECT_TRUE(keySet.count("a"));
    EXPECT_TRUE(keySet.count("b"));
    EXPECT_TRUE(keySet.count("c"));
}

TEST(SaveSystemTest, KeysEmptyMod) {
    SaveSystem ss;
    auto keyList = ss.keys("nonexistent");
    EXPECT_TRUE(keyList.empty());
}

// =============================================================================
// SaveSystem — Per-Mod Namespacing
// =============================================================================

TEST(SaveSystemTest, ModNamespacing) {
    SaveSystem ss;

    ss.set("mod_a", "score", 100);
    ss.set("mod_b", "score", 200);

    EXPECT_EQ(ss.get("mod_a", "score"), 100);
    EXPECT_EQ(ss.get("mod_b", "score"), 200);

    // Deleting from one doesn't affect the other
    ss.remove("mod_a", "score");
    EXPECT_FALSE(ss.has("mod_a", "score"));
    EXPECT_TRUE(ss.has("mod_b", "score"));
}

// =============================================================================
// SaveSystem — Nesting Depth Validation
// =============================================================================

TEST(SaveSystemTest, ShallowNestingAllowed) {
    SaveSystem ss;

    nlohmann::json shallow = {{"a", {{"b", {{"c", 1}}}}}};
    EXPECT_TRUE(ss.set("mod", "nested", shallow));
}

TEST(SaveSystemTest, DeepNestingRejected) {
    SaveSystem ss;

    // Build a deeply nested structure (>8 levels)
    nlohmann::json deep = 42;
    for (int i = 0; i < 10; ++i) {
        nlohmann::json wrapper;
        wrapper["level"] = deep;
        deep = wrapper;
    }

    EXPECT_FALSE(ss.set("mod", "too_deep", deep));
}

// =============================================================================
// SaveSystem — Size Limit
// =============================================================================

TEST(SaveSystemTest, SizeLimitEnforced) {
    SaveSystem ss;

    // Create a large string that would exceed the 1MB limit
    std::string bigValue(MAX_SAVE_FILE_SIZE + 1, 'x');
    EXPECT_FALSE(ss.set("mod", "big", bigValue));
    EXPECT_FALSE(ss.has("mod", "big"));
}

// =============================================================================
// SaveSystem — File Operations
// =============================================================================

TEST(SaveSystemTest, SaveAndLoadMod) {
    // Use a temporary directory
    std::string tmpDir = "/tmp/gloaming_test_save_" + std::to_string(::getpid());
    std::filesystem::create_directories(tmpDir);

    {
        SaveSystem ss;
        ss.setWorldPath(tmpDir);

        ss.set("test_mod", "player_level", 5);
        ss.set("test_mod", "quest_done", true);
        ss.set("test_mod", "name", "Hero");

        EXPECT_TRUE(ss.saveMod("test_mod"));
        EXPECT_TRUE(std::filesystem::exists(tmpDir + "/moddata/test_mod.json"));
    }

    {
        SaveSystem ss;
        ss.setWorldPath(tmpDir);

        EXPECT_TRUE(ss.loadMod("test_mod"));
        EXPECT_EQ(ss.get("test_mod", "player_level"), 5);
        EXPECT_EQ(ss.get("test_mod", "quest_done"), true);
        EXPECT_EQ(ss.get("test_mod", "name"), "Hero");
    }

    // Cleanup
    std::filesystem::remove_all(tmpDir);
}

TEST(SaveSystemTest, SaveAllLoadAll) {
    std::string tmpDir = "/tmp/gloaming_test_save_all_" + std::to_string(::getpid());
    std::filesystem::create_directories(tmpDir);

    {
        SaveSystem ss;
        ss.setWorldPath(tmpDir);

        ss.set("mod_a", "key1", 100);
        ss.set("mod_b", "key2", "hello");

        int saved = ss.saveAll();
        EXPECT_EQ(saved, 2);
    }

    {
        SaveSystem ss;
        ss.setWorldPath(tmpDir);

        int loaded = ss.loadAll();
        EXPECT_EQ(loaded, 2);

        EXPECT_EQ(ss.get("mod_a", "key1"), 100);
        EXPECT_EQ(ss.get("mod_b", "key2"), "hello");
    }

    std::filesystem::remove_all(tmpDir);
}

TEST(SaveSystemTest, BackupOnSave) {
    std::string tmpDir = "/tmp/gloaming_test_backup_" + std::to_string(::getpid());
    std::filesystem::create_directories(tmpDir);

    SaveSystem ss;
    ss.setWorldPath(tmpDir);

    // First save
    ss.set("mod", "version", 1);
    ss.saveMod("mod");

    // Second save (should create backup of first)
    ss.set("mod", "version", 2);
    ss.saveMod("mod");

    EXPECT_TRUE(std::filesystem::exists(tmpDir + "/moddata/mod.json"));
    EXPECT_TRUE(std::filesystem::exists(tmpDir + "/moddata/mod.json.bak"));

    // Backup should contain version 1
    std::ifstream bakFile(tmpDir + "/moddata/mod.json.bak");
    nlohmann::json bakData = nlohmann::json::parse(bakFile);
    EXPECT_EQ(bakData["version"], 1);

    std::filesystem::remove_all(tmpDir);
}

TEST(SaveSystemTest, LoadFromBackupOnCorruption) {
    std::string tmpDir = "/tmp/gloaming_test_corrupt_" + std::to_string(::getpid());
    std::filesystem::create_directories(tmpDir + "/moddata");

    // Write a corrupt primary file
    {
        std::ofstream f(tmpDir + "/moddata/mod.json");
        f << "not valid json {{{";
    }

    // Write a valid backup
    {
        std::ofstream f(tmpDir + "/moddata/mod.json.bak");
        nlohmann::json backup = {{"rescued", true}};
        f << backup.dump();
    }

    SaveSystem ss;
    ss.setWorldPath(tmpDir);

    EXPECT_TRUE(ss.loadMod("mod"));
    EXPECT_EQ(ss.get("mod", "rescued"), true);

    std::filesystem::remove_all(tmpDir);
}

TEST(SaveSystemTest, LoadNonExistentModSucceeds) {
    SaveSystem ss;
    ss.setWorldPath("/tmp/gloaming_nonexist");

    // Loading a mod with no file is not an error
    EXPECT_TRUE(ss.loadMod("never_saved"));
}

// =============================================================================
// SaveSystem — Statistics
// =============================================================================

TEST(SaveSystemTest, ModCount) {
    SaveSystem ss;
    EXPECT_EQ(ss.modCount(), 0u);

    ss.set("mod_a", "k", 1);
    EXPECT_EQ(ss.modCount(), 1u);

    ss.set("mod_b", "k", 2);
    EXPECT_EQ(ss.modCount(), 2u);
}

TEST(SaveSystemTest, KeyCount) {
    SaveSystem ss;
    EXPECT_EQ(ss.keyCount("mod"), 0u);

    ss.set("mod", "a", 1);
    ss.set("mod", "b", 2);
    EXPECT_EQ(ss.keyCount("mod"), 2u);
}

TEST(SaveSystemTest, DirtyFlag) {
    SaveSystem ss;
    EXPECT_FALSE(ss.isDirty());

    ss.set("mod", "key", 1);
    EXPECT_TRUE(ss.isDirty());
}

TEST(SaveSystemTest, Clear) {
    SaveSystem ss;
    ss.set("mod_a", "k", 1);
    ss.set("mod_b", "k", 2);
    EXPECT_EQ(ss.modCount(), 2u);

    ss.clear();
    EXPECT_EQ(ss.modCount(), 0u);
    EXPECT_FALSE(ss.isDirty());
}

// =============================================================================
// SaveSystem — EstimateSize
// =============================================================================

TEST(SaveSystemTest, EstimateSize) {
    SaveSystem ss;
    EXPECT_EQ(ss.estimateSize("mod"), 0u);

    ss.set("mod", "key", "value");
    EXPECT_GT(ss.estimateSize("mod"), 0u);
}

// =============================================================================
// SceneDefinition
// =============================================================================

TEST(SceneDefinitionTest, DefaultValues) {
    SceneDefinition def;
    EXPECT_TRUE(def.name.empty());
    EXPECT_TRUE(def.tilesPath.empty());
    EXPECT_EQ(def.width, 0);
    EXPECT_EQ(def.height, 0);
    EXPECT_FALSE(def.camera.configured);
    EXPECT_FALSE(def.onEnter);
    EXPECT_FALSE(def.onExit);
    EXPECT_FALSE(def.isOverlay);
}

TEST(SceneDefinitionTest, WithCallbacks) {
    SceneDefinition def;
    bool enterCalled = false;
    bool exitCalled = false;

    def.onEnter = [&enterCalled]() { enterCalled = true; };
    def.onExit = [&exitCalled]() { exitCalled = true; };

    EXPECT_TRUE(def.onEnter != nullptr);
    EXPECT_TRUE(def.onExit != nullptr);

    def.onEnter();
    EXPECT_TRUE(enterCalled);

    def.onExit();
    EXPECT_TRUE(exitCalled);
}

// =============================================================================
// TimerSystem — Callback Exception Safety
// =============================================================================

TEST(TimerSystemTest, CallbackExceptionDoesNotCrash) {
    TimerSystem ts;
    Registry registry;
    bool secondFired = false;

    ts.after(0.1f, []() {
        throw std::runtime_error("intentional test error");
    });
    ts.after(0.2f, [&secondFired]() { secondFired = true; });

    // Should not throw, even though first callback throws
    EXPECT_NO_THROW(ts.update(0.3f, registry));

    // Second timer should still fire
    EXPECT_TRUE(secondFired);
}

// =============================================================================
// SaveSystem — Complex Table Values
// =============================================================================

TEST(SaveSystemTest, NestedObject) {
    SaveSystem ss;

    nlohmann::json config = {
        {"display", {
            {"width", 1920},
            {"height", 1080},
            {"fullscreen", true}
        }},
        {"audio", {
            {"volume", 0.8},
            {"muted", false}
        }}
    };

    EXPECT_TRUE(ss.set("mod", "config", config));
    auto result = ss.get("mod", "config");
    EXPECT_EQ(result["display"]["width"], 1920);
    EXPECT_EQ(result["audio"]["volume"], 0.8);
}

TEST(SaveSystemTest, ArrayOfObjects) {
    SaveSystem ss;

    nlohmann::json items = nlohmann::json::array({
        {{"id", "sword"}, {"slot", 1}, {"enchanted", true}},
        {{"id", "shield"}, {"slot", 2}, {"enchanted", false}},
        {{"id", "potion"}, {"slot", 3}, {"count", 5}}
    });

    EXPECT_TRUE(ss.set("mod", "equipment", items));
    auto result = ss.get("mod", "equipment");
    ASSERT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 3u);
    EXPECT_EQ(result[2]["count"], 5);
}

// =============================================================================
// SaveSystem — Null Value
// =============================================================================

TEST(SaveSystemTest, NullValue) {
    SaveSystem ss;

    EXPECT_TRUE(ss.set("mod", "nullable", nullptr));
    auto result = ss.get("mod", "nullable");
    EXPECT_TRUE(result.is_null());
}
