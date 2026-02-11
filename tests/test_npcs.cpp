#include <gtest/gtest.h>

#include "gameplay/NPCSystem.hpp"
#include "gameplay/HousingSystem.hpp"
#include "gameplay/ShopSystem.hpp"
#include "gameplay/GameplayLoop.hpp"
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "mod/ContentRegistry.hpp"

#include <cmath>

using namespace gloaming;

// =============================================================================
// NPCAI Component Tests
// =============================================================================

TEST(NPCAITest, DefaultConstruction) {
    NPCAI ai;
    EXPECT_EQ(ai.behavior, "idle");
    EXPECT_EQ(ai.defaultBehavior, "idle");
    EXPECT_FLOAT_EQ(ai.moveSpeed, 40.0f);
    EXPECT_FLOAT_EQ(ai.wanderRadius, 80.0f);
    EXPECT_FLOAT_EQ(ai.interactionRange, 48.0f);
    EXPECT_FALSE(ai.playerInRange);
    EXPECT_EQ(ai.interactingPlayer, NullEntity);
    EXPECT_TRUE(ai.schedule.empty());
    EXPECT_EQ(ai.wanderDirection, 0);
}

TEST(NPCAITest, ExplicitBehavior) {
    NPCAI ai("wander");
    EXPECT_EQ(ai.behavior, "wander");
    EXPECT_EQ(ai.defaultBehavior, "wander");
}

TEST(NPCAITest, BehaviorConstants) {
    EXPECT_STREQ(NPCBehavior::Idle, "idle");
    EXPECT_STREQ(NPCBehavior::Wander, "wander");
    EXPECT_STREQ(NPCBehavior::Schedule, "schedule");
    EXPECT_STREQ(NPCBehavior::Stationed, "stationed");
    EXPECT_STREQ(NPCBehavior::Custom, "custom");
}

TEST(NPCAITest, ScheduleEntry) {
    NPCAI ai;
    NPCAI::ScheduleEntry entry;
    entry.hour = 6;
    entry.behavior = "wander";
    entry.targetPosition = Vec2(100.0f, 200.0f);
    ai.schedule.push_back(entry);

    EXPECT_EQ(ai.schedule.size(), 1u);
    EXPECT_EQ(ai.schedule[0].hour, 6);
    EXPECT_EQ(ai.schedule[0].behavior, "wander");
    EXPECT_FLOAT_EQ(ai.schedule[0].targetPosition.x, 100.0f);
}

// =============================================================================
// NPCDialogue Component Tests
// =============================================================================

TEST(NPCDialogueTest, DefaultValues) {
    NPCDialogue dlg;
    EXPECT_TRUE(dlg.dialogueId.empty());
    EXPECT_TRUE(dlg.greetingNodeId.empty());
    EXPECT_FALSE(dlg.hasBeenTalkedTo);
    EXPECT_EQ(dlg.currentMood, "neutral");
}

TEST(NPCDialogueTest, AssignDialogue) {
    NPCDialogue dlg;
    dlg.dialogueId = "base:merchant_dialogue";
    dlg.greetingNodeId = "greeting_1";
    EXPECT_EQ(dlg.dialogueId, "base:merchant_dialogue");
    EXPECT_EQ(dlg.greetingNodeId, "greeting_1");
}

// =============================================================================
// ShopKeeper Component Tests
// =============================================================================

TEST(ShopKeeperTest, DefaultValues) {
    ShopKeeper sk;
    EXPECT_TRUE(sk.shopId.empty());
    EXPECT_FALSE(sk.shopOpen);
}

TEST(ShopKeeperTest, AssignShop) {
    ShopKeeper sk;
    sk.shopId = "base:general_store";
    sk.shopOpen = true;
    EXPECT_EQ(sk.shopId, "base:general_store");
    EXPECT_TRUE(sk.shopOpen);
}

// =============================================================================
// NPC ECS Integration Tests
// =============================================================================

