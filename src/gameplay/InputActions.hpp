#pragma once

#include "engine/Input.hpp"

#include <string>
#include <vector>
#include <unordered_map>

namespace gloaming {

/// An abstract input action that can be bound to multiple keys/buttons.
/// Games define their own actions (e.g., "move_up", "jump", "interact", "fire")
/// and bind them to keys via configuration or Lua.
struct InputBinding {
    Key key = Key::Space;
    bool requireShift = false;
    bool requireCtrl = false;
    bool requireAlt = false;
};

/// Input action map — maps named actions to key bindings.
/// Mods define actions and bind them; game code queries actions instead of raw keys.
class InputActionMap {
public:
    /// Register a named action with a default key binding
    void registerAction(const std::string& name, Key defaultKey) {
        m_actions[name] = {{defaultKey}};
    }

    /// Register a named action with multiple bindings
    void registerAction(const std::string& name, std::vector<InputBinding> bindings) {
        m_actions[name] = std::move(bindings);
    }

    /// Add an additional binding to an existing action
    void addBinding(const std::string& name, Key key) {
        m_actions[name].push_back({key});
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
        m_actions[name] = {{key}};
    }

    /// Check if an action was just pressed this frame
    bool isActionPressed(const std::string& name, const Input& input) const {
        auto it = m_actions.find(name);
        if (it == m_actions.end()) return false;
        for (const auto& binding : it->second) {
            if (checkModifiers(binding, input) && input.isKeyPressed(binding.key)) {
                return true;
            }
        }
        return false;
    }

    /// Check if an action is currently held down
    bool isActionDown(const std::string& name, const Input& input) const {
        auto it = m_actions.find(name);
        if (it == m_actions.end()) return false;
        for (const auto& binding : it->second) {
            if (checkModifiers(binding, input) && input.isKeyDown(binding.key)) {
                return true;
            }
        }
        return false;
    }

    /// Check if an action was just released this frame
    bool isActionReleased(const std::string& name, const Input& input) const {
        auto it = m_actions.find(name);
        if (it == m_actions.end()) return false;
        for (const auto& binding : it->second) {
            if (input.isKeyReleased(binding.key)) {
                return true;
            }
        }
        return false;
    }

    /// Check if an action exists
    bool hasAction(const std::string& name) const {
        return m_actions.count(name) > 0;
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

    /// Register common presets for different game types

    /// Platformer preset: left, right, jump, attack, interact, menu
    void registerPlatformerDefaults() {
        registerAction("move_left",  Key::A);
        addBinding("move_left", Key::Left);
        registerAction("move_right", Key::D);
        addBinding("move_right", Key::Right);
        registerAction("move_up",    Key::W);
        addBinding("move_up", Key::Up);
        registerAction("move_down",  Key::S);
        addBinding("move_down", Key::Down);
        registerAction("jump",       Key::Space);
        registerAction("attack",     Key::Z);
        registerAction("interact",   Key::E);
        registerAction("menu",       Key::Escape);
        registerAction("inventory",  Key::Tab);
    }

    /// Top-down RPG preset: directional movement, interact, menu
    void registerTopDownDefaults() {
        registerAction("move_left",  Key::A);
        addBinding("move_left", Key::Left);
        registerAction("move_right", Key::D);
        addBinding("move_right", Key::Right);
        registerAction("move_up",    Key::W);
        addBinding("move_up", Key::Up);
        registerAction("move_down",  Key::S);
        addBinding("move_down", Key::Down);
        registerAction("interact",   Key::Z);
        addBinding("interact", Key::Enter);
        registerAction("cancel",     Key::X);
        addBinding("cancel", Key::Escape);
        registerAction("menu",       Key::Escape);
        registerAction("run",        Key::LeftShift);
    }

    /// Flight/shooter preset: pitch, thrust, fire, bomb
    void registerFlightDefaults() {
        registerAction("pitch_up",    Key::W);
        addBinding("pitch_up", Key::Up);
        registerAction("pitch_down",  Key::S);
        addBinding("pitch_down", Key::Down);
        registerAction("thrust",      Key::D);
        addBinding("thrust", Key::Right);
        registerAction("brake",       Key::A);
        addBinding("brake", Key::Left);
        registerAction("fire",        Key::Space);
        registerAction("bomb",        Key::B);
        registerAction("menu",        Key::Escape);
    }

private:
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

    std::unordered_map<std::string, std::vector<InputBinding>> m_actions;
};

} // namespace gloaming
