#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <cstdint>

namespace gloaming {

/// Lightweight color type for content definitions (avoids coupling to rendering)
struct ContentColor {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;

    constexpr ContentColor() = default;
    constexpr ContentColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}

    static constexpr ContentColor White() { return {255, 255, 255, 255}; }
};

// ---------------------------------------------------------------------------
// Content ID: "mod_id:content_id" namespacing
// ---------------------------------------------------------------------------

/// Parse a namespaced content ID (e.g. "base-game:dirt" -> {"base-game", "dirt"})
struct ContentId {
    std::string modId;
    std::string localId;

    ContentId() = default;
    ContentId(const std::string& mod, const std::string& local) : modId(mod), localId(local) {}

    /// Full qualified name
    std::string full() const { return modId + ":" + localId; }

    /// Parse from string. If no ":", uses defaultMod as the mod prefix.
    static ContentId parse(const std::string& str, const std::string& defaultMod = "") {
        auto pos = str.find(':');
        if (pos != std::string::npos) {
            return {str.substr(0, pos), str.substr(pos + 1)};
        }
        return {defaultMod, str};
    }
};

// ---------------------------------------------------------------------------
// Tile Definition
// ---------------------------------------------------------------------------

struct TileContentDef {
    std::string id;                  // Local ID (unqualified)
    std::string qualifiedId;         // "mod:id" fully qualified
    std::string name;                // Display name (or localization key)
    std::string texturePath;         // Path to texture relative to mod dir
    int variants = 1;                // Number of visual variants
    bool solid = true;
    bool transparent = false;
    float hardness = 1.0f;
    float requiredPickaxePower = 0.0f;

    // Drop when broken
    std::string dropItem;            // Item ID to drop
    int dropCount = 1;

    // Light emission
    bool emitsLight = false;
    ContentColor lightColor = ContentColor::White();
    float lightIntensity = 0.0f;

    // Sounds
    std::string breakSound;
    std::string placeSound;

    // Flags for physics
    bool isPlatform = false;
    bool isSlopeLeft = false;
    bool isSlopeRight = false;

    // Runtime tile ID assigned by the registry
    uint16_t runtimeId = 0;
};

// ---------------------------------------------------------------------------
// Item Definition
// ---------------------------------------------------------------------------

struct ItemDefinition {
    std::string id;
    std::string qualifiedId;
    std::string name;
    std::string description;
    std::string texturePath;

    std::string type = "material";   // material, weapon, tool, consumable, block, accessory
    std::string weaponType;          // melee, ranged, magic (if type == weapon)

    int damage = 0;
    float knockback = 0.0f;
    int useTime = 30;                // Ticks between uses
    float swingArc = 0.0f;           // Degrees for melee
    float critChance = 0.04f;

    std::string rarity = "common";   // common, uncommon, rare, epic, legendary
    int sellValue = 0;
    int maxStack = 999;

    // Tool properties
    float pickaxePower = 0.0f;
    float axePower = 0.0f;

    // Places a tile
    std::string placesTile;

    // Light emission when held
    bool emitsLight = false;
    ContentColor lightColor = ContentColor::White();
    float lightIntensity = 0.0f;

    // Script for custom behavior
    std::string onHitScript;
    std::string onUseScript;
};

// ---------------------------------------------------------------------------
// Enemy Definition
// ---------------------------------------------------------------------------

struct EnemyAnimationDef {
    std::string name;
    std::vector<int> frames;
    int fps = 8;
};

struct EnemyDropDef {
    std::string item;
    int countMin = 1;
    int countMax = 1;
    float chance = 1.0f;
};

struct EnemySpawnConditions {
    std::vector<std::string> biomes;
    float depthMin = 0.0f;
    float depthMax = 10000.0f;
    float lightLevelMax = 1.0f;
};

struct EnemyDefinition {
    std::string id;
    std::string qualifiedId;
    std::string name;
    std::string texturePath;

    std::vector<EnemyAnimationDef> animations;

    float health = 100.0f;
    int damage = 10;
    int defense = 0;
    float knockbackResist = 0.0f;

    std::string behaviorScript;

    EnemySpawnConditions spawnConditions;
    std::vector<EnemyDropDef> drops;

    // Sounds
    std::string hurtSound;
    std::string deathSound;

