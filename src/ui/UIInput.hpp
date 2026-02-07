#pragma once

#include "ui/UIElement.hpp"
#include "engine/Input.hpp"

#include <vector>

namespace gloaming {

/// Manages focus, keyboard navigation, and input routing for the UI system.
class UIInput {
public:
    UIInput() = default;

    /// Update input state and route events to the UI tree.
    /// Returns true if the UI consumed the input (game should ignore it).
    bool update(UIElement* root, const Input& input);

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

private:
    /// Collect all focusable elements in tree order
    void collectFocusable(UIElement* element, std::vector<UIElement*>& out) const;

    /// Find the focus index in the list
    int findFocusIndex(const std::vector<UIElement*>& list) const;

    UIElement* m_focused = nullptr;
    bool m_consumedInput = false;
    float m_lastMouseX = 0.0f;
    float m_lastMouseY = 0.0f;
};

} // namespace gloaming
