#include "ui/UIInput.hpp"

#include <cmath>
#include <limits>

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

bool UIInput::update(UIElement* root, const Input& input, const Gamepad& gamepad,
                     InputDevice activeDevice) {
    if (!root) return false;

    // Process keyboard/mouse input as before
    bool consumed = update(root, input);

    // Process gamepad input if enabled
    if (m_gamepadNavEnabled && activeDevice == InputDevice::Gamepad) {
        if (processGamepadInput(root, gamepad)) {
            consumed = true;
            m_consumedInput = true;
        }
    }

    // Auto-focus first element when gamepad is active and nothing is focused
    if (activeDevice == InputDevice::Gamepad && !m_focused && m_gamepadNavEnabled) {
        std::vector<UIElement*> focusable;
        collectFocusable(root, focusable);
        if (!focusable.empty()) {
            setFocus(focusable[0]);
        }
    }

    return consumed;
}

bool UIInput::processGamepadInput(UIElement* root, const Gamepad& gamepad) {
    bool consumed = false;

    // D-pad navigation
    int dx = 0, dy = 0;
    if (gamepad.isButtonPressed(GamepadButton::DpadLeft))  dx = -1;
    if (gamepad.isButtonPressed(GamepadButton::DpadRight)) dx = 1;
    if (gamepad.isButtonPressed(GamepadButton::DpadUp))    dy = -1;
    if (gamepad.isButtonPressed(GamepadButton::DpadDown))  dy = 1;

    // Left stick navigation with auto-repeat
    Vec2 stick = gamepad.getLeftStick();
    bool stickActive = (stick.lengthSquared() > 0.25f);

    if (stickActive) {
        int stickDx = 0, stickDy = 0;
        if (stick.x < -0.5f) stickDx = -1;
        if (stick.x > 0.5f)  stickDx = 1;
        if (stick.y < -0.5f) stickDy = -1;
        if (stick.y > 0.5f)  stickDy = 1;

        bool directionChanged = (stickDx != m_lastNavDx || stickDy != m_lastNavDy);
        m_lastNavDx = stickDx;
        m_lastNavDy = stickDy;

        if (directionChanged) {
            dx = stickDx;
            dy = stickDy;
            m_navTimer = m_navRepeatDelay;
        } else {
            m_navTimer -= 1.0f / 60.0f; // Approximate frame time
            if (m_navTimer <= 0.0f) {
                dx = stickDx;
                dy = stickDy;
                m_navTimer = m_navRepeatRate;
            }
        }
    } else {
        m_lastNavDx = 0;
        m_lastNavDy = 0;
        m_navTimer = 0.0f;
    }

    if (dx != 0 || dy != 0) {
        navigateFocus(root, dx, dy);
        consumed = true;
    }

    // A button = confirm/click the focused element
    if (gamepad.isButtonPressed(GamepadButton::FaceDown)) {
        confirmFocus();
        consumed = true;
    }

    // B button = cancel/back
    if (gamepad.isButtonPressed(GamepadButton::FaceRight)) {
        if (cancelFocus()) {
            consumed = true;
        }
    }

    // Bumpers for tab switching (LB = prev tab, RB = next tab)
    if (m_focused) {
        if (gamepad.isButtonPressed(GamepadButton::LeftBumper)) {
            m_focused->handleKeyPress(static_cast<int>(Key::Left));
            consumed = true;
        }
        if (gamepad.isButtonPressed(GamepadButton::RightBumper)) {
            m_focused->handleKeyPress(static_cast<int>(Key::Right));
            consumed = true;
        }
    }

    return consumed;
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

void UIInput::setGamepadNavigationEnabled(bool enabled) {
    m_gamepadNavEnabled = enabled;
}

void UIInput::setSpatialNavigation(bool enabled) {
    m_spatialNav = enabled;
}

void UIInput::navigateFocus(UIElement* root, int dx, int dy) {
    if (!root) return;

    if (m_spatialNav && m_focused) {
        UIElement* neighbor = findSpatialNeighbor(root, dx, dy);
        if (neighbor) {
            setFocus(neighbor);
            return;
        }
    }

    // Linear navigation: up/left = prev, down/right = next
    if (dy < 0 || dx < 0) {
        focusPrev(root);
    } else if (dy > 0 || dx > 0) {
        focusNext(root);
    }
}

void UIInput::confirmFocus() {
    if (!m_focused) return;

    auto& layout = m_focused->getLayout();
    float cx = layout.x + layout.width * 0.5f;
    float cy = layout.y + layout.height * 0.5f;
    m_focused->handleMousePress(cx, cy);
    m_focused->handleMouseRelease(cx, cy);
}

bool UIInput::cancelFocus() {
    // B button navigates back — clear focus as a basic behavior.
    // In a full implementation, this would trigger screen pop/navigation back.
    if (m_focused) {
        setFocus(nullptr);
        return true;
    }
    return false;
}

UIElement* UIInput::findSpatialNeighbor(UIElement* root, int dx, int dy) const {
    if (!m_focused) return nullptr;

    std::vector<UIElement*> focusable;
    collectFocusable(root, focusable);
    if (focusable.empty()) return nullptr;

    auto& currentLayout = m_focused->getLayout();
    float cx = currentLayout.x + currentLayout.width * 0.5f;
    float cy = currentLayout.y + currentLayout.height * 0.5f;

    UIElement* best = nullptr;
    float bestScore = std::numeric_limits<float>::max();

    for (auto* candidate : focusable) {
        if (candidate == m_focused) continue;

        auto& candLayout = candidate->getLayout();
        float candX = candLayout.x + candLayout.width * 0.5f;
        float candY = candLayout.y + candLayout.height * 0.5f;

        float deltaX = candX - cx;
        float deltaY = candY - cy;

        // Filter: only consider candidates in the correct direction
        bool valid = false;
        if (dx > 0 && deltaX > 0) valid = true;
        if (dx < 0 && deltaX < 0) valid = true;
        if (dy > 0 && deltaY > 0) valid = true;
        if (dy < 0 && deltaY < 0) valid = true;
        if (!valid) continue;

        // Score: prefer candidates that are mostly aligned on the primary axis
        // Use weighted distance: primary axis distance + 3x cross-axis distance
        float primaryDist = 0.0f;
        float crossDist = 0.0f;
        if (dx != 0) {
            primaryDist = std::abs(deltaX);
            crossDist = std::abs(deltaY);
        } else {
            primaryDist = std::abs(deltaY);
            crossDist = std::abs(deltaX);
        }

        float score = primaryDist + crossDist * 3.0f;
        if (score < bestScore) {
            bestScore = score;
            best = candidate;
        }
    }

    return best;
}

} // namespace gloaming