TEST(NPCIntegrationTest, AddComponentToEntity) {
    Registry registry;
    Entity entity = registry.create(
        Transform{Vec2(100.0f, 200.0f)},
        Name{"merchant", "base:merchant"},
        NPCTag{"base:merchant"}
    );

    NPCAI ai("stationed");
    ai.homePosition = Vec2(100.0f, 200.0f);
    ai.interactionRange = 64.0f;
    registry.add<NPCAI>(entity, std::move(ai));

    NPCDialogue dlg;
    dlg.dialogueId = "base:merchant_dialogue";
    registry.add<NPCDialogue>(entity, std::move(dlg));

    EXPECT_TRUE(registry.has<NPCAI>(entity));
    EXPECT_TRUE(registry.has<NPCTag>(entity));
    EXPECT_TRUE(registry.has<NPCDialogue>(entity));

    auto& retrievedAI = registry.get<NPCAI>(entity);
    EXPECT_EQ(retrievedAI.behavior, "stationed");
    EXPECT_FLOAT_EQ(retrievedAI.interactionRange, 64.0f);
    EXPECT_FLOAT_EQ(retrievedAI.homePosition.x, 100.0f);
}

TEST(NPCIntegrationTest, BehaviorSwitch) {
    Registry registry;
    Entity entity = registry.create(Transform{Vec2(0.0f, 0.0f)});
    NPCAI ai("idle");
    ai.defaultBehavior = "idle";
    registry.add<NPCAI>(entity, std::move(ai));

    auto& retrievedAI = registry.get<NPCAI>(entity);

    // Switch to wander
    retrievedAI.behavior = NPCBehavior::Wander;
    EXPECT_EQ(retrievedAI.behavior, "wander");

    // Switch back to default
    retrievedAI.behavior = retrievedAI.defaultBehavior;
    EXPECT_EQ(retrievedAI.behavior, "idle");
}

TEST(NPCIntegrationTest, InteractionRangeCheck) {
    NPCAI ai;
    ai.interactionRange = 48.0f;

    Vec2 npcPos(100.0f, 100.0f);
    Vec2 playerNear(130.0f, 110.0f);   // ~33 units away
    Vec2 playerFar(200.0f, 200.0f);    // ~141 units away

    float dx1 = playerNear.x - npcPos.x;
    float dy1 = playerNear.y - npcPos.y;
    float dist1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
    EXPECT_LT(dist1, ai.interactionRange);

    float dx2 = playerFar.x - npcPos.x;
    float dy2 = playerFar.y - npcPos.y;
    float dist2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
    EXPECT_GT(dist2, ai.interactionRange);
}

// =============================================================================
// NPCSystem Custom Behavior Registration Tests
// =============================================================================

TEST(NPCSystemTest, RegisterCustomBehavior) {
    NPCSystem system;
    EXPECT_FALSE(system.hasBehavior("custom_greet"));

    system.registerBehavior("custom_greet", [](Entity, NPCAI&, float) {});

    EXPECT_TRUE(system.hasBehavior("custom_greet"));
}

TEST(NPCSystemTest, BuiltInBehaviorsNotRegistered) {
    NPCSystem system;
    EXPECT_FALSE(system.hasBehavior("idle"));
    EXPECT_FALSE(system.hasBehavior("wander"));
    EXPECT_FALSE(system.hasBehavior("stationed"));
}

// =============================================================================
// NPCDefinition Tests
// =============================================================================

TEST(NPCDefinitionTest, DefaultValues) {
    NPCDefinition def;
    EXPECT_EQ(def.aiBehavior, "idle");
    EXPECT_FLOAT_EQ(def.moveSpeed, 40.0f);
    EXPECT_FLOAT_EQ(def.wanderRadius, 80.0f);
    EXPECT_FLOAT_EQ(def.interactionRange, 48.0f);
    EXPECT_TRUE(def.dialogueId.empty());
    EXPECT_TRUE(def.shopId.empty());
    EXPECT_TRUE(def.requiresHousing);
    EXPECT_FLOAT_EQ(def.colliderWidth, 16.0f);
    EXPECT_FLOAT_EQ(def.colliderHeight, 16.0f);
}