    // AI configuration (Stage 14)
    std::string aiBehavior;              // Built-in behavior name (empty = auto-select by view mode)
    float detectionRange = 200.0f;
    float attackRange = 32.0f;
    float moveSpeed = 60.0f;
    float patrolRadius = 100.0f;
    float fleeThreshold = 0.2f;         // Flee when health % drops below this
    float despawnDistance = 1500.0f;     // Distance from player to despawn (0 = never)
    float orbitDistance = 100.0f;        // For orbit behavior
    float orbitSpeed = 2.0f;            // For orbit behavior

    // Collider size (defaults to 16x16)
    float colliderWidth = 16.0f;
    float colliderHeight = 16.0f;
};

// ---------------------------------------------------------------------------
// NPC Definition (Stage 15)
// ---------------------------------------------------------------------------

struct NPCAnimationDef {
    std::string name;
    std::vector<int> frames;
    int fps = 8;
};

struct NPCDefinition {
    std::string id;
    std::string qualifiedId;
    std::string name;
    std::string texturePath;

    std::vector<NPCAnimationDef> animations;

    // AI configuration
    std::string aiBehavior = "idle";     // idle, wander, schedule, stationed
    float moveSpeed = 40.0f;
    float wanderRadius = 80.0f;
    float interactionRange = 48.0f;

    // Dialogue reference
    std::string dialogueId;              // References a DialogueTreeDef

    // Shop reference
    std::string shopId;                  // References a ShopDefinition (empty = no shop)

    // Housing
    bool requiresHousing = true;

    // Collider
    float colliderWidth = 16.0f;
    float colliderHeight = 16.0f;
};

// ---------------------------------------------------------------------------
// Dialogue Tree Definition (Stage 15)
// ---------------------------------------------------------------------------

struct DialogueChoiceDef {
    std::string text;
    std::string nextNodeId;              // Empty = end dialogue
};

struct DialogueNodeDef {
    std::string id;
    std::string speaker;
    std::string text;
    std::string portraitId;
    std::vector<DialogueChoiceDef> choices;
    std::string nextNodeId;              // Auto-advance (if no choices)
};

struct DialogueTreeDef {
    std::string id;
    std::string qualifiedId;
    std::string greetingNodeId;          // Starting node ID
    std::vector<DialogueNodeDef> nodes;
};

// ---------------------------------------------------------------------------
// Shop Definition (Stage 15)
// ---------------------------------------------------------------------------

struct ShopItemEntry {
    std::string itemId;                  // Content-registry qualified item ID
    int buyPrice = 10;                   // Cost to buy from shop
    int sellPrice = 5;                   // Price shop pays for this item
    int stock = -1;                      // -1 = infinite
    bool available = true;
};

struct ShopDefinition {
    std::string id;
    std::string qualifiedId;
    std::string name;
    std::vector<ShopItemEntry> items;
    float buyMultiplier = 1.0f;          // Price modifier for buying
    float sellMultiplier = 0.5f;         // Sell ratio
    std::string currencyItem = "base:coins"; // Item used as currency
};

// ---------------------------------------------------------------------------
// Recipe Definition
// ---------------------------------------------------------------------------

struct RecipeIngredient {
    std::string item;
    int count = 1;
};

struct RecipeDefinition {
    std::string id;
    std::string qualifiedId;

    std::string resultItem;
    int resultCount = 1;

    std::vector<RecipeIngredient> ingredients;

    std::string station;             // Required crafting station (tile ID), empty = hand-craft
    std::string category = "misc";
};

// ---------------------------------------------------------------------------
// Content Registry
// ---------------------------------------------------------------------------

class ContentRegistry {
public:
    ContentRegistry() = default;

    // ---- Registration ----

    /// Register a tile definition. Returns the assigned runtime ID.
    uint16_t registerTile(const TileContentDef& def);

    /// Register an item definition.
    void registerItem(const ItemDefinition& def);

    /// Register an enemy definition.
    void registerEnemy(const EnemyDefinition& def);

    /// Register a recipe definition.
    void registerRecipe(const RecipeDefinition& def);

    /// Register an NPC definition (Stage 15).
    void registerNPC(const NPCDefinition& def);

    /// Register a dialogue tree definition (Stage 15).
    void registerDialogueTree(const DialogueTreeDef& def);

    /// Register a shop definition (Stage 15).
    void registerShop(const ShopDefinition& def);

    // ---- Bulk loading from JSON ----

