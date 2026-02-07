#pragma once

#include "physics/PhysicsSystem.hpp"

namespace gloaming {

/// View perspective for the game
enum class ViewMode {
    SideView,       // Terraria, Sopwith — gravity points down, horizontal scrolling
    TopDown,        // Pokemon, Zelda — no gravity, overhead camera
    Custom          // Mod-defined custom physics and camera behavior
};

/// Preconfigured physics presets for common game types
struct PhysicsPresets {
    /// Platformer physics: strong downward gravity, jump-friendly terminal velocity
    static PhysicsConfig Platformer() {
        PhysicsConfig config;
        config.gravity = {0.0f, 980.0f};
        config.maxFallSpeed = 1000.0f;
        config.maxHorizontalSpeed = 500.0f;
        config.collisionIterations = 4;
        config.enableSweepCollision = true;
        return config;
    }

    /// Top-down RPG: no gravity, symmetric speed limits
    static PhysicsConfig TopDown() {
        PhysicsConfig config;
        config.gravity = {0.0f, 0.0f};
        config.maxFallSpeed = 500.0f;
        config.maxHorizontalSpeed = 500.0f;
        config.collisionIterations = 4;
        config.enableSweepCollision = false;
        return config;
    }

    /// Side-scrolling flight: light gravity, high horizontal speed (Sopwith-style)
    static PhysicsConfig Flight() {
        PhysicsConfig config;
        config.gravity = {0.0f, 200.0f};
        config.maxFallSpeed = 600.0f;
        config.maxHorizontalSpeed = 800.0f;
        config.collisionIterations = 4;
        config.enableSweepCollision = true;
        config.sweepThreshold = 50.0f;
        return config;
    }

    /// Zero gravity: free-floating space game
    static PhysicsConfig ZeroG() {
        PhysicsConfig config;
        config.gravity = {0.0f, 0.0f};
        config.maxFallSpeed = 800.0f;
        config.maxHorizontalSpeed = 800.0f;
        config.collisionIterations = 4;
        config.enableSweepCollision = true;
        return config;
    }
};

/// High-level game mode configuration that mods set to declare their game type.
/// The engine uses this to configure defaults for physics, camera, and input.
struct GameModeConfig {
    ViewMode viewMode = ViewMode::SideView;

    // Physics override — if set, overrides the preset derived from viewMode
    bool useCustomPhysics = false;
    PhysicsConfig customPhysics;

    // Grid movement settings (relevant for TopDown mode)
    bool enableGridMovement = false;
    int gridSize = 16;              // Grid cell size in pixels
    float gridMoveSpeed = 4.0f;     // Tiles per second

    // Camera defaults
    bool cameraFollowPlayer = true;
    float cameraSmoothness = 5.0f;

    /// Get the appropriate physics config for this game mode
    PhysicsConfig getPhysicsConfig() const {
        if (useCustomPhysics) return customPhysics;
        switch (viewMode) {
            case ViewMode::SideView: return PhysicsPresets::Platformer();
            case ViewMode::TopDown:  return PhysicsPresets::TopDown();
            case ViewMode::Custom:   return PhysicsConfig{};
        }
        return PhysicsConfig{};
    }
};

} // namespace gloaming
