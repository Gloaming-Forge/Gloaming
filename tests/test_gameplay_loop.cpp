#include <gtest/gtest.h>

#include "gameplay/GameplayLoop.hpp"
#include "gameplay/CraftingSystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "mod/ContentRegistry.hpp"

using namespace gloaming;

// =============================================================================
// ItemStack Tests
// =============================================================================

TEST(ItemStackTest, DefaultIsEmpty) {
    ItemStack stack;
    EXPECT_TRUE(stack.isEmpty());
    EXPECT_EQ(stack.count, 0);
    EXPECT_TRUE(stack.itemId.empty());
}

TEST(ItemStackTest, NonEmpty) {
    ItemStack stack;
    stack.itemId = "base:dirt";
    stack.count = 10;
    EXPECT_FALSE(stack.isEmpty());
    EXPECT_TRUE(stack.matches("base:dirt"));
    EXPECT_FALSE(stack.matches("base:stone"));
}

TEST(ItemStackTest, Clear) {
    ItemStack stack;
    stack.itemId = "base:dirt";
    stack.count = 10;
    stack.clear();
    EXPECT_TRUE(stack.isEmpty());
}

TEST(ItemStackTest, ZeroCountIsEmpty) {
    ItemStack stack;
    stack.itemId = "base:dirt";
    stack.count = 0;
    EXPECT_TRUE(stack.isEmpty());
}

// =============================================================================
// Inventory Tests
// =============================================================================

TEST(InventoryTest, DefaultEmpty) {
    Inventory inv;
    EXPECT_EQ(inv.selectedSlot, 0);
    EXPECT_EQ(inv.occupiedSlotCount(), 0);
    EXPECT_EQ(inv.findEmptySlot(), 0);
    EXPECT_EQ(inv.countItem("base:dirt"), 0);
    EXPECT_FALSE(inv.hasItem("base:dirt"));
}

TEST(InventoryTest, AddSingleItem) {
    Inventory inv;
    int leftover = inv.addItem("base:dirt", 10);
    EXPECT_EQ(leftover, 0);
    EXPECT_EQ(inv.countItem("base:dirt"), 10);
    EXPECT_TRUE(inv.hasItem("base:dirt", 10));
    EXPECT_FALSE(inv.hasItem("base:dirt", 11));
    EXPECT_EQ(inv.occupiedSlotCount(), 1);
}

TEST(InventoryTest, AddStacksOnExisting) {
    Inventory inv;
    inv.addItem("base:dirt", 50);
    inv.addItem("base:dirt", 30);
    EXPECT_EQ(inv.countItem("base:dirt"), 80);
    EXPECT_EQ(inv.occupiedSlotCount(), 1);
}

TEST(InventoryTest, AddOverflowsToNewSlot) {
    Inventory inv;
    inv.addItem("base:dirt", 500, 999);
    inv.addItem("base:dirt", 600, 999);
    EXPECT_EQ(inv.countItem("base:dirt"), 1100);
    EXPECT_EQ(inv.occupiedSlotCount(), 2);
}

TEST(InventoryTest, AddFullInventory) {
    Inventory inv;
    // Fill all slots with different items
    for (int i = 0; i < Inventory::MaxSlots; ++i) {
        inv.slots[static_cast<size_t>(i)].itemId = "item_" + std::to_string(i);
        inv.slots[static_cast<size_t>(i)].count = 1;
    }
    // Try to add new item
    int leftover = inv.addItem("base:new_item", 10);
    EXPECT_EQ(leftover, 10);
}

TEST(InventoryTest, AddWithSmallMaxStack) {
    Inventory inv;
    int leftover = inv.addItem("base:sword", 5, 1);
    // Should create 5 stacks of 1 each
    EXPECT_EQ(leftover, 0);
    EXPECT_EQ(inv.countItem("base:sword"), 5);
    EXPECT_EQ(inv.occupiedSlotCount(), 5);
}

TEST(InventoryTest, RemoveItem) {
    Inventory inv;
    inv.addItem("base:dirt", 50);
    int removed = inv.removeItem("base:dirt", 20);
    EXPECT_EQ(removed, 20);
    EXPECT_EQ(inv.countItem("base:dirt"), 30);
}

TEST(InventoryTest, RemoveExact) {
    Inventory inv;
    inv.addItem("base:dirt", 10);
    int removed = inv.removeItem("base:dirt", 10);
    EXPECT_EQ(removed, 10);
    EXPECT_EQ(inv.countItem("base:dirt"), 0);
    EXPECT_EQ(inv.occupiedSlotCount(), 0);
}

TEST(InventoryTest, RemoveMoreThanAvailable) {
    Inventory inv;
    inv.addItem("base:dirt", 10);
    int removed = inv.removeItem("base:dirt", 20);
    EXPECT_EQ(removed, 10);
    EXPECT_EQ(inv.countItem("base:dirt"), 0);
}

