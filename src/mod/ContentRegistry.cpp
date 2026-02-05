#include "mod/ContentRegistry.hpp"
#include "engine/Log.hpp"

namespace gloaming {

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

uint16_t ContentRegistry::registerTile(const TileContentDef& def) {
    TileContentDef tile = def;
    tile.runtimeId = m_nextTileId++;
    std::string qid = tile.qualifiedId.empty()
        ? tile.id  // Fallback if not namespaced
        : tile.qualifiedId;

    if (m_tiles.count(qid)) {
        LOG_WARN("ContentRegistry: overwriting tile '{}'", qid);
    }

    m_runtimeToTile[tile.runtimeId] = qid;
    m_tiles[qid] = std::move(tile);
    LOG_DEBUG("ContentRegistry: registered tile '{}' (runtime ID {})", qid, m_tiles[qid].runtimeId);
    return m_tiles[qid].runtimeId;
}

void ContentRegistry::registerItem(const ItemDefinition& def) {
    std::string qid = def.qualifiedId.empty() ? def.id : def.qualifiedId;
    if (m_items.count(qid)) {
        LOG_WARN("ContentRegistry: overwriting item '{}'", qid);
    }
    m_items[qid] = def;
    LOG_DEBUG("ContentRegistry: registered item '{}'", qid);
}

void ContentRegistry::registerEnemy(const EnemyDefinition& def) {
    std::string qid = def.qualifiedId.empty() ? def.id : def.qualifiedId;
    if (m_enemies.count(qid)) {
        LOG_WARN("ContentRegistry: overwriting enemy '{}'", qid);
    }
    m_enemies[qid] = def;
    LOG_DEBUG("ContentRegistry: registered enemy '{}'", qid);
}

void ContentRegistry::registerRecipe(const RecipeDefinition& def) {
    std::string qid = def.qualifiedId.empty() ? def.id : def.qualifiedId;
    if (m_recipes.count(qid)) {
        LOG_WARN("ContentRegistry: overwriting recipe '{}'", qid);
    }
    m_recipes[qid] = def;
    LOG_DEBUG("ContentRegistry: registered recipe '{}'", qid);
}

// ---------------------------------------------------------------------------
// JSON Loading
// ---------------------------------------------------------------------------

bool ContentRegistry::loadTilesFromJson(const nlohmann::json& json,
                                         const std::string& modId,
                                         const std::string& modDir) {
    if (!json.contains("tiles") || !json["tiles"].is_array()) {
        LOG_WARN("ContentRegistry: no 'tiles' array in JSON for mod '{}'", modId);
        return false;
    }

    int count = 0;
    for (const auto& tileJson : json["tiles"]) {
        TileContentDef tile;
        tile.id = tileJson.value("id", "");
        if (tile.id.empty()) {
            LOG_WARN("ContentRegistry: tile missing 'id' in mod '{}'", modId);
            continue;
        }
        tile.qualifiedId = modId + ":" + tile.id;
        tile.name = tileJson.value("name", tile.id);

        // Texture path: resolve relative to mod directory
        std::string texRel = tileJson.value("texture", "");
        tile.texturePath = texRel.empty() ? "" : modDir + "/" + texRel;

        tile.variants = tileJson.value("variants", 1);
        tile.solid = tileJson.value("solid", true);
        tile.transparent = tileJson.value("transparent", false);
        tile.hardness = tileJson.value("hardness", 1.0f);
        tile.requiredPickaxePower = tileJson.value("required_pickaxe_power", 0.0f);

        // Drop
        if (tileJson.contains("drop") && tileJson["drop"].is_object()) {
            const auto& drop = tileJson["drop"];
            tile.dropItem = drop.value("item", "");
            tile.dropCount = drop.value("count", 1);
        }

        // Light emission
        if (tileJson.contains("light_emission") && tileJson["light_emission"].is_object()) {
            const auto& light = tileJson["light_emission"];
            tile.emitsLight = true;
            tile.lightColor = Color(
                static_cast<uint8_t>(light.value("r", 255)),
                static_cast<uint8_t>(light.value("g", 255)),
                static_cast<uint8_t>(light.value("b", 255))
            );
            tile.lightIntensity = light.value("intensity", 0.5f);
        }

        // Sounds
        std::string breakSnd = tileJson.value("break_sound", "");
        tile.breakSound = breakSnd.empty() ? "" : modDir + "/" + breakSnd;
        std::string placeSnd = tileJson.value("place_sound", "");
        tile.placeSound = placeSnd.empty() ? "" : modDir + "/" + placeSnd;

        // Platform / slope flags
        tile.isPlatform = tileJson.value("platform", false);
        tile.isSlopeLeft = tileJson.value("slope_left", false);
        tile.isSlopeRight = tileJson.value("slope_right", false);

        registerTile(tile);
        ++count;
    }

    LOG_INFO("ContentRegistry: loaded {} tiles from mod '{}'", count, modId);
    return true;
}

bool ContentRegistry::loadItemsFromJson(const nlohmann::json& json,
                                          const std::string& modId,
                                          const std::string& modDir) {
    if (!json.contains("items") || !json["items"].is_array()) {
        LOG_WARN("ContentRegistry: no 'items' array in JSON for mod '{}'", modId);
        return false;
    }

    int count = 0;
    for (const auto& itemJson : json["items"]) {
        ItemDefinition item;
        item.id = itemJson.value("id", "");
        if (item.id.empty()) {
            LOG_WARN("ContentRegistry: item missing 'id' in mod '{}'", modId);
            continue;
        }
        item.qualifiedId = modId + ":" + item.id;
        item.name = itemJson.value("name", item.id);
        item.description = itemJson.value("description", "");

        std::string texRel = itemJson.value("texture", "");
        item.texturePath = texRel.empty() ? "" : modDir + "/" + texRel;

        item.type = itemJson.value("type", "material");
        item.weaponType = itemJson.value("weapon_type", "");
        item.damage = itemJson.value("damage", 0);
        item.knockback = itemJson.value("knockback", 0.0f);
        item.useTime = itemJson.value("use_time", 30);
        item.swingArc = itemJson.value("swing_arc", 0.0f);
        item.critChance = itemJson.value("crit_chance", 0.04f);
        item.rarity = itemJson.value("rarity", "common");
        item.sellValue = itemJson.value("sell_value", 0);
        item.maxStack = itemJson.value("max_stack", 999);

        item.pickaxePower = itemJson.value("pickaxe_power", 0.0f);
        item.axePower = itemJson.value("axe_power", 0.0f);
        item.placesTile = itemJson.value("places_tile", "");

        // Light
        if (itemJson.contains("light_emission") && itemJson["light_emission"].is_object()) {
            const auto& light = itemJson["light_emission"];
            item.emitsLight = true;
            item.lightColor = Color(
                static_cast<uint8_t>(light.value("r", 255)),
                static_cast<uint8_t>(light.value("g", 255)),
                static_cast<uint8_t>(light.value("b", 255))
            );
            item.lightIntensity = light.value("intensity", 0.5f);
        }

        // Scripts
        std::string onHit = itemJson.value("on_hit", "");
        item.onHitScript = onHit.empty() ? "" : modDir + "/" + onHit;
        std::string onUse = itemJson.value("on_use", "");
        item.onUseScript = onUse.empty() ? "" : modDir + "/" + onUse;

        registerItem(item);
        ++count;
    }

    LOG_INFO("ContentRegistry: loaded {} items from mod '{}'", count, modId);
    return true;
}

bool ContentRegistry::loadEnemiesFromJson(const nlohmann::json& json,
                                            const std::string& modId,
                                            const std::string& modDir) {
    if (!json.contains("enemies") || !json["enemies"].is_array()) {
        LOG_WARN("ContentRegistry: no 'enemies' array in JSON for mod '{}'", modId);
        return false;
    }

    int count = 0;
    for (const auto& enemyJson : json["enemies"]) {
        EnemyDefinition enemy;
        enemy.id = enemyJson.value("id", "");
        if (enemy.id.empty()) {
            LOG_WARN("ContentRegistry: enemy missing 'id' in mod '{}'", modId);
            continue;
        }
        enemy.qualifiedId = modId + ":" + enemy.id;
        enemy.name = enemyJson.value("name", enemy.id);

        std::string texRel = enemyJson.value("texture", "");
        enemy.texturePath = texRel.empty() ? "" : modDir + "/" + texRel;

        // Animations
        if (enemyJson.contains("animations") && enemyJson["animations"].is_object()) {
            for (auto& [animName, animData] : enemyJson["animations"].items()) {
                EnemyAnimationDef anim;
                anim.name = animName;
                if (animData.contains("frames") && animData["frames"].is_array()) {
                    for (const auto& f : animData["frames"]) {
                        anim.frames.push_back(f.get<int>());
                    }
                }
                anim.fps = animData.value("fps", 8);
                enemy.animations.push_back(std::move(anim));
            }
        }

        enemy.health = enemyJson.value("health", 100.0f);
        enemy.damage = enemyJson.value("damage", 10);
        enemy.defense = enemyJson.value("defense", 0);
        enemy.knockbackResist = enemyJson.value("knockback_resist", 0.0f);

        std::string behav = enemyJson.value("behavior", "");
        enemy.behaviorScript = behav.empty() ? "" : modDir + "/" + behav;

        // Spawn conditions
        if (enemyJson.contains("spawn_conditions") && enemyJson["spawn_conditions"].is_object()) {
            const auto& sc = enemyJson["spawn_conditions"];
            if (sc.contains("biomes") && sc["biomes"].is_array()) {
                for (const auto& b : sc["biomes"]) {
                    enemy.spawnConditions.biomes.push_back(b.get<std::string>());
                }
            }
            if (sc.contains("depth") && sc["depth"].is_object()) {
                enemy.spawnConditions.depthMin = sc["depth"].value("min", 0.0f);
                enemy.spawnConditions.depthMax = sc["depth"].value("max", 10000.0f);
            }
            if (sc.contains("light_level") && sc["light_level"].is_object()) {
                enemy.spawnConditions.lightLevelMax = sc["light_level"].value("max", 1.0f);
            }
        }

        // Drops
        if (enemyJson.contains("drops") && enemyJson["drops"].is_array()) {
            for (const auto& dropJson : enemyJson["drops"]) {
                EnemyDropDef drop;
                drop.item = dropJson.value("item", "");
                drop.chance = dropJson.value("chance", 1.0f);
                if (dropJson.contains("count")) {
                    if (dropJson["count"].is_array() && dropJson["count"].size() == 2) {
                        drop.countMin = dropJson["count"][0].get<int>();
                        drop.countMax = dropJson["count"][1].get<int>();
                    } else {
                        drop.countMin = drop.countMax = dropJson["count"].get<int>();
                    }
                }
                enemy.drops.push_back(std::move(drop));
            }
        }

        // Sounds
        if (enemyJson.contains("sounds") && enemyJson["sounds"].is_object()) {
            std::string hurt = enemyJson["sounds"].value("hurt", "");
            enemy.hurtSound = hurt.empty() ? "" : modDir + "/" + hurt;
            std::string death = enemyJson["sounds"].value("death", "");
            enemy.deathSound = death.empty() ? "" : modDir + "/" + death;
        }

        registerEnemy(enemy);
        ++count;
    }

    LOG_INFO("ContentRegistry: loaded {} enemies from mod '{}'", count, modId);
    return true;
}

bool ContentRegistry::loadRecipesFromJson(const nlohmann::json& json,
                                            const std::string& modId) {
    if (!json.contains("recipes") || !json["recipes"].is_array()) {
        LOG_WARN("ContentRegistry: no 'recipes' array in JSON for mod '{}'", modId);
        return false;
    }

    int count = 0;
    for (const auto& recipeJson : json["recipes"]) {
        RecipeDefinition recipe;
        recipe.id = recipeJson.value("id", "");
        if (recipe.id.empty()) {
            LOG_WARN("ContentRegistry: recipe missing 'id' in mod '{}'", modId);
            continue;
        }
        recipe.qualifiedId = modId + ":" + recipe.id;

        if (recipeJson.contains("result") && recipeJson["result"].is_object()) {
            recipe.resultItem = recipeJson["result"].value("item", "");
            recipe.resultCount = recipeJson["result"].value("count", 1);
        }

        if (recipeJson.contains("ingredients") && recipeJson["ingredients"].is_array()) {
            for (const auto& ingJson : recipeJson["ingredients"]) {
                RecipeIngredient ing;
                ing.item = ingJson.value("item", "");
                ing.count = ingJson.value("count", 1);
                recipe.ingredients.push_back(std::move(ing));
            }
        }

        recipe.station = recipeJson.value("station", "");
        recipe.category = recipeJson.value("category", "misc");

        registerRecipe(recipe);
        ++count;
    }

    LOG_INFO("ContentRegistry: loaded {} recipes from mod '{}'", count, modId);
    return true;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

const TileContentDef* ContentRegistry::getTile(const std::string& qualifiedId) const {
    auto it = m_tiles.find(qualifiedId);
    return it != m_tiles.end() ? &it->second : nullptr;
}

const TileContentDef* ContentRegistry::getTileByRuntime(uint16_t runtimeId) const {
    auto it = m_runtimeToTile.find(runtimeId);
    if (it == m_runtimeToTile.end()) return nullptr;
    return getTile(it->second);
}

const ItemDefinition* ContentRegistry::getItem(const std::string& qualifiedId) const {
    auto it = m_items.find(qualifiedId);
    return it != m_items.end() ? &it->second : nullptr;
}

const EnemyDefinition* ContentRegistry::getEnemy(const std::string& qualifiedId) const {
    auto it = m_enemies.find(qualifiedId);
    return it != m_enemies.end() ? &it->second : nullptr;
}

const RecipeDefinition* ContentRegistry::getRecipe(const std::string& qualifiedId) const {
    auto it = m_recipes.find(qualifiedId);
    return it != m_recipes.end() ? &it->second : nullptr;
}

bool ContentRegistry::hasTile(const std::string& qualifiedId) const {
    return m_tiles.count(qualifiedId) > 0;
}

bool ContentRegistry::hasItem(const std::string& qualifiedId) const {
    return m_items.count(qualifiedId) > 0;
}

bool ContentRegistry::hasEnemy(const std::string& qualifiedId) const {
    return m_enemies.count(qualifiedId) > 0;
}

std::vector<std::string> ContentRegistry::getTileIds() const {
    std::vector<std::string> ids;
    ids.reserve(m_tiles.size());
    for (const auto& [id, _] : m_tiles) ids.push_back(id);
    return ids;
}

std::vector<std::string> ContentRegistry::getItemIds() const {
    std::vector<std::string> ids;
    ids.reserve(m_items.size());
    for (const auto& [id, _] : m_items) ids.push_back(id);
    return ids;
}

std::vector<std::string> ContentRegistry::getEnemyIds() const {
    std::vector<std::string> ids;
    ids.reserve(m_enemies.size());
    for (const auto& [id, _] : m_enemies) ids.push_back(id);
    return ids;
}

std::vector<std::string> ContentRegistry::getRecipeIds() const {
    std::vector<std::string> ids;
    ids.reserve(m_recipes.size());
    for (const auto& [id, _] : m_recipes) ids.push_back(id);
    return ids;
}

std::vector<const RecipeDefinition*> ContentRegistry::getRecipesByCategory(
        const std::string& category) const {
    std::vector<const RecipeDefinition*> results;
    for (const auto& [_, recipe] : m_recipes) {
        if (recipe.category == category) {
            results.push_back(&recipe);
        }
    }
    return results;
}

std::vector<const RecipeDefinition*> ContentRegistry::getRecipesForItem(
        const std::string& itemId) const {
    std::vector<const RecipeDefinition*> results;
    for (const auto& [_, recipe] : m_recipes) {
        if (recipe.resultItem == itemId) {
            results.push_back(&recipe);
        }
    }
    return results;
}

void ContentRegistry::clear() {
    m_tiles.clear();
    m_runtimeToTile.clear();
    m_nextTileId = 1;
    m_items.clear();
    m_enemies.clear();
    m_recipes.clear();
}

} // namespace gloaming
