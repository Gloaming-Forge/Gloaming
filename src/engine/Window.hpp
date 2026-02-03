#pragma once

#include <string>

namespace gloaming {

struct WindowConfig {
    int         width      = 1280;
    int         height     = 720;
    std::string title      = "Gloaming";
    bool        fullscreen = false;
    bool        vsync      = true;
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

private:
    bool m_initialized = false;
};

} // namespace gloaming
