#pragma once

#include "gameplay/GameplayLoop.hpp"
#include "mod/ContentRegistry.hpp"
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "world/TileMap.hpp"

#include <string>
#include <vector>

namespace gloaming {

/// Result of a craft attempt
struct CraftResult {
    bool success = false;
    std::string resultItem;
    int resultCount = 0;
    std::string failReason;     // empty on success
};

/// Manages crafting operations — checks recipes, station proximity, and performs crafts.
/// Not an ECS System (doesn't need per-frame updates). Called on demand from Lua.
class CraftingManager {
public:
    CraftingManager() = default;

    void setContentRegistry(ContentRegistry* registry) { m_contentRegistry = registry; }
    void setTileMap(TileMap* tileMap) { m_tileMap = tileMap; }

    /// Set the search radius (in pixels) for nearby crafting stations
    void setStationSearchRadius(float radius) { m_stationRadius = radius; }
    float getStationSearchRadius() const { return m_stationRadius; }

    /// Check if a recipe can be crafted with the given inventory and position.
    /// Checks ingredients and station proximity.
    bool canCraft(const std::string& recipeId, const Inventory& inventory,
                  const Vec2& position) const;

    /// Attempt to craft a recipe. Removes ingredients from inventory and returns result.
    CraftResult craft(const std::string& recipeId, Inventory& inventory,
                      const Vec2& position);

    /// Get all recipes the player can currently craft given their inventory and position
    std::vector<std::string> getAvailableRecipes(const Inventory& inventory,
                                                   const Vec2& position) const;

    /// Get all recipes regardless of whether they can be crafted
    std::vector<std::string> getAllRecipes() const;

    /// Get recipes for a specific category
    std::vector<std::string> getRecipesByCategory(const std::string& category) const;

    /// Check if a crafting station tile is within range of a position
    bool isStationNearby(const std::string& stationTileId, const Vec2& position) const;

private:
    /// Check if all ingredients are present in the inventory
    bool hasIngredients(const RecipeDefinition& recipe, const Inventory& inventory) const;

    /// Remove recipe ingredients from inventory
    void consumeIngredients(const RecipeDefinition& recipe, Inventory& inventory);

    ContentRegistry* m_contentRegistry = nullptr;
    TileMap* m_tileMap = nullptr;
    float m_stationRadius = 64.0f;  // pixels (4 tiles at 16px)
};

} // namespace gloaming
