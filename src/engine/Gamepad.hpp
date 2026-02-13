#pragma once

#include "rendering/IRenderer.hpp"

#include <cstdint>

namespace gloaming {

/// Gamepad button codes — Xbox-style layout (matches Steam Deck physical buttons).
enum class GamepadButton : int {
    // Face buttons (Xbox layout — matches Deck)
    FaceDown    = 0,   // A / Cross
    FaceRight   = 1,   // B / Circle
    FaceLeft    = 2,   // X / Square
    FaceUp      = 3,   // Y / Triangle

    // Shoulder buttons
    LeftBumper  = 4,
    RightBumper = 5,

    // Center buttons
    Select      = 6,   // Back / View
    Start       = 7,   // Start / Menu
    Guide       = 8,   // Steam button

    // Stick clicks
    LeftThumb   = 9,
    RightThumb  = 10,

    // D-pad
    DpadUp      = 11,
    DpadDown    = 12,
    DpadLeft    = 13,
    DpadRight   = 14,
};

/// Gamepad axis identifiers.
enum class GamepadAxis : int {
    LeftX        = 0,
    LeftY        = 1,
    RightX       = 2,
    RightY       = 3,
    LeftTrigger  = 4,
    RightTrigger = 5,
};

/// Thin abstraction over Raylib gamepad input.
/// Supports up to MAX_GAMEPADS simultaneously connected controllers.
class Gamepad {
public:
    void update();

    // Connection state
    bool isConnected(int gamepadId = 0) const;
    int  getConnectedCount() const;

    // Button queries
    bool isButtonPressed(GamepadButton button, int gamepadId = 0) const;
    bool isButtonDown(GamepadButton button, int gamepadId = 0) const;
    bool isButtonReleased(GamepadButton button, int gamepadId = 0) const;

    // Axis queries (returns -1.0 to 1.0 for sticks, 0.0 to 1.0 for triggers)
    float getAxis(GamepadAxis axis, int gamepadId = 0) const;

    // Convenience — applies deadzone and returns normalized direction
    Vec2  getLeftStick(int gamepadId = 0) const;
    Vec2  getRightStick(int gamepadId = 0) const;
    float getLeftTrigger(int gamepadId = 0) const;
    float getRightTrigger(int gamepadId = 0) const;

    // Configuration
    void  setDeadzone(float deadzone);
    float getDeadzone() const;

    // Check if any button or significant axis was active this frame
    bool hadAnyInput(int gamepadId = 0) const;

    static constexpr int MAX_GAMEPADS = 4;

private:
    /// Apply radial deadzone to a stick vector.
    Vec2 applyRadialDeadzone(float x, float y) const;

    /// Apply linear deadzone to a trigger value.
    float applyTriggerDeadzone(float value) const;

    float m_deadzone = 0.15f;
};

} // namespace gloaming
