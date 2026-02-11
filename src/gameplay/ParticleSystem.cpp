#include "gameplay/ParticleSystem.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"

#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace gloaming {

ParticleSystem::ParticleSystem()
    : System("ParticleSystem", 0) {
    m_particles.resize(1000); // Initial pool size
}

void ParticleSystem::init(Registry& registry, Engine& engine) {
    System::init(registry, engine);
    LOG_INFO("ParticleSystem initialized (pool size: {})", m_particles.size());
}

void ParticleSystem::update(float dt) {
    auto& registry = getRegistry();

    // Update emitter positions for entity-attached emitters
    for (auto& emitter : m_emitters) {
        if (!emitter.alive) continue;

        if (emitter.entity != NullEntity) {
            if (!registry.valid(emitter.entity)) {
                // Entity destroyed — stop emitting, let particles die
                emitter.active = false;
                emitter.entity = NullEntity;
                continue;
            }
            auto* transform = registry.tryGet<Transform>(emitter.entity);
            if (transform) {
                emitter.position = transform->position + emitter.config.offset;
            }
        }

        // Continuous emission
        if (emitter.active && emitter.config.count == 0 && emitter.config.rate > 0.0f) {
            emitter.emitAccumulator += emitter.config.rate * dt;
            int toEmit = static_cast<int>(emitter.emitAccumulator);
            if (toEmit > 0) {
                emitter.emitAccumulator -= static_cast<float>(toEmit);
                emitParticles(emitter, toEmit);
            }
        }
    }

    // Update all particles
    bool anyAlive = false;
    for (auto& particle : m_particles) {
        if (!particle.alive) continue;

        particle.age += dt;
        if (particle.age >= particle.lifetime) {
            particle.alive = false;
            continue;
        }

        anyAlive = true;

        // Apply gravity
        if (particle.gravity != 0.0f) {
            particle.velocity.y += particle.gravity * dt;
        }

        // Move
        particle.position += particle.velocity * dt;

        // Update size based on curve
        float t = particle.age / particle.lifetime;
        particle.size = particle.sizeCurve.evaluate(t);
    }

    // Clean up dead emitters (not active + no alive particles from them)
    // We track this by checking if the emitter is not active; once all particles
    // from it die naturally, it can be removed. For simplicity, we mark emitters
    // as !alive when they're stopped and have been around long enough.
    for (auto& emitter : m_emitters) {
        if (!emitter.alive) continue;
        if (!emitter.active) {
            // Check if enough time has passed for all particles to die
            // Use max lifetime from config as a conservative bound
            float maxLife = emitter.config.lifetime.max;
            emitter.emitAccumulator += dt; // reuse accumulator as death timer
            if (emitter.emitAccumulator >= maxLife + 0.1f) {
                emitter.alive = false;
            }
        }
    }

    // Compact dead emitters periodically
    m_emitters.erase(
        std::remove_if(m_emitters.begin(), m_emitters.end(),
                        [](const EmitterInstance& e) { return !e.alive; }),
        m_emitters.end());
}

void ParticleSystem::render(IRenderer* renderer, const Camera& camera) {
    if (!renderer) return;

    Rect visibleArea = camera.getVisibleArea();
    // Expand visible area slightly for particles near edges
    visibleArea.x -= 50.0f;
    visibleArea.y -= 50.0f;
    visibleArea.width += 100.0f;
    visibleArea.height += 100.0f;

    for (const auto& particle : m_particles) {
        if (!particle.alive) continue;

        // Culling: skip particles outside visible area
        if (!visibleArea.contains(particle.position)) continue;

        // Interpolate color
        float t = particle.age / particle.lifetime;
        ColorF currentColor = ColorF::lerp(particle.colorStart, particle.colorEnd, t);
        Color drawColor = currentColor.toColor();

        // Convert to screen space
        Vec2 screenPos = camera.worldToScreen(particle.position);
        float halfSize = particle.size * 0.5f * camera.getZoom();

        // Draw as filled rectangle (simple colored particle)
        Rect drawRect(
            screenPos.x - halfSize,
            screenPos.y - halfSize,
            particle.size * camera.getZoom(),
            particle.size * camera.getZoom()
        );
        renderer->drawRectangle(drawRect, drawColor);
    }
}

EmitterId ParticleSystem::burst(const ParticleEmitterConfig& config, Vec2 position) {
    EmitterInstance emitter;
    emitter.id = m_nextId++;
    emitter.config = config;
    emitter.position = position + config.offset;
    emitter.active = false; // Burst emitters don't continuously emit

    // Emit all particles immediately
    int count = config.count > 0 ? config.count : 8; // Default burst count
    emitParticles(emitter, count);

    // Store emitter so it tracks particle lifetimes for cleanup
    m_emitters.push_back(std::move(emitter));

    return m_emitters.back().id;
}

