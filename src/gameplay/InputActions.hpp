#pragma once

#include "engine/Input.hpp"
#include "engine/Gamepad.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>

namespace gloaming {

/// Source type for an input binding — keyboard key, gamepad button, or gamepad axis.
enum class InputSourceType {
    Key,            // Keyboard key
    GamepadButton,  // Gamepad digital button
    GamepadAxis,    // Gamepad analog axis (treated as digital with threshold)
};

/// An abstract input action that can be bound to multiple keys/buttons.
/// Games define their own actions (e.g., "move_up", "jump", "interact", "fire")
/// and bind them to keys or gamepad controls via configuration or Lua.
struct InputBinding {
    InputSourceType sourceType = InputSourceType::Key;

    // Keyboard binding
    Key key = Key::Space;
    bool requireShift = false;
    bool requireCtrl = false;
    bool requireAlt = false;

    // Gamepad binding
    GamepadButton gamepadButton = GamepadButton::FaceDown;
    GamepadAxis   gamepadAxis = GamepadAxis::LeftX;
    float         axisThreshold = 0.5f;  // Axis value at which it counts as "pressed"
    bool          axisPositive = true;   // true = positive direction, false = negative
};

/// Input action map — maps named actions to key and gamepad bindings.
/// Mods define actions and bind them; game code queries actions instead of raw keys.
class InputActionMap {
public:
    /// Register a named action with a default key binding
    void registerAction(const std::string& name, Key defaultKey) {
        InputBinding binding;
        binding.sourceType = InputSourceType::Key;
        binding.key = defaultKey;
        m_actions[name] = {binding};
    }

    /// Register a named action with multiple bindings
    void registerAction(const std::string& name, std::vector<InputBinding> bindings) {
        m_actions[name] = std::move(bindings);
    }

    /// Add an additional keyboard binding to an existing action
    void addBinding(const std::string& name, Key key) {
        InputBinding binding;
        binding.sourceType = InputSourceType::Key;
        binding.key = key;
        m_actions[name].push_back(binding);
    }

    /// Add a gamepad button binding to an existing action
    void addGamepadBinding(const std::string& name, GamepadButton button) {
        InputBinding binding;
        binding.sourceType = InputSourceType::GamepadButton;
        binding.gamepadButton = button;
        m_actions[name].push_back(binding);
    }

    /// Add a gamepad axis binding to an existing action (e.g., left stick left = LeftX, threshold -0.5)
    void addGamepadBinding(const std::string& name, GamepadAxis axis, float threshold) {
        InputBinding binding;
        binding.sourceType = InputSourceType::GamepadAxis;
        binding.gamepadAxis = axis;
        binding.axisThreshold = std::abs(threshold);
        binding.axisPositive = (threshold >= 0.0f);
        m_actions[name].push_back(binding);
    }

    /// Remove all bindings for an action
    void clearBindings(const std::string& name) {
        auto it = m_actions.find(name);
        if (it != m_actions.end()) {
            it->second.clear();
        }
    }

    /// Rebind an action to a single key (replaces all existing bindings)
    void rebind(const std::string& name, Key key) {
        InputBinding binding;
        binding.sourceType = InputSourceType::Key;
        binding.key = key;
        m_actions[name] = {binding};
    }

    /// Check if an action was just pressed this frame (keyboard + gamepad)
    bool isActionPressed(const std::string& name, const Input& input,
                         const Gamepad& gamepad) const {
        auto it = m_actions.find(name);
        if (it == m_actions.end()) return false;
        for (const auto& binding : it->second) {
            if (checkBinding(binding, input, gamepad, BindingCheck::Pressed)) {
                return true;
            }
        }
        return false;
    }

    /// Backward-compatible overload: keyboard only
    bool isActionPressed(const std::string& name, const Input& input) const {
        auto it = m_actions.find(name);
        if (it == m_actions.end()) return false;
        for (const auto& binding : it->second) {
            if (binding.sourceType == InputSourceType::Key &&
                checkModifiers(binding, input) && input.isKeyPressed(binding.key)) {
                return true;
            }
        }
        return false;
    }

    /// Check if an action is currently held down (keyboard + gamepad)
    bool isActionDown(const std::string& name, const Input& input,
                      const Gamepad& gamepad) const {
        auto it = m_actions.find(name);
        if (it == m_actions.end()) return false;
        for (const auto& binding : it->second) {
            if (checkBinding(binding, input, gamepad, BindingCheck::Down)) {
                return true;
            }
        }
        return false;
    }

    /// Backward-compatible overload: keyboard only
    bool isActionDown(const std::string& name, const Input& input) const {
        auto it = m_actions.find(name);
        if (it == m_actions.end()) return false;
        for (const auto& binding : it->second) {
            if (binding.sourceType == InputSourceType::Key &&
                checkModifiers(binding, input) && input.isKeyDown(binding.key)) {
                return true;
            }
        }
        return false;
    }

