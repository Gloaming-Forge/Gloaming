#include "gameplay/ParticlePolishLuaBindings.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "gameplay/ParticleSystem.hpp"
#include "gameplay/TweenSystem.hpp"
#include "gameplay/DebugDrawSystem.hpp"

namespace gloaming {

/// Helper: read a Color from a Lua table { r, g, b, a }
static ColorF readColorF(const sol::table& tbl) {
    return ColorF(
        tbl.get_or("r", 255.0f),
        tbl.get_or("g", 255.0f),
        tbl.get_or("b", 255.0f),
        tbl.get_or("a", 255.0f)
    );
}

/// Helper: read a Color from a Lua table for debug drawing
static Color readColor(const sol::optional<sol::table>& optTbl) {
    if (!optTbl) return Color::Green();
    const auto& tbl = *optTbl;
    return Color(
        tbl.get_or<uint8_t>("r", 0),
        tbl.get_or<uint8_t>("g", 255),
        tbl.get_or<uint8_t>("b", 0),
        tbl.get_or<uint8_t>("a", 255)
    );
}

/// Helper: build ParticleEmitterConfig from a Lua table
static ParticleEmitterConfig readEmitterConfig(const sol::table& opts) {
    ParticleEmitterConfig config;

    // Emission
    config.rate = opts.get_or("rate", 10.0f);
    config.count = opts.get_or("count", 0);

    // Speed
    sol::optional<sol::table> speedTbl = opts.get<sol::optional<sol::table>>("speed");
    if (speedTbl) {
        config.speed.min = speedTbl->get_or("min", 20.0f);
        config.speed.max = speedTbl->get_or("max", 60.0f);
    } else {
        float speedVal = opts.get_or("speed", 40.0f);
        config.speed = RangeF(speedVal, speedVal);
    }

    // Angle
    sol::optional<sol::table> angleTbl = opts.get<sol::optional<sol::table>>("angle");
    if (angleTbl) {
        config.angle.min = angleTbl->get_or("min", 0.0f);
        config.angle.max = angleTbl->get_or("max", 360.0f);
    } else {
        float angleVal = opts.get_or("angle", 0.0f);
        config.angle = RangeF(angleVal, angleVal);
    }

    // Lifetime
    sol::optional<sol::table> lifeTbl = opts.get<sol::optional<sol::table>>("lifetime");
    if (lifeTbl) {
        config.lifetime.min = lifeTbl->get_or("min", 0.3f);
        config.lifetime.max = lifeTbl->get_or("max", 0.8f);
    } else {
        float lifeVal = opts.get_or("lifetime", 0.5f);
        config.lifetime = RangeF(lifeVal, lifeVal);
    }

    // Size
    sol::optional<sol::table> sizeTbl = opts.get<sol::optional<sol::table>>("size");
    if (sizeTbl) {
        config.size.start = sizeTbl->get_or("start", 4.0f);
        config.size.finish = sizeTbl->get_or("finish", 1.0f);
    }

    // Colors
    sol::optional<sol::table> colorTbl = opts.get<sol::optional<sol::table>>("color");
    sol::optional<sol::table> colorStartTbl = opts.get<sol::optional<sol::table>>("color_start");
    sol::optional<sol::table> colorEndTbl = opts.get<sol::optional<sol::table>>("color_end");

    if (colorStartTbl) {
        config.colorStart = readColorF(*colorStartTbl);
    } else if (colorTbl) {
        config.colorStart = readColorF(*colorTbl);
    }

    if (colorEndTbl) {
        config.colorEnd = readColorF(*colorEndTbl);
    } else if (colorTbl) {
        config.colorEnd = readColorF(*colorTbl);
    }

    // Fade
    config.fade = opts.get_or("fade", true);

    // Physics
    config.gravity = opts.get_or("gravity", 0.0f);

    // Offset
    sol::optional<sol::table> offsetTbl = opts.get<sol::optional<sol::table>>("offset");
    if (offsetTbl) {
        config.offset.x = offsetTbl->get_or("x", 0.0f);
        config.offset.y = offsetTbl->get_or("y", 0.0f);
    }

    // Width (for area emitters)
    config.width = opts.get_or("width", 0.0f);

    // Follow camera
    config.followCamera = opts.get_or("follow_camera", false);

    return config;
}

void bindParticlePolishAPI(sol::state& lua, Engine& engine,
                           ParticleSystem& particleSystem,
                           TweenSystem& tweenSystem,
                           DebugDrawSystem& debugDrawSystem) {

    // =========================================================================
    // particles API — particle emitters
    // =========================================================================
    auto particlesApi = lua.create_named_table("particles");

    // particles.burst({ x, y, count, speed, angle, lifetime, size, color, ... })
    particlesApi["burst"] = [&particleSystem](sol::table opts) -> uint32_t {
        ParticleEmitterConfig config = readEmitterConfig(opts);

        Vec2 position(
            opts.get_or("x", 0.0f),
            opts.get_or("y", 0.0f)
        );

        return particleSystem.burst(config, position);
    };

    // particles.attach(entityId, { rate, speed, angle, ... }) -> emitterId
    particlesApi["attach"] = [&particleSystem](uint32_t entityId, sol::table opts) -> uint32_t {
        ParticleEmitterConfig config = readEmitterConfig(opts);
        Entity entity = static_cast<Entity>(entityId);
        return particleSystem.attach(entity, config);
    };

    // particles.spawn_emitter({ x, y, rate, speed, angle, ... }) -> emitterId
    particlesApi["spawn_emitter"] = [&particleSystem](sol::table opts) -> uint32_t {
        ParticleEmitterConfig config = readEmitterConfig(opts);

        Vec2 position(
            opts.get_or("x", 0.0f),
            opts.get_or("y", 0.0f)
        );

        return particleSystem.spawnEmitter(config, position);
    };

    // particles.stop(emitterId)
    particlesApi["stop"] = [&particleSystem](uint32_t id) {
        particleSystem.stopEmitter(id);
    };

    // particles.destroy(emitterId)
    particlesApi["destroy"] = [&particleSystem](uint32_t id) {
        particleSystem.destroyEmitter(id);
    };

    // particles.set_position(emitterId, x, y)
    particlesApi["set_position"] = [&particleSystem](uint32_t id, float x, float y) {
        particleSystem.setEmitterPosition(id, {x, y});
    };

    // particles.is_alive(emitterId) -> bool
    particlesApi["is_alive"] = [&particleSystem](uint32_t id) -> bool {
        return particleSystem.isAlive(id);
    };

    // particles.stats() -> { active_emitters, active_particles, pool_size }
    particlesApi["stats"] = [&particleSystem, &lua]() -> sol::table {
        auto stats = particleSystem.getStats();
        sol::table result = lua.create_table();
        result["active_emitters"] = stats.activeEmitters;
        result["active_particles"] = stats.activeParticles;
        result["pool_size"] = stats.poolSize;
        return result;
    };

    // =========================================================================
    // tween API — property tweening and easing
    // =========================================================================
    auto tweenApi = lua.create_named_table("tween");

    // tween.to(entityId, { x = 100, y = 200, ... }, duration, easing, on_complete)
    // Supports tweening: x, y, rotation, scale_x, scale_y, alpha
    tweenApi["to"] = [&tweenSystem](uint32_t entityId, sol::table properties,
                                     float duration,
                                     sol::optional<std::string> easingName,
                                     sol::optional<sol::function> onComplete) -> sol::object {
        Entity entity = static_cast<Entity>(entityId);
        EasingFunction easing = Easing::linear;

        if (easingName) {
            easing = getEasingByName(*easingName);
        }

        // We may create multiple tweens if multiple properties are specified.
        // Return the last one's ID, or a table of IDs.
        TweenId lastId = InvalidTweenId;

        // Map Lua property names to TweenProperty enum
        struct PropMapping {
            const char* name;
            TweenProperty prop;
        };
        static const PropMapping mappings[] = {
            {"x",       TweenProperty::X},
            {"y",       TweenProperty::Y},
            {"rotation", TweenProperty::Rotation},
            {"scale_x", TweenProperty::ScaleX},
            {"scale_y", TweenProperty::ScaleY},
            {"scale",   TweenProperty::ScaleX}, // 'scale' tweens both X and Y
            {"alpha",   TweenProperty::Alpha},
        };

        // Count how many properties are being tweened
        int propCount = 0;
        for (const auto& m : mappings) {
            sol::optional<float> val = properties.get<sol::optional<float>>(m.name);
            if (val) ++propCount;
        }

        int propIdx = 0;
        for (const auto& m : mappings) {
            sol::optional<float> val = properties.get<sol::optional<float>>(m.name);
            if (!val) continue;

            ++propIdx;
            bool isLast = (propIdx == propCount);

            // Only attach completion callback to the last property tween
            std::function<void()> completionCb = nullptr;
            if (isLast && onComplete) {
                sol::function fn = *onComplete;
                completionCb = [fn]() {
                    try { fn(); }
                    catch (const std::exception& ex) {
                        MOD_LOG_ERROR("tween on_complete error: {}", ex.what());
                    }
                };
            }

            lastId = tweenSystem.tweenTo(entity, m.prop, *val,
                                          duration, easing, std::move(completionCb));

            // Handle 'scale' which tweens both X and Y
            if (std::string(m.name) == "scale") {
                tweenSystem.tweenTo(entity, TweenProperty::ScaleY, *val,
                                    duration, easing, nullptr);
            }
        }

        return sol::make_object(properties.lua_state(), lastId);
    };

    // tween.cancel(tweenId) -> bool
    tweenApi["cancel"] = [&tweenSystem](uint32_t id) -> bool {
        return tweenSystem.cancel(id);
    };

    // tween.cancel_all(entityId) -> int
    tweenApi["cancel_all"] = [&tweenSystem](uint32_t entityId) -> int {
        return tweenSystem.cancelAllForEntity(static_cast<Entity>(entityId));
    };

    // tween.shake({ intensity, duration, decay })
    // or tween.shake(camera_entity_unused, { intensity, duration, decay })
    tweenApi["shake"] = [&tweenSystem](sol::variadic_args va) {
        sol::table opts;
        // Accept both tween.shake({ ... }) and tween.shake(camera, { ... })
        if (va.size() >= 2 && va[1].get_type() == sol::type::table) {
            opts = va[1].as<sol::table>();
        } else if (va.size() >= 1 && va[0].get_type() == sol::type::table) {
            opts = va[0].as<sol::table>();
        } else {
            return;
        }

        float intensity = opts.get_or("intensity", 8.0f);
        float duration = opts.get_or("duration", 0.3f);
        std::string decayName = opts.get_or<std::string>("decay", "ease_out_quad");
        EasingFunction decay = getEasingByName(decayName);

        tweenSystem.shake(intensity, duration, decay);
    };

    // tween.is_shaking() -> bool
    tweenApi["is_shaking"] = [&tweenSystem]() -> bool {
        return tweenSystem.isShaking();
    };

    // tween.active_count() -> int
    tweenApi["active_count"] = [&tweenSystem]() -> int {
        return static_cast<int>(tweenSystem.activeCount());
    };

    // =========================================================================
    // debug API — debug overlay drawing
    // =========================================================================
    auto debugApi = lua.create_named_table("debug");

    // debug.set_enabled(bool)
    debugApi["set_enabled"] = [&debugDrawSystem](bool enabled) {
        debugDrawSystem.setEnabled(enabled);
    };

    // debug.is_enabled() -> bool
    debugApi["is_enabled"] = [&debugDrawSystem]() -> bool {
        return debugDrawSystem.isEnabled();
    };

    // debug.toggle()
    debugApi["toggle"] = [&debugDrawSystem]() {
        debugDrawSystem.toggle();
    };

    // debug.draw_rect(x, y, width, height, { r, g, b, a })
    debugApi["draw_rect"] = [&debugDrawSystem](float x, float y, float w, float h,
                                                sol::optional<sol::table> colorOpt) {
        debugDrawSystem.drawRect(x, y, w, h, readColor(colorOpt));
    };

    // debug.draw_rect_outline(x, y, width, height, { r, g, b, a })
    debugApi["draw_rect_outline"] = [&debugDrawSystem](float x, float y, float w, float h,
                                                        sol::optional<sol::table> colorOpt) {
        debugDrawSystem.drawRectOutline(x, y, w, h, readColor(colorOpt));
    };

    // debug.draw_circle(x, y, radius, { r, g, b, a })
    debugApi["draw_circle"] = [&debugDrawSystem](float x, float y, float radius,
                                                  sol::optional<sol::table> colorOpt) {
        debugDrawSystem.drawCircle(x, y, radius, readColor(colorOpt));
    };

    // debug.draw_circle_outline(x, y, radius, { r, g, b, a })
    debugApi["draw_circle_outline"] = [&debugDrawSystem](float x, float y, float radius,
                                                          sol::optional<sol::table> colorOpt) {
        debugDrawSystem.drawCircleOutline(x, y, radius, readColor(colorOpt));
    };

    // debug.draw_line(x1, y1, x2, y2, { r, g, b, a })
    debugApi["draw_line"] = [&debugDrawSystem](float x1, float y1, float x2, float y2,
                                                sol::optional<sol::table> colorOpt) {
        debugDrawSystem.drawLine(x1, y1, x2, y2, readColor(colorOpt));
    };

    // debug.draw_point(x, y, { r, g, b, a })
    debugApi["draw_point"] = [&debugDrawSystem](float x, float y,
                                                 sol::optional<sol::table> colorOpt) {
        debugDrawSystem.drawPoint(x, y, readColor(colorOpt));
    };

    // debug.draw_text(text, x, y, { r, g, b, a })
    debugApi["draw_text"] = [&debugDrawSystem](const std::string& text,
                                                float x, float y,
                                                sol::optional<sol::table> colorOpt) {
        debugDrawSystem.drawText(text, x, y, readColor(colorOpt));
    };

    // debug.draw_path(path_table, { r, g, b, a })
    // path_table is array of { x, y } or { point.x, point.y }
    debugApi["draw_path"] = [&debugDrawSystem](sol::table pathTable,
                                                sol::optional<sol::table> colorOpt) {
        std::vector<Vec2> points;
        pathTable.for_each([&](const sol::object&, const sol::object& val) {
            if (val.get_type() == sol::type::table) {
                sol::table pt = val.as<sol::table>();
                float x = pt.get_or("x", 0.0f);
                float y = pt.get_or("y", 0.0f);
                points.push_back({x, y});
            }
        });
        debugDrawSystem.drawPath(points, readColor(colorOpt));
    };

    // debug.draw_text_screen(text, x, y, { r, g, b, a })
    debugApi["draw_text_screen"] = [&debugDrawSystem](const std::string& text,
                                                       float x, float y,
                                                       sol::optional<sol::table> colorOpt) {
        debugDrawSystem.drawTextScreen(text, x, y, readColor(colorOpt));
    };

    // debug.draw_rect_screen(x, y, w, h, { r, g, b, a })
    debugApi["draw_rect_screen"] = [&debugDrawSystem](float x, float y, float w, float h,
                                                       sol::optional<sol::table> colorOpt) {
        debugDrawSystem.drawRectScreen(x, y, w, h, readColor(colorOpt));
    };

    // debug.draw_line_screen(x1, y1, x2, y2, { r, g, b, a })
    debugApi["draw_line_screen"] = [&debugDrawSystem](float x1, float y1, float x2, float y2,
                                                       sol::optional<sol::table> colorOpt) {
        debugDrawSystem.drawLineScreen(x1, y1, x2, y2, readColor(colorOpt));
    };
}

} // namespace gloaming
