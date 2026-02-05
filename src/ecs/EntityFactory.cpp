#include "ecs/EntityFactory.hpp"
#include "rendering/Texture.hpp"
#include "engine/Log.hpp"

#include <fstream>
#include <sstream>

namespace gloaming {

bool EntityFactory::registerFromJson(const nlohmann::json& json) {
    try {
        EntityDefinition def;

        // Required fields
        if (!json.contains("type")) {
            LOG_ERROR("Entity definition missing 'type' field");
            return false;
        }
        def.type = json["type"].get<std::string>();
        def.name = json.value("name", def.type);

        // Size
        if (json.contains("size")) {
            def.size = parseVec2(json["size"]);
        }

        // Color/tint
        if (json.contains("color") || json.contains("tint")) {
            def.color = parseColor(json.contains("color") ? json["color"] : json["tint"]);
        }

        // Layer
        if (json.contains("layer")) {
            def.layer = json["layer"].get<int>();
        }

        // Sprite settings
        def.texturePath = json.value("texture", "");
        if (json.contains("sourceRect") || json.contains("source_rect")) {
            const auto& srcJson = json.contains("sourceRect") ? json["sourceRect"] : json["source_rect"];
            def.sourceRect = parseRect(srcJson);
        }
        if (json.contains("origin") || json.contains("pivot")) {
            const auto& origJson = json.contains("origin") ? json["origin"] : json["pivot"];
            def.origin = parseVec2(origJson);
        }

        // Collider settings
        if (json.contains("collider")) {
            const auto& col = json["collider"];
            if (col.contains("offset")) {
                def.colliderOffset = parseVec2(col["offset"]);
            }
            if (col.contains("size")) {
                def.colliderSize = parseVec2(col["size"]);
            }
            if (col.contains("layer")) {
                def.colliderLayer = col["layer"].get<uint32_t>();
            }
            if (col.contains("mask")) {
                def.colliderMask = col["mask"].get<uint32_t>();
            }
            def.isTrigger = col.value("trigger", false);
        }

        // Health settings
        if (json.contains("health")) {
            const auto& hp = json["health"];
            if (hp.is_number()) {
                def.health = hp.get<float>();
                def.maxHealth = hp.get<float>();
            } else if (hp.is_object()) {
                def.health = hp.value("current", 100.0f);
                def.maxHealth = hp.value("max", def.health.value_or(100.0f));
                if (hp.contains("invincibility") || hp.contains("invincibility_duration")) {
                    def.invincibilityDuration = hp.contains("invincibility")
                        ? hp["invincibility"].get<float>()
                        : hp["invincibility_duration"].get<float>();
                }
            }
        }

        // Light settings
        if (json.contains("light")) {
            const auto& light = json["light"];
            if (light.contains("color")) {
                def.lightColor = parseColor(light["color"]);
            }
            if (light.contains("radius")) {
                def.lightRadius = light["radius"].get<float>();
            }
            if (light.contains("intensity")) {
                def.lightIntensity = light["intensity"].get<float>();
            }
            def.lightFlicker = light.value("flicker", false);
        }

        // Physics
        if (json.contains("gravity") || json.contains("gravity_scale")) {
            const auto& grav = json.contains("gravity") ? json["gravity"] : json["gravity_scale"];
            if (grav.is_number()) {
                def.gravityScale = grav.get<float>();
            } else if (grav.is_boolean()) {
                def.gravityScale = grav.get<bool>() ? 1.0f : 0.0f;
            }
        }

        // Lifetime
        if (json.contains("lifetime")) {
            def.lifetime = json["lifetime"].get<float>();
        }

        // Animations
        if (json.contains("animations")) {
            for (const auto& animJson : json["animations"]) {
                EntityDefinition::AnimationDef animDef;
                animDef.name = animJson.value("name", "default");
                animDef.frameTime = animJson.value("frame_time", animJson.value("frameTime", 0.1f));
                animDef.looping = animJson.value("loop", animJson.value("looping", true));

                if (animJson.contains("frames")) {
                    for (const auto& frameJson : animJson["frames"]) {
                        animDef.frames.push_back(parseRect(frameJson));
                    }
                }

                def.animations.push_back(std::move(animDef));
            }
        }
        def.defaultAnimation = json.value("default_animation", json.value("defaultAnimation", ""));

        registerDefinition(def);
        LOG_DEBUG("Registered entity definition: {}", def.type);
        return true;

    } catch (const nlohmann::json::exception& e) {
        LOG_ERROR("Failed to parse entity definition JSON: {}", e.what());
        return false;
    }
}

bool EntityFactory::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open entity definitions file: {}", path);
        return false;
    }

    try {
        nlohmann::json json;
        file >> json;

        // Handle array of definitions or single definition
        if (json.is_array()) {
            for (const auto& defJson : json) {
                if (!registerFromJson(defJson)) {
                    LOG_WARN("Failed to register entity definition from file: {}", path);
                }
            }
        } else if (json.contains("entities")) {
            for (const auto& defJson : json["entities"]) {
                if (!registerFromJson(defJson)) {
                    LOG_WARN("Failed to register entity definition from file: {}", path);
                }
            }
        } else {
            return registerFromJson(json);
        }

        LOG_INFO("Loaded entity definitions from: {}", path);
        return true;

    } catch (const nlohmann::json::exception& e) {
        LOG_ERROR("Failed to parse entity definitions file '{}': {}", path, e.what());
        return false;
    }
}