EmitterId ParticleSystem::attach(Entity entity, const ParticleEmitterConfig& config) {
    auto& registry = getRegistry();

    EmitterInstance emitter;
    emitter.id = m_nextId++;
    emitter.config = config;
    emitter.entity = entity;
    emitter.active = true;

    // Set initial position from entity
    auto* transform = registry.tryGet<Transform>(entity);
    if (transform) {
        emitter.position = transform->position + config.offset;
    }

    m_emitters.push_back(std::move(emitter));
    return m_emitters.back().id;
}

EmitterId ParticleSystem::spawnEmitter(const ParticleEmitterConfig& config, Vec2 position) {
    EmitterInstance emitter;
    emitter.id = m_nextId++;
    emitter.config = config;
    emitter.position = position + config.offset;
    emitter.active = true;

    m_emitters.push_back(std::move(emitter));
    return m_emitters.back().id;
}

void ParticleSystem::stopEmitter(EmitterId id) {
    for (auto& emitter : m_emitters) {
        if (emitter.id == id && emitter.alive) {
            emitter.active = false;
            emitter.emitAccumulator = 0.0f; // Reset for use as death timer
            return;
        }
    }
}

void ParticleSystem::destroyEmitter(EmitterId id) {
    for (auto& emitter : m_emitters) {
        if (emitter.id == id) {
            emitter.alive = false;
            emitter.active = false;
            return;
        }
    }
}

void ParticleSystem::setEmitterPosition(EmitterId id, Vec2 position) {
    for (auto& emitter : m_emitters) {
        if (emitter.id == id && emitter.alive) {
            emitter.position = position;
            return;
        }
    }
}

bool ParticleSystem::isAlive(EmitterId id) const {
    for (const auto& emitter : m_emitters) {
        if (emitter.id == id) return emitter.alive;
    }
    return false;
}

void ParticleSystem::removeEmittersForEntity(Entity entity) {
    for (auto& emitter : m_emitters) {
        if (emitter.entity == entity) {
            emitter.active = false;
            emitter.entity = NullEntity;
            emitter.emitAccumulator = 0.0f;
        }
    }
}

ParticleSystem::Stats ParticleSystem::getStats() const {
    Stats stats;
    for (const auto& e : m_emitters) {
        if (e.alive) ++stats.activeEmitters;
    }
    for (const auto& p : m_particles) {
        if (p.alive) ++stats.activeParticles;
    }
    stats.poolSize = m_particles.size();
    return stats;
}

void ParticleSystem::emitParticles(EmitterInstance& emitter, int count) {
    for (int i = 0; i < count; ++i) {
        Particle* p = allocateParticle();
        if (!p) return; // Pool full

        p->alive = true;
        p->age = 0.0f;

        // Position: emitter position + random spread for area emitters
        p->position = emitter.position;
        if (emitter.config.width > 0.0f) {
            p->position.x += randomRange(-emitter.config.width * 0.5f,
                                          emitter.config.width * 0.5f);
        }

        // Velocity from angle and speed
        float speed = randomRange(emitter.config.speed.min, emitter.config.speed.max);
        float angleDeg = randomRange(emitter.config.angle.min, emitter.config.angle.max);
        float angleRad = angleDeg * DEG_TO_RAD;
        p->velocity = Vec2(std::cos(angleRad) * speed, std::sin(angleRad) * speed);

        // Lifetime
        p->lifetime = randomRange(emitter.config.lifetime.min, emitter.config.lifetime.max);

        // Size curve
        p->sizeCurve = emitter.config.size;
        p->size = emitter.config.size.start;

        // Colors
        p->colorStart = emitter.config.colorStart;
        p->colorEnd = emitter.config.colorEnd;
        if (emitter.config.fade) {
            // Ensure end alpha fades to 0 if fade is enabled
            p->colorEnd.a = 0.0f;
        }

        // Gravity
        p->gravity = emitter.config.gravity;
    }
}

Particle* ParticleSystem::allocateParticle() {
    // Find a dead particle to reuse
    for (auto& p : m_particles) {
        if (!p.alive) return &p;
    }

    // Pool is full — grow if under limit
    if (m_particles.size() < m_maxParticles) {
        size_t newSize = std::min(m_particles.size() * 2, m_maxParticles);
        size_t oldSize = m_particles.size();
        m_particles.resize(newSize);
        return &m_particles[oldSize]; // Return first new slot
    }

    return nullptr; // At capacity
}

float ParticleSystem::randomRange(float min, float max) {
    if (min >= max) return min;
    float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    return min + t * (max - min);
}

} // namespace gloaming
