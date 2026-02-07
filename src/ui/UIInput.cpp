#include "ui/UIInput.hpp"

namespace gloaming {

bool UIInput::update(UIElement* root, const Input& input) {
    if (!root) return false;

    m_consumedInput = false;
    float mx = input.getMouseX();
    float my = input.getMouseY();

    // Update hover states
    root->handleMouseMove(mx, my);

    // Handle mouse press
    if (input.isMouseButtonPressed(MouseButton::Left)) {
        if (root->handleMousePress(mx, my)) {
            m_consumedInput = true;
        }
    }

    // Handle mouse release
    if (input.isMouseButtonPressed(MouseButton::Left) == false &&
        input.isMouseButtonDown(MouseButton::Left) == false) {
        // Check if we need to release (mouse was down last frame)
        root->handleMouseRelease(mx, my);
    }

    // Handle mouse move for drag operations (sliders)
    if (input.isMouseButtonDown(MouseButton::Left)) {
        if (mx != m_lastMouseX || my != m_lastMouseY) {
            root->handleMouseMove(mx, my);
        }
    }

    m_lastMouseX = mx;
    m_lastMouseY = my;

    // Handle keyboard navigation
    if (input.isKeyPressed(Key::Tab)) {
        if (input.isKeyDown(Key::LeftShift) || input.isKeyDown(Key::RightShift)) {
            focusPrev(root);
        } else {
            focusNext(root);
        }
        m_consumedInput = true;
    }

    // Handle Enter/Space on focused element
    if (m_focused) {
        if (input.isKeyPressed(Key::Enter) || input.isKeyPressed(Key::Space)) {
            if (m_focused->handleKeyPress(static_cast<int>(Key::Enter))) {
                m_consumedInput = true;
            }
            // Simulate click for buttons
            auto& layout = m_focused->getLayout();
            float cx = layout.x + layout.width * 0.5f;
            float cy = layout.y + layout.height * 0.5f;
            m_focused->handleMousePress(cx, cy);
            m_focused->handleMouseRelease(cx, cy);
            m_consumedInput = true;
        }

        // Arrow keys for sliders
        if (input.isKeyPressed(Key::Left) || input.isKeyPressed(Key::Right)) {
            int key = static_cast<int>(input.isKeyPressed(Key::Left) ? Key::Left : Key::Right);
            if (m_focused->handleKeyPress(key)) {
                m_consumedInput = true;
            }
        }
    }

    // Handle scroll wheel on hovered scroll panels
    float wheel = input.getMouseWheelDelta();
    if (wheel != 0.0f) {
        // Find the innermost ScrollPanel under the cursor
        // (simple approach: check if root is scrollable)
        // The UISystem will handle more specific scroll routing
    }

    return m_consumedInput;
}

void UIInput::setFocus(UIElement* element) {
    if (m_focused) {
        m_focused->setFocused(false);
    }
    m_focused = element;
    if (m_focused) {
        m_focused->setFocused(true);
    }
}

void UIInput::collectFocusable(UIElement* element, std::vector<UIElement*>& out) const {
    if (!element || !element->getStyle().visible) return;

    if (element->isFocusable()) {
        out.push_back(element);
    }
    for (auto& child : element->getChildren()) {
        collectFocusable(child.get(), out);
    }
}

int UIInput::findFocusIndex(const std::vector<UIElement*>& list) const {
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i] == m_focused) return static_cast<int>(i);
    }
    return -1;
}

void UIInput::focusNext(UIElement* root) {
    std::vector<UIElement*> focusable;
    collectFocusable(root, focusable);
    if (focusable.empty()) return;

    int idx = findFocusIndex(focusable);
    int next = (idx + 1) % static_cast<int>(focusable.size());
    setFocus(focusable[next]);
}

void UIInput::focusPrev(UIElement* root) {
    std::vector<UIElement*> focusable;
    collectFocusable(root, focusable);
    if (focusable.empty()) return;

    int idx = findFocusIndex(focusable);
    int prev = (idx <= 0) ? static_cast<int>(focusable.size()) - 1 : idx - 1;
    setFocus(focusable[prev]);
}

} // namespace gloaming
