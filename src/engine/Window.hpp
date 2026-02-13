#pragma once

#include <string>

namespace gloaming {

enum class FullscreenMode {
    Windowed,
    Fullscreen,           // Exclusive fullscreen (changes display mode)
    BorderlessFullscreen,  // Borderless window at desktop resolution (preferred for Deck)
};

struct WindowConfig {
    int         width      = 1280;
    int         height     = 720;
    std::string title      = "Gloaming";
    bool        fullscreen = false;
    bool        vsync      = true;
    FullscreenMode fullscreenMode = FullscreenMode::BorderlessFullscreen;
};

class Window {
public:
    bool init(const WindowConfig& config);
    void shutdown();

    bool shouldClose() const;
    void beginFrame();
    void endFrame();

    int  getWidth() const;
    int  getHeight() const;
    void setTitle(const std::string& title);
    void toggleFullscreen();

    /// Set the fullscreen mode explicitly
    void setFullscreenMode(FullscreenMode mode);
    FullscreenMode getFullscreenMode() const { return m_fullscreenMode; }

    /// Get the monitor refresh rate in Hz
    int getRefreshRate() const;

    /// Check if currently in any fullscreen mode
    bool isFullscreen() const;

    /// Check if the window is focused (for suspend/resume detection)
    bool isFocused() const;

private:
    bool m_initialized = false;
    FullscreenMode m_fullscreenMode = FullscreenMode::Windowed;
};

} // namespace gloaming