    /// Check if an action was just released this frame (keyboard + gamepad)
    bool isActionReleased(const std::string& name, const Input& input,
                          const Gamepad& gamepad) const {
        auto it = m_actions.find(name);
        if (it == m_actions.end()) return false;
        for (const auto& binding : it->second) {
            if (checkBinding(binding, input, gamepad, BindingCheck::Released)) {
                return true;
            }
        }
        return false;
    }

    /// Backward-compatible overload: keyboard only
    bool isActionReleased(const std::string& name, const Input& input) const {
        auto it = m_actions.find(name);
        if (it == m_actions.end()) return false;
        for (const auto& binding : it->second) {
            if (binding.sourceType == InputSourceType::Key &&
                checkModifiers(binding, input) && input.isKeyReleased(binding.key)) {
                return true;
            }
        }
        return false;
    }

    /// Get analog value for an action (0.0–1.0 for digital, raw axis for analog).
    /// Falls back to 1.0 if a keyboard key is held, or axis value if gamepad.
    float getActionValue(const std::string& name, const Input& input,
                         const Gamepad& gamepad) const {
        auto it = m_actions.find(name);
        if (it == m_actions.end()) return 0.0f;

        float maxVal = 0.0f;
        for (const auto& binding : it->second) {
            float val = getBindingValue(binding, input, gamepad);
            if (val > maxVal) maxVal = val;
        }
        return maxVal;
    }

    /// Get 2D vector for a movement pair.
    /// Returns normalized direction with analog magnitude from sticks.
    Vec2 getMovementVector(const std::string& leftAction, const std::string& rightAction,
                           const std::string& upAction, const std::string& downAction,
                           const Input& input, const Gamepad& gamepad) const {
        float left  = getActionValue(leftAction, input, gamepad);
        float right = getActionValue(rightAction, input, gamepad);
        float up    = getActionValue(upAction, input, gamepad);
        float down  = getActionValue(downAction, input, gamepad);

        Vec2 dir(right - left, down - up);

        // Clamp magnitude to 1.0 to prevent diagonal speed boost
        float mag = dir.length();
        if (mag > 1.0f) {
            dir = dir * (1.0f / mag);
        }
        return dir;
    }

    /// Check if an action exists
    bool hasAction(const std::string& name) const {
        return m_actions.count(name) > 0;
    }

    /// Get the bindings for an action
    const std::vector<InputBinding>& getBindings(const std::string& name) const {
        static const std::vector<InputBinding> empty;
        auto it = m_actions.find(name);
        if (it == m_actions.end()) return empty;
        return it->second;
    }

    /// Get all registered action names
    std::vector<std::string> getActionNames() const {
        std::vector<std::string> names;
        names.reserve(m_actions.size());
        for (const auto& [name, _] : m_actions) {
            names.push_back(name);
        }
        return names;
    }

    /// Clear all registered actions and bindings
    void clearAll() {
        m_actions.clear();
    }

    // ========================================================================
    // Presets — register common bindings for different game types
    // ========================================================================

    /// Platformer preset: movement, jump, attack, interact, menu
    void registerPlatformerDefaults() {
        registerAction("move_left",  Key::A);
        addBinding("move_left", Key::Left);
        addGamepadBinding("move_left", GamepadAxis::LeftX, -0.5f);
        addGamepadBinding("move_left", GamepadButton::DpadLeft);

        registerAction("move_right", Key::D);
        addBinding("move_right", Key::Right);
        addGamepadBinding("move_right", GamepadAxis::LeftX, 0.5f);
        addGamepadBinding("move_right", GamepadButton::DpadRight);

        registerAction("move_up",    Key::W);
        addBinding("move_up", Key::Up);
        addGamepadBinding("move_up", GamepadAxis::LeftY, -0.5f);
        addGamepadBinding("move_up", GamepadButton::DpadUp);

        registerAction("move_down",  Key::S);
        addBinding("move_down", Key::Down);
        addGamepadBinding("move_down", GamepadAxis::LeftY, 0.5f);
        addGamepadBinding("move_down", GamepadButton::DpadDown);

        registerAction("jump",       Key::Space);
        addGamepadBinding("jump", GamepadButton::FaceDown);   // A button

        registerAction("attack",     Key::Z);
        addGamepadBinding("attack", GamepadButton::FaceRight); // B button

        registerAction("interact",   Key::E);
        addGamepadBinding("interact", GamepadButton::FaceUp);  // Y button

        registerAction("menu",       Key::Escape);
        addGamepadBinding("menu", GamepadButton::Start);

        registerAction("inventory",  Key::Tab);
        addGamepadBinding("inventory", GamepadButton::Select);
    }

