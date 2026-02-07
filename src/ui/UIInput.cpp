#include "ui/UIInput.hpp"

namespace gloaming {

bool UIInput::update(UIElement* root, const Input& input) {
    if (!root) return false;

    m_consumedInput = false;
    float mx = input.getMouseX();
    float my = input.getMouseY();

    // Update hover states
    root->handleMouseMove(mx, my);

    // Handle mouse press (fires only on the frame the button goes down)
    if (input.isMouseButtonPressed(MouseButton::Left)) {
        if (root->handleMousePress(mx, my)) {
            m_consumedInput = true;
        }
    }

    // Handle mouse release (fires only on the frame the button goes up)
    if (input.isMouseButtonReleased(MouseButton::Left)) {
        if (root->handleMouseRelease(mx, my)) {
            m_consumedInput = true;
        }
    }

    // Handle mouse move for drag operations (sliders) while button held
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
            // Simulate click for focusable elements (buttons, etc.)
            auto& layout = m_focused->getLayout();
            float cx = layout.x + layout.width * 0.5f;
            float cy = layout.y + layout.height * 0.5f;
            m_focused->handleMousePress(cx, cy);
            m_focused->handleMouseRelease(cx, cy);
            m_consumedInput = true;
        }

        // Arrow keys for sliders
        if (input.isKeyPressed(Key::Left)) {
            m_focused->handleKeyPress(static_cast<int>(Key::Left));
            m_consumedInput = true;
        }
        if (input.isKeyPressed(Key::Right)) {
            m_focused->handleKeyPress(static_cast<int>(Key::Right));
            m_consumedInput = true;
        }
    }

    // Scroll wheel routing is handled by UISystem, not here.

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
