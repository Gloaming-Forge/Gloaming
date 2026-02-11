#include "gameplay/ShopSystem.hpp"
#include "mod/EventBus.hpp"
#include "engine/Log.hpp"

#include <cmath>

namespace gloaming {

TradeResult ShopManager::buyItem(const std::string& shopId, const std::string& itemId,
                                  int count, Inventory& playerInventory) {
    TradeResult result;

    if (count <= 0) {
        result.failReason = "invalid count";
        return result;
    }

    if (!m_contentRegistry) {
        result.failReason = "no content registry";
        return result;
    }

    const ShopDefinition* shop = m_contentRegistry->getShop(shopId);
    if (!shop) {
        result.failReason = "shop not found";
        return result;
    }

    const ShopItemEntry* entry = findShopItem(*shop, itemId);
    if (!entry) {
        result.failReason = "item not sold here";
        return result;
    }

    if (!entry->available) {
        result.failReason = "item not available";
        return result;
    }

    if (entry->stock >= 0 && entry->stock < count) {
        result.failReason = "insufficient stock";
        return result;
    }

    int totalPrice = static_cast<int>(std::ceil(entry->buyPrice * count * shop->buyMultiplier));
    result.finalPrice = totalPrice;

    // Check player has enough currency
    if (!playerInventory.hasItem(shop->currencyItem, totalPrice)) {
        result.failReason = "insufficient funds";
        return result;
    }

    // Check player has inventory space
    int leftover = playerInventory.addItem(itemId, count);
    if (leftover == count) {
        result.failReason = "inventory full";
        return result;
    }

    // If only partial fit, adjust count and price
    int actualBought = count - leftover;
    totalPrice = static_cast<int>(std::ceil(entry->buyPrice * actualBought * shop->buyMultiplier));
    result.finalPrice = totalPrice;

    // Deduct currency
    playerInventory.removeItem(shop->currencyItem, totalPrice);

    // Emit event
    if (m_eventBus) {
        EventData data;
        data.setString("shop_id", shopId);
        data.setString("item_id", itemId);
        data.setInt("count", actualBought);
        data.setInt("price", totalPrice);
        m_eventBus->emit("shop_buy", data);
    }

    result.success = true;
    return result;
}

TradeResult ShopManager::sellItem(const std::string& shopId, const std::string& itemId,
                                   int count, Inventory& playerInventory) {
    TradeResult result;

    if (count <= 0) {
        result.failReason = "invalid count";
        return result;
    }

    if (!m_contentRegistry) {
        result.failReason = "no content registry";
        return result;
    }

    const ShopDefinition* shop = m_contentRegistry->getShop(shopId);
    if (!shop) {
        result.failReason = "shop not found";
        return result;
    }

    // Check player actually has the item
    int available = playerInventory.countItem(itemId);
    if (available <= 0) {
        result.failReason = "item not in inventory";
        return result;
    }

    int actualSold = std::min(count, available);

    // Calculate sell price — use shop entry if available, otherwise use item definition
    int unitPrice = 0;
    const ShopItemEntry* entry = findShopItem(*shop, itemId);
    if (entry) {
        unitPrice = entry->sellPrice;
    } else {
        // Fall back to item definition sell value
        const ItemDefinition* itemDef = m_contentRegistry->getItem(itemId);
        if (itemDef) {
            unitPrice = itemDef->sellValue;
        }
    }

    int totalPrice = static_cast<int>(std::floor(unitPrice * actualSold * shop->sellMultiplier));
    result.finalPrice = totalPrice;

    // Remove item from inventory
    playerInventory.removeItem(itemId, actualSold);

    // Add currency
    if (totalPrice > 0) {
        playerInventory.addItem(shop->currencyItem, totalPrice);
    }

    // Emit event
    if (m_eventBus) {
        EventData data;
        data.setString("shop_id", shopId);
        data.setString("item_id", itemId);
        data.setInt("count", actualSold);
        data.setInt("price", totalPrice);
        m_eventBus->emit("shop_sell", data);
    }

    result.success = true;
    return result;
}

int ShopManager::getBuyPrice(const std::string& shopId, const std::string& itemId) const {
    if (!m_contentRegistry) return 0;

    const ShopDefinition* shop = m_contentRegistry->getShop(shopId);
    if (!shop) return 0;

    const ShopItemEntry* entry = findShopItem(*shop, itemId);
    if (!entry) return 0;

    return static_cast<int>(std::ceil(entry->buyPrice * shop->buyMultiplier));
}

int ShopManager::getSellPrice(const std::string& shopId, const std::string& itemId) const {
    if (!m_contentRegistry) return 0;

    const ShopDefinition* shop = m_contentRegistry->getShop(shopId);
    if (!shop) return 0;

    const ShopItemEntry* entry = findShopItem(*shop, itemId);
    if (entry) {
        return static_cast<int>(std::floor(entry->sellPrice * shop->sellMultiplier));
    }

    // Fall back to item definition
    const ItemDefinition* itemDef = m_contentRegistry->getItem(itemId);
    if (itemDef) {
        return static_cast<int>(std::floor(itemDef->sellValue * shop->sellMultiplier));
    }

    return 0;
}

const ShopDefinition* ShopManager::getShop(const std::string& shopId) const {
    if (!m_contentRegistry) return nullptr;
    return m_contentRegistry->getShop(shopId);
}

const ShopItemEntry* ShopManager::findShopItem(const ShopDefinition& shop,
                                                const std::string& itemId) const {
    for (const auto& entry : shop.items) {
        if (entry.itemId == itemId) return &entry;
    }
    return nullptr;
}

} // namespace gloaming