TEST(NPCDefinitionTest, CustomValues) {
    NPCDefinition def;
    def.id = "merchant";
    def.qualifiedId = "base:merchant";
    def.name = "Traveling Merchant";
    def.aiBehavior = "stationed";
    def.moveSpeed = 0.0f;
    def.interactionRange = 64.0f;
    def.dialogueId = "base:merchant_dialogue";
    def.shopId = "base:merchant_shop";
    def.requiresHousing = false;

    EXPECT_EQ(def.name, "Traveling Merchant");
    EXPECT_EQ(def.aiBehavior, "stationed");
    EXPECT_FLOAT_EQ(def.moveSpeed, 0.0f);
    EXPECT_FLOAT_EQ(def.interactionRange, 64.0f);
    EXPECT_EQ(def.shopId, "base:merchant_shop");
    EXPECT_FALSE(def.requiresHousing);
}

// =============================================================================
// Content Registry NPC Tests
// =============================================================================

TEST(ContentRegistryNPCTest, RegisterAndRetrieve) {
    ContentRegistry registry;

    NPCDefinition def;
    def.id = "merchant";
    def.qualifiedId = "base:merchant";
    def.name = "Merchant";
    def.aiBehavior = "stationed";
    def.dialogueId = "base:merchant_dialogue";
    def.shopId = "base:merchant_shop";
    def.interactionRange = 64.0f;

    registry.registerNPC(def);

    EXPECT_TRUE(registry.hasNPC("base:merchant"));
    EXPECT_EQ(registry.npcCount(), 1u);

    const NPCDefinition* retrieved = registry.getNPC("base:merchant");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->name, "Merchant");
    EXPECT_EQ(retrieved->aiBehavior, "stationed");
    EXPECT_EQ(retrieved->dialogueId, "base:merchant_dialogue");
    EXPECT_EQ(retrieved->shopId, "base:merchant_shop");
    EXPECT_FLOAT_EQ(retrieved->interactionRange, 64.0f);
}

TEST(ContentRegistryNPCTest, GetNPCIds) {
    ContentRegistry registry;

    NPCDefinition merchant;
    merchant.id = "merchant";
    merchant.qualifiedId = "base:merchant";
    merchant.name = "Merchant";
    registry.registerNPC(merchant);

    NPCDefinition nurse;
    nurse.id = "nurse";
    nurse.qualifiedId = "base:nurse";
    nurse.name = "Nurse";
    registry.registerNPC(nurse);

    auto ids = registry.getNPCIds();
    EXPECT_EQ(ids.size(), 2u);

    bool hasMerchant = false, hasNurse = false;
    for (const auto& id : ids) {
        if (id == "base:merchant") hasMerchant = true;
        if (id == "base:nurse") hasNurse = true;
    }
    EXPECT_TRUE(hasMerchant);
    EXPECT_TRUE(hasNurse);
}

// =============================================================================
// Content Registry Shop Tests
// =============================================================================

TEST(ContentRegistryShopTest, RegisterAndRetrieve) {
    ContentRegistry registry;

    ShopDefinition shop;
    shop.id = "general_store";
    shop.qualifiedId = "base:general_store";
    shop.name = "General Store";
    shop.buyMultiplier = 1.2f;
    shop.sellMultiplier = 0.4f;

    ShopItemEntry potion;
    potion.itemId = "base:healing_potion";
    potion.buyPrice = 50;
    potion.sellPrice = 10;
    potion.stock = -1;
    shop.items.push_back(potion);

    ShopItemEntry torch;
    torch.itemId = "base:torch";
    torch.buyPrice = 5;
    torch.sellPrice = 1;
    torch.stock = 20;
    shop.items.push_back(torch);

    registry.registerShop(shop);

    EXPECT_TRUE(registry.hasShop("base:general_store"));
    EXPECT_EQ(registry.shopCount(), 1u);

    const ShopDefinition* retrieved = registry.getShop("base:general_store");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->name, "General Store");
    EXPECT_FLOAT_EQ(retrieved->buyMultiplier, 1.2f);
    EXPECT_FLOAT_EQ(retrieved->sellMultiplier, 0.4f);
    EXPECT_EQ(retrieved->items.size(), 2u);
    EXPECT_EQ(retrieved->items[0].itemId, "base:healing_potion");
    EXPECT_EQ(retrieved->items[1].stock, 20);
}

