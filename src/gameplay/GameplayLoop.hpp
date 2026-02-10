#pragma once

#include "ecs/Components.hpp"
#include "rendering/IRenderer.hpp"

#include <string>
#include <array>
#include <algorithm>
#include <cstdint>

namespace gloaming {

// ============================================================================
// ItemStack — a single inventory slot (item ID + count)
// ============================================================================

struct ItemStack {
    std::string itemId;      // Content-registry qualified ID (e.g. "base:dirt")
    int count = 0;

    bool isEmpty() const { return itemId.empty() || count <= 0; }

    void clear() {
        itemId.clear();
        count = 0;
    }

    bool matches(const std::string& id) const {
        return !isEmpty() && itemId == id;
    }
};

// ============================================================================
// Inventory — component attached to entities that carry items
// ============================================================================

struct Inventory {
    static constexpr int MaxSlots = 40;
    static constexpr int HotbarSlots = 10;

    std::array<ItemStack, MaxSlots> slots{};
    int selectedSlot = 0;      // Active hotbar slot (0 .. HotbarSlots-1)

    /// Get the currently selected hotbar slot
    const ItemStack& getSelected() const {
        if (selectedSlot < 0 || selectedSlot >= HotbarSlots) {
            static const ItemStack empty;
            return empty;
        }
        return slots[static_cast<size_t>(selectedSlot)];
    }

    ItemStack& getSelectedMut() {
        return slots[static_cast<size_t>(std::clamp(selectedSlot, 0, HotbarSlots - 1))];
    }

    /// Add items to inventory. Returns leftover count that couldn't fit.
    int addItem(const std::string& itemId, int amount, int maxStack = 999) {
        if (itemId.empty() || amount <= 0) return 0;

        int remaining = amount;

        // First pass: stack onto existing matching slots
        for (auto& slot : slots) {
            if (remaining <= 0) break;
            if (slot.matches(itemId) && slot.count < maxStack) {
                int space = maxStack - slot.count;
                int toAdd = std::min(remaining, space);
                slot.count += toAdd;
                remaining -= toAdd;
            }
        }

        // Second pass: fill empty slots
        for (auto& slot : slots) {
            if (remaining <= 0) break;
            if (slot.isEmpty()) {
                int toAdd = std::min(remaining, maxStack);
                slot.itemId = itemId;
                slot.count = toAdd;
                remaining -= toAdd;
            }
        }

        return remaining;
    }

    /// Remove items from inventory. Returns count actually removed.
    int removeItem(const std::string& itemId, int amount) {
        if (itemId.empty() || amount <= 0) return 0;

        int remaining = amount;

        for (auto& slot : slots) {
            if (remaining <= 0) break;
            if (slot.matches(itemId)) {
                int toRemove = std::min(remaining, slot.count);
                slot.count -= toRemove;
                remaining -= toRemove;
                if (slot.count <= 0) {
                    slot.clear();
                }
            }
        }

        return amount - remaining;
    }

    /// Check if the inventory contains at least `amount` of an item
    bool hasItem(const std::string& itemId, int amount = 1) const {
        return countItem(itemId) >= amount;
    }

    /// Count the total quantity of an item across all slots
    int countItem(const std::string& itemId) const {
        int total = 0;
        for (const auto& slot : slots) {
            if (slot.matches(itemId)) {
                total += slot.count;
            }
        }
        return total;
    }

    /// Swap two slots
    void swapSlots(int a, int b) {
        if (a < 0 || a >= MaxSlots || b < 0 || b >= MaxSlots || a == b) return;
        std::swap(slots[static_cast<size_t>(a)], slots[static_cast<size_t>(b)]);
    }

    /// Clear a specific slot
    void clearSlot(int index) {
        if (index >= 0 && index < MaxSlots) {
            slots[static_cast<size_t>(index)].clear();
        }
    }

    /// Find the first slot containing a given item. Returns -1 if not found.
    int findItem(const std::string& itemId) const {
        for (int i = 0; i < MaxSlots; ++i) {
            if (slots[static_cast<size_t>(i)].matches(itemId)) return i;
        }
        return -1;
    }

    /// Find the first empty slot. Returns -1 if full.
    int findEmptySlot() const {
        for (int i = 0; i < MaxSlots; ++i) {
            if (slots[static_cast<size_t>(i)].isEmpty()) return i;
        }
        return -1;
    }

