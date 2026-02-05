#include "engine/Engine.hpp"
#include "engine/Log.hpp"

int main() {
    gloaming::Engine engine;

    if (!engine.init("config.json")) {
        return 1;
    }

    LOG_INFO("Hello World from Gloaming!");

    engine.run();
    engine.shutdown();

    return 0;
}
