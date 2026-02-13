#include "engine/Gamepad.hpp"

#include <raylib.h>
#include <cmath>

namespace gloaming {

void Gamepad::update() {
    // Raylib handles gamepad state internally; no per-frame work needed.
}

bool Gamepad::isConnected(int gamepadId) const {
    if (gamepadId < 0 || gamepadId >= MAX_GAMEPADS) return false;
    return IsGamepadAvailable(gamepadId);
}

int Gamepad::getConnectedCount() const {
    int count = 0;
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (IsGamepadAvailable(i)) ++count;
    }
    return count;
}

bool Gamepad::isButtonPressed(GamepadButton button, int gamepadId) const {
    if (!isConnected(gamepadId)) return false;
    return IsGamepadButtonPressed(gamepadId, static_cast<int>(button));
}

bool Gamepad::isButtonDown(GamepadButton button, int gamepadId) const {
    if (!isConnected(gamepadId)) return false;
    return IsGamepadButtonDown(gamepadId, static_cast<int>(button));
}

bool Gamepad::isButtonReleased(GamepadButton button, int gamepadId) const {
    if (!isConnected(gamepadId)) return false;
    return IsGamepadButtonReleased(gamepadId, static_cast<int>(button));
}

float Gamepad::getAxis(GamepadAxis axis, int gamepadId) const {
    if (!isConnected(gamepadId)) return 0.0f;
    return GetGamepadAxisMovement(gamepadId, static_cast<int>(axis));
}

Vec2 Gamepad::getLeftStick(int gamepadId) const {
    float x = getAxis(GamepadAxis::LeftX, gamepadId);
    float y = getAxis(GamepadAxis::LeftY, gamepadId);
    return applyRadialDeadzone(x, y);
}

Vec2 Gamepad::getRightStick(int gamepadId) const {
    float x = getAxis(GamepadAxis::RightX, gamepadId);
    float y = getAxis(GamepadAxis::RightY, gamepadId);
    return applyRadialDeadzone(x, y);
}

float Gamepad::getLeftTrigger(int gamepadId) const {
    float val = getAxis(GamepadAxis::LeftTrigger, gamepadId);
    return applyTriggerDeadzone(val);
}

float Gamepad::getRightTrigger(int gamepadId) const {
    float val = getAxis(GamepadAxis::RightTrigger, gamepadId);
    return applyTriggerDeadzone(val);
}

void Gamepad::setDeadzone(float deadzone) {
    m_deadzone = deadzone;
}

float Gamepad::getDeadzone() const {
    return m_deadzone;
}

bool Gamepad::hadAnyInput(int gamepadId) const {
    if (!isConnected(gamepadId)) return false;

    // Check all buttons
    for (int b = 0; b <= static_cast<int>(GamepadButton::DpadRight); ++b) {
        if (IsGamepadButtonPressed(gamepadId, b) || IsGamepadButtonDown(gamepadId, b)) {
            return true;
        }
    }

    // Check sticks (with deadzone)
    Vec2 left = getLeftStick(gamepadId);
    Vec2 right = getRightStick(gamepadId);
    if (left.lengthSquared() > 0.0f || right.lengthSquared() > 0.0f) {
        return true;
    }

    // Check triggers
    if (getLeftTrigger(gamepadId) > 0.0f || getRightTrigger(gamepadId) > 0.0f) {
        return true;
    }

    return false;
}

Vec2 Gamepad::applyRadialDeadzone(float x, float y) const {
    float magnitude = std::sqrt(x * x + y * y);
    if (magnitude < m_deadzone) {
        return {0.0f, 0.0f};
    }

    // Remap from [deadzone, 1.0] to [0.0, 1.0]
    float normalized = (magnitude - m_deadzone) / (1.0f - m_deadzone);
    // Clamp to [0, 1]
    if (normalized > 1.0f) normalized = 1.0f;

    float scale = normalized / magnitude;
    return {x * scale, y * scale};
}

float Gamepad::applyTriggerDeadzone(float value) const {
    if (value < m_deadzone) return 0.0f;
    // Remap from [deadzone, 1.0] to [0.0, 1.0]
    float remapped = (value - m_deadzone) / (1.0f - m_deadzone);
    return remapped > 1.0f ? 1.0f : remapped;
}

} // namespace gloaming
