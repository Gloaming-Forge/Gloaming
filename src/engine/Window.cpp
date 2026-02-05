#include "engine/Window.hpp"

#include <raylib.h>

namespace gloaming {

bool Window::init(const WindowConfig& config) {
    unsigned int flags = FLAG_WINDOW_RESIZABLE;
    if (config.vsync) {
        flags |= FLAG_VSYNC_HINT;
    }
    SetConfigFlags(flags);

    InitWindow(config.width, config.height, config.title.c_str());

    if (!IsWindowReady()) {
        return false;
    }

    if (config.fullscreen) {
        ToggleFullscreen();
    }

    m_initialized = true;
    return true;
}

void Window::shutdown() {
    if (m_initialized) {
        CloseWindow();
        m_initialized = false;
    }
}

bool Window::shouldClose() const {
    return WindowShouldClose();
}

void Window::beginFrame() {
    BeginDrawing();
    ClearBackground({20, 20, 30, 255});
}

void Window::endFrame() {
    EndDrawing();
}

int Window::getWidth() const {
    return GetScreenWidth();
}

int Window::getHeight() const {
    return GetScreenHeight();
}

void Window::setTitle(const std::string& title) {
    SetWindowTitle(title.c_str());
}

void Window::toggleFullscreen() {
    ToggleFullscreen();
}

} // namespace gloaming