    /// Top-down RPG preset: directional movement, interact, menu
    void registerTopDownDefaults() {
        registerAction("move_left",  Key::A);
        addBinding("move_left", Key::Left);
        addGamepadBinding("move_left", GamepadAxis::LeftX, -0.5f);
        addGamepadBinding("move_left", GamepadButton::DpadLeft);

        registerAction("move_right", Key::D);
        addBinding("move_right", Key::Right);
        addGamepadBinding("move_right", GamepadAxis::LeftX, 0.5f);
        addGamepadBinding("move_right", GamepadButton::DpadRight);

        registerAction("move_up",    Key::W);
        addBinding("move_up", Key::Up);
        addGamepadBinding("move_up", GamepadAxis::LeftY, -0.5f);
        addGamepadBinding("move_up", GamepadButton::DpadUp);

        registerAction("move_down",  Key::S);
        addBinding("move_down", Key::Down);
        addGamepadBinding("move_down", GamepadAxis::LeftY, 0.5f);
        addGamepadBinding("move_down", GamepadButton::DpadDown);

        registerAction("interact",   Key::Z);
        addBinding("interact", Key::Enter);
        addGamepadBinding("interact", GamepadButton::FaceDown);

        registerAction("cancel",     Key::X);
        addBinding("cancel", Key::Escape);
        addGamepadBinding("cancel", GamepadButton::FaceRight);

        registerAction("menu",       Key::Escape);
        addGamepadBinding("menu", GamepadButton::Start);

        registerAction("run",        Key::LeftShift);
        addGamepadBinding("run", GamepadButton::FaceLeft);
    }

    /// Flight/shooter preset: pitch, thrust, fire, bomb
    void registerFlightDefaults() {
        registerAction("pitch_up",    Key::W);
        addBinding("pitch_up", Key::Up);
        addGamepadBinding("pitch_up", GamepadAxis::LeftY, -0.5f);

        registerAction("pitch_down",  Key::S);
        addBinding("pitch_down", Key::Down);
        addGamepadBinding("pitch_down", GamepadAxis::LeftY, 0.5f);

        registerAction("thrust",      Key::D);
        addBinding("thrust", Key::Right);
        addGamepadBinding("thrust", GamepadButton::FaceDown);

        registerAction("brake",       Key::A);
        addBinding("brake", Key::Left);
        addGamepadBinding("brake", GamepadButton::FaceLeft);

        registerAction("fire",        Key::Space);
        addGamepadBinding("fire", GamepadButton::RightBumper);

        registerAction("bomb",        Key::B);
        addGamepadBinding("bomb", GamepadButton::LeftBumper);

        registerAction("menu",        Key::Escape);
        addGamepadBinding("menu", GamepadButton::Start);
    }

private:
    enum class BindingCheck { Pressed, Down, Released };

    bool checkModifiers(const InputBinding& binding, const Input& input) const {
        if (binding.requireShift &&
            !input.isKeyDown(Key::LeftShift) && !input.isKeyDown(Key::RightShift)) {
            return false;
        }
        if (binding.requireCtrl &&
            !input.isKeyDown(Key::LeftControl) && !input.isKeyDown(Key::RightControl)) {
            return false;
        }
        if (binding.requireAlt &&
            !input.isKeyDown(Key::LeftAlt) && !input.isKeyDown(Key::RightAlt)) {
            return false;
        }
        return true;
    }

    bool checkBinding(const InputBinding& binding, const Input& input,
                      const Gamepad& gamepad, BindingCheck check) const {
        switch (binding.sourceType) {
            case InputSourceType::Key:
                if (!checkModifiers(binding, input)) return false;
                switch (check) {
                    case BindingCheck::Pressed:  return input.isKeyPressed(binding.key);
                    case BindingCheck::Down:      return input.isKeyDown(binding.key);
                    case BindingCheck::Released:  return input.isKeyReleased(binding.key);
                }
                break;

            case InputSourceType::GamepadButton:
                switch (check) {
                    case BindingCheck::Pressed:  return gamepad.isButtonPressed(binding.gamepadButton);
                    case BindingCheck::Down:      return gamepad.isButtonDown(binding.gamepadButton);
                    case BindingCheck::Released:  return gamepad.isButtonReleased(binding.gamepadButton);
                }
                break;

            case InputSourceType::GamepadAxis: {
                float val = gamepad.getAxis(binding.gamepadAxis);
                bool active = binding.axisPositive
                    ? (val >= binding.axisThreshold)
                    : (val <= -binding.axisThreshold);

                // For axis-as-digital, Pressed/Released can't be tracked per-frame
                // without previous-frame state, so treat them the same as Down.
                switch (check) {
                    case BindingCheck::Pressed:
                    case BindingCheck::Down:
                    case BindingCheck::Released:
                        return active;
                }
                break;
            }
        }
        return false;
    }

    float getBindingValue(const InputBinding& binding, const Input& input,
                          const Gamepad& gamepad) const {
        switch (binding.sourceType) {
            case InputSourceType::Key:
                if (checkModifiers(binding, input) && input.isKeyDown(binding.key)) {
                    return 1.0f;
                }
                return 0.0f;

            case InputSourceType::GamepadButton:
                return gamepad.isButtonDown(binding.gamepadButton) ? 1.0f : 0.0f;

            case InputSourceType::GamepadAxis: {
                float val = gamepad.getAxis(binding.gamepadAxis);
                if (binding.axisPositive) {
                    return val > 0.0f ? val : 0.0f;
                } else {
                    return val < 0.0f ? -val : 0.0f;
                }
            }
        }
        return 0.0f;
    }

    std::unordered_map<std::string, std::vector<InputBinding>> m_actions;
};

} // namespace gloaming
