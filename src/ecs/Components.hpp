#pragma once

#include "rendering/IRenderer.hpp"
#include "rendering/Texture.hpp"

#include <string>
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <functional>

namespace gloaming {

// Forward declarations
class Entity;

/// Transform component - position, rotation, scale in world space
struct Transform {
    Vec2 position{0.0f, 0.0f};
    float rotation = 0.0f;      // Degrees
    Vec2 scale{1.0f, 1.0f};

    constexpr Transform() = default;
    constexpr Transform(Vec2 pos) : position(pos) {}
    constexpr Transform(Vec2 pos, float rot) : position(pos), rotation(rot) {}
    constexpr Transform(Vec2 pos, float rot, Vec2 scl)
        : position(pos), rotation(rot), scale(scl) {}
};

/// Velocity component - movement vector and angular velocity
struct Velocity {
    Vec2 linear{0.0f, 0.0f};    // Pixels per second
    float angular = 0.0f;        // Degrees per second

    constexpr Velocity() = default;
    constexpr Velocity(Vec2 vel) : linear(vel) {}
    constexpr Velocity(Vec2 vel, float angVel) : linear(vel), angular(angVel) {}
    constexpr Velocity(float x, float y) : linear(x, y) {}
};

/// Animation frame data
struct AnimationFrame {
    Rect sourceRect;             // Region in texture atlas
    float duration = 0.1f;       // Seconds per frame
};

/// Animation state
struct Animation {
    std::string name;
    std::vector<AnimationFrame> frames;
    bool looping = true;
};

/// Sprite component - visual representation
struct Sprite {
    const Texture* texture = nullptr;
    Rect sourceRect{0, 0, 0, 0};        // Region in texture (0,0,0,0 = entire texture)
    Vec2 origin{0.5f, 0.5f};            // Pivot point (0-1, 0.5 = center)
    Color tint = Color::White();
    int layer = 0;                       // Render order (higher = on top)
    bool visible = true;
    bool flipX = false;
    bool flipY = false;

    // Animation state
    std::vector<Animation> animations;
    int currentAnimation = -1;           // -1 = no animation
    int currentFrame = 0;
    float frameTimer = 0.0f;
    bool animationFinished = false;

    Sprite() = default;
    explicit Sprite(const Texture* tex) : texture(tex) {
        if (tex) {
            sourceRect = Rect(0, 0, static_cast<float>(tex->getWidth()),
                                   static_cast<float>(tex->getHeight()));
        }
    }
    Sprite(const Texture* tex, const Rect& src) : texture(tex), sourceRect(src) {}
    Sprite(const Texture* tex, const Rect& src, int renderLayer)
        : texture(tex), sourceRect(src), layer(renderLayer) {}

    /// Add an animation
    void addAnimation(const std::string& name, std::vector<AnimationFrame> frames, bool loop = true) {
        animations.push_back({name, std::move(frames), loop});
    }

    /// Play an animation by name
    bool playAnimation(const std::string& name) {
        for (size_t i = 0; i < animations.size(); ++i) {
            if (animations[i].name == name) {
                if (static_cast<int>(i) != currentAnimation) {
                    currentAnimation = static_cast<int>(i);
                    currentFrame = 0;
                    frameTimer = 0.0f;
                    animationFinished = false;
                }
                return true;
            }
        }
        return false;
    }

    /// Get current animation name (empty if none)
    const std::string& getCurrentAnimationName() const {
        static const std::string empty;
        if (currentAnimation >= 0 && currentAnimation < static_cast<int>(animations.size())) {
            return animations[currentAnimation].name;
        }
        return empty;
    }
};

/// Collision layer flags (bitmask)
namespace CollisionLayer {
    constexpr uint32_t None       = 0;
    constexpr uint32_t Default    = 1 << 0;
    constexpr uint32_t Player     = 1 << 1;
    constexpr uint32_t Enemy      = 1 << 2;
    constexpr uint32_t Projectile = 1 << 3;
    constexpr uint32_t Tile       = 1 << 4;
    constexpr uint32_t Trigger    = 1 << 5;
    constexpr uint32_t Item       = 1 << 6;
    constexpr uint32_t NPC        = 1 << 7;
    constexpr uint32_t All        = 0xFFFFFFFF;
}

/// Collider component - axis-aligned bounding box
struct Collider {
    Vec2 offset{0.0f, 0.0f};     // Offset from transform position
    Vec2 size{16.0f, 16.0f};     // Width and height
    uint32_t layer = CollisionLayer::Default;   // What layer this collider is on
    uint32_t mask = CollisionLayer::All;        // What layers to collide with
    bool isTrigger = false;      // If true, doesn't block movement, just detects overlap
    bool enabled = true;

    constexpr Collider() = default;
    constexpr Collider(Vec2 sz) : size(sz) {}
    constexpr Collider(Vec2 off, Vec2 sz) : offset(off), size(sz) {}
    constexpr Collider(Vec2 off, Vec2 sz, uint32_t lay, uint32_t msk)
        : offset(off), size(sz), layer(lay), mask(msk) {}

    /// Get the world-space bounding box given a transform
    Rect getBounds(const Transform& transform) const {
        return Rect(
            transform.position.x + offset.x - size.x * 0.5f,
            transform.position.y + offset.y - size.y * 0.5f,
            size.x * transform.scale.x,
            size.y * transform.scale.y
        );
    }

    /// Check if this collider can interact with another based on layers
    bool canCollideWith(const Collider& other) const {
        return enabled && other.enabled &&
               (layer & other.mask) != 0 &&
               (other.layer & mask) != 0;
    }
};

/// Health component - for damageable entities
struct Health {
    float current = 100.0f;
    float max = 100.0f;
    float invincibilityTime = 0.0f;      // Remaining invincibility seconds
    float invincibilityDuration = 0.5f;  // How long to be invincible after taking damage

