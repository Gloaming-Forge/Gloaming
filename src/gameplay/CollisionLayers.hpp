#pragma once

#include "ecs/Components.hpp"
#include "engine/Log.hpp"

#include <string>
#include <vector>
#include <unordered_map>

namespace gloaming {

/// Named collision layer registry.
///
/// Maps human-readable layer names (e.g. "player", "enemy") to bit positions
/// within the 16-bit collision bitmask.  The registry is pre-populated with the
/// engine's default layers; mods can register additional layers (bits 8-15).
class CollisionLayerRegistry {
public:
    CollisionLayerRegistry() {
        // Register default layers matching the existing CollisionLayer constants
        registerLayer("default",    0);   // CollisionLayer::Default    = 1 << 0
        registerLayer("player",     1);   // CollisionLayer::Player     = 1 << 1
        registerLayer("enemy",      2);   // CollisionLayer::Enemy      = 1 << 2
        registerLayer("projectile", 3);   // CollisionLayer::Projectile = 1 << 3
        registerLayer("tile",       4);   // CollisionLayer::Tile       = 1 << 4
        registerLayer("trigger",    5);   // CollisionLayer::Trigger    = 1 << 5
        registerLayer("item",       6);   // CollisionLayer::Item       = 1 << 6
        registerLayer("npc",        7);   // CollisionLayer::NPC        = 1 << 7
    }

    // -----------------------------------------------------------------
    // Registration
    // -----------------------------------------------------------------

    /// Register (or re-register) a named layer at a specific bit position.
    /// @param name  Case-sensitive layer name
    /// @param bit   Bit position (0-15)
    /// @return true on success
    bool registerLayer(const std::string& name, int bit) {
        if (bit < 0 || bit > 15) {
            LOG_WARN("CollisionLayerRegistry: bit {} out of range for '{}'", bit, name);
            return false;
        }
        m_nameToBit[name] = bit;
        return true;
    }

    // -----------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------

    /// Get the bitmask for a single named layer (one bit set).
    /// Returns 0 if the name is not registered.
    uint32_t getLayerBit(const std::string& name) const {
        auto it = m_nameToBit.find(name);
        if (it == m_nameToBit.end()) {
            LOG_WARN("CollisionLayerRegistry: unknown layer '{}'", name);
            return 0;
        }
        return static_cast<uint32_t>(1) << it->second;
    }

    /// Combine multiple named layers into a single bitmask.
    uint32_t getMask(const std::vector<std::string>& names) const {
        uint32_t mask = 0;
        for (const auto& name : names) {
            mask |= getLayerBit(name);
        }
        return mask;
    }

    /// Check if a layer name is registered
    bool hasLayer(const std::string& name) const {
        return m_nameToBit.find(name) != m_nameToBit.end();
    }

    /// Get the bit position for a named layer, or -1 if not found
    int getBitPosition(const std::string& name) const {
        auto it = m_nameToBit.find(name);
        return it != m_nameToBit.end() ? it->second : -1;
    }

    // -----------------------------------------------------------------
    // Entity helpers — operate directly on Collider components
    // -----------------------------------------------------------------

    /// Set which layer this entity occupies (replaces current layer)
    void setLayer(Collider& collider, const std::string& name) const {
        collider.layer = getLayerBit(name);
    }

    /// Set which layers this entity collides with (replaces current mask)
    void setMask(Collider& collider, const std::vector<std::string>& names) const {
        collider.mask = getMask(names);
    }

    /// Add a layer to the entity's collision mask
    void addMask(Collider& collider, const std::string& name) const {
        collider.mask |= getLayerBit(name);
    }

    /// Remove a layer from the entity's collision mask
    void removeMask(Collider& collider, const std::string& name) const {
        collider.mask &= ~getLayerBit(name);
    }

    /// Set multiple layers on the entity (OR together)
    void setLayers(Collider& collider, const std::vector<std::string>& names) const {
        collider.layer = getMask(names);
    }

private:
    std::unordered_map<std::string, int> m_nameToBit;
};

} // namespace gloaming