bool EntityFactory::loadFromString(const std::string& jsonStr) {
    try {
        nlohmann::json json = nlohmann::json::parse(jsonStr);

        if (json.is_array()) {
            for (const auto& defJson : json) {
                if (!registerFromJson(defJson)) {
                    return false;
                }
            }
        } else if (json.contains("entities")) {
            for (const auto& defJson : json["entities"]) {
                if (!registerFromJson(defJson)) {
                    return false;
                }
            }
        } else {
            return registerFromJson(json);
        }
        return true;

    } catch (const nlohmann::json::exception& e) {
        LOG_ERROR("Failed to parse entity definitions JSON string: {}", e.what());
        return false;
    }
}

Entity EntityFactory::spawn(Registry& registry, const std::string& type, Vec2 position) {
    return spawn(registry, type, Transform{position});
}

Entity EntityFactory::spawn(Registry& registry, const std::string& type, Vec2 position, Vec2 velocity) {
    Entity entity = spawn(registry, type, Transform{position});
    if (entity != NullEntity) {
        registry.add<Velocity>(entity, velocity);
    }
    return entity;
}

Entity EntityFactory::spawn(Registry& registry, const std::string& type, const Transform& transform) {
    auto it = m_definitions.find(type);
    if (it == m_definitions.end()) {
        LOG_WARN("Unknown entity type: {}", type);
        return NullEntity;
    }

    Entity entity = registry.create();
    registry.add<Transform>(entity, transform);
    registry.add<Name>(entity, it->second.name, type);

    applyDefinition(registry, entity, it->second);

    // Call custom spawn callback if registered
    auto callbackIt = m_spawnCallbacks.find(type);
    if (callbackIt != m_spawnCallbacks.end()) {
        callbackIt->second(registry, entity, it->second);
    }

    return entity;
}

Entity EntityFactory::createSprite(Registry& registry, Vec2 position, const Texture* texture, int layer) {
    Entity entity = registry.create();
    registry.add<Transform>(entity, Transform{position});
    registry.add<Name>(entity, "sprite");

    Sprite sprite(texture);
    sprite.layer = layer;
    registry.add<Sprite>(entity, std::move(sprite));

    return entity;
}