// =============================================================================
// Content Registry Dialogue Tree Tests
// =============================================================================

TEST(ContentRegistryDialogueTest, RegisterAndRetrieve) {
    ContentRegistry registry;

    DialogueTreeDef tree;
    tree.id = "merchant_dialogue";
    tree.qualifiedId = "base:merchant_dialogue";
    tree.greetingNodeId = "greeting";

    DialogueNodeDef node;
    node.id = "greeting";
    node.speaker = "Merchant";
    node.text = "Welcome to my shop!";

    DialogueChoiceDef choice1;
    choice1.text = "Show me your wares";
    choice1.nextNodeId = "shop";
    node.choices.push_back(choice1);

    DialogueChoiceDef choice2;
    choice2.text = "Goodbye";
    choice2.nextNodeId = "";
    node.choices.push_back(choice2);

    tree.nodes.push_back(node);
    registry.registerDialogueTree(tree);

    EXPECT_TRUE(registry.hasDialogueTree("base:merchant_dialogue"));

    const DialogueTreeDef* retrieved = registry.getDialogueTree("base:merchant_dialogue");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->greetingNodeId, "greeting");
    EXPECT_EQ(retrieved->nodes.size(), 1u);
    EXPECT_EQ(retrieved->nodes[0].speaker, "Merchant");
    EXPECT_EQ(retrieved->nodes[0].choices.size(), 2u);
    EXPECT_EQ(retrieved->nodes[0].choices[0].text, "Show me your wares");
}

// =============================================================================
// ShopDefinition Tests
// =============================================================================

TEST(ShopDefinitionTest, DefaultValues) {
    ShopDefinition shop;
    EXPECT_TRUE(shop.id.empty());
    EXPECT_TRUE(shop.name.empty());
    EXPECT_FLOAT_EQ(shop.buyMultiplier, 1.0f);
    EXPECT_FLOAT_EQ(shop.sellMultiplier, 0.5f);
    EXPECT_EQ(shop.currencyItem, "base:coins");
    EXPECT_TRUE(shop.items.empty());
}

TEST(ShopItemEntryTest, DefaultValues) {
    ShopItemEntry entry;
    EXPECT_TRUE(entry.itemId.empty());
    EXPECT_EQ(entry.buyPrice, 10);
    EXPECT_EQ(entry.sellPrice, 5);
    EXPECT_EQ(entry.stock, -1);
    EXPECT_TRUE(entry.available);
}

// =============================================================================
// ShopManager Buy/Sell Logic Tests
// =============================================================================

TEST(ShopManagerTest, BuySuccess) {
    ContentRegistry contentRegistry;

    ShopDefinition shop;
    shop.id = "store";
    shop.qualifiedId = "base:store";
    shop.name = "Test Store";
    shop.buyMultiplier = 1.0f;
    shop.currencyItem = "base:coins";

    ShopItemEntry item;
    item.itemId = "base:potion";
    item.buyPrice = 10;
    item.stock = -1;
    item.available = true;
    shop.items.push_back(item);

    contentRegistry.registerShop(shop);

    ShopManager manager;
    manager.setContentRegistry(&contentRegistry);

    Inventory inv;
    inv.addItem("base:coins", 100);

    TradeResult result = manager.buyItem("base:store", "base:potion", 3, inv);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.finalPrice, 30);
    EXPECT_EQ(inv.countItem("base:coins"), 70);
    EXPECT_EQ(inv.countItem("base:potion"), 3);
}

TEST(ShopManagerTest, BuyInsufficientFunds) {
    ContentRegistry contentRegistry;

    ShopDefinition shop;
    shop.id = "store";
    shop.qualifiedId = "base:store";
    shop.buyMultiplier = 1.0f;
    shop.currencyItem = "base:coins";

    ShopItemEntry item;
    item.itemId = "base:potion";
    item.buyPrice = 50;
    item.stock = -1;
    item.available = true;
    shop.items.push_back(item);

    contentRegistry.registerShop(shop);

    ShopManager manager;
    manager.setContentRegistry(&contentRegistry);

    Inventory inv;
    inv.addItem("base:coins", 20);

    TradeResult result = manager.buyItem("base:store", "base:potion", 1, inv);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.failReason, "insufficient funds");
    EXPECT_EQ(inv.countItem("base:coins"), 20);
    EXPECT_EQ(inv.countItem("base:potion"), 0);
}

