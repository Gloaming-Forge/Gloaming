#pragma once

#include "gameplay/GameplayLoop.hpp"
#include "mod/ContentRegistry.hpp"

#include <string>

namespace gloaming {

// Forward declarations
class EventBus;

/// Result of a buy/sell transaction
struct TradeResult {
    bool success = false;
    std::string failReason;
    int finalPrice = 0;
};

/// Utility class managing shop buy/sell operations.
/// Not a per-frame System — called on demand from Lua or UI events.
/// Follows the CraftingManager pattern (Stage 13).
class ShopManager {
public:
    ShopManager() = default;

    void setContentRegistry(ContentRegistry* registry) { m_contentRegistry = registry; }
    void setEventBus(EventBus* bus) { m_eventBus = bus; }

    /// Buy item from shop into player inventory.
    /// Currency is deducted as an inventory item (e.g. "base:coins").
    TradeResult buyItem(const std::string& shopId, const std::string& itemId,
                        int count, Inventory& playerInventory);

    /// Sell item from player inventory to shop.
    /// Currency is added as an inventory item.
    TradeResult sellItem(const std::string& shopId, const std::string& itemId,
                         int count, Inventory& playerInventory);

    /// Get buy price for an item in a shop (includes multiplier).
    int getBuyPrice(const std::string& shopId, const std::string& itemId) const;

    /// Get sell price for an item in a shop (includes multiplier).
    int getSellPrice(const std::string& shopId, const std::string& itemId) const;

    /// Get the shop definition (or nullptr if not found).
    const ShopDefinition* getShop(const std::string& shopId) const;

private:
    const ShopItemEntry* findShopItem(const ShopDefinition& shop,
                                       const std::string& itemId) const;

    ContentRegistry* m_contentRegistry = nullptr;
    EventBus* m_eventBus = nullptr;
};

} // namespace gloaming