    /// Count how many slots are occupied
    int occupiedSlotCount() const {
        int count = 0;
        for (const auto& slot : slots) {
            if (!slot.isEmpty()) ++count;
        }
        return count;
    }
};

// ============================================================================
// ItemDrop — component for items dropped in the world (pickupable entities)
// ============================================================================

struct ItemDrop {
    std::string itemId;
    int count = 1;
    float magnetRadius = 48.0f;     // Start pulling toward player at this distance
    float pickupRadius = 16.0f;     // Actually collect at this distance
    float pickupDelay = 0.5f;       // Seconds before can be picked up
    float age = 0.0f;
    float despawnTime = 300.0f;     // Seconds until auto-despawn (5 minutes)
    bool magnetic = true;           // Whether the item is pulled toward nearby players
    float magnetSpeed = 200.0f;     // Speed of magnet pull (pixels/sec)

    ItemDrop() = default;
    ItemDrop(const std::string& id, int cnt) : itemId(id), count(cnt) {}

    bool canPickup() const { return age >= pickupDelay; }
    bool isExpired() const { return age >= despawnTime; }
};

// ============================================================================
// ToolUse — tracks tile-breaking progress for the player
// ============================================================================

struct ToolUse {
    int targetTileX = 0;
    int targetTileY = 0;
    float progress = 0.0f;          // 0 to 1 (1 = broken)
    float breakTime = 1.0f;         // Total time to break the current tile
    bool active = false;            // Currently mining/chopping

    void reset() {
        targetTileX = 0;
        targetTileY = 0;
        progress = 0.0f;
        breakTime = 1.0f;
        active = false;
    }

    float getProgressPercent() const {
        return breakTime > 0.0f ? std::min(progress / breakTime, 1.0f) : 1.0f;
    }

    bool isComplete() const {
        return progress >= breakTime;
    }
};

// ============================================================================
// MeleeAttack — melee weapon swing state
// ============================================================================

struct MeleeAttack {
    float damage = 10.0f;
    float knockback = 5.0f;
    float arc = 120.0f;              // Swing arc in degrees
    float range = 32.0f;             // Reach in pixels
    float cooldownTime = 0.4f;       // Seconds between attacks
    float cooldownRemaining = 0.0f;  // Time until can attack again
    float swingDuration = 0.3f;      // How long the swing animation lasts
    float swingTimer = 0.0f;         // Current swing progress
    bool swinging = false;
    bool hitChecked = false;         // Whether hit detection has run for this swing
    float swingAngle = 0.0f;         // Current visual angle of swing (degrees)
    Vec2 aimDirection{1.0f, 0.0f};   // Direction the player is aiming

    bool canAttack() const { return !swinging && cooldownRemaining <= 0.0f; }

    void startSwing(float dmg, float kb, float arcDeg, float rng, float useTime) {
        damage = dmg;
        knockback = kb;
        arc = arcDeg;
        range = rng;
        swingDuration = useTime;
        cooldownTime = useTime;
        swingTimer = 0.0f;
        swinging = true;
        hitChecked = false;
    }

    void update(float dt) {
        if (swinging) {
            swingTimer += dt;
            if (swingTimer >= swingDuration) {
                swinging = false;
                cooldownRemaining = cooldownTime;
                swingTimer = 0.0f;
            } else {
                // Interpolate swing angle from -arc/2 to +arc/2
                float t = swingTimer / swingDuration;
                swingAngle = -arc * 0.5f + arc * t;
            }
        }

        if (cooldownRemaining > 0.0f) {
            cooldownRemaining -= dt;
            if (cooldownRemaining < 0.0f) cooldownRemaining = 0.0f;
        }
    }
};

// ============================================================================
// PlayerCombat — tracks player death and respawn state
// ============================================================================

struct PlayerCombat {
    Vec2 spawnPoint{0.0f, 0.0f};
    float respawnDelay = 3.0f;       // Seconds before respawn
    float respawnTimer = 0.0f;       // Countdown when dead
    bool dead = false;
    int deathCount = 0;

    void die() {
        if (!dead) {
            dead = true;
            respawnTimer = respawnDelay;
            deathCount++;
        }
    }

    /// Update respawn timer. Returns true when respawn should happen.
    bool updateRespawn(float dt) {
        if (!dead) return false;
        respawnTimer -= dt;
        if (respawnTimer <= 0.0f) {
            dead = false;
            respawnTimer = 0.0f;
            return true;
        }
        return false;
    }
};

} // namespace gloaming