TEST(InventoryTest, RemoveNonexistent) {
    Inventory inv;
    int removed = inv.removeItem("base:dirt", 10);
    EXPECT_EQ(removed, 0);
}

TEST(InventoryTest, HasItem) {
    Inventory inv;
    inv.addItem("base:dirt", 10);
    EXPECT_TRUE(inv.hasItem("base:dirt"));
    EXPECT_TRUE(inv.hasItem("base:dirt", 10));
    EXPECT_FALSE(inv.hasItem("base:dirt", 11));
    EXPECT_FALSE(inv.hasItem("base:stone"));
}

TEST(InventoryTest, SwapSlots) {
    Inventory inv;
    inv.slots[0].itemId = "base:dirt";
    inv.slots[0].count = 10;
    inv.slots[5].itemId = "base:stone";
    inv.slots[5].count = 5;

    inv.swapSlots(0, 5);

    EXPECT_EQ(inv.slots[0].itemId, "base:stone");
    EXPECT_EQ(inv.slots[0].count, 5);
    EXPECT_EQ(inv.slots[5].itemId, "base:dirt");
    EXPECT_EQ(inv.slots[5].count, 10);
}

TEST(InventoryTest, SwapInvalidSlots) {
    Inventory inv;
    inv.slots[0].itemId = "base:dirt";
    inv.slots[0].count = 10;
    // Should not crash
    inv.swapSlots(-1, 0);
    inv.swapSlots(0, Inventory::MaxSlots);
    inv.swapSlots(0, 0); // Same slot
    EXPECT_EQ(inv.slots[0].count, 10);
}

TEST(InventoryTest, ClearSlot) {
    Inventory inv;
    inv.addItem("base:dirt", 10);
    inv.clearSlot(0);
    EXPECT_TRUE(inv.slots[0].isEmpty());
    EXPECT_EQ(inv.countItem("base:dirt"), 0);
}

TEST(InventoryTest, FindItem) {
    Inventory inv;
    inv.addItem("base:dirt", 10);
    inv.addItem("base:stone", 5);

    EXPECT_EQ(inv.findItem("base:dirt"), 0);
    EXPECT_EQ(inv.findItem("base:stone"), 1);
    EXPECT_EQ(inv.findItem("base:gold"), -1);
}

TEST(InventoryTest, FindEmptySlot) {
    Inventory inv;
    EXPECT_EQ(inv.findEmptySlot(), 0);
    inv.addItem("base:dirt", 10);
    EXPECT_EQ(inv.findEmptySlot(), 1);
}

TEST(InventoryTest, SelectedSlot) {
    Inventory inv;
    inv.slots[3].itemId = "base:sword";
    inv.slots[3].count = 1;
    inv.selectedSlot = 3;

    const auto& selected = inv.getSelected();
    EXPECT_EQ(selected.itemId, "base:sword");
    EXPECT_EQ(selected.count, 1);
}

TEST(InventoryTest, SelectedSlotEmpty) {
    Inventory inv;
    inv.selectedSlot = 0;
    const auto& selected = inv.getSelected();
    EXPECT_TRUE(selected.isEmpty());
}

TEST(InventoryTest, AddZeroOrNegative) {
    Inventory inv;
    int leftover = inv.addItem("base:dirt", 0);
    EXPECT_EQ(leftover, 0);
    EXPECT_EQ(inv.countItem("base:dirt"), 0);

    leftover = inv.addItem("base:dirt", -5);
    EXPECT_EQ(leftover, 0);
}

TEST(InventoryTest, AddEmptyId) {
    Inventory inv;
    int leftover = inv.addItem("", 10);
    EXPECT_EQ(leftover, 0);
    EXPECT_EQ(inv.occupiedSlotCount(), 0);
}

// =============================================================================
// Inventory Component on Entity Tests
// =============================================================================

TEST(InventoryEntityTest, AddToEntity) {
    Registry registry;
    Entity player = registry.create(
        Transform{Vec2(100, 200)},
        Name{"player", "player"},
        PlayerTag{}
    );

    EXPECT_FALSE(registry.has<Inventory>(player));
    registry.add<Inventory>(player);
    EXPECT_TRUE(registry.has<Inventory>(player));

    auto& inv = registry.get<Inventory>(player);
    inv.addItem("base:sword", 1);
    EXPECT_TRUE(inv.hasItem("base:sword"));
}

// =============================================================================
// ItemDrop Tests
// =============================================================================

