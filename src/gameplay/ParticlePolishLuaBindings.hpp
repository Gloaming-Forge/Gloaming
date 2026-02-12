#pragma once

#include <sol/sol.hpp>

namespace gloaming {

class Engine;
class ParticleSystem;
class TweenSystem;
class DebugDrawSystem;

/// Registers all Stage 17 Lua APIs: Particles, Tweening, Debug Drawing.
///
/// Provides:
///   particles.*  — Burst and continuous particle emitters, entity attachment
///   tween.*      — Property tweening with easing functions, camera shake
///   debug.*      — Overlay drawing (rects, circles, lines, paths, text)
void bindParticlePolishAPI(sol::state& lua, Engine& engine,
                           ParticleSystem& particleSystem,
                           TweenSystem& tweenSystem,
                           DebugDrawSystem& debugDrawSystem);

} // namespace gloaming
