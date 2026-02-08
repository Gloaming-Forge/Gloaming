#include <gtest/gtest.h>

#include "gameplay/SpriteAnimation.hpp"
#include "gameplay/CollisionLayers.hpp"
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"

using namespace gloaming;

// =============================================================================
// AnimationClip Tests
// =============================================================================

TEST(AnimationClipTest, DefaultValues) {
    AnimationClip clip;
    EXPECT_TRUE(clip.frames.empty());
    EXPECT_FLOAT_EQ(clip.fps, 10.0f);
    EXPECT_EQ(clip.mode, PlaybackMode::Loop);
}

TEST(AnimationClipTest, ManualFrames) {
    AnimationClip clip;
    clip.fps = 12.0f;
    clip.mode = PlaybackMode::Once;
    clip.frames.push_back(Rect(0, 0, 16, 16));
    clip.frames.push_back(Rect(16, 0, 16, 16));
    clip.frames.push_back(Rect(32, 0, 16, 16));

    EXPECT_EQ(clip.frames.size(), 3u);
    EXPECT_FLOAT_EQ(clip.fps, 12.0f);
    EXPECT_EQ(clip.mode, PlaybackMode::Once);
}

// =============================================================================
// AnimationController Tests
// =============================================================================

TEST(AnimationControllerTest, AddClipFromSheet) {
    AnimationController ctrl;
    ctrl.addClipFromSheet("walk", /*row=*/1, /*frameCount=*/4,
                          /*frameWidth=*/16, /*frameHeight=*/16,
                          /*fps=*/10.0f);

    EXPECT_EQ(ctrl.clips.size(), 1u);
    EXPECT_TRUE(ctrl.clips.count("walk") > 0);

    const auto& clip = ctrl.clips["walk"];
    EXPECT_EQ(clip.frames.size(), 4u);
    EXPECT_FLOAT_EQ(clip.fps, 10.0f);
    EXPECT_EQ(clip.mode, PlaybackMode::Loop);

    // Frame 0: column 0, row 1
    EXPECT_FLOAT_EQ(clip.frames[0].x, 0.0f);
    EXPECT_FLOAT_EQ(clip.frames[0].y, 16.0f);
    EXPECT_FLOAT_EQ(clip.frames[0].width, 16.0f);
    EXPECT_FLOAT_EQ(clip.frames[0].height, 16.0f);

    // Frame 1: column 1, row 1
    EXPECT_FLOAT_EQ(clip.frames[1].x, 16.0f);
    EXPECT_FLOAT_EQ(clip.frames[1].y, 16.0f);

    // Frame 3: column 3, row 1
    EXPECT_FLOAT_EQ(clip.frames[3].x, 48.0f);
    EXPECT_FLOAT_EQ(clip.frames[3].y, 16.0f);
}

TEST(AnimationControllerTest, AddClipFromSheetWithMode) {
    AnimationController ctrl;
    ctrl.addClipFromSheet("attack", 2, 3, 32, 32, 12.0f, PlaybackMode::Once);

    const auto& clip = ctrl.clips["attack"];
    EXPECT_EQ(clip.mode, PlaybackMode::Once);
    EXPECT_FLOAT_EQ(clip.fps, 12.0f);
    EXPECT_EQ(clip.frames.size(), 3u);
    // Row 2 with 32px frames
    EXPECT_FLOAT_EQ(clip.frames[0].y, 64.0f);
    EXPECT_FLOAT_EQ(clip.frames[0].width, 32.0f);
}