TEST(ShopManagerTest, BuyItemNotSold) {
    ContentRegistry contentRegistry;

    ShopDefinition shop;
    shop.id = "store";
    shop.qualifiedId = "base:store";
    contentRegistry.registerShop(shop);

    ShopManager manager;
    manager.setContentRegistry(&contentRegistry);

    Inventory inv;
    inv.addItem("base:coins", 100);

    TradeResult result = manager.buyItem("base:store", "base:nonexistent", 1, inv);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.failReason, "item not sold here");
}

TEST(ShopManagerTest, SellItem) {
    ContentRegistry contentRegistry;

    ShopDefinition shop;
    shop.id = "store";
    shop.qualifiedId = "base:store";
    shop.sellMultiplier = 1.0f;
    shop.currencyItem = "base:coins";

    ShopItemEntry entry;
    entry.itemId = "base:gem";
    entry.sellPrice = 25;
    shop.items.push_back(entry);

    contentRegistry.registerShop(shop);

    ShopManager manager;
    manager.setContentRegistry(&contentRegistry);

    Inventory inv;
    inv.addItem("base:gem", 5);
    inv.addItem("base:coins", 10);

    TradeResult result = manager.sellItem("base:store", "base:gem", 2, inv);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.finalPrice, 50);
    EXPECT_EQ(inv.countItem("base:gem"), 3);
    EXPECT_EQ(inv.countItem("base:coins"), 60);
}

TEST(ShopManagerTest, SellItemNotInInventory) {
    ContentRegistry contentRegistry;

    ShopDefinition shop;
    shop.id = "store";
    shop.qualifiedId = "base:store";
    contentRegistry.registerShop(shop);

    ShopManager manager;
    manager.setContentRegistry(&contentRegistry);

    Inventory inv;

    TradeResult result = manager.sellItem("base:store", "base:gem", 1, inv);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.failReason, "item not in inventory");
}

TEST(ShopManagerTest, BuyDecrementsStock) {
    ContentRegistry contentRegistry;

    ShopDefinition shop;
    shop.id = "store";
    shop.qualifiedId = "base:store";
    shop.buyMultiplier = 1.0f;
    shop.currencyItem = "base:coins";

    ShopItemEntry item;
    item.itemId = "base:torch";
    item.buyPrice = 5;
    item.stock = 10;
    item.available = true;
    shop.items.push_back(item);

    contentRegistry.registerShop(shop);

    ShopManager manager;
    manager.setContentRegistry(&contentRegistry);

    Inventory inv;
    inv.addItem("base:coins", 200);

    // Buy 3 torches (stock 10 -> 7)
    TradeResult result = manager.buyItem("base:store", "base:torch", 3, inv);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.finalPrice, 15);
    EXPECT_EQ(inv.countItem("base:torch"), 3);
    EXPECT_EQ(manager.getRemainingStock("base:store", "base:torch"), 7);

    // Buy 8 more — only 7 in stock, should clamp to 7
    result = manager.buyItem("base:store", "base:torch", 8, inv);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(inv.countItem("base:torch"), 10); // 3 + 7
    EXPECT_EQ(result.finalPrice, 35); // 7 * 5
    EXPECT_EQ(manager.getRemainingStock("base:store", "base:torch"), 0);

    // Try to buy more (0 stock)
    result = manager.buyItem("base:store", "base:torch", 1, inv);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.failReason, "out of stock");
}

