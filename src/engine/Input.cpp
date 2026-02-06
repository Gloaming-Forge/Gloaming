#include "engine/Input.hpp"

#include <raylib.h>

namespace gloaming {

void Input::update() {
    m_mouseWheelDelta = GetMouseWheelMove();
}

// Key enum overloads
bool Input::isKeyPressed(Key key) const {
    return IsKeyPressed(static_cast<int>(key));
}

bool Input::isKeyDown(Key key) const {
    return IsKeyDown(static_cast<int>(key));
}

bool Input::isKeyReleased(Key key) const {
    return IsKeyReleased(static_cast<int>(key));
}

// Raw int overloads (backward compatibility)
bool Input::isKeyPressed(int key) const {
    return IsKeyPressed(key);
}

bool Input::isKeyDown(int key) const {
    return IsKeyDown(key);
}

bool Input::isKeyReleased(int key) const {
    return IsKeyReleased(key);
}

float Input::getMouseX() const {
    return static_cast<float>(GetMouseX());
}

float Input::getMouseY() const {
    return static_cast<float>(GetMouseY());
}

// MouseButton enum overloads
bool Input::isMouseButtonPressed(MouseButton button) const {
    return IsMouseButtonPressed(static_cast<int>(button));
}

bool Input::isMouseButtonDown(MouseButton button) const {
    return IsMouseButtonDown(static_cast<int>(button));
}

// Raw int overloads (backward compatibility)
bool Input::isMouseButtonPressed(int button) const {
    return IsMouseButtonPressed(button);
}

bool Input::isMouseButtonDown(int button) const {
    return IsMouseButtonDown(button);
}

float Input::getMouseWheelDelta() const {
    return m_mouseWheelDelta;
}

} // namespace gloaming
