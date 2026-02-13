#include "gameplay/InputActions.hpp"

#include <cmath>
#include <cstring>

namespace gloaming {

// --- Registration ---

void InputActionMap::registerAction(const std::string& name, Key defaultKey) {
    InputBinding binding;
    binding.sourceType = InputSourceType::Key;
    binding.key = defaultKey;
    m_actions[name] = {binding};
}

void InputActionMap::registerAction(const std::string& name, std::vector<InputBinding> bindings) {
    m_actions[name] = std::move(bindings);
}

void InputActionMap::addBinding(const std::string& name, Key key) {
    InputBinding binding;
    binding.sourceType = InputSourceType::Key;
    binding.key = key;
    m_actions[name].push_back(binding);
}

void InputActionMap::addGamepadBinding(const std::string& name, GamepadButton button) {
    InputBinding binding;
    binding.sourceType = InputSourceType::GamepadButton;
    binding.gamepadButton = button;
    m_actions[name].push_back(binding);
}

void InputActionMap::addGamepadBinding(const std::string& name, GamepadAxis axis, float threshold) {
    InputBinding binding;
    binding.sourceType = InputSourceType::GamepadAxis;
    binding.gamepadAxis = axis;
    binding.axisThreshold = std::abs(threshold);
    binding.axisPositive = (threshold >= 0.0f);
    m_actions[name].push_back(binding);
}

void InputActionMap::clearBindings(const std::string& name) {
    auto it = m_actions.find(name);
    if (it != m_actions.end()) {
        it->second.clear();
    }
}

void InputActionMap::rebind(const std::string& name, Key key) {
    InputBinding binding;
    binding.sourceType = InputSourceType::Key;
    binding.key = key;
    m_actions[name] = {binding};
}

// --- Action queries ---

bool InputActionMap::isActionPressed(const std::string& name, const Input& input,
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

bool InputActionMap::isActionPressed(const std::string& name, const Input& input) const {
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

bool InputActionMap::isActionDown(const std::string& name, const Input& input,
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

bool InputActionMap::isActionDown(const std::string& name, const Input& input) const {
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

bool InputActionMap::isActionReleased(const std::string& name, const Input& input,
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

bool InputActionMap::isActionReleased(const std::string& name, const Input& input) const {
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

float InputActionMap::getActionValue(const std::string& name, const Input& input,
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

Vec2 InputActionMap::getMovementVector(const std::string& leftAction, const std::string& rightAction,
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

bool InputActionMap::hasAction(const std::string& name) const {
    return m_actions.count(name) > 0;
}

const std::vector<InputBinding>& InputActionMap::getBindings(const std::string& name) const {
    static const std::vector<InputBinding> empty;
    auto it = m_actions.find(name);
    if (it == m_actions.end()) return empty;
    return it->second;
}

std::vector<std::string> InputActionMap::getActionNames() const {
    std::vector<std::string> names;
    names.reserve(m_actions.size());
    for (const auto& [name, _] : m_actions) {
        names.push_back(name);
    }
    return names;
}

void InputActionMap::clearAll() {
    m_actions.clear();
}

// --- Presets ---

void InputActionMap::registerPlatformerDefaults() {
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

void InputActionMap::registerTopDownDefaults() {
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

void InputActionMap::registerFlightDefaults() {
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

// --- Axis state tracking ---

void InputActionMap::latchAxisState(const Gamepad& gamepad) {
    for (int i = 0; i < AXIS_COUNT; ++i) {
        m_prevAxisValues[i] = gamepad.getAxis(static_cast<GamepadAxis>(i));
    }
}

// --- Private helpers ---

bool InputActionMap::checkModifiers(const InputBinding& binding, const Input& input) const {
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

bool InputActionMap::checkBinding(const InputBinding& binding, const Input& input,
                                  const Gamepad& gamepad, BindingCheck check) const {
    switch (binding.sourceType) {
        case InputSourceType::Key:
            if (!checkModifiers(binding, input)) return false;
            switch (check) {
                case BindingCheck::Pressed:  return input.isKeyPressed(binding.key);
                case BindingCheck::Down:     return input.isKeyDown(binding.key);
                case BindingCheck::Released: return input.isKeyReleased(binding.key);
            }
            break;

        case InputSourceType::GamepadButton:
            switch (check) {
                case BindingCheck::Pressed:  return gamepad.isButtonPressed(binding.gamepadButton);
                case BindingCheck::Down:     return gamepad.isButtonDown(binding.gamepadButton);
                case BindingCheck::Released: return gamepad.isButtonReleased(binding.gamepadButton);
            }
            break;

        case InputSourceType::GamepadAxis: {
            float val = gamepad.getAxis(binding.gamepadAxis);
            float prevVal = m_prevAxisValues[static_cast<int>(binding.gamepadAxis)];

            bool active = binding.axisPositive
                ? (val >= binding.axisThreshold)
                : (val <= -binding.axisThreshold);
            bool wasActive = binding.axisPositive
                ? (prevVal >= binding.axisThreshold)
                : (prevVal <= -binding.axisThreshold);

            switch (check) {
                case BindingCheck::Pressed:  return active && !wasActive;
                case BindingCheck::Down:     return active;
                case BindingCheck::Released: return !active && wasActive;
            }
            break;
        }
    }
    return false;
}

float InputActionMap::getBindingValue(const InputBinding& binding, const Input& input,
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
            // Use deadzone-aware stick/trigger values instead of raw axis
            GamepadAxis axis = binding.gamepadAxis;
            float val = 0.0f;

            if (axis == GamepadAxis::LeftX) {
                val = gamepad.getLeftStick().x;
            } else if (axis == GamepadAxis::LeftY) {
                val = gamepad.getLeftStick().y;
            } else if (axis == GamepadAxis::RightX) {
                val = gamepad.getRightStick().x;
            } else if (axis == GamepadAxis::RightY) {
                val = gamepad.getRightStick().y;
            } else if (axis == GamepadAxis::LeftTrigger) {
                return gamepad.getLeftTrigger();
            } else if (axis == GamepadAxis::RightTrigger) {
                return gamepad.getRightTrigger();
            }

            if (binding.axisPositive) {
                return val > 0.0f ? val : 0.0f;
            } else {
                return val < 0.0f ? -val : 0.0f;
            }
        }
    }
    return 0.0f;
}

} // namespace gloaming
