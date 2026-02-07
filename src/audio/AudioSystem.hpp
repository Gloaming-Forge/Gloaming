#pragma once

#include "audio/AudioTypes.hpp"
#include "ecs/Systems.hpp"
#include "rendering/IRenderer.hpp"  // For Vec2

#include <string>
#include <memory>
#include <unordered_map>

namespace gloaming {

// Forward declarations (avoid including raylib.h in this header)
class SoundManager;
class MusicManager;
class EventBus;

/// Configuration for the audio system
struct AudioConfig {
    bool enabled = true;
    float masterVolume = 1.0f;
    float sfxVolume = 0.8f;
    float musicVolume = 0.7f;
    float ambientVolume = 0.8f;
    int maxConcurrentSounds = 32;
    float positionalRange = 1000.0f;  // World units for max hearing distance
    float minCrossfade = 0.5f;        // Minimum crossfade duration in seconds (0 = allow instant)
};

/// Runtime statistics for the audio system
struct AudioStats {
    size_t registeredSounds = 0;
    size_t activeSounds = 0;
    bool musicPlaying = false;
    std::string currentMusic;
    bool deviceInitialized = false;
};

/// Main audio system for the Gloaming engine.
///
/// Coordinates sound effect playback (via SoundManager), music streaming with
/// crossfade (via MusicManager), volume channel management, positional audio,
/// and event→sound bindings for the mod API.
///
/// Uses Raylib's audio API (backed by miniaudio) for actual playback.
class AudioSystem : public System {
public:
    AudioSystem();
    explicit AudioSystem(const AudioConfig& config);
    ~AudioSystem() override;

    void init(Registry& registry, Engine& engine) override;
    void update(float dt) override;
    void shutdown() override;

    // ================================================================
    // Sound Effect API
    // ================================================================

    /// Register a sound effect definition.
    /// The sound file is lazy-loaded on first play.
    void registerSound(const std::string& id, const std::string& filePath,
                       float volume = 1.0f, float pitchVariance = 0.0f,
                       float cooldown = 0.0f);

    /// Play a sound effect (non-positional, centered).
    SoundHandle playSound(const std::string& id);

    /// Play a sound effect at a world position (positional audio).
    /// Volume and pan are calculated based on listener distance.
    SoundHandle playSoundAt(const std::string& id, Vec2 position);

    /// Play a sound effect at a world position (convenience overload).
    SoundHandle playSoundAt(const std::string& id, float x, float y);

    /// Stop a specific playing sound.
    void stopSound(SoundHandle handle);

    /// Stop all playing sounds.
    void stopAllSounds();

    // ================================================================
    // Music API
    // ================================================================

    /// Play a music track with optional fade-in. Crossfades if a track is already playing.
    void playMusic(const std::string& filePath, float fadeIn = 0.0f, bool loop = true);

    /// Stop current music with optional fade-out.
    void stopMusic(float fadeOut = 0.0f);

    /// Pause or resume music.
    void setMusicPaused(bool paused);

    /// Check if music is currently playing.
    bool isMusicPlaying() const;

    /// Get the current music track path.
    std::string getCurrentMusic() const;

    // ================================================================
    // Volume Control (channels)
    // ================================================================

    void setMasterVolume(float volume);
    void setSfxVolume(float volume);
    void setMusicVolume(float volume);
    void setAmbientVolume(float volume);

    float getMasterVolume() const { return m_config.masterVolume; }
    float getSfxVolume() const { return m_config.sfxVolume; }
    float getMusicVolume() const { return m_config.musicVolume; }
    float getAmbientVolume() const { return m_config.ambientVolume; }

    // ================================================================
    // Listener (for positional audio)
    // ================================================================

    /// Set the listener position (typically the camera or player position).
    void setListenerPosition(Vec2 position);
    Vec2 getListenerPosition() const { return m_listenerPos; }

    // ================================================================
    // Event → Sound Bindings
    // ================================================================

    /// Bind a registered sound to an engine event.
    /// When the event fires, the sound plays automatically.
    void bindSoundToEvent(const std::string& eventName, const std::string& soundId);

    /// Remove a sound binding from an event.
    void unbindEvent(const std::string& eventName);

    // ================================================================
    // State / Statistics
    // ================================================================

    bool isDeviceReady() const { return m_deviceReady; }
    const AudioConfig& getConfig() const { return m_config; }
    AudioStats getStats() const;
    size_t getRegisteredSoundCount() const;
    size_t getActiveSoundCount() const;

private:
    void initDevice();

    AudioConfig m_config;

    std::unique_ptr<SoundManager> m_soundManager;
    std::unique_ptr<MusicManager> m_musicManager;

    Vec2 m_listenerPos{0.0f, 0.0f};
    float m_time = 0.0f;
    bool m_deviceReady = false;

    // Event→Sound bindings
    struct EventBinding {
        std::string eventName;
        std::string soundId;
        uint64_t handlerId = 0;
    };
    std::unordered_map<std::string, EventBinding> m_eventBindings;
    EventBus* m_eventBus = nullptr;

    // Weak-reference guard for event callbacks. Shared with lambdas registered
    // on the EventBus so they can detect AudioSystem destruction and become
    // no-ops, preventing dangling-pointer dereferences.
    std::shared_ptr<bool> m_alive;
};

} // namespace gloaming
