#include "audio/SoundManager.hpp"
#include "engine/Log.hpp"

#include <cmath>
#include <algorithm>
#include <random>

namespace gloaming {

SoundManager::~SoundManager() {
    if (m_initialized) {
        shutdown();
    }
}

bool SoundManager::init(int maxConcurrentSounds) {
    m_maxConcurrentSounds = maxConcurrentSounds;
    m_initialized = true;
    LOG_DEBUG("SoundManager: initialized (max concurrent: {})", maxConcurrentSounds);
    return true;
}

void SoundManager::shutdown() {
    // Stop and unload all active sound instances
    for (auto& active : m_activeSounds) {
        if (active.isAlias) {
            StopSound(active.aliasSound);
            UnloadSoundAlias(active.aliasSound);
        }
    }
    m_activeSounds.clear();

    // Unload all base sound data
    for (auto& [id, loaded] : m_loadedSounds) {
        if (loaded.loaded) {
            UnloadSound(loaded.raylibSound);
            loaded.loaded = false;
        }
    }
    m_loadedSounds.clear();
    m_definitions.clear();

    m_initialized = false;
    LOG_DEBUG("SoundManager: shut down");
}

void SoundManager::registerSound(const std::string& id, const std::string& filePath,
                                  float volume, float pitchVariance, float cooldown) {
    SoundDef def;
    def.id = id;
    def.filePath = filePath;
    def.baseVolume = std::max(0.0f, std::min(1.0f, volume));
    def.pitchVariance = std::max(0.0f, pitchVariance);
    def.cooldown = std::max(0.0f, cooldown);
    m_definitions[id] = std::move(def);

    LOG_DEBUG("SoundManager: registered sound '{}' -> '{}'", id, filePath);
}

bool SoundManager::ensureLoaded(const std::string& id) {
    auto loadIt = m_loadedSounds.find(id);
    if (loadIt != m_loadedSounds.end() && loadIt->second.loaded) {
        return true;  // Already loaded
    }

    auto defIt = m_definitions.find(id);
    if (defIt == m_definitions.end()) {
        return false;  // No definition
    }

    LoadedSound loaded;
    loaded.raylibSound = LoadSound(defIt->second.filePath.c_str());
    if (loaded.raylibSound.frameCount == 0) {
        LOG_WARN("SoundManager: failed to load sound file '{}'", defIt->second.filePath);
        return false;
    }

    loaded.loaded = true;
    m_loadedSounds[id] = loaded;
    LOG_DEBUG("SoundManager: loaded audio data for '{}'", id);
    return true;
}

SoundHandle SoundManager::play(const std::string& id, float volumeMultiplier,
                                float currentTime) {
    auto defIt = m_definitions.find(id);
    if (defIt == m_definitions.end()) {
        LOG_WARN("SoundManager: cannot play unregistered sound '{}'", id);
        return INVALID_SOUND_HANDLE;
    }

    const SoundDef& def = defIt->second;

    // Apply pitch variance
    float pitch = 1.0f;
    if (def.pitchVariance > 0.0f) {
        static thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> dist(-def.pitchVariance, def.pitchVariance);
        pitch = 1.0f + dist(rng);
    }

    float volume = def.baseVolume * volumeMultiplier * m_sfxVolume;
    return playWithParams(id, volume, pitch, 0.5f, currentTime);
}

SoundHandle SoundManager::playWithParams(const std::string& id, float volume, float pitch,
                                          float pan, float currentTime) {
    auto defIt = m_definitions.find(id);
    if (defIt == m_definitions.end()) {
        return INVALID_SOUND_HANDLE;
    }

    SoundDef& def = defIt->second;

    // Check cooldown
    if (def.cooldown > 0.0f && (currentTime - def.lastPlayTime) < def.cooldown) {
        return INVALID_SOUND_HANDLE;
    }

    // Enforce max concurrent sounds
    if (static_cast<int>(m_activeSounds.size()) >= m_maxConcurrentSounds) {
        LOG_TRACE("SoundManager: max concurrent sounds reached, skipping '{}'", id);
        return INVALID_SOUND_HANDLE;
    }

    // Ensure the sound data is loaded
    if (!ensureLoaded(id)) {
        return INVALID_SOUND_HANDLE;
    }

    auto& loaded = m_loadedSounds[id];

    // Create an alias for concurrent playback
    ActiveSoundInstance instance;
    instance.handle = m_nextHandle++;
    instance.defId = id;
    instance.aliasSound = LoadSoundAlias(loaded.raylibSound);
    instance.isAlias = true;

    // Apply parameters
    SetSoundVolume(instance.aliasSound, std::max(0.0f, std::min(1.0f, volume)));
    SetSoundPitch(instance.aliasSound, std::max(0.1f, std::min(3.0f, pitch)));
    SetSoundPan(instance.aliasSound, std::max(0.0f, std::min(1.0f, pan)));

    // Play
    PlaySound(instance.aliasSound);

    def.lastPlayTime = currentTime;

    m_activeSounds.push_back(std::move(instance));
    return instance.handle;
}

void SoundManager::stop(SoundHandle handle) {
    for (auto it = m_activeSounds.begin(); it != m_activeSounds.end(); ++it) {
        if (it->handle == handle) {
            if (it->isAlias) {
                StopSound(it->aliasSound);
                UnloadSoundAlias(it->aliasSound);
            }
            m_activeSounds.erase(it);
            return;
        }
    }
}

void SoundManager::stopAll() {
    for (auto& active : m_activeSounds) {
        if (active.isAlias) {
            StopSound(active.aliasSound);
            UnloadSoundAlias(active.aliasSound);
        }
    }
    m_activeSounds.clear();
}

void SoundManager::update() {
    // Remove finished sound instances
    auto it = std::remove_if(m_activeSounds.begin(), m_activeSounds.end(),
        [](ActiveSoundInstance& instance) {
            if (!IsSoundPlaying(instance.aliasSound)) {
                if (instance.isAlias) {
                    UnloadSoundAlias(instance.aliasSound);
                }
                return true;
            }
            return false;
        });
    m_activeSounds.erase(it, m_activeSounds.end());
}

bool SoundManager::hasSound(const std::string& id) const {
    return m_definitions.count(id) > 0;
}

const SoundDef* SoundManager::getSoundDef(const std::string& id) const {
    auto it = m_definitions.find(id);
    return it != m_definitions.end() ? &it->second : nullptr;
}

float SoundManager::calculateDistanceAttenuation(float sourceX, float sourceY,
                                                   float listenerX, float listenerY,
                                                   float maxRange) {
    if (maxRange <= 0.0f) return 0.0f;

    float dx = sourceX - listenerX;
    float dy = sourceY - listenerY;
    float distance = std::sqrt(dx * dx + dy * dy);

    if (distance >= maxRange) return 0.0f;
    if (distance <= 0.0f) return 1.0f;

    // Inverse distance attenuation with smooth falloff
    float normalized = distance / maxRange;
    return 1.0f - (normalized * normalized);
}

float SoundManager::calculatePan(float sourceX, float listenerX, float maxRange) {
    if (maxRange <= 0.0f) return 0.5f;

    float offset = sourceX - listenerX;
    // Map horizontal offset to pan (0.0=left, 0.5=center, 1.0=right)
    float normalizedOffset = offset / maxRange;
    // Clamp to [-1, 1] then map to [0, 1]
    normalizedOffset = std::max(-1.0f, std::min(1.0f, normalizedOffset));
    return 0.5f + normalizedOffset * 0.5f;
}

} // namespace gloaming