TEST(ItemDropTest, DefaultValues) {
    ItemDrop drop;
    EXPECT_TRUE(drop.itemId.empty());
    EXPECT_EQ(drop.count, 1);
    EXPECT_FLOAT_EQ(drop.magnetRadius, 48.0f);
    EXPECT_FLOAT_EQ(drop.pickupRadius, 16.0f);
    EXPECT_FLOAT_EQ(drop.pickupDelay, 0.5f);
    EXPECT_FLOAT_EQ(drop.age, 0.0f);
    EXPECT_FLOAT_EQ(drop.despawnTime, 300.0f);
    EXPECT_TRUE(drop.magnetic);
}

TEST(ItemDropTest, Construction) {
    ItemDrop drop("base:dirt", 10);
    EXPECT_EQ(drop.itemId, "base:dirt");
    EXPECT_EQ(drop.count, 10);
}

TEST(ItemDropTest, CanPickup) {
    ItemDrop drop("base:dirt", 1);
    EXPECT_FALSE(drop.canPickup()); // age = 0, delay = 0.5

    drop.age = 0.3f;
    EXPECT_FALSE(drop.canPickup());

    drop.age = 0.5f;
    EXPECT_TRUE(drop.canPickup());
}

TEST(ItemDropTest, IsExpired) {
    ItemDrop drop("base:dirt", 1);
    EXPECT_FALSE(drop.isExpired());

    drop.age = 299.9f;
    EXPECT_FALSE(drop.isExpired());

    drop.age = 300.0f;
    EXPECT_TRUE(drop.isExpired());
}

TEST(ItemDropTest, AddToEntity) {
    Registry registry;
    Entity entity = registry.create(
        Transform{Vec2(50, 50)},
        Name{"dirt", "item_drop"}
    );

    ItemDrop drop("base:dirt", 5);
    registry.add<ItemDrop>(entity, std::move(drop));

    EXPECT_TRUE(registry.has<ItemDrop>(entity));
    EXPECT_EQ(registry.get<ItemDrop>(entity).itemId, "base:dirt");
    EXPECT_EQ(registry.get<ItemDrop>(entity).count, 5);
}

// =============================================================================
// ToolUse Tests
// =============================================================================

TEST(ToolUseTest, DefaultValues) {
    ToolUse tool;
    EXPECT_FALSE(tool.active);
    EXPECT_FLOAT_EQ(tool.progress, 0.0f);
    EXPECT_FLOAT_EQ(tool.breakTime, 1.0f);
}

TEST(ToolUseTest, Progress) {
    ToolUse tool;
    tool.active = true;
    tool.breakTime = 2.0f;
    tool.progress = 1.0f;

    EXPECT_FLOAT_EQ(tool.getProgressPercent(), 0.5f);
    EXPECT_FALSE(tool.isComplete());

    tool.progress = 2.0f;
    EXPECT_FLOAT_EQ(tool.getProgressPercent(), 1.0f);
    EXPECT_TRUE(tool.isComplete());
}

TEST(ToolUseTest, Reset) {
    ToolUse tool;
    tool.active = true;
    tool.targetTileX = 10;
    tool.targetTileY = 20;
    tool.progress = 0.5f;
    tool.breakTime = 2.0f;

    tool.reset();
    EXPECT_FALSE(tool.active);
    EXPECT_EQ(tool.targetTileX, 0);
    EXPECT_EQ(tool.targetTileY, 0);
    EXPECT_FLOAT_EQ(tool.progress, 0.0f);
}

TEST(ToolUseTest, ProgressClampedToOne) {
    ToolUse tool;
    tool.breakTime = 1.0f;
    tool.progress = 5.0f;
    EXPECT_FLOAT_EQ(tool.getProgressPercent(), 1.0f);
}

// =============================================================================
// MeleeAttack Tests
// =============================================================================

TEST(MeleeAttackTest, DefaultValues) {
    MeleeAttack melee;
    EXPECT_FALSE(melee.swinging);
    EXPECT_FLOAT_EQ(melee.cooldownRemaining, 0.0f);
    EXPECT_TRUE(melee.canAttack());
}

TEST(MeleeAttackTest, StartSwing) {
    MeleeAttack melee;
    melee.startSwing(25.0f, 8.0f, 90.0f, 40.0f, 0.5f);

    EXPECT_TRUE(melee.swinging);
    EXPECT_FLOAT_EQ(melee.damage, 25.0f);
    EXPECT_FLOAT_EQ(melee.knockback, 8.0f);
    EXPECT_FLOAT_EQ(melee.arc, 90.0f);
    EXPECT_FLOAT_EQ(melee.range, 40.0f);
    EXPECT_FLOAT_EQ(melee.swingDuration, 0.5f);
    EXPECT_FALSE(melee.canAttack());
}

