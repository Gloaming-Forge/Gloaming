#pragma once

#include "engine/Input.hpp"
#include "engine/Gamepad.hpp"

#include <string>
#include <vector>
#include <unordered_map>

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
    void registerAction(const std::string& name, Key defaultKey);

    /// Register a named action with multiple bindings
    void registerAction(const std::string& name, std::vector<InputBinding> bindings);

    /// Add an additional keyboard binding to an existing action
    void addBinding(const std::string& name, Key key);

    /// Add a gamepad button binding to an existing action
    void addGamepadBinding(const std::string& name, GamepadButton button);

    /// Add a gamepad axis binding to an existing action (e.g., left stick left = LeftX, threshold -0.5)
    void addGamepadBinding(const std::string& name, GamepadAxis axis, float threshold);

    /// Remove all bindings for an action
    void clearBindings(const std::string& name);

    /// Rebind an action to a single key (replaces all existing bindings)
    void rebind(const std::string& name, Key key);

    /// Check if an action was just pressed this frame (keyboard + gamepad)
    bool isActionPressed(const std::string& name, const Input& input,
                         const Gamepad& gamepad) const;

    /// Backward-compatible overload: keyboard only
    bool isActionPressed(const std::string& name, const Input& input) const;

    /// Check if an action is currently held down (keyboard + gamepad)
    bool isActionDown(const std::string& name, const Input& input,
                      const Gamepad& gamepad) const;

    /// Backward-compatible overload: keyboard only
    bool isActionDown(const std::string& name, const Input& input) const;

    /// Check if an action was just released this frame (keyboard + gamepad)
    bool isActionReleased(const std::string& name, const Input& input,
                          const Gamepad& gamepad) const;

    /// Backward-compatible overload: keyboard only
    bool isActionReleased(const std::string& name, const Input& input) const;

    /// Get analog value for an action (0.0–1.0 for digital, raw axis for analog).
    /// Falls back to 1.0 if a keyboard key is held, or axis value if gamepad.
    float getActionValue(const std::string& name, const Input& input,
                         const Gamepad& gamepad) const;

    /// Get 2D vector for a movement pair.
    /// Returns normalized direction with analog magnitude from sticks.
    Vec2 getMovementVector(const std::string& leftAction, const std::string& rightAction,
                           const std::string& upAction, const std::string& downAction,
                           const Input& input, const Gamepad& gamepad) const;

    /// Check if an action exists
    bool hasAction(const std::string& name) const;

    /// Get the bindings for an action
    const std::vector<InputBinding>& getBindings(const std::string& name) const;

    /// Get all registered action names
    std::vector<std::string> getActionNames() const;

    /// Clear all registered actions and bindings
    void clearAll();

    // ========================================================================
    // Presets — register common bindings for different game types
    // ========================================================================

    /// Platformer preset: movement, jump, attack, interact, menu
    void registerPlatformerDefaults();

    /// Top-down RPG preset: directional movement, interact, menu
    void registerTopDownDefaults();

    /// Flight/shooter preset: pitch, thrust, fire, bomb
    void registerFlightDefaults();

    /// Call once per frame after gamepad update to latch axis values.
    /// Required for correct Pressed/Released edge detection on axis bindings.
    void latchAxisState(const Gamepad& gamepad);

private:
    enum class BindingCheck { Pressed, Down, Released };

    bool checkModifiers(const InputBinding& binding, const Input& input) const;

    bool checkBinding(const InputBinding& binding, const Input& input,
                      const Gamepad& gamepad, BindingCheck check) const;

    float getBindingValue(const InputBinding& binding, const Input& input,
                          const Gamepad& gamepad) const;

    std::unordered_map<std::string, std::vector<InputBinding>> m_actions;

    // Previous-frame axis values for Pressed/Released edge detection
    static constexpr int AXIS_COUNT = 6;
    float m_prevAxisValues[AXIS_COUNT] = {};
};

} // namespace gloaming