void EntityFactory::applyDefinition(Registry& registry, Entity entity, const EntityDefinition& def) {
    // Sprite component
    if (!def.texturePath.empty() && m_textureManager) {
        const Texture* texture = m_textureManager->loadTexture(def.texturePath);
        if (texture) {
            Sprite sprite(texture);

            if (def.sourceRect) {
                sprite.sourceRect = *def.sourceRect;
            }
            if (def.origin) {
                sprite.origin = *def.origin;
            }
            if (def.color) {
                sprite.tint = *def.color;
            }
            if (def.layer) {
                sprite.layer = *def.layer;
            }

            // Add animations
            for (const auto& animDef : def.animations) {
                std::vector<AnimationFrame> frames;
                for (const auto& rect : animDef.frames) {
                    AnimationFrame frame;
                    frame.sourceRect = rect;
                    frame.duration = animDef.frameTime;
                    frames.push_back(frame);
                }
                sprite.addAnimation(animDef.name, std::move(frames), animDef.looping);
            }

            // Start default animation
            if (!def.defaultAnimation.empty()) {
                sprite.playAnimation(def.defaultAnimation);
            }

            registry.add<Sprite>(entity, std::move(sprite));
        }
    } else if (def.layer || def.color) {
        // Create sprite even without texture (placeholder)
        Sprite sprite;
        if (def.color) sprite.tint = *def.color;
        if (def.layer) sprite.layer = *def.layer;
        registry.add<Sprite>(entity, std::move(sprite));
    }

    // Collider component
    if (def.colliderSize) {
        Collider collider;
        collider.size = *def.colliderSize;
        if (def.colliderOffset) collider.offset = *def.colliderOffset;
        if (def.colliderLayer) collider.layer = *def.colliderLayer;
        if (def.colliderMask) collider.mask = *def.colliderMask;
        collider.isTrigger = def.isTrigger;
        registry.add<Collider>(entity, collider);
    }

    // Health component
    if (def.health || def.maxHealth) {
        Health health;
        health.max = def.maxHealth.value_or(100.0f);
        health.current = def.health.value_or(health.max);
        if (def.invincibilityDuration) {
            health.invincibilityDuration = *def.invincibilityDuration;
        }
        registry.add<Health>(entity, health);
    }

    // Light source component
    if (def.lightRadius) {
        LightSource light;
        if (def.lightColor) light.color = *def.lightColor;
        light.radius = *def.lightRadius;
        if (def.lightIntensity) light.intensity = *def.lightIntensity;
        light.flicker = def.lightFlicker;
        registry.add<LightSource>(entity, light);
    }

    // Gravity component
    if (def.gravityScale) {
        registry.add<Gravity>(entity, *def.gravityScale);
    }

    // Lifetime component
    if (def.lifetime) {
        registry.add<Lifetime>(entity, *def.lifetime);
    }
}

Color EntityFactory::parseColor(const nlohmann::json& json) {
    if (json.is_array()) {
        uint8_t r = json.size() > 0 ? json[0].get<uint8_t>() : 255;
        uint8_t g = json.size() > 1 ? json[1].get<uint8_t>() : 255;
        uint8_t b = json.size() > 2 ? json[2].get<uint8_t>() : 255;
        uint8_t a = json.size() > 3 ? json[3].get<uint8_t>() : 255;
        return Color(r, g, b, a);
    } else if (json.is_object()) {
        return Color(
            json.value("r", 255),
            json.value("g", 255),
            json.value("b", 255),
            json.value("a", 255)
        );
    } else if (json.is_string()) {
        std::string str = json.get<std::string>();
        // Parse hex color like "#RRGGBB" or "#RRGGBBAA"
        if (str.length() >= 7 && str[0] == '#') {
            unsigned int hex = std::stoul(str.substr(1), nullptr, 16);
            if (str.length() == 7) {
                return Color((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, 255);
            } else if (str.length() == 9) {
                return Color((hex >> 24) & 0xFF, (hex >> 16) & 0xFF,
                            (hex >> 8) & 0xFF, hex & 0xFF);
            }
        }
    }
    return Color::White();
}

Vec2 EntityFactory::parseVec2(const nlohmann::json& json) {
    if (json.is_array() && json.size() >= 2) {
        return Vec2(json[0].get<float>(), json[1].get<float>());
    } else if (json.is_object()) {
        return Vec2(json.value("x", 0.0f), json.value("y", 0.0f));
    }
    return Vec2();
}

Rect EntityFactory::parseRect(const nlohmann::json& json) {
    if (json.is_array() && json.size() >= 4) {
        return Rect(json[0].get<float>(), json[1].get<float>(),
                   json[2].get<float>(), json[3].get<float>());
    } else if (json.is_object()) {
        return Rect(
            json.value("x", 0.0f),
            json.value("y", 0.0f),
            json.value("width", json.value("w", 0.0f)),
            json.value("height", json.value("h", 0.0f))
        );
    }
    return Rect();
}

} // namespace gloaming
