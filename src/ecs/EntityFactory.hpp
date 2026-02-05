#pragma once

#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>
#include <functional>
#include <optional>

namespace gloaming {

// Forward declarations
class TextureManager;

/// Entity definition loaded from JSON
struct EntityDefinition {
    std::string type;
    std::string name;

    // Optional components (nullopt means not defined)
    std::optional<Vec2> size;
    std::optional<Color> color;
    std::optional<int> layer;

    // Sprite settings
    std::string texturePath;
    std::optional<Rect> sourceRect;
    std::optional<Vec2> origin;

    // Collider settings
    std::optional<Vec2> colliderOffset;
    std::optional<Vec2> colliderSize;
    std::optional<uint32_t> colliderLayer;
    std::optional<uint32_t> colliderMask;
    bool isTrigger = false;

    // Health settings
    std::optional<float> health;
    std::optional<float> maxHealth;
    std::optional<float> invincibilityDuration;

    // Light settings
    std::optional<Color> lightColor;
    std::optional<float> lightRadius;
    std::optional<float> lightIntensity;
    bool lightFlicker = false;

    // Physics
    std::optional<float> gravityScale;

    // Lifetime
    std::optional<float> lifetime;

    // Animation definitions
    struct AnimationDef {
        std::string name;
        std::vector<Rect> frames;
        float frameTime = 0.1f;
        bool looping = true;
    };
    std::vector<AnimationDef> animations;
    std::string defaultAnimation;
};

/// Factory for creating entities from definitions
class EntityFactory {
public:
    using SpawnCallback = std::function<void(Registry&, Entity, const EntityDefinition&)>;

    EntityFactory() = default;
    ~EntityFactory() = default;

    /// Set the texture manager for loading sprites
    void setTextureManager(TextureManager* texManager) {
        m_textureManager = texManager;
    }

    /// Register an entity definition
    void registerDefinition(const EntityDefinition& def) {
        m_definitions[def.type] = def;
    }

    /// Register a definition from JSON
    bool registerFromJson(const nlohmann::json& json);

    /// Load definitions from a JSON file
    bool loadFromFile(const std::string& path);

    /// Load definitions from a JSON string
    bool loadFromString(const std::string& jsonStr);

    /// Check if a definition exists
    bool hasDefinition(const std::string& type) const {
        return m_definitions.find(type) != m_definitions.end();
    }

    /// Get a definition by type
    const EntityDefinition* getDefinition(const std::string& type) const {
        auto it = m_definitions.find(type);
        return it != m_definitions.end() ? &it->second : nullptr;
    }

    /// Spawn an entity from a definition
    Entity spawn(Registry& registry, const std::string& type, Vec2 position);

    /// Spawn an entity from a definition with velocity
    Entity spawn(Registry& registry, const std::string& type, Vec2 position, Vec2 velocity);

    /// Spawn an entity with custom transform
    Entity spawn(Registry& registry, const std::string& type, const Transform& transform);

    /// Create a basic entity with transform only
    Entity createBasic(Registry& registry, Vec2 position) {
        return registry.create(Transform{position}, Name{"entity"});
    }

    /// Create a sprite entity
    Entity createSprite(Registry& registry, Vec2 position, const Texture* texture, int layer = 0);

    /// Register a custom spawn callback for a type
    void registerSpawnCallback(const std::string& type, SpawnCallback callback) {
        m_spawnCallbacks[type] = std::move(callback);
    }

    /// Get all registered definition types
    std::vector<std::string> getDefinitionTypes() const {
        std::vector<std::string> types;
        types.reserve(m_definitions.size());
        for (const auto& [type, def] : m_definitions) {
            types.push_back(type);
        }
        return types;
    }

    /// Clear all definitions
    void clear() {
        m_definitions.clear();
        m_spawnCallbacks.clear();
    }

private:
    /// Apply components from a definition to an entity
    void applyDefinition(Registry& registry, Entity entity, const EntityDefinition& def);

    /// Parse a color from JSON
    static Color parseColor(const nlohmann::json& json);

    /// Parse a Vec2 from JSON
    static Vec2 parseVec2(const nlohmann::json& json);

    /// Parse a Rect from JSON
    static Rect parseRect(const nlohmann::json& json);

    std::unordered_map<std::string, EntityDefinition> m_definitions;
    std::unordered_map<std::string, SpawnCallback> m_spawnCallbacks;
    TextureManager* m_textureManager = nullptr;
};

} // namespace gloaming
