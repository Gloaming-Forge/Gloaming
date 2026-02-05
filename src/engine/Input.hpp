#pragma once

#include <cstdint>

namespace gloaming {

/// Engine-defined key codes, independent of the backend (Raylib, SDL, etc.)
/// Maps 1:1 with Raylib key codes for the current backend.
enum class Key : int {
    // Alphanumeric
    A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71, H = 72,
    I = 73, J = 74, K = 75, L = 76, M = 77, N = 78, O = 79, P = 80,
    Q = 81, R = 82, S = 83, T = 84, U = 85, V = 86, W = 87, X = 88,
    Y = 89, Z = 90,

    // Numbers
    Num0 = 48, Num1 = 49, Num2 = 50, Num3 = 51, Num4 = 52,
    Num5 = 53, Num6 = 54, Num7 = 55, Num8 = 56, Num9 = 57,

    // Function keys
    F1 = 290, F2 = 291, F3 = 292, F4 = 293, F5 = 294, F6 = 295,
    F7 = 296, F8 = 297, F9 = 298, F10 = 299, F11 = 300, F12 = 301,

    // Arrow keys
    Up = 265, Down = 264, Left = 263, Right = 262,

    // Modifiers
    LeftShift = 340, RightShift = 344,
    LeftControl = 341, RightControl = 345,
    LeftAlt = 342, RightAlt = 346,

    // Special keys
    Space = 32,
    Enter = 257,
    Escape = 256,
    Backspace = 259,
    Tab = 258,
    Delete = 261,
    Insert = 260,
    Home = 268,
    End = 269,
    PageUp = 266,
    PageDown = 267,

    // Punctuation
    Minus = 45,
    Equal = 61,
    LeftBracket = 91,
    RightBracket = 93,
    Backslash = 92,
    Semicolon = 59,
    Apostrophe = 39,
    Comma = 44,
    Period = 46,
    Slash = 47,
    GraveAccent = 96,
};

/// Engine-defined mouse button codes
enum class MouseButton : int {
    Left = 0,
    Right = 1,
    Middle = 2,
};

/// Thin abstraction over Raylib input.
/// Keeps game code decoupled from the backend.
class Input {
public:
    void update();

    // Keyboard (engine Key enum)
    bool isKeyPressed(Key key) const;
    bool isKeyDown(Key key) const;
    bool isKeyReleased(Key key) const;

    // Keyboard (raw int, for backward compatibility)
    bool isKeyPressed(int key) const;
    bool isKeyDown(int key) const;
    bool isKeyReleased(int key) const;

    // Mouse
    float getMouseX() const;
    float getMouseY() const;
    bool  isMouseButtonPressed(MouseButton button) const;
    bool  isMouseButtonDown(MouseButton button) const;
    bool  isMouseButtonPressed(int button) const;
    bool  isMouseButtonDown(int button) const;
    float getMouseWheelDelta() const;

private:
    float m_mouseWheelDelta = 0.0f;
};

} // namespace gloaming