    /// Load tile definitions from a JSON file. modId is used for namespacing.
    bool loadTilesFromJson(const nlohmann::json& json, const std::string& modId,
                           const std::string& modDir);

    /// Load item definitions from JSON.
    bool loadItemsFromJson(const nlohmann::json& json, const std::string& modId,
                           const std::string& modDir);

    /// Load enemy definitions from JSON.
    bool loadEnemiesFromJson(const nlohmann::json& json, const std::string& modId,
                             const std::string& modDir);

    /// Load recipe definitions from JSON.
    bool loadRecipesFromJson(const nlohmann::json& json, const std::string& modId);

    /// Load NPC definitions from JSON (Stage 15).
    bool loadNPCsFromJson(const nlohmann::json& json, const std::string& modId,
                          const std::string& modDir);

    /// Load dialogue tree definitions from JSON (Stage 15).
    bool loadDialogueFromJson(const nlohmann::json& json, const std::string& modId);

    /// Load shop definitions from JSON (Stage 15).
    bool loadShopsFromJson(const nlohmann::json& json, const std::string& modId);

    // ---- Queries ----

    const TileContentDef* getTile(const std::string& qualifiedId) const;
    const TileContentDef* getTileByRuntime(uint16_t runtimeId) const;
    const ItemDefinition* getItem(const std::string& qualifiedId) const;
    const EnemyDefinition* getEnemy(const std::string& qualifiedId) const;
    const RecipeDefinition* getRecipe(const std::string& qualifiedId) const;
    const NPCDefinition* getNPC(const std::string& qualifiedId) const;
    const DialogueTreeDef* getDialogueTree(const std::string& qualifiedId) const;
    const ShopDefinition* getShop(const std::string& qualifiedId) const;

    bool hasTile(const std::string& qualifiedId) const;
    bool hasItem(const std::string& qualifiedId) const;
    bool hasEnemy(const std::string& qualifiedId) const;
    bool hasNPC(const std::string& qualifiedId) const;
    bool hasShop(const std::string& qualifiedId) const;
    bool hasDialogueTree(const std::string& qualifiedId) const;

    /// Get all tile IDs
    std::vector<std::string> getTileIds() const;
    std::vector<std::string> getItemIds() const;
    std::vector<std::string> getEnemyIds() const;
    std::vector<std::string> getRecipeIds() const;
    std::vector<std::string> getNPCIds() const;
    std::vector<std::string> getShopIds() const;

    /// Get recipes by category
    std::vector<const RecipeDefinition*> getRecipesByCategory(const std::string& category) const;

    /// Get recipes that produce a specific item
    std::vector<const RecipeDefinition*> getRecipesForItem(const std::string& itemId) const;

    /// Get the item ID that places a given tile (reverse lookup). Returns empty string if none.
    std::string getItemForTile(const std::string& tileId) const;

    // ---- Validation ----

    /// Validate that NPC dialogue/shop references exist. Call after all content is loaded.
    /// Logs warnings for any broken references.
    void validateNPCReferences() const;

    // ---- Stats ----

    size_t tileCount() const { return m_tiles.size(); }
    size_t itemCount() const { return m_items.size(); }
    size_t enemyCount() const { return m_enemies.size(); }
    size_t recipeCount() const { return m_recipes.size(); }
    size_t npcCount() const { return m_npcs.size(); }
    size_t shopCount() const { return m_shops.size(); }

    /// Clear all registered content
    void clear();

private:
    std::unordered_map<std::string, TileContentDef> m_tiles;
    std::unordered_map<uint16_t, std::string> m_runtimeToTile;  // runtime ID -> qualified ID
    uint16_t m_nextTileId = 1;  // 0 = air/empty

    std::unordered_map<std::string, ItemDefinition> m_items;
    std::unordered_map<std::string, EnemyDefinition> m_enemies;
    std::unordered_map<std::string, RecipeDefinition> m_recipes;

    // Stage 15
    std::unordered_map<std::string, NPCDefinition> m_npcs;
    std::unordered_map<std::string, DialogueTreeDef> m_dialogueTrees;
    std::unordered_map<std::string, ShopDefinition> m_shops;

    /// Lazily built reverse lookup: tileId -> itemId (for items with placesTile)
    mutable std::unordered_map<std::string, std::string> m_tileToItem;
    mutable bool m_tileToItemDirty = true;
    void rebuildTileItemLookup() const;
};

} // namespace gloaming