TEST(MeleeAttackTest, SwingCompletes) {
    MeleeAttack melee;
    melee.startSwing(10.0f, 5.0f, 120.0f, 32.0f, 0.3f);

    // Mid-swing
    melee.update(0.15f);
    EXPECT_TRUE(melee.swinging);

    // Complete swing
    melee.update(0.2f);
    EXPECT_FALSE(melee.swinging);
    EXPECT_GT(melee.cooldownRemaining, 0.0f);
}

TEST(MeleeAttackTest, CooldownDecays) {
    MeleeAttack melee;
    melee.startSwing(10.0f, 5.0f, 120.0f, 32.0f, 0.3f);

    // Complete swing
    melee.update(0.4f);
    EXPECT_FALSE(melee.swinging);

    // Wait through cooldown
    float cooldown = melee.cooldownRemaining;
    melee.update(cooldown);
    EXPECT_FLOAT_EQ(melee.cooldownRemaining, 0.0f);
    EXPECT_TRUE(melee.canAttack());
}

TEST(MeleeAttackTest, SwingAngleInterpolation) {
    MeleeAttack melee;
    melee.startSwing(10.0f, 5.0f, 120.0f, 32.0f, 1.0f);

    // At t=0, angle should be at start of arc (-60 degrees for arc=120)
    melee.update(0.0f);
    EXPECT_NEAR(melee.swingAngle, -60.0f, 0.1f);

    // At t=0.5, angle should be at center (0 degrees)
    melee.update(0.5f);
    EXPECT_NEAR(melee.swingAngle, 0.0f, 0.1f);
}

// =============================================================================
// PlayerCombat Tests
// =============================================================================

TEST(PlayerCombatTest, DefaultValues) {
    PlayerCombat combat;
    EXPECT_FALSE(combat.dead);
    EXPECT_FLOAT_EQ(combat.respawnDelay, 3.0f);
    EXPECT_EQ(combat.deathCount, 0);
}

TEST(PlayerCombatTest, Die) {
    PlayerCombat combat;
    combat.die();

    EXPECT_TRUE(combat.dead);
    EXPECT_FLOAT_EQ(combat.respawnTimer, 3.0f);
    EXPECT_EQ(combat.deathCount, 1);
}

TEST(PlayerCombatTest, DieMultipleTimes) {
    PlayerCombat combat;
    combat.die();

    // While dead, die() should not re-trigger
    combat.die();
    EXPECT_EQ(combat.deathCount, 1);
}

TEST(PlayerCombatTest, RespawnTimer) {
    PlayerCombat combat;
    combat.die();

    EXPECT_FALSE(combat.updateRespawn(1.0f));
    EXPECT_TRUE(combat.dead);

    EXPECT_FALSE(combat.updateRespawn(1.0f));
    EXPECT_TRUE(combat.dead);

    // Should respawn after 3 seconds total
    EXPECT_TRUE(combat.updateRespawn(1.0f));
    EXPECT_FALSE(combat.dead);
}

TEST(PlayerCombatTest, UpdateRespawnWhenAlive) {
    PlayerCombat combat;
    EXPECT_FALSE(combat.updateRespawn(1.0f)); // Not dead, returns false
}

TEST(PlayerCombatTest, SpawnPoint) {
    PlayerCombat combat;
    combat.spawnPoint = Vec2(100.0f, 200.0f);
    EXPECT_FLOAT_EQ(combat.spawnPoint.x, 100.0f);
    EXPECT_FLOAT_EQ(combat.spawnPoint.y, 200.0f);
}

// =============================================================================
// CraftingManager Tests
// =============================================================================

TEST(CraftingManagerTest, NullRegistryCannotCraft) {
    CraftingManager crafting;
    Inventory inv;
    EXPECT_FALSE(crafting.canCraft("recipe:test", inv, Vec2()));
}

TEST(CraftingManagerTest, UnknownRecipeCannotCraft) {
    ContentRegistry content;
    CraftingManager crafting;
    crafting.setContentRegistry(&content);

    Inventory inv;
    EXPECT_FALSE(crafting.canCraft("recipe:nonexistent", inv, Vec2()));
}