TEST(AnimationControllerTest, PlayClip) {
    AnimationController ctrl;
    ctrl.addClipFromSheet("idle", 0, 4, 16, 16, 8.0f);
    ctrl.addClipFromSheet("walk", 1, 6, 16, 16, 10.0f);

    // Play idle
    EXPECT_TRUE(ctrl.play("idle"));
    EXPECT_EQ(ctrl.getCurrentClipName(), "idle");
    EXPECT_EQ(ctrl.currentFrame, 0);
    EXPECT_FALSE(ctrl.isFinished());

    // Playing the same clip again is a no-op (returns true, doesn't reset)
    ctrl.currentFrame = 2; // simulate advancement
    EXPECT_TRUE(ctrl.play("idle"));
    EXPECT_EQ(ctrl.currentFrame, 2); // unchanged

    // Switch to walk
    EXPECT_TRUE(ctrl.play("walk"));
    EXPECT_EQ(ctrl.getCurrentClipName(), "walk");
    EXPECT_EQ(ctrl.currentFrame, 0); // reset

    // Play nonexistent clip
    EXPECT_FALSE(ctrl.play("nonexistent"));
}

TEST(AnimationControllerTest, StopClip) {
    AnimationController ctrl;
    ctrl.addClipFromSheet("idle", 0, 4, 16, 16, 8.0f);
    ctrl.play("idle");

    ctrl.stop();
    EXPECT_TRUE(ctrl.getCurrentClipName().empty());
    EXPECT_EQ(ctrl.currentFrame, 0);
    EXPECT_FALSE(ctrl.isFinished());
}

TEST(AnimationControllerTest, PlayDirectional) {
    AnimationController ctrl;
    ctrl.addClipFromSheet("walk_up",    0, 4, 16, 16, 10.0f);
    ctrl.addClipFromSheet("walk_down",  1, 4, 16, 16, 10.0f);
    ctrl.addClipFromSheet("walk_left",  2, 4, 16, 16, 10.0f);
    ctrl.addClipFromSheet("walk_right", 3, 4, 16, 16, 10.0f);

    EXPECT_TRUE(ctrl.playDirectional("walk", "up"));
    EXPECT_EQ(ctrl.getCurrentClipName(), "walk_up");

    EXPECT_TRUE(ctrl.playDirectional("walk", "down"));
    EXPECT_EQ(ctrl.getCurrentClipName(), "walk_down");

    EXPECT_TRUE(ctrl.playDirectional("walk", "left"));
    EXPECT_EQ(ctrl.getCurrentClipName(), "walk_left");
}

TEST(AnimationControllerTest, PlayDirectionalFallback) {
    AnimationController ctrl;
    ctrl.addClipFromSheet("idle", 0, 4, 16, 16, 6.0f);

    // No directional variant: should fall back to "idle"
    EXPECT_TRUE(ctrl.playDirectional("idle", "down"));
    EXPECT_EQ(ctrl.getCurrentClipName(), "idle");

    // No clip at all
    EXPECT_FALSE(ctrl.playDirectional("run", "down"));
}

TEST(AnimationControllerTest, GetCurrentFrameRect) {
    AnimationController ctrl;
    ctrl.addClipFromSheet("idle", 0, 4, 16, 16, 8.0f);

    // No clip playing -> null
    EXPECT_EQ(ctrl.getCurrentFrameRect(), nullptr);

    ctrl.play("idle");
    const Rect* rect = ctrl.getCurrentFrameRect();
    ASSERT_NE(rect, nullptr);
    EXPECT_FLOAT_EQ(rect->x, 0.0f);
    EXPECT_FLOAT_EQ(rect->y, 0.0f);
    EXPECT_FLOAT_EQ(rect->width, 16.0f);
    EXPECT_FLOAT_EQ(rect->height, 16.0f);
}

TEST(AnimationControllerTest, AddFrameEvent) {
    AnimationController ctrl;
    ctrl.addClipFromSheet("attack", 0, 4, 16, 16, 10.0f, PlaybackMode::Once);

    int eventCount = 0;
    ctrl.addFrameEvent("attack", 2, [&eventCount](Entity) {
        eventCount++;
    });

    EXPECT_EQ(ctrl.frameEvents.size(), 1u);
    EXPECT_EQ(ctrl.frameEvents["attack"].size(), 1u);
    EXPECT_EQ(ctrl.frameEvents["attack"][2].size(), 1u);

    // Fire manually
    ctrl.frameEvents["attack"][2][0](NullEntity);
    EXPECT_EQ(eventCount, 1);
}

