#pragma once

#include "ecs/Entity.hpp"
#include "ecs/Systems.hpp"
#include "rendering/IRenderer.hpp"
#include "rendering/Camera.hpp"

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <random>

namespace gloaming {

// Forward declarations
class Registry;

/// Unique identifier for an emitter instance
using EmitterId = uint32_t;

/// Invalid emitter ID sentinel
constexpr EmitterId InvalidEmitterId = 0;

/// Range helper for random values
struct RangeF {
    float min = 0.0f;
    float max = 0.0f;

    constexpr RangeF() = default;
    constexpr RangeF(float val) : min(val), max(val) {}
    constexpr RangeF(float mn, float mx) : min(mn), max(mx) {}
};

/// Color with float components for interpolation
struct ColorF {
    float r = 255.0f;
    float g = 255.0f;
    float b = 255.0f;
    float a = 255.0f;

    constexpr ColorF() = default;
    constexpr ColorF(float r, float g, float b, float a = 255.0f)
        : r(r), g(g), b(b), a(a) {}

    Color toColor() const {
        return Color(
            static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, r))),
            static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, g))),
            static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, b))),
            static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, a)))
        );
    }

    static ColorF lerp(const ColorF& a, const ColorF& b, float t) {
        return ColorF(
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t,
            a.a + (b.a - a.a) * t
        );
    }
};

/// Size curve for particles (start to finish)
struct SizeCurve {
    float start = 4.0f;
    float finish = 1.0f;

    constexpr SizeCurve() = default;
    constexpr SizeCurve(float s, float f) : start(s), finish(f) {}

    float evaluate(float t) const {
        return start + (finish - start) * t;
    }
};

/// Configuration for a particle emitter
struct ParticleEmitterConfig {
    // Emission
    float rate = 10.0f;             // Particles per second (continuous mode)
    int count = 0;                  // Particles per burst (burst mode, 0 = continuous)

    // Particle properties
    RangeF speed{20.0f, 60.0f};     // Initial speed range
    RangeF angle{0.0f, 360.0f};     // Emission angle range (degrees)
    RangeF lifetime{0.3f, 0.8f};    // Particle lifetime range
    SizeCurve size{4.0f, 1.0f};     // Size over lifetime
    ColorF colorStart{255, 255, 255, 255};
    ColorF colorEnd{255, 255, 255, 0};
    bool fade = true;               // Fade alpha over lifetime

    // Physics
    float gravity = 0.0f;           // Gravity applied to particles (pixels/sec^2)

    // Position offset from emitter origin
    Vec2 offset{0.0f, 0.0f};
    float width = 0.0f;             // Emitter width (for area emitters like rain)

    // Behavior
    bool followCamera = false;      // Emitter follows camera position
    bool worldSpace = true;         // Particles simulate in world space
};

/// A single particle in the pool
struct Particle {
    Vec2 position{0.0f, 0.0f};
    Vec2 velocity{0.0f, 0.0f};
    float lifetime = 1.0f;         // Total lifetime
    float age = 0.0f;              // Current age
    float size = 4.0f;             // Current size
    ColorF colorStart;
    ColorF colorEnd;
    SizeCurve sizeCurve;
    float gravity = 0.0f;
    EmitterId emitterId = InvalidEmitterId; // Which emitter spawned this particle
    bool alive = false;
};

/// An active emitter instance
struct EmitterInstance {
    EmitterId id = InvalidEmitterId;
    ParticleEmitterConfig config;
    Vec2 position{0.0f, 0.0f};     // World position (or camera-relative if followCamera)
    Entity entity = NullEntity;     // Attached entity (NullEntity = world position)
    float emitAccumulator = 0.0f;   // Fractional particle accumulation for continuous mode
    float deathTimer = 0.0f;        // Timer counting up after emitter stops, for cleanup
    bool active = true;             // If false, stops emitting but existing particles continue
    bool alive = true;              // If false, marked for removal once all particles die
};

/// Particle system — manages emitters and their particle pools.
///
/// Provides:
///   - Data-driven emitter configurations
///   - Burst mode (emit N particles at once) and continuous mode (emit N per second)
///   - Entity-attached or world-position emitters
///   - Configurable lifetime, speed, angle, color, size curves
///   - Particle pool for zero-allocation emission
///   - Camera-following emitters (for weather effects)
class ParticleSystem : public System {
public:
    ParticleSystem();
    ~ParticleSystem() override = default;

    void init(Registry& registry, Engine& engine) override;
    void update(float dt) override;
    void shutdown() override {}

    /// Render all active particles
    void render(IRenderer* renderer, const Camera& camera);

    /// Burst: emit a set of particles at a world position (one-shot)
    /// @return emitter ID (particles will play out and auto-remove)
    EmitterId burst(const ParticleEmitterConfig& config, Vec2 position);

    /// Attach a continuous emitter to an entity
    /// @return emitter ID for later control
    EmitterId attach(Entity entity, const ParticleEmitterConfig& config);

    /// Spawn a free-standing continuous emitter at a world position
    /// @return emitter ID for later control
    EmitterId spawnEmitter(const ParticleEmitterConfig& config, Vec2 position);

    /// Stop an emitter (existing particles continue, no new particles emitted)
    void stopEmitter(EmitterId id);

    /// Destroy an emitter and all its particles immediately
    void destroyEmitter(EmitterId id);

    /// Set emitter position (for world-position emitters)
    void setEmitterPosition(EmitterId id, Vec2 position);

    /// Check if an emitter is still alive
    bool isAlive(EmitterId id) const;

    /// Remove all emitters attached to a specific entity
    void removeEmittersForEntity(Entity entity);

    /// Get statistics
    struct Stats {
        size_t activeEmitters = 0;
        size_t activeParticles = 0;
        size_t poolSize = 0;
    };
    Stats getStats() const;

    /// Set maximum particle count (pool size limit)
    void setMaxParticles(size_t maxParticles) { m_maxParticles = maxParticles; }

private:
    /// Emit particles from an emitter
    void emitParticles(EmitterInstance& emitter, int count);

    /// Get or allocate a particle slot from the pool (O(1) via free-list)
    Particle* allocateParticle();

    /// Return a particle slot to the free-list
    void freeParticle(size_t index);

    /// Random float in range
    float randomRange(float min, float max);

    std::vector<EmitterInstance> m_emitters;
    std::vector<Particle> m_particles;
    std::vector<size_t> m_freeList;          // Indices of dead particles for O(1) allocation
    size_t m_maxParticles = 10000;
    EmitterId m_nextId = 1;
    std::mt19937 m_rng{std::random_device{}()};
    std::uniform_real_distribution<float> m_dist{0.0f, 1.0f};
};

} // namespace gloaming
