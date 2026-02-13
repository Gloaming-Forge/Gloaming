#pragma once

#include "engine/Input.hpp"
#include "engine/Gamepad.hpp"

namespace gloaming {

/// Which input device was most recently used.
enum class InputDevice {
    KeyboardMouse,
    Gamepad
};

/// Tracks which input device (keyboard/mouse or gamepad) was most recently used.
/// Required for glyph switching — Deck Verified mandates showing controller glyphs
/// when using controller, keyboard glyphs when using keyboard.
class InputDeviceTracker {
public:
    void update(const Input& input, const gloaming::Gamepad& gamepad);

    /// Which device was most recently used?
    InputDevice getActiveDevice() const;

    /// Did the active device change this frame?
    bool didDeviceChange() const;

private:
    /// Check if any keyboard or mouse input occurred this frame.
    bool detectKeyboardMouseInput(const Input& input) const;

    InputDevice m_activeDevice = InputDevice::KeyboardMouse;
    bool m_changed = false;
    int m_hysteresisFrames = 0;
    static constexpr int HYSTERESIS_THRESHOLD = 2;
};

} // namespace gloaming