// =============================================================================
// AnimationControllerSystem Tests
// =============================================================================

// Helper: delegates directly to AnimationController::tick()
static void simulateAnimUpdate(AnimationController& ctrl, Sprite& sprite, Entity entity, float dt) {
    ctrl.tick(dt, entity, sprite);
}

TEST(AnimationSystemTest, LoopPlayback) {
    AnimationController ctrl;
    Sprite sprite;
    ctrl.addClipFromSheet("walk", 0, 4, 16, 16, 10.0f); // 10 FPS = 0.1s per frame
    ctrl.play("walk");

    // Frame 0 initially
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.0f);
    EXPECT_EQ(ctrl.currentFrame, 0);

    // After 0.1s -> frame 1
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_EQ(ctrl.currentFrame, 1);

    // After 0.1s -> frame 2
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_EQ(ctrl.currentFrame, 2);

    // After 0.1s -> frame 3
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_EQ(ctrl.currentFrame, 3);

    // After 0.1s -> wraps to frame 0 (loop)
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_EQ(ctrl.currentFrame, 0);
    EXPECT_FALSE(ctrl.isFinished());
}

TEST(AnimationSystemTest, OncePlayback) {
    AnimationController ctrl;
    Sprite sprite;
    ctrl.addClipFromSheet("attack", 0, 3, 16, 16, 10.0f, PlaybackMode::Once);
    ctrl.play("attack");

    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.0f); // init
    EXPECT_EQ(ctrl.currentFrame, 0);
    EXPECT_FALSE(ctrl.isFinished());

    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f); // -> frame 1
    EXPECT_EQ(ctrl.currentFrame, 1);

    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f); // -> frame 2 (last)
    EXPECT_EQ(ctrl.currentFrame, 2);

    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f); // should stay on frame 2, finished
    EXPECT_EQ(ctrl.currentFrame, 2);
    EXPECT_TRUE(ctrl.isFinished());

    // Further updates keep it finished
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.5f);
    EXPECT_EQ(ctrl.currentFrame, 2);
    EXPECT_TRUE(ctrl.isFinished());
}

TEST(AnimationSystemTest, PingPongPlayback) {
    AnimationController ctrl;
    Sprite sprite;
    // 4 frames at 10 FPS: 0, 1, 2, 3, 2, 1, 0, 1, 2, 3, ...
    ctrl.addClipFromSheet("pulse", 0, 4, 16, 16, 10.0f, PlaybackMode::PingPong);
    ctrl.play("pulse");

    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.0f);
    EXPECT_EQ(ctrl.currentFrame, 0);

    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_EQ(ctrl.currentFrame, 1);

    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_EQ(ctrl.currentFrame, 2);

    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_EQ(ctrl.currentFrame, 3); // peak

    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_EQ(ctrl.currentFrame, 2); // reverse

    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_EQ(ctrl.currentFrame, 1);

    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_EQ(ctrl.currentFrame, 0); // back to start

    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_EQ(ctrl.currentFrame, 1); // forward again

    EXPECT_FALSE(ctrl.isFinished()); // PingPong never finishes
}

TEST(AnimationSystemTest, PingPongTwoFrames) {
    AnimationController ctrl;
    Sprite sprite;
    // 2 frames: should alternate 0, 1, 0, 1, ... without stutter
    ctrl.addClipFromSheet("blink", 0, 2, 16, 16, 10.0f, PlaybackMode::PingPong);
    ctrl.play("blink");

    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.0f);
    EXPECT_EQ(ctrl.currentFrame, 0);

    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_EQ(ctrl.currentFrame, 1);

    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_EQ(ctrl.currentFrame, 0);

    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_EQ(ctrl.currentFrame, 1);

    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_EQ(ctrl.currentFrame, 0);

    EXPECT_FALSE(ctrl.isFinished());
}