TEST(ShopManagerTest, BuyDeductsCurrencyBeforeAddingItems) {
    ContentRegistry contentRegistry;

    ShopDefinition shop;
    shop.id = "store";
    shop.qualifiedId = "base:store";
    shop.buyMultiplier = 1.0f;
    shop.currencyItem = "base:coins";

    ShopItemEntry item;
    item.itemId = "base:potion";
    item.buyPrice = 10;
    item.stock = -1;
    item.available = true;
    shop.items.push_back(item);

    contentRegistry.registerShop(shop);

    ShopManager manager;
    manager.setContentRegistry(&contentRegistry);

    // Insufficient funds should leave inventory unchanged
    Inventory inv;
    inv.addItem("base:coins", 5);

    TradeResult result = manager.buyItem("base:store", "base:potion", 1, inv);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.failReason, "insufficient funds");
    EXPECT_EQ(inv.countItem("base:coins"), 5);
    EXPECT_EQ(inv.countItem("base:potion"), 0);
}

TEST(ShopManagerTest, GetBuySellPrices) {
    ContentRegistry contentRegistry;

    ShopDefinition shop;
    shop.id = "store";
    shop.qualifiedId = "base:store";
    shop.buyMultiplier = 1.5f;
    shop.sellMultiplier = 0.5f;

    ShopItemEntry entry;
    entry.itemId = "base:potion";
    entry.buyPrice = 100;
    entry.sellPrice = 40;
    shop.items.push_back(entry);

    contentRegistry.registerShop(shop);

    ShopManager manager;
    manager.setContentRegistry(&contentRegistry);

    // buyPrice = ceil(100 * 1.5) = 150
    EXPECT_EQ(manager.getBuyPrice("base:store", "base:potion"), 150);
    // sellPrice = floor(40 * 0.5) = 20
    EXPECT_EQ(manager.getSellPrice("base:store", "base:potion"), 20);
}

// =============================================================================
// HousingRequirements Tests
// =============================================================================

TEST(HousingRequirementsTest, DefaultValues) {
    HousingRequirements reqs;
    EXPECT_EQ(reqs.minWidth, 6);
    EXPECT_EQ(reqs.minHeight, 4);
    EXPECT_EQ(reqs.maxWidth, 50);
    EXPECT_EQ(reqs.maxHeight, 50);
    EXPECT_TRUE(reqs.requireDoor);
    EXPECT_TRUE(reqs.requireLightSource);
    EXPECT_TRUE(reqs.requireFurniture);
    EXPECT_TRUE(reqs.doorTiles.empty());
    EXPECT_TRUE(reqs.lightTiles.empty());
    EXPECT_TRUE(reqs.furnitureTiles.empty());
}

TEST(HousingRequirementsTest, CustomValues) {
    HousingRequirements reqs;
    reqs.minWidth = 8;
    reqs.minHeight = 6;
    reqs.requireDoor = false;
    reqs.doorTiles = {"base:wooden_door", "base:iron_door"};

    EXPECT_EQ(reqs.minWidth, 8);
    EXPECT_EQ(reqs.minHeight, 6);
    EXPECT_FALSE(reqs.requireDoor);
    EXPECT_EQ(reqs.doorTiles.size(), 2u);
}

// =============================================================================
// ValidatedRoom Tests
// =============================================================================

TEST(ValidatedRoomTest, DefaultInvalid) {
    ValidatedRoom room;
    EXPECT_EQ(room.id, 0);
    EXPECT_FALSE(room.isValid);
    EXPECT_FALSE(room.hasDoor);
    EXPECT_FALSE(room.hasLight);
    EXPECT_FALSE(room.hasFurniture);
    EXPECT_EQ(room.assignedNPC, NullEntity);
}

TEST(ValidatedRoomTest, AssignNPC) {
    ValidatedRoom room;
    room.isValid = true;
    room.assignedNPC = static_cast<Entity>(42);
    EXPECT_EQ(room.assignedNPC, static_cast<Entity>(42));
}

// =============================================================================
// TradeResult Tests
// =============================================================================

TEST(TradeResultTest, DefaultValues) {
    TradeResult result;
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.failReason.empty());
    EXPECT_EQ(result.finalPrice, 0);
}

// =============================================================================
// Multiple NPC Entity Creation Test
// =============================================================================

