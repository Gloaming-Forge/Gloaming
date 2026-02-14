#include "engine/Window.hpp"

#include <raylib.h>

namespace gloaming {

bool Window::init(const WindowConfig& config) {
    unsigned int flags = FLAG_WINDOW_RESIZABLE;
    if (config.vsync) {
        flags |= FLAG_VSYNC_HINT;
    }

    // For borderless fullscreen, set the undecorated + fullscreen flags
    if (config.fullscreen && config.fullscreenMode == FullscreenMode::BorderlessFullscreen) {
        flags |= FLAG_BORDERLESS_WINDOWED_MODE;
    }

    SetConfigFlags(flags);

    InitWindow(config.width, config.height, config.title.c_str());

    if (!IsWindowReady()) {
        return false;
    }

    if (config.fullscreen) {
        if (config.fullscreenMode == FullscreenMode::Fullscreen) {
            ToggleFullscreen();
            m_fullscreenMode = FullscreenMode::Fullscreen;
        } else if (config.fullscreenMode == FullscreenMode::BorderlessFullscreen) {
            // Borderless is already applied via FLAG_BORDERLESS_WINDOWED_MODE above;
            // calling ToggleBorderlessWindowed() here would double-toggle it off.
            m_fullscreenMode = FullscreenMode::BorderlessFullscreen;
        }
    } else {
        m_fullscreenMode = FullscreenMode::Windowed;
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
    if (m_fullscreenMode == FullscreenMode::Windowed) {
        // Enter borderless fullscreen by default when toggling
        ToggleBorderlessWindowed();
        m_fullscreenMode = FullscreenMode::BorderlessFullscreen;
    } else {
        // Return to windowed mode
        if (m_fullscreenMode == FullscreenMode::Fullscreen) {
            ToggleFullscreen();
        } else {
            ToggleBorderlessWindowed();
        }
        m_fullscreenMode = FullscreenMode::Windowed;
    }
}

void Window::setFullscreenMode(FullscreenMode mode) {
    if (mode == m_fullscreenMode) return;

    // First, exit current mode
    if (m_fullscreenMode == FullscreenMode::Fullscreen) {
        ToggleFullscreen();
    } else if (m_fullscreenMode == FullscreenMode::BorderlessFullscreen) {
        ToggleBorderlessWindowed();
    }

    // Then, enter new mode
    if (mode == FullscreenMode::Fullscreen) {
        ToggleFullscreen();
    } else if (mode == FullscreenMode::BorderlessFullscreen) {
        ToggleBorderlessWindowed();
    }

    m_fullscreenMode = mode;
}

int Window::getRefreshRate() const {
    return GetMonitorRefreshRate(GetCurrentMonitor());
}

bool Window::isFullscreen() const {
    return m_fullscreenMode != FullscreenMode::Windowed;
}

bool Window::isFocused() const {
    return IsWindowFocused();
}

bool Window::pollSizeChanged() {
    int w = getWidth();
    int h = getHeight();
    if (w != m_lastWidth || h != m_lastHeight) {
        m_lastWidth = w;
        m_lastHeight = h;
        return true;
    }
    return false;
}

} // namespace gloaming
