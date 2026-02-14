#include "engine/SeamlessnessLuaBindings.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"

namespace gloaming {

void bindSeamlessnessAPI(sol::state& lua, Engine& engine) {
    // Create or extend the "engine" table
    sol::table engineApi = lua["engine"].valid()
        ? lua["engine"].get<sol::table>()
        : lua.create_named_table("engine");

    // engine.is_suspended() -> bool
    engineApi["is_suspended"] = [&engine]() -> bool {
        return engine.isSuspended();
    };

    // engine.request_exit() — request graceful shutdown
    engineApi["request_exit"] = [&engine]() {
        LOG_INFO("Graceful exit requested via Lua API");
        engine.requestShutdown();
    };

    // engine.on_suspend(callback) -> handler_id
    // Shorthand for event.on("engine.suspend", callback)
    engineApi["on_suspend"] = [&engine](sol::function callback) -> uint64_t {
        sol::function fn = callback;
        auto& eventBus = engine.getEventBus();
        return eventBus.on("engine.suspend", [fn](const EventData&) -> bool {
            try { fn(); }
            catch (const std::exception& ex) {
                MOD_LOG_ERROR("engine.on_suspend callback error: {}", ex.what());
            }
            return false;  // Don't cancel the event
        });
    };

    // engine.on_resume(callback) -> handler_id
    // Shorthand for event.on("engine.resume", callback)
    engineApi["on_resume"] = [&engine](sol::function callback) -> uint64_t {
        sol::function fn = callback;
        auto& eventBus = engine.getEventBus();
        return eventBus.on("engine.resume", [fn](const EventData&) -> bool {
            try { fn(); }
            catch (const std::exception& ex) {
                MOD_LOG_ERROR("engine.on_resume callback error: {}", ex.what());
            }
            return false;
        });
    };

    // engine.on_shutdown(callback) -> handler_id
    // Shorthand for event.on("engine.shutdown", callback)
    engineApi["on_shutdown"] = [&engine](sol::function callback) -> uint64_t {
        sol::function fn = callback;
        auto& eventBus = engine.getEventBus();
        return eventBus.on("engine.shutdown", [fn](const EventData&) -> bool {
            try { fn(); }
            catch (const std::exception& ex) {
                MOD_LOG_ERROR("engine.on_shutdown callback error: {}", ex.what());
            }
            return false;
        });
    };
}

} // namespace gloaming