TEST(CraftingManagerTest, CanCraftSimpleRecipe) {
    ContentRegistry content;

    // Register items
    ItemDefinition swordDef;
    swordDef.id = "sword";
    swordDef.qualifiedId = "base:sword";
    swordDef.name = "Iron Sword";
    content.registerItem(swordDef);

    ItemDefinition ironDef;
    ironDef.id = "iron";
    ironDef.qualifiedId = "base:iron";
    ironDef.name = "Iron Bar";
    content.registerItem(ironDef);

    ItemDefinition woodDef;
    woodDef.id = "wood";
    woodDef.qualifiedId = "base:wood";
    woodDef.name = "Wood";
    content.registerItem(woodDef);

    // Register recipe
    RecipeDefinition recipe;
    recipe.id = "iron_sword";
    recipe.qualifiedId = "base:iron_sword";
    recipe.resultItem = "base:sword";
    recipe.resultCount = 1;
    recipe.ingredients.push_back({"base:iron", 3});
    recipe.ingredients.push_back({"base:wood", 2});
    content.registerRecipe(recipe);

    CraftingManager crafting;
    crafting.setContentRegistry(&content);

    Inventory inv;
    // Missing ingredients
    EXPECT_FALSE(crafting.canCraft("base:iron_sword", inv, Vec2()));

    // Partial ingredients
    inv.addItem("base:iron", 3);
    EXPECT_FALSE(crafting.canCraft("base:iron_sword", inv, Vec2()));

    // All ingredients present
    inv.addItem("base:wood", 2);
    EXPECT_TRUE(crafting.canCraft("base:iron_sword", inv, Vec2()));
}

TEST(CraftingManagerTest, CraftConsumesIngredients) {
    ContentRegistry content;

    ItemDefinition result;
    result.id = "torch";
    result.qualifiedId = "base:torch";
    content.registerItem(result);

    ItemDefinition wood;
    wood.id = "wood";
    wood.qualifiedId = "base:wood";
    content.registerItem(wood);

    ItemDefinition gel;
    gel.id = "gel";
    gel.qualifiedId = "base:gel";
    content.registerItem(gel);

    RecipeDefinition recipe;
    recipe.id = "torch";
    recipe.qualifiedId = "base:torch";
    recipe.resultItem = "base:torch";
    recipe.resultCount = 5;
    recipe.ingredients.push_back({"base:wood", 1});
    recipe.ingredients.push_back({"base:gel", 1});
    content.registerRecipe(recipe);

    CraftingManager crafting;
    crafting.setContentRegistry(&content);

    Inventory inv;
    inv.addItem("base:wood", 10);
    inv.addItem("base:gel", 5);

    CraftResult craftResult = crafting.craft("base:torch", inv, Vec2());
    EXPECT_TRUE(craftResult.success);
    EXPECT_EQ(craftResult.resultItem, "base:torch");
    EXPECT_EQ(craftResult.resultCount, 5);

    EXPECT_EQ(inv.countItem("base:wood"), 9);
    EXPECT_EQ(inv.countItem("base:gel"), 4);
    EXPECT_EQ(inv.countItem("base:torch"), 5);
}

TEST(CraftingManagerTest, CraftFailsMissingIngredients) {
    ContentRegistry content;

    ItemDefinition item;
    item.id = "item";
    item.qualifiedId = "base:item";
    content.registerItem(item);

    RecipeDefinition recipe;
    recipe.id = "recipe";
    recipe.qualifiedId = "base:recipe";
    recipe.resultItem = "base:item";
    recipe.resultCount = 1;
    recipe.ingredients.push_back({"base:missing", 1});
    content.registerRecipe(recipe);

    CraftingManager crafting;
    crafting.setContentRegistry(&content);

    Inventory inv;
    CraftResult result = crafting.craft("base:recipe", inv, Vec2());
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.failReason, "missing ingredients");
}

TEST(CraftingManagerTest, GetAvailableRecipes) {
    ContentRegistry content;

    ItemDefinition iron;
    iron.id = "iron";
    iron.qualifiedId = "base:iron";
    content.registerItem(iron);

    ItemDefinition sword;
    sword.id = "sword";
    sword.qualifiedId = "base:sword";
    content.registerItem(sword);

    // Recipe player can make
    RecipeDefinition canMake;
    canMake.id = "simple";
    canMake.qualifiedId = "base:simple";
    canMake.resultItem = "base:sword";
    canMake.resultCount = 1;
    canMake.ingredients.push_back({"base:iron", 1});
    content.registerRecipe(canMake);

    // Recipe player cannot make (needs too many items)
    RecipeDefinition cannotMake;
    cannotMake.id = "expensive";
    cannotMake.qualifiedId = "base:expensive";
    cannotMake.resultItem = "base:sword";
    cannotMake.resultCount = 1;
    cannotMake.ingredients.push_back({"base:iron", 100});
    content.registerRecipe(cannotMake);

    CraftingManager crafting;
    crafting.setContentRegistry(&content);

    Inventory inv;
    inv.addItem("base:iron", 5);

    auto available = crafting.getAvailableRecipes(inv, Vec2());
    EXPECT_EQ(available.size(), 1u);
    EXPECT_EQ(available[0], "base:simple");
}

