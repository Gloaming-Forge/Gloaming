#pragma once

#include "ui/UIElement.hpp"
#include "engine/Input.hpp"
#include "engine/Gamepad.hpp"
#include "engine/InputDeviceTracker.hpp"

#include <vector>

namespace gloaming {

/// Manages focus, keyboard/gamepad navigation, and input routing for the UI system.
class UIInput {
public:
    UIInput() = default;

    /// Update input state and route events to the UI tree.
    /// Returns true if the UI consumed the input (game should ignore it).
    bool update(UIElement* root, const Input& input);

    /// Update with gamepad support.
    /// Returns true if the UI consumed the input.
    bool update(UIElement* root, const Input& input, const Gamepad& gamepad,
                InputDevice activeDevice, float dt);

    /// Get the currently focused element
    UIElement* getFocusedElement() const { return m_focused; }

    /// Set focus to a specific element (or nullptr to clear)
    void setFocus(UIElement* element);

    /// Move focus to the next focusable element
    void focusNext(UIElement* root);

    /// Move focus to the previous focusable element
    void focusPrev(UIElement* root);

    /// Check if the UI consumed input this frame
    bool didConsumeInput() const { return m_consumedInput; }

    /// Enable/disable gamepad navigation
    void setGamepadNavigationEnabled(bool enabled);
    bool isGamepadNavigationEnabled() const { return m_gamepadNavEnabled; }

    /// Set the spatial navigation mode
    void setSpatialNavigation(bool enabled);
    bool isSpatialNavigation() const { return m_spatialNav; }

    /// Navigate focus in a direction (for D-pad/stick)
    void navigateFocus(UIElement* root, int dx, int dy);

    /// Confirm the focused element (A button)
    void confirmFocus();

    /// Cancel / go back (B button)
    bool cancelFocus();

private:
    /// Collect all focusable elements in tree order
    void collectFocusable(UIElement* element, std::vector<UIElement*>& out) const;

    /// Find the focus index in the list
    int findFocusIndex(const std::vector<UIElement*>& list) const;

    /// Process gamepad input for UI navigation
    bool processGamepadInput(UIElement* root, const Gamepad& gamepad, float dt);

    /// Find the nearest focusable element in a spatial direction
    UIElement* findSpatialNeighbor(UIElement* root, int dx, int dy) const;

    UIElement* m_focused = nullptr;
    bool m_consumedInput = false;
    float m_lastMouseX = 0.0f;
    float m_lastMouseY = 0.0f;

    // Gamepad navigation state
    bool m_gamepadNavEnabled = true;
    bool m_spatialNav = false;
    float m_navRepeatDelay = 0.4f;  // Initial delay before auto-repeat
    float m_navRepeatRate = 0.1f;   // Rate of auto-repeat
    float m_navTimer = 0.0f;
    int m_lastNavDx = 0;
    int m_lastNavDy = 0;
};

} // namespace gloaming