TEST(AnimationSystemTest, SpriteSourceRectUpdated) {
    AnimationController ctrl;
    Sprite sprite;
    ctrl.addClipFromSheet("walk", 0, 3, 16, 16, 10.0f);
    ctrl.play("walk");

    // Initial: frame 0
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.0f);
    EXPECT_FLOAT_EQ(sprite.sourceRect.x, 0.0f);
    EXPECT_FLOAT_EQ(sprite.sourceRect.y, 0.0f);

    // Advance to frame 1
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_FLOAT_EQ(sprite.sourceRect.x, 16.0f);
    EXPECT_FLOAT_EQ(sprite.sourceRect.y, 0.0f);

    // Advance to frame 2
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_FLOAT_EQ(sprite.sourceRect.x, 32.0f);
}

TEST(AnimationSystemTest, FrameEventsFireAtCorrectFrame) {
    AnimationController ctrl;
    Sprite sprite;
    ctrl.addClipFromSheet("attack", 0, 4, 16, 16, 10.0f, PlaybackMode::Once);

    std::vector<int> firedFrames;
    ctrl.addFrameEvent("attack", 0, [&firedFrames](Entity) { firedFrames.push_back(0); });
    ctrl.addFrameEvent("attack", 2, [&firedFrames](Entity) { firedFrames.push_back(2); });
    ctrl.addFrameEvent("attack", 3, [&firedFrames](Entity) { firedFrames.push_back(3); });

    ctrl.play("attack");

    // First update fires frame 0 event
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.0f);
    EXPECT_EQ(firedFrames.size(), 1u);
    EXPECT_EQ(firedFrames[0], 0);

    // Advance past frame 1 (no event)
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_EQ(firedFrames.size(), 1u);

    // Advance to frame 2 (event!)
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_EQ(firedFrames.size(), 2u);
    EXPECT_EQ(firedFrames[1], 2);

    // Advance to frame 3 (event!)
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f);
    EXPECT_EQ(firedFrames.size(), 3u);
    EXPECT_EQ(firedFrames[2], 3);
}

TEST(AnimationSystemTest, FrameEventsFireOnLoop) {
    AnimationController ctrl;
    Sprite sprite;
    ctrl.addClipFromSheet("run", 0, 3, 16, 16, 10.0f); // Loop mode

    int frame0Count = 0;
    ctrl.addFrameEvent("run", 0, [&frame0Count](Entity) { frame0Count++; });

    ctrl.play("run");

    // First tick: frame 0 event fires (initial)
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.0f);
    EXPECT_EQ(frame0Count, 1);

    // Advance through frames 1, 2, back to 0
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f); // frame 1
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f); // frame 2
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.1f); // frame 0 again
    EXPECT_EQ(frame0Count, 2);
}

TEST(AnimationSystemTest, LargeDtAdvancesMultipleFrames) {
    AnimationController ctrl;
    Sprite sprite;
    ctrl.addClipFromSheet("walk", 0, 4, 16, 16, 10.0f); // 0.1s per frame
    ctrl.play("walk");

    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.0f); // init at frame 0

    // Jump 0.3s = 3 frame advances -> frame 3
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.3f);
    EXPECT_EQ(ctrl.currentFrame, 3);
}

TEST(AnimationSystemTest, SwitchingClipsResets) {
    AnimationController ctrl;
    Sprite sprite;
    ctrl.addClipFromSheet("idle", 0, 4, 16, 16, 6.0f);
    ctrl.addClipFromSheet("walk", 1, 6, 16, 16, 10.0f);

    ctrl.play("idle");
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.0f);
    simulateAnimUpdate(ctrl, sprite, NullEntity, 0.2f); // advance a couple frames
    EXPECT_GT(ctrl.currentFrame, 0);

    // Switch to walk: should reset
    ctrl.play("walk");
    EXPECT_EQ(ctrl.currentFrame, 0);
    EXPECT_FLOAT_EQ(ctrl.frameTimer, 0.0f);
    EXPECT_EQ(ctrl.lastEventFrame, -1);
}

// =============================================================================
// PlaybackMode Enum Test
// =============================================================================