TEST(CraftingManagerTest, GetAllRecipes) {
    ContentRegistry content;

    RecipeDefinition r1;
    r1.id = "r1";
    r1.qualifiedId = "base:r1";
    r1.resultItem = "base:item";
    r1.resultCount = 1;
    content.registerRecipe(r1);

    RecipeDefinition r2;
    r2.id = "r2";
    r2.qualifiedId = "base:r2";
    r2.resultItem = "base:item";
    r2.resultCount = 1;
    content.registerRecipe(r2);

    CraftingManager crafting;
    crafting.setContentRegistry(&content);

    auto all = crafting.getAllRecipes();
    EXPECT_EQ(all.size(), 2u);
}

TEST(CraftingManagerTest, HasIngredients) {
    ContentRegistry content;

    ItemDefinition a;
    a.id = "a";
    a.qualifiedId = "base:a";
    content.registerItem(a);

    RecipeDefinition recipe;
    recipe.id = "r";
    recipe.qualifiedId = "base:r";
    recipe.resultItem = "base:a";
    recipe.resultCount = 1;
    recipe.ingredients.push_back({"base:a", 5});
    content.registerRecipe(recipe);

    CraftingManager crafting;
    crafting.setContentRegistry(&content);

    Inventory inv;
    inv.addItem("base:a", 3);
    EXPECT_FALSE(crafting.canCraft("base:r", inv, Vec2()));

    inv.addItem("base:a", 2);
    EXPECT_TRUE(crafting.canCraft("base:r", inv, Vec2()));
}

// =============================================================================
// Integration: Inventory + Entity + Health
// =============================================================================

TEST(GameplayLoopIntegrationTest, PlayerWithFullLoadout) {
    Registry registry;

    // Create a fully equipped player entity
    Entity player = registry.create(
        Transform{Vec2(100, 200)},
        Velocity{},
        Name{"player", "player"},
        PlayerTag{},
        Health{100.0f}
    );

    // Add gameplay loop components
    registry.add<Inventory>(player);
    registry.add<ToolUse>(player);
    registry.add<MeleeAttack>(player);
    PlayerCombat combat;
    combat.spawnPoint = Vec2(0, 0);
    registry.add<PlayerCombat>(player, combat);

    // Verify all components
    EXPECT_TRUE(registry.has<Inventory>(player));
    EXPECT_TRUE(registry.has<ToolUse>(player));
    EXPECT_TRUE(registry.has<MeleeAttack>(player));
    EXPECT_TRUE(registry.has<PlayerCombat>(player));
    EXPECT_TRUE(registry.has<Health>(player));

    // Equip sword
    auto& inv = registry.get<Inventory>(player);
    inv.addItem("base:sword", 1);
    inv.addItem("base:torch", 10);
    inv.selectedSlot = 0;

    EXPECT_EQ(inv.getSelected().itemId, "base:sword");
}

TEST(GameplayLoopIntegrationTest, DeathAndRespawnCycle) {
    Registry registry;

    Entity player = registry.create(
        Transform{Vec2(500, 300)},
        Velocity{},
        Health{100.0f}
    );

    PlayerCombat combat;
    combat.spawnPoint = Vec2(0, 0);
    combat.respawnDelay = 1.0f;
    registry.add<PlayerCombat>(player, combat);

    auto& health = registry.get<Health>(player);
    auto& transform = registry.get<Transform>(player);
    auto& pc = registry.get<PlayerCombat>(player);

    // Take lethal damage
    health.takeDamage(100.0f);
    EXPECT_TRUE(health.isDead());

    // Trigger death detection
    if (health.isDead() && !pc.dead) {
        pc.die();
    }
    EXPECT_TRUE(pc.dead);
    EXPECT_EQ(pc.deathCount, 1);

    // Wait for respawn
    EXPECT_FALSE(pc.updateRespawn(0.5f));
    EXPECT_TRUE(pc.dead);

    EXPECT_TRUE(pc.updateRespawn(0.5f));
    EXPECT_FALSE(pc.dead);

    // Simulate respawn logic
    health.current = health.max;
    transform.position = pc.spawnPoint;

    EXPECT_FLOAT_EQ(health.current, 100.0f);
    EXPECT_FLOAT_EQ(transform.position.x, 0.0f);
    EXPECT_FLOAT_EQ(transform.position.y, 0.0f);
}

TEST(GameplayLoopIntegrationTest, ItemDropPickupFlow) {
    Registry registry;

    // Create player
    Entity player = registry.create(
        Transform{Vec2(100, 100)},
        Name{"player", "player"},
        PlayerTag{}
    );
    registry.add<Inventory>(player);

    // Create item drop near player
    Entity drop = registry.create(
        Transform{Vec2(105, 100)},
        Name{"dirt", "item_drop"}
    );
    ItemDrop itemDrop("base:dirt", 5);
    itemDrop.age = 1.0f; // Already past pickup delay
    registry.add<ItemDrop>(drop, std::move(itemDrop));

    // Simulate pickup
    auto& playerPos = registry.get<Transform>(player).position;
    auto& dropPos = registry.get<Transform>(drop).position;
    auto& dropComp = registry.get<ItemDrop>(drop);

    float dx = playerPos.x - dropPos.x;
    float dy = playerPos.y - dropPos.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    EXPECT_TRUE(dropComp.canPickup());
    EXPECT_LE(dist, dropComp.pickupRadius);

    // Do pickup
    auto& inv = registry.get<Inventory>(player);
    int leftover = inv.addItem(dropComp.itemId, dropComp.count);
    EXPECT_EQ(leftover, 0);
    EXPECT_EQ(inv.countItem("base:dirt"), 5);
}

