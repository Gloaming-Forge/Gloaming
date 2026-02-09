#include "gameplay/CraftingSystem.hpp"
#include "engine/Log.hpp"

#include <cmath>

namespace gloaming {

bool CraftingManager::canCraft(const std::string& recipeId, const Inventory& inventory,
                                const Vec2& position) const {
    if (!m_contentRegistry) return false;

    const RecipeDefinition* recipe = m_contentRegistry->getRecipe(recipeId);
    if (!recipe) return false;

    // Check ingredients
    if (!hasIngredients(*recipe, inventory)) return false;

    // Check station proximity
    if (!recipe->station.empty()) {
        if (!isStationNearby(recipe->station, position)) return false;
    }

    return true;
}

CraftResult CraftingManager::craft(const std::string& recipeId, Inventory& inventory,
                                    const Vec2& position) {
    CraftResult result;

    if (!m_contentRegistry) {
        result.failReason = "no content registry";
        return result;
    }

    const RecipeDefinition* recipe = m_contentRegistry->getRecipe(recipeId);
    if (!recipe) {
        result.failReason = "unknown recipe";
        return result;
    }

    if (!hasIngredients(*recipe, inventory)) {
        result.failReason = "missing ingredients";
        return result;
    }

    if (!recipe->station.empty() && !isStationNearby(recipe->station, position)) {
        result.failReason = "crafting station not nearby";
        return result;
    }

    // Check if the result item can fit in inventory
    // We look up the max stack from the item definition
    int maxStack = 999;
    const ItemDefinition* resultItemDef = m_contentRegistry->getItem(recipe->resultItem);
    if (resultItemDef) {
        maxStack = resultItemDef->maxStack;
    }

    // Consume ingredients
    consumeIngredients(*recipe, inventory);

    // Add result to inventory
    int leftover = inventory.addItem(recipe->resultItem, recipe->resultCount, maxStack);

    result.success = true;
    result.resultItem = recipe->resultItem;
    result.resultCount = recipe->resultCount - leftover;

    if (leftover > 0) {
        LOG_WARN("Crafting '{}': {} items couldn't fit in inventory", recipeId, leftover);
    }

    return result;
}

std::vector<std::string> CraftingManager::getAvailableRecipes(
        const Inventory& inventory, const Vec2& position) const {
    std::vector<std::string> result;
    if (!m_contentRegistry) return result;

    auto allIds = m_contentRegistry->getRecipeIds();
    for (const auto& id : allIds) {
        if (canCraft(id, inventory, position)) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<std::string> CraftingManager::getAllRecipes() const {
    if (!m_contentRegistry) return {};
    return m_contentRegistry->getRecipeIds();
}

std::vector<std::string> CraftingManager::getRecipesByCategory(
        const std::string& category) const {
    std::vector<std::string> result;
    if (!m_contentRegistry) return result;

    auto recipes = m_contentRegistry->getRecipesByCategory(category);
    for (const auto* recipe : recipes) {
        result.push_back(recipe->qualifiedId);
    }
    return result;
}

bool CraftingManager::isStationNearby(const std::string& stationTileId,
                                        const Vec2& position) const {
    if (!m_tileMap || !m_contentRegistry) return false;

    // Look up the runtime tile ID for the station
    const TileContentDef* tileDef = m_contentRegistry->getTile(stationTileId);
    if (!tileDef) return false;
    uint16_t runtimeId = tileDef->runtimeId;

    // Search tiles in a square around the player position
    int tileSize = 16; // default tile size
    int searchTiles = static_cast<int>(std::ceil(m_stationRadius / tileSize));

    int centerTileX = static_cast<int>(std::floor(position.x / tileSize));
    int centerTileY = static_cast<int>(std::floor(position.y / tileSize));

    for (int dy = -searchTiles; dy <= searchTiles; ++dy) {
        for (int dx = -searchTiles; dx <= searchTiles; ++dx) {
            int tx = centerTileX + dx;
            int ty = centerTileY + dy;

            Tile tile = m_tileMap->getTile(tx, ty);
            if (tile.id == runtimeId) {
                // Check actual pixel distance
                float tileCenterX = (tx + 0.5f) * tileSize;
                float tileCenterY = (ty + 0.5f) * tileSize;
                float distX = tileCenterX - position.x;
                float distY = tileCenterY - position.y;
                float distSq = distX * distX + distY * distY;

                if (distSq <= m_stationRadius * m_stationRadius) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool CraftingManager::hasIngredients(const RecipeDefinition& recipe,
                                       const Inventory& inventory) const {
    for (const auto& ingredient : recipe.ingredients) {
        if (!inventory.hasItem(ingredient.item, ingredient.count)) {
            return false;
        }
    }
    return true;
}

void CraftingManager::consumeIngredients(const RecipeDefinition& recipe,
                                           Inventory& inventory) {
    for (const auto& ingredient : recipe.ingredients) {
        inventory.removeItem(ingredient.item, ingredient.count);
    }
}

} // namespace gloaming