TEST(PlaybackModeTest, Values) {
    EXPECT_NE(PlaybackMode::Loop, PlaybackMode::Once);
    EXPECT_NE(PlaybackMode::Once, PlaybackMode::PingPong);
    EXPECT_NE(PlaybackMode::Loop, PlaybackMode::PingPong);
}

// =============================================================================
// CollisionLayerRegistry Tests
// =============================================================================

TEST(CollisionLayerRegistryTest, DefaultLayers) {
    CollisionLayerRegistry reg;

    // Default layers should be registered
    EXPECT_TRUE(reg.hasLayer("default"));
    EXPECT_TRUE(reg.hasLayer("player"));
    EXPECT_TRUE(reg.hasLayer("enemy"));
    EXPECT_TRUE(reg.hasLayer("projectile"));
    EXPECT_TRUE(reg.hasLayer("tile"));
    EXPECT_TRUE(reg.hasLayer("trigger"));
    EXPECT_TRUE(reg.hasLayer("item"));
    EXPECT_TRUE(reg.hasLayer("npc"));
}

TEST(CollisionLayerRegistryTest, DefaultLayerBitValues) {
    CollisionLayerRegistry reg;

    // Verify bit values match the CollisionLayer namespace constants
    EXPECT_EQ(reg.getLayerBit("default"),    CollisionLayer::Default);
    EXPECT_EQ(reg.getLayerBit("player"),     CollisionLayer::Player);
    EXPECT_EQ(reg.getLayerBit("enemy"),      CollisionLayer::Enemy);
    EXPECT_EQ(reg.getLayerBit("projectile"), CollisionLayer::Projectile);
    EXPECT_EQ(reg.getLayerBit("tile"),       CollisionLayer::Tile);
    EXPECT_EQ(reg.getLayerBit("trigger"),    CollisionLayer::Trigger);
    EXPECT_EQ(reg.getLayerBit("item"),       CollisionLayer::Item);
    EXPECT_EQ(reg.getLayerBit("npc"),        CollisionLayer::NPC);
}

TEST(CollisionLayerRegistryTest, RegisterCustomLayer) {
    CollisionLayerRegistry reg;

    EXPECT_TRUE(reg.registerLayer("custom_a", 8));
    EXPECT_TRUE(reg.registerLayer("custom_b", 15));
    EXPECT_TRUE(reg.hasLayer("custom_a"));
    EXPECT_TRUE(reg.hasLayer("custom_b"));

    EXPECT_EQ(reg.getLayerBit("custom_a"), static_cast<uint32_t>(1 << 8));
    EXPECT_EQ(reg.getLayerBit("custom_b"), static_cast<uint32_t>(1 << 15));
}

TEST(CollisionLayerRegistryTest, RegisterOutOfRangeBit) {
    CollisionLayerRegistry reg;
    EXPECT_FALSE(reg.registerLayer("bad_neg", -1));
    EXPECT_FALSE(reg.registerLayer("bad_high", 32));
    // Bit 31 should be valid (full 32-bit range)
    EXPECT_TRUE(reg.registerLayer("high_bit", 31));
    EXPECT_EQ(reg.getLayerBit("high_bit"), static_cast<uint32_t>(1u << 31));
}

TEST(CollisionLayerRegistryTest, UnknownLayerReturnsZero) {
    CollisionLayerRegistry reg;
    EXPECT_EQ(reg.getLayerBit("nonexistent"), 0u);
}

TEST(CollisionLayerRegistryTest, GetBitPosition) {
    CollisionLayerRegistry reg;
    EXPECT_EQ(reg.getBitPosition("player"), 1);
    EXPECT_EQ(reg.getBitPosition("tile"), 4);
    EXPECT_EQ(reg.getBitPosition("unknown"), -1);
}

TEST(CollisionLayerRegistryTest, GetMask) {
    CollisionLayerRegistry reg;

    std::vector<std::string> names = {"tile", "enemy", "npc"};
    uint32_t mask = reg.getMask(names);
    EXPECT_EQ(mask, CollisionLayer::Tile | CollisionLayer::Enemy | CollisionLayer::NPC);
}

