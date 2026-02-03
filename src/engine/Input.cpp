#include "engine/Input.hpp"

#include <raylib.h>

namespace gloaming {

void Input::update() {
    m_mouseWheelDelta = GetMouseWheelMove();
}

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