TEST(GameplayLoopIntegrationTest, MiningTileFlow) {
    Registry registry;

    Entity player = registry.create(
        Transform{Vec2(100, 100)},
        Name{"player", "player"}
    );
    registry.add<Inventory>(player);
    registry.add<ToolUse>(player);

    auto& tool = registry.get<ToolUse>(player);
    tool.active = true;
    tool.targetTileX = 5;
    tool.targetTileY = 10;
    tool.breakTime = 1.0f;

    // Simulate mining progress
    tool.progress += 0.5f;
    EXPECT_FLOAT_EQ(tool.getProgressPercent(), 0.5f);
    EXPECT_FALSE(tool.isComplete());

    tool.progress += 0.5f;
    EXPECT_TRUE(tool.isComplete());
}

// =============================================================================
// MeleeAttack Hit Detection Simulation
// =============================================================================

TEST(MeleeHitTest, InRangeInArc) {
    Vec2 attackerPos(100, 100);
    Vec2 aimDir(1, 0);  // Aiming right
    float arc = 120.0f;
    float range = 50.0f;

    Vec2 targetPos(130, 110);  // In front and slightly below

    float dx = targetPos.x - attackerPos.x;
    float dy = targetPos.y - attackerPos.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    EXPECT_LE(dist, range);

    float aimAngle = std::atan2(aimDir.y, aimDir.x) * RAD_TO_DEG;
    float angleToTarget = std::atan2(dy, dx) * RAD_TO_DEG;
    float angleDiff = angleToTarget - aimAngle;
    while (angleDiff > 180.0f) angleDiff -= 360.0f;
    while (angleDiff < -180.0f) angleDiff += 360.0f;

    EXPECT_LE(std::abs(angleDiff), arc / 2.0f);
}

TEST(MeleeHitTest, InRangeOutOfArc) {
    Vec2 attackerPos(100, 100);
    Vec2 aimDir(1, 0);  // Aiming right
    float arc = 60.0f;

    Vec2 targetPos(90, 100);  // Behind attacker

    float dx = targetPos.x - attackerPos.x;
    float dy = targetPos.y - attackerPos.y;

    float aimAngle = std::atan2(aimDir.y, aimDir.x) * RAD_TO_DEG;
    float angleToTarget = std::atan2(dy, dx) * RAD_TO_DEG;
    float angleDiff = angleToTarget - aimAngle;
    while (angleDiff > 180.0f) angleDiff -= 360.0f;
    while (angleDiff < -180.0f) angleDiff += 360.0f;

    EXPECT_GT(std::abs(angleDiff), arc / 2.0f);
}

TEST(MeleeHitTest, OutOfRange) {
    Vec2 attackerPos(100, 100);
    float range = 32.0f;

    Vec2 targetPos(200, 100);  // Too far

    float dx = targetPos.x - attackerPos.x;
    float dy = targetPos.y - attackerPos.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    EXPECT_GT(dist, range);
}

// =============================================================================
// ItemDrop Magnet Simulation
// =============================================================================

TEST(MagnetTest, PullTowardPlayer) {
    Vec2 playerPos(100, 100);
    Vec2 dropPos(130, 100);
    float magnetSpeed = 200.0f;
    float dt = 0.1f;

    float dx = playerPos.x - dropPos.x;
    float dy = playerPos.y - dropPos.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    float moveAmount = std::min(magnetSpeed * dt, dist);
    dropPos.x += (dx / dist) * moveAmount;
    dropPos.y += (dy / dist) * moveAmount;

    // Should have moved closer
    float newDist = std::sqrt(
        (playerPos.x - dropPos.x) * (playerPos.x - dropPos.x) +
        (playerPos.y - dropPos.y) * (playerPos.y - dropPos.y)
    );
    EXPECT_LT(newDist, dist);
}

// =============================================================================
// Crafting Station Proximity Tests
// =============================================================================