TEST(CollisionLayerRegistryTest, GetMaskEmpty) {
    CollisionLayerRegistry reg;
    std::vector<std::string> empty;
    EXPECT_EQ(reg.getMask(empty), 0u);
}

// =============================================================================
// CollisionLayerRegistry Entity Helper Tests
// =============================================================================

TEST(CollisionLayerRegistryTest, SetLayer) {
    CollisionLayerRegistry reg;
    Collider collider;

    reg.setLayer(collider, "player");
    EXPECT_EQ(collider.layer, CollisionLayer::Player);

    reg.setLayer(collider, "enemy");
    EXPECT_EQ(collider.layer, CollisionLayer::Enemy);
}

TEST(CollisionLayerRegistryTest, SetMask) {
    CollisionLayerRegistry reg;
    Collider collider;

    std::vector<std::string> mask = {"tile", "enemy", "npc", "item", "trigger"};
    reg.setMask(collider, mask);

    uint32_t expected = CollisionLayer::Tile | CollisionLayer::Enemy |
                        CollisionLayer::NPC | CollisionLayer::Item |
                        CollisionLayer::Trigger;
    EXPECT_EQ(collider.mask, expected);
}

TEST(CollisionLayerRegistryTest, AddMask) {
    CollisionLayerRegistry reg;
    Collider collider;
    collider.mask = 0; // start empty

    reg.addMask(collider, "player");
    EXPECT_EQ(collider.mask, CollisionLayer::Player);

    reg.addMask(collider, "enemy");
    EXPECT_EQ(collider.mask, CollisionLayer::Player | CollisionLayer::Enemy);

    // Adding same layer again is idempotent
    reg.addMask(collider, "player");
    EXPECT_EQ(collider.mask, CollisionLayer::Player | CollisionLayer::Enemy);
}

TEST(CollisionLayerRegistryTest, RemoveMask) {
    CollisionLayerRegistry reg;
    Collider collider;
    collider.mask = CollisionLayer::Player | CollisionLayer::Enemy | CollisionLayer::Tile;

    reg.removeMask(collider, "enemy");
    EXPECT_EQ(collider.mask, CollisionLayer::Player | CollisionLayer::Tile);

    reg.removeMask(collider, "player");
    EXPECT_EQ(collider.mask, CollisionLayer::Tile);

    // Removing a layer not in mask is a no-op
    reg.removeMask(collider, "npc");
    EXPECT_EQ(collider.mask, CollisionLayer::Tile);
}

TEST(CollisionLayerRegistryTest, SetLayers) {
    CollisionLayerRegistry reg;
    Collider collider;

    std::vector<std::string> layers = {"player", "npc"};
    reg.setLayers(collider, layers);
    EXPECT_EQ(collider.layer, CollisionLayer::Player | CollisionLayer::NPC);
}

// =============================================================================
// Collision Layer Integration Tests (with canCollideWith)
// =============================================================================

TEST(CollisionLayerIntegrationTest, PlayerCollidesWithEnemy) {
    CollisionLayerRegistry reg;

    Collider player;
    reg.setLayer(player, "player");
    reg.setMask(player, {"tile", "enemy", "npc", "item", "trigger"});

    Collider enemy;
    reg.setLayer(enemy, "enemy");
    reg.setMask(enemy, {"tile", "player", "projectile"});

    EXPECT_TRUE(player.canCollideWith(enemy));
    EXPECT_TRUE(enemy.canCollideWith(player));
}

TEST(CollisionLayerIntegrationTest, EnemyIgnoresNPC) {
    CollisionLayerRegistry reg;

    Collider enemy;
    reg.setLayer(enemy, "enemy");
    reg.setMask(enemy, {"tile", "player", "projectile"});

    Collider npc;
    reg.setLayer(npc, "npc");
    reg.setMask(npc, {"tile", "player"});

    // Enemy's mask doesn't include NPC, and NPC's mask doesn't include Enemy
    EXPECT_FALSE(enemy.canCollideWith(npc));
    EXPECT_FALSE(npc.canCollideWith(enemy));
}

