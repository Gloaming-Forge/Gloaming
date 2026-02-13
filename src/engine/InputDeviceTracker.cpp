#include "engine/InputDeviceTracker.hpp"

#include <raylib.h>

namespace gloaming {

void InputDeviceTracker::update(const Input& input, const gloaming::Gamepad& gamepad) {
    m_changed = false;

    bool kbActive = detectKeyboardMouseInput(input);
    bool gpActive = gamepad.hadAnyInput();

    // Determine candidate device based on this frame's activity
    InputDevice candidate = m_activeDevice;
    if (gpActive && !kbActive) {
        candidate = InputDevice::Gamepad;
    } else if (kbActive && !gpActive) {
        candidate = InputDevice::KeyboardMouse;
    }
    // If both or neither, keep current device (no change)

    // Hysteresis: require sustained input on a new device before switching
    if (candidate != m_activeDevice) {
        ++m_hysteresisFrames;
        if (m_hysteresisFrames >= HYSTERESIS_THRESHOLD) {
            m_activeDevice = candidate;
            m_changed = true;
            m_hysteresisFrames = 0;
        }
    } else {
        m_hysteresisFrames = 0;
    }
}

InputDevice InputDeviceTracker::getActiveDevice() const {
    return m_activeDevice;
}

bool InputDeviceTracker::didDeviceChange() const {
    return m_changed;
}

bool InputDeviceTracker::detectKeyboardMouseInput(const Input& input) const {
    // Non-consuming keyboard activity check using IsKeyDown instead of
    // GetKeyPressed() which would consume keys from the input queue.
    for (int key = 32; key < 127; ++key) {
        if (IsKeyDown(key)) return true;
    }
    for (int key = 256; key <= 348; ++key) {
        if (IsKeyDown(key)) return true;
    }

    // Check mouse movement
    float dx = static_cast<float>(GetMouseDelta().x);
    float dy = static_cast<float>(GetMouseDelta().y);
    if (dx != 0.0f || dy != 0.0f) return true;

    // Check mouse buttons
    if (input.isMouseButtonPressed(MouseButton::Left) ||
        input.isMouseButtonPressed(MouseButton::Right) ||
        input.isMouseButtonPressed(MouseButton::Middle)) {
        return true;
    }

    // Check scroll wheel
    if (input.getMouseWheelDelta() != 0.0f) return true;

    return false;
}

} // namespace gloaming
