#pragma once

#include "engine/Input.hpp"
#include "engine/Gamepad.hpp"
#include "rendering/IRenderer.hpp"

#include <string>
#include <functional>

namespace gloaming {

/// Built-in on-screen keyboard for text input when Steam overlay is not available.
/// For Steam Deck, the Steam overlay keyboard is preferred (via SteamIntegration).
/// This provides a gamepad-navigable fallback for non-Steam builds.
class OnScreenKeyboard {
public:
    /// Request text input. Shows built-in keyboard UI.
    /// callback is invoked with the result string when the user confirms,
    /// or empty string if cancelled.
    void requestTextInput(const std::string& description,
                          const std::string& existingText,
                          int maxChars,
                          std::function<void(const std::string&)> callback);

    /// Check if the keyboard is currently visible
    bool isVisible() const;

    /// Update (process keyboard input from gamepad and keyboard)
    void update(const Input& input, const Gamepad& gamepad);

    /// Render the built-in keyboard overlay
    void render(IRenderer* renderer);

    /// Dismiss without confirming
    void dismiss();

private:
    void confirm();
    void moveCursor(int dx, int dy);
    void pressSelectedKey();
    char getKeyAt(int row, int col) const;

    bool m_visible = false;
    std::string m_description;
    std::string m_text;
    int m_maxChars = 256;
    std::function<void(const std::string&)> m_callback;

    // Keyboard grid state
    int m_cursorRow = 1;    // Row in the key grid (0 = numbers, 1-3 = letters, 4 = bottom)
    int m_cursorCol = 0;    // Column in the key grid
    bool m_shiftActive = false;

    // Navigation repeat timer
    float m_navTimer = 0.0f;
    float m_navRepeatDelay = 0.4f;
    float m_navRepeatRate = 0.1f;
    bool m_navHeld = false;

    // Key layout (QWERTY)
    static constexpr int ROWS = 5;
    static constexpr int COLS = 10;
    static constexpr char LAYOUT[ROWS][COLS + 1] = {
        "1234567890",
        "qwertyuiop",
        "asdfghjkl;",
        "zxcvbnm,./",
        " <=>      "  // Space, Backspace(<), Shift(=), Enter(>)
    };
};

} // namespace gloaming