TEST(CollisionLayerIntegrationTest, ProjectileHitsEnemyNotPlayer) {
    CollisionLayerRegistry reg;

    // Player projectile
    Collider arrow;
    reg.setLayer(arrow, "projectile");
    reg.setMask(arrow, {"tile", "enemy"});

    Collider player;
    reg.setLayer(player, "player");
    reg.setMask(player, {"tile", "enemy", "npc", "item", "trigger"});

    Collider enemy;
    reg.setLayer(enemy, "enemy");
    reg.setMask(enemy, {"tile", "player", "projectile"});

    // Arrow should hit enemy (arrow's mask includes enemy, enemy's mask includes projectile)
    EXPECT_TRUE(arrow.canCollideWith(enemy));

    // Arrow should NOT hit player (arrow's mask doesn't include player)
    EXPECT_FALSE(arrow.canCollideWith(player));
}

TEST(CollisionLayerIntegrationTest, InvincibilityFrames) {
    CollisionLayerRegistry reg;

    Collider player;
    reg.setLayer(player, "player");
    reg.setMask(player, {"tile", "enemy", "npc", "item", "trigger"});

    Collider enemy;
    reg.setLayer(enemy, "enemy");
    reg.setMask(enemy, {"tile", "player", "projectile"});

    EXPECT_TRUE(player.canCollideWith(enemy));

    // Simulate invincibility: remove enemy from player's mask
    reg.removeMask(player, "enemy");
    EXPECT_FALSE(player.canCollideWith(enemy));

    // Re-enable after invincibility ends
    reg.addMask(player, "enemy");
    EXPECT_TRUE(player.canCollideWith(enemy));
}

TEST(CollisionLayerIntegrationTest, TriggerOnlyAffectsPlayer) {
    CollisionLayerRegistry reg;

    Collider trigger;
    reg.setLayer(trigger, "trigger");
    reg.setMask(trigger, {"player"});
    trigger.isTrigger = true;

    Collider player;
    reg.setLayer(player, "player");
    reg.setMask(player, {"tile", "enemy", "trigger"});

    Collider enemy;
    reg.setLayer(enemy, "enemy");
    reg.setMask(enemy, {"tile", "player", "projectile"});

    // Trigger should detect player
    EXPECT_TRUE(trigger.canCollideWith(player));

    // Trigger should NOT detect enemy (trigger's mask only has player)
    EXPECT_FALSE(trigger.canCollideWith(enemy));
}

TEST(CollisionLayerIntegrationTest, DisabledCollider) {
    CollisionLayerRegistry reg;

    Collider a;
    reg.setLayer(a, "player");
    reg.setMask(a, {"enemy"});

    Collider b;
    reg.setLayer(b, "enemy");
    reg.setMask(b, {"player"});

    EXPECT_TRUE(a.canCollideWith(b));

    // Disable one
    a.enabled = false;
    EXPECT_FALSE(a.canCollideWith(b));
}

// =============================================================================
// AnimationController as ECS Component Tests
// =============================================================================

TEST(AnimationControllerECSTest, AddToEntity) {
    Registry registry;
    Entity entity = registry.create(Transform{}, Sprite{});

    EXPECT_FALSE(registry.has<AnimationController>(entity));

    registry.add<AnimationController>(entity);
    EXPECT_TRUE(registry.has<AnimationController>(entity));

    auto& ctrl = registry.get<AnimationController>(entity);
    EXPECT_TRUE(ctrl.clips.empty());
    EXPECT_TRUE(ctrl.currentClip.empty());
}

TEST(AnimationControllerECSTest, AddClipViaRegistry) {
    Registry registry;
    Entity entity = registry.create(Transform{}, Sprite{});
    registry.add<AnimationController>(entity);

    auto& ctrl = registry.get<AnimationController>(entity);
    ctrl.addClipFromSheet("idle", 0, 4, 16, 16, 8.0f);
    ctrl.play("idle");

    EXPECT_EQ(ctrl.getCurrentClipName(), "idle");
    EXPECT_EQ(ctrl.currentFrame, 0);
}