TEST(CraftingStationTest, StationRequired) {
    ContentRegistry content;

    // Register a crafting station tile
    TileContentDef anvil;
    anvil.id = "anvil";
    anvil.qualifiedId = "base:anvil";
    anvil.name = "Iron Anvil";
    anvil.solid = true;
    content.registerTile(anvil);

    // Register recipe requiring anvil
    RecipeDefinition recipe;
    recipe.id = "iron_sword";
    recipe.qualifiedId = "base:iron_sword";
    recipe.resultItem = "base:sword";
    recipe.resultCount = 1;
    recipe.station = "base:anvil";
    recipe.ingredients.push_back({"base:iron", 3});
    content.registerRecipe(recipe);

    ItemDefinition ironItem;
    ironItem.id = "iron";
    ironItem.qualifiedId = "base:iron";
    content.registerItem(ironItem);

    ItemDefinition swordItem;
    swordItem.id = "sword";
    swordItem.qualifiedId = "base:sword";
    content.registerItem(swordItem);

    CraftingManager crafting;
    crafting.setContentRegistry(&content);
    // No tile map set — station check should fail

    Inventory inv;
    inv.addItem("base:iron", 3);

    // Has ingredients but no station nearby (no tile map)
    EXPECT_FALSE(crafting.canCraft("base:iron_sword", inv, Vec2()));
}

TEST(CraftingStationTest, HandCraftNoStation) {
    ContentRegistry content;

    RecipeDefinition recipe;
    recipe.id = "rope";
    recipe.qualifiedId = "base:rope";
    recipe.resultItem = "base:rope";
    recipe.resultCount = 1;
    recipe.station = ""; // Hand craft
    recipe.ingredients.push_back({"base:fiber", 2});
    content.registerRecipe(recipe);

    ItemDefinition fiber;
    fiber.id = "fiber";
    fiber.qualifiedId = "base:fiber";
    content.registerItem(fiber);

    ItemDefinition rope;
    rope.id = "rope";
    rope.qualifiedId = "base:rope";
    content.registerItem(rope);

    CraftingManager crafting;
    crafting.setContentRegistry(&content);

    Inventory inv;
    inv.addItem("base:fiber", 10);

    // Should be able to craft without a station
    EXPECT_TRUE(crafting.canCraft("base:rope", inv, Vec2()));

    CraftResult result = crafting.craft("base:rope", inv, Vec2());
    EXPECT_TRUE(result.success);
    EXPECT_EQ(inv.countItem("base:fiber"), 8);
    EXPECT_EQ(inv.countItem("base:rope"), 1);
}

// =============================================================================
// CraftResult Tests
// =============================================================================

TEST(CraftResultTest, DefaultValues) {
    CraftResult result;
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.resultItem.empty());
    EXPECT_EQ(result.resultCount, 0);
    EXPECT_TRUE(result.failReason.empty());
}

// =============================================================================
// Multiple Items in Inventory Tests
// =============================================================================

TEST(InventoryMultiTest, MultipleDifferentItems) {
    Inventory inv;
    inv.addItem("base:dirt", 100);
    inv.addItem("base:stone", 50);
    inv.addItem("base:wood", 75);

    EXPECT_EQ(inv.countItem("base:dirt"), 100);
    EXPECT_EQ(inv.countItem("base:stone"), 50);
    EXPECT_EQ(inv.countItem("base:wood"), 75);
    EXPECT_EQ(inv.occupiedSlotCount(), 3);

    inv.removeItem("base:stone", 50);
    EXPECT_EQ(inv.countItem("base:stone"), 0);
    EXPECT_EQ(inv.occupiedSlotCount(), 2);
}

TEST(InventoryMultiTest, RemoveFromMultipleSlots) {
    Inventory inv;
    // Create two separate slots of the same item
    inv.slots[0].itemId = "base:dirt";
    inv.slots[0].count = 5;
    inv.slots[2].itemId = "base:dirt";
    inv.slots[2].count = 5;

    int removed = inv.removeItem("base:dirt", 8);
    EXPECT_EQ(removed, 8);
    EXPECT_EQ(inv.countItem("base:dirt"), 2);
}

// =============================================================================
// Null Safety Tests
// =============================================================================

TEST(CraftingNullSafetyTest, NullContentRegistry) {
    CraftingManager crafting;
    // No content registry set

    Inventory inv;
    EXPECT_FALSE(crafting.canCraft("any", inv, Vec2()));

    CraftResult result = crafting.craft("any", inv, Vec2());
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.failReason, "no content registry");

    auto available = crafting.getAvailableRecipes(inv, Vec2());
    EXPECT_TRUE(available.empty());

    auto all = crafting.getAllRecipes();
    EXPECT_TRUE(all.empty());
}

TEST(CraftingNullSafetyTest, NullTileMap) {
    ContentRegistry content;
    CraftingManager crafting;
    crafting.setContentRegistry(&content);
    // No tile map — station checks should fail gracefully

    EXPECT_FALSE(crafting.isStationNearby("base:anvil", Vec2()));
}
