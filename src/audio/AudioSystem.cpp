#include "audio/AudioSystem.hpp"
#include "audio/SoundManager.hpp"
#include "audio/MusicManager.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "mod/EventBus.hpp"

#include <raylib.h>
#include <algorithm>

namespace gloaming {

AudioSystem::AudioSystem()
    : System("AudioSystem", 10),
      m_soundManager(std::make_unique<SoundManager>()),
      m_musicManager(std::make_unique<MusicManager>()),
      m_alive(std::make_shared<bool>(true)) {
}

AudioSystem::AudioSystem(const AudioConfig& config)
    : System("AudioSystem", 10), m_config(config),
      m_soundManager(std::make_unique<SoundManager>()),
      m_musicManager(std::make_unique<MusicManager>()),
      m_alive(std::make_shared<bool>(true)) {
}

AudioSystem::~AudioSystem() {
    *m_alive = false;
    if (m_deviceReady) {
        shutdown();
    }
}

void AudioSystem::init(Registry& registry, Engine& engine) {
    System::init(registry, engine);

    m_eventBus = &engine.getEventBus();

    if (!m_config.enabled) {
        LOG_INFO("AudioSystem: disabled by configuration");
        return;
    }

    initDevice();
}

void AudioSystem::initDevice() {
    InitAudioDevice();

    if (!IsAudioDeviceReady()) {
        LOG_WARN("AudioSystem: audio device not available (continuing without audio)");
        m_deviceReady = false;
        return;
    }

    m_deviceReady = true;

    // Apply master volume
    SetMasterVolume(m_config.masterVolume);

    // Initialize sub-managers
    m_soundManager->init(m_config.maxConcurrentSounds);
    m_soundManager->setSfxVolume(m_config.sfxVolume);

    m_musicManager->init();
    m_musicManager->setVolume(m_config.musicVolume);
    m_musicManager->setMinCrossfade(m_config.minCrossfade);

    LOG_INFO("AudioSystem: initialized (master={:.0f}% sfx={:.0f}% music={:.0f}% range={})",
             m_config.masterVolume * 100.0f, m_config.sfxVolume * 100.0f,
             m_config.musicVolume * 100.0f, m_config.positionalRange);
}

void AudioSystem::update(float dt) {
    if (!m_config.enabled || !m_deviceReady) return;

    m_time += dt;

    // Update listener position from camera
    Camera& camera = getEngine().getCamera();
    m_listenerPos = camera.getPosition();

    // Update sub-managers
    m_soundManager->update();
    m_musicManager->update(dt);
}

void AudioSystem::shutdown() {
    // Invalidate the alive flag so event callbacks become no-ops
    *m_alive = false;

    // Remove all event bindings
    for (auto& [name, binding] : m_eventBindings) {
        if (m_eventBus && binding.handlerId != 0) {
            m_eventBus->off(binding.handlerId);
        }
    }
    m_eventBindings.clear();

    if (m_soundManager) m_soundManager->shutdown();
    if (m_musicManager) m_musicManager->shutdown();

    if (m_deviceReady) {
        CloseAudioDevice();
        m_deviceReady = false;
    }

    LOG_INFO("AudioSystem: shut down");
}

// ============================================================
// Sound Effect API
// ============================================================

void AudioSystem::registerSound(const std::string& id, const std::string& filePath,
                                 float volume, float pitchVariance, float cooldown) {
    if (m_soundManager) {
        m_soundManager->registerSound(id, filePath, volume, pitchVariance, cooldown);
    }
}

SoundHandle AudioSystem::playSound(const std::string& id) {
    if (!m_deviceReady || !m_soundManager) return INVALID_SOUND_HANDLE;
    return m_soundManager->play(id, 1.0f, m_time);
}

SoundHandle AudioSystem::playSoundAt(const std::string& id, Vec2 position) {
    return playSoundAt(id, position.x, position.y);
}

SoundHandle AudioSystem::playSoundAt(const std::string& id, float x, float y) {
    if (!m_deviceReady || !m_soundManager) return INVALID_SOUND_HANDLE;

    float range = m_config.positionalRange;

    // Calculate distance-based volume attenuation
    float attenuation = SoundManager::calculateDistanceAttenuation(
        x, y, m_listenerPos.x, m_listenerPos.y, range);

    if (attenuation <= 0.0f) return INVALID_SOUND_HANDLE;  // Too far away

    // Calculate stereo pan
    float pan = SoundManager::calculatePan(x, m_listenerPos.x, range);

    // Get the sound definition for base volume and pitch variance
    const SoundDef* def = m_soundManager->getSoundDef(id);
    if (!def) return INVALID_SOUND_HANDLE;

    float volume = def->baseVolume * attenuation * m_config.sfxVolume;

    // Apply pitch variance using shared RNG
    float pitch = 1.0f;
    if (def->pitchVariance > 0.0f) {
        pitch = 1.0f + randomPitchOffset(def->pitchVariance);
    }

    return m_soundManager->playWithParams(id, volume, pitch, pan, m_time);
}

void AudioSystem::stopSound(SoundHandle handle) {
    if (m_soundManager) m_soundManager->stop(handle);
}

void AudioSystem::stopAllSounds() {
    if (m_soundManager) m_soundManager->stopAll();
}

// ============================================================
// Music API
// ============================================================

void AudioSystem::playMusic(const std::string& filePath, float fadeIn, bool loop) {
    if (!m_deviceReady || !m_musicManager) return;
    m_musicManager->play(filePath, fadeIn, loop);
}

void AudioSystem::stopMusic(float fadeOut) {
    if (!m_deviceReady || !m_musicManager) return;
    m_musicManager->stop(fadeOut);
}

void AudioSystem::setMusicPaused(bool paused) {
    if (m_musicManager) m_musicManager->setPaused(paused);
}

bool AudioSystem::isMusicPlaying() const {
    return m_musicManager && m_musicManager->isPlaying();
}

std::string AudioSystem::getCurrentMusic() const {
    if (m_musicManager) return m_musicManager->getCurrentTrack();
    return "";
}

// ============================================================
// Volume Control
// ============================================================

void AudioSystem::setMasterVolume(float volume) {
    m_config.masterVolume = std::max(0.0f, std::min(1.0f, volume));
    if (m_deviceReady) {
        SetMasterVolume(m_config.masterVolume);
    }
}

void AudioSystem::setSfxVolume(float volume) {
    m_config.sfxVolume = std::max(0.0f, std::min(1.0f, volume));
    if (m_soundManager) {
        m_soundManager->setSfxVolume(m_config.sfxVolume);
    }
}

void AudioSystem::setMusicVolume(float volume) {
    m_config.musicVolume = std::max(0.0f, std::min(1.0f, volume));
    if (m_musicManager) {
        m_musicManager->setVolume(m_config.musicVolume);
    }
}

void AudioSystem::setAmbientVolume(float volume) {
    m_config.ambientVolume = std::max(0.0f, std::min(1.0f, volume));
}

// ============================================================
// Listener
// ============================================================

void AudioSystem::setListenerPosition(Vec2 position) {
    m_listenerPos = position;
}

// ============================================================
// Event → Sound Bindings
// ============================================================

void AudioSystem::bindSoundToEvent(const std::string& eventName, const std::string& soundId) {
    if (!m_eventBus) return;

    // Remove existing binding for this event if any
    unbindEvent(eventName);

    // Subscribe to the event
    EventBinding binding;
    binding.eventName = eventName;
    binding.soundId = soundId;

    // Capture a weak_ptr<bool> guard alongside the raw pointer. The callback
    // checks the guard before dereferencing 'self', preventing use-after-free
    // if the AudioSystem is destroyed before the EventBus removes the handler.
    std::weak_ptr<bool> weak = m_alive;
    AudioSystem* self = this;
    binding.handlerId = m_eventBus->on(eventName,
        [weak, self, soundId](const EventData& data) -> bool {
            auto alive = weak.lock();
            if (!alive || !*alive) return false;

            if (data.hasFloat("x") && data.hasFloat("y")) {
                float x = data.getFloat("x");
                float y = data.getFloat("y");
                self->playSoundAt(soundId, x, y);
            } else {
                self->playSound(soundId);
            }
            return false;  // Don't cancel the event
        }, 100);  // Low priority so other handlers run first

    m_eventBindings[eventName] = std::move(binding);
    LOG_DEBUG("AudioSystem: bound sound '{}' to event '{}'", soundId, eventName);
}

void AudioSystem::unbindEvent(const std::string& eventName) {
    auto it = m_eventBindings.find(eventName);
    if (it != m_eventBindings.end()) {
        if (m_eventBus && it->second.handlerId != 0) {
            m_eventBus->off(it->second.handlerId);
        }
        m_eventBindings.erase(it);
    }
}

// ============================================================
// State / Statistics
// ============================================================

AudioStats AudioSystem::getStats() const {
    AudioStats stats;
    stats.registeredSounds = getRegisteredSoundCount();
    stats.activeSounds = getActiveSoundCount();
    stats.musicPlaying = isMusicPlaying();
    stats.currentMusic = getCurrentMusic();
    stats.deviceInitialized = m_deviceReady;
    return stats;
}

size_t AudioSystem::getRegisteredSoundCount() const {
    return m_soundManager ? m_soundManager->registeredCount() : 0;
}

size_t AudioSystem::getActiveSoundCount() const {
    return m_soundManager ? m_soundManager->activeCount() : 0;
}

} // namespace gloaming