TEST(NPCEntityTest, MultipleNPCsWithDifferentBehaviors) {
    Registry registry;

    // Create a merchant
    Entity merchant = registry.create(
        Transform{Vec2(100.0f, 300.0f)},
        NPCTag{"base:merchant"}
    );
    registry.add<NPCAI>(merchant, NPCAI("stationed"));
    registry.add<NPCDialogue>(merchant);
    registry.add<ShopKeeper>(merchant, ShopKeeper{"base:merchant_shop"});

    // Create a wandering guide
    Entity guide = registry.create(
        Transform{Vec2(200.0f, 300.0f)},
        NPCTag{"base:guide"}
    );
    registry.add<NPCAI>(guide, NPCAI("wander"));
    registry.add<NPCDialogue>(guide);

    // Create an idle nurse
    Entity nurse = registry.create(
        Transform{Vec2(300.0f, 300.0f)},
        NPCTag{"base:nurse"}
    );
    registry.add<NPCAI>(nurse, NPCAI("idle"));

    // Verify all have different behaviors
    EXPECT_EQ(registry.get<NPCAI>(merchant).behavior, "stationed");
    EXPECT_EQ(registry.get<NPCAI>(guide).behavior, "wander");
    EXPECT_EQ(registry.get<NPCAI>(nurse).behavior, "idle");

    // Only merchant has ShopKeeper
    EXPECT_TRUE(registry.has<ShopKeeper>(merchant));
    EXPECT_FALSE(registry.has<ShopKeeper>(guide));
    EXPECT_FALSE(registry.has<ShopKeeper>(nurse));

    // Count NPCs
    int npcCount = 0;
    registry.each<NPCTag>([&npcCount](Entity, const NPCTag&) { ++npcCount; });
    EXPECT_EQ(npcCount, 3);
}

// =============================================================================
// Content Registry Clear includes NPC data
// =============================================================================

TEST(ContentRegistryClearTest, ClearsNPCAndShopData) {
    ContentRegistry registry;

    NPCDefinition npc;
    npc.id = "test";
    npc.qualifiedId = "base:test";
    registry.registerNPC(npc);

    ShopDefinition shop;
    shop.id = "shop";
    shop.qualifiedId = "base:shop";
    registry.registerShop(shop);

    DialogueTreeDef tree;
    tree.id = "dlg";
    tree.qualifiedId = "base:dlg";
    registry.registerDialogueTree(tree);

    EXPECT_EQ(registry.npcCount(), 1u);
    EXPECT_EQ(registry.shopCount(), 1u);
    EXPECT_TRUE(registry.hasDialogueTree("base:dlg"));

    registry.clear();

    EXPECT_EQ(registry.npcCount(), 0u);
    EXPECT_EQ(registry.shopCount(), 0u);
    EXPECT_FALSE(registry.hasNPC("base:test"));
    EXPECT_FALSE(registry.hasShop("base:shop"));
    EXPECT_FALSE(registry.hasDialogueTree("base:dlg"));
}

// =============================================================================
// NPC Reference Validation
// =============================================================================

TEST(ContentRegistryValidationTest, ValidateNPCReferencesNoThrow) {
    ContentRegistry registry;

    // NPC with valid references
    DialogueTreeDef tree;
    tree.id = "dlg";
    tree.qualifiedId = "base:dlg";
    registry.registerDialogueTree(tree);

    ShopDefinition shop;
    shop.id = "shop";
    shop.qualifiedId = "base:shop";
    registry.registerShop(shop);

    NPCDefinition npc;
    npc.id = "merchant";
    npc.qualifiedId = "base:merchant";
    npc.dialogueId = "base:dlg";
    npc.shopId = "base:shop";
    registry.registerNPC(npc);

    // Should not throw — references are valid
    EXPECT_NO_THROW(registry.validateNPCReferences());

    // NPC with broken references should also not throw (just logs warnings)
    NPCDefinition broken;
    broken.id = "broken";
    broken.qualifiedId = "base:broken";
    broken.dialogueId = "base:nonexistent_dlg";
    broken.shopId = "base:nonexistent_shop";
    registry.registerNPC(broken);

    EXPECT_NO_THROW(registry.validateNPCReferences());
}

// =============================================================================
// NPCAI wanderDirectionY default
// =============================================================================

TEST(NPCAITest, WanderDirectionYDefault) {
    NPCAI ai;
    EXPECT_EQ(ai.wanderDirectionY, 0);
}
