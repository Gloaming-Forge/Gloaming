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

    // Check runtime stock
    int remaining = initStock(shopId, itemId, entry->stock);
    if (remaining == 0) {
        result.failReason = "out of stock";
        return result;
    }

    // Clamp count to available stock (-1 means infinite)
    int buyCount = count;
    if (remaining > 0) {
        buyCount = std::min(buyCount, remaining);
    }

    int totalPrice = static_cast<int>(std::ceil(entry->buyPrice * buyCount * shop->buyMultiplier));
    result.finalPrice = totalPrice;

    // Check player has enough currency
    if (!playerInventory.hasItem(shop->currencyItem, totalPrice)) {
        result.failReason = "insufficient funds";
        return result;
    }

    // Deduct currency FIRST (before adding items)
    playerInventory.removeItem(shop->currencyItem, totalPrice);

    // Add items to inventory
    int leftover = playerInventory.addItem(itemId, buyCount);
    int actualBought = buyCount - leftover;

    if (actualBought == 0) {
        // Nothing was added — refund currency
        playerInventory.addItem(shop->currencyItem, totalPrice);
        result.failReason = "inventory full";
        return result;
    }

    // If partial add, refund the difference
    if (actualBought < buyCount) {
        int actualPrice = static_cast<int>(std::ceil(entry->buyPrice * actualBought * shop->buyMultiplier));
        int refund = totalPrice - actualPrice;
        if (refund > 0) {
            playerInventory.addItem(shop->currencyItem, refund);
        }
        totalPrice = actualPrice;
    }

    // Decrement runtime stock
    decrementStock(shopId, itemId, actualBought);

    result.finalPrice = totalPrice;

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

int ShopManager::getRemainingStock(const std::string& shopId, const std::string& itemId) const {
    auto shopIt = m_runtimeStock.find(shopId);
    if (shopIt == m_runtimeStock.end()) {
        // Not yet tracked — return defined stock
        if (!m_contentRegistry) return -1;
        const ShopDefinition* shop = m_contentRegistry->getShop(shopId);
        if (!shop) return -1;
        const ShopItemEntry* entry = findShopItem(*shop, itemId);
        return entry ? entry->stock : -1;
    }
    auto itemIt = shopIt->second.find(itemId);
    if (itemIt == shopIt->second.end()) return -1;
    return itemIt->second;
}

const ShopItemEntry* ShopManager::findShopItem(const ShopDefinition& shop,
                                                const std::string& itemId) const {
    for (const auto& entry : shop.items) {
        if (entry.itemId == itemId) return &entry;
    }
    return nullptr;
}

int ShopManager::initStock(const std::string& shopId, const std::string& itemId, int definedStock) {
    if (definedStock < 0) return -1; // infinite

    auto& shopStock = m_runtimeStock[shopId];
    auto it = shopStock.find(itemId);
    if (it == shopStock.end()) {
        shopStock[itemId] = definedStock;
        return definedStock;
    }
    return it->second;
}

void ShopManager::decrementStock(const std::string& shopId, const std::string& itemId, int amount) {
    auto shopIt = m_runtimeStock.find(shopId);
    if (shopIt == m_runtimeStock.end()) return;
    auto itemIt = shopIt->second.find(itemId);
    if (itemIt == shopIt->second.end()) return;
    if (itemIt->second < 0) return; // infinite stock
    itemIt->second = std::max(0, itemIt->second - amount);
}

} // namespace gloaming
