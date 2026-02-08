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

/// Game mode configuration — declares the game's view perspective.
/// Mods set this via game_mode.set_view() so other systems (and other mods)
/// can query the intended game style. Physics, camera, and input are configured
/// through their own dedicated APIs rather than through this struct.
struct GameModeConfig {
    ViewMode viewMode = ViewMode::SideView;
};

} // namespace gloaming