    constexpr Health() = default;
    constexpr Health(float hp) : current(hp), max(hp) {}
    constexpr Health(float hp, float maxHp) : current(hp), max(maxHp) {}

    /// Take damage, returns actual damage dealt (0 if invincible)
    float takeDamage(float amount) {
        if (invincibilityTime > 0.0f || current <= 0.0f) {
            return 0.0f;
        }
        float actualDamage = std::min(amount, current);
        current -= actualDamage;
        if (actualDamage > 0.0f) {
            invincibilityTime = invincibilityDuration;
        }
        return actualDamage;
    }

    /// Heal, returns actual health restored
    float heal(float amount) {
        float actualHeal = std::min(amount, max - current);
        current += actualHeal;
        return actualHeal;
    }

    /// Check if dead
    bool isDead() const { return current <= 0.0f; }

    /// Check if invincible
    bool isInvincible() const { return invincibilityTime > 0.0f; }

    /// Get health percentage (0-1)
    float getPercentage() const { return max > 0.0f ? current / max : 0.0f; }

    /// Update invincibility timer
    void update(float dt) {
        if (invincibilityTime > 0.0f) {
            invincibilityTime -= dt;
            if (invincibilityTime < 0.0f) {
                invincibilityTime = 0.0f;
            }
        }
    }
};

/// Light source component - for entities that emit light
struct LightSource {
    Color color = Color::White();
    float radius = 100.0f;           // Light radius in pixels
    float intensity = 1.0f;          // 0-1 brightness
    Vec2 offset{0.0f, 0.0f};         // Offset from entity position
    bool enabled = true;

    // Flicker settings
    bool flicker = false;
    float flickerSpeed = 10.0f;      // Flicker frequency
    float flickerAmount = 0.1f;      // Intensity variation (0-1)
    float flickerPhase = 0.0f;       // Internal state

    constexpr LightSource() = default;
    constexpr LightSource(Color col, float rad) : color(col), radius(rad) {}
    constexpr LightSource(Color col, float rad, float intens)
        : color(col), radius(rad), intensity(intens) {}

    /// Get effective intensity (accounts for flicker)
    float getEffectiveIntensity() const {
        if (!flicker) return intensity;
        // flickerPhase oscillates, creating variation
        float variation = std::sin(flickerPhase) * flickerAmount;
        return std::max(0.0f, std::min(1.0f, intensity + variation));
    }

    /// Update flicker
    void update(float dt) {
        if (flicker) {
            flickerPhase += flickerSpeed * dt;
            if (flickerPhase > PI * 2.0f) {
                flickerPhase -= PI * 2.0f;
            }
        }
    }
};

/// Particle emitter type reference
struct ParticleEmitter {
    std::string emitterType;         // References registered particle system
    Vec2 offset{0.0f, 0.0f};         // Offset from entity position
    bool enabled = true;
    bool autoStart = true;

    ParticleEmitter() = default;
    explicit ParticleEmitter(const std::string& type) : emitterType(type) {}
};

/// Trigger callback type
using TriggerCallback = std::function<void(uint32_t thisEntity, uint32_t otherEntity)>;

/// Trigger component - calls callbacks when entities enter/exit
struct Trigger {
    TriggerCallback onEnter;
    TriggerCallback onStay;
    TriggerCallback onExit;

    Trigger() = default;
    explicit Trigger(TriggerCallback enter) : onEnter(std::move(enter)) {}
    Trigger(TriggerCallback enter, TriggerCallback exit)
        : onEnter(std::move(enter)), onExit(std::move(exit)) {}
};

/// Network sync settings (future multiplayer support)
struct NetworkSync {
    bool syncPosition = true;
    bool syncRotation = true;
    bool syncVelocity = true;
    float interpolationDelay = 0.1f;    // Seconds of interpolation buffer
    uint32_t ownerClientId = 0;         // 0 = server owned

    constexpr NetworkSync() = default;
};

/// Tag component for player entities
struct PlayerTag {
    int playerIndex = 0;                 // For local multiplayer
};

/// Tag component for enemy entities
struct EnemyTag {
    std::string enemyType;
};

/// Tag component for NPC entities
struct NPCTag {
    std::string npcType;
};

/// Tag component for projectile entities
struct ProjectileTag {
    std::string projectileType;
    uint32_t ownerEntity = 0;            // Entity that fired this projectile
    float damage = 10.0f;
    float lifetime = 5.0f;               // Seconds until despawn
    float age = 0.0f;                    // Current age
    bool piercing = false;               // Can hit multiple enemies
};

/// Gravity-affected entity
struct Gravity {
    float scale = 1.0f;                  // Multiplier for world gravity
    bool grounded = false;               // Is touching ground

    constexpr Gravity() = default;
    constexpr explicit Gravity(float gravScale) : scale(gravScale) {}
};

/// Name/ID for debugging and lookup
struct Name {
    std::string name;
    std::string type;                    // Entity type for factory lookup

    Name() = default;
    explicit Name(const std::string& n) : name(n) {}
    Name(const std::string& n, const std::string& t) : name(n), type(t) {}
};

/// Lifetime component - entity despawns after duration
struct Lifetime {
    float duration = 5.0f;
    float elapsed = 0.0f;

    constexpr Lifetime() = default;
    constexpr explicit Lifetime(float dur) : duration(dur) {}

    bool isExpired() const { return elapsed >= duration; }
    float getRemaining() const { return std::max(0.0f, duration - elapsed); }
    float getProgress() const { return duration > 0.0f ? elapsed / duration : 1.0f; }
};

} // namespace gloaming
