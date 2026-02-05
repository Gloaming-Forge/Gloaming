#pragma once

namespace gloaming {

/// Thin abstraction over Raylib input.
/// Keeps game code decoupled from the backend.
class Input {
public:
    void update();

    // Keyboard
    bool isKeyPressed(int key) const;
    bool isKeyDown(int key) const;
    bool isKeyReleased(int key) const;

    // Mouse
    float getMouseX() const;
    float getMouseY() const;
    bool  isMouseButtonPressed(int button) const;
    bool  isMouseButtonDown(int button) const;
    float getMouseWheelDelta() const;

private:
    float m_mouseWheelDelta = 0.0f;
};

} // namespace gloaming
