#pragma once

#include "engine/Config.hpp"
#include "engine/Window.hpp"
#include "engine/Input.hpp"
#include "engine/Time.hpp"

#include <string>

namespace gloaming {

class Engine {
public:
    bool init(const std::string& configPath = "config.json");
    void run();
    void shutdown();

    Config& getConfig() { return m_config; }
    Window& getWindow() { return m_window; }
    Input&  getInput()  { return m_input; }
    Time&   getTime()   { return m_time; }

private:
    void processInput();
    void update(double dt);
    void render();

    Config m_config;
    Window m_window;
    Input  m_input;
    Time   m_time;

    bool m_running = false;
};

} // namespace gloaming
