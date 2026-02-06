#pragma once

#include <raylib.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace gloaming {

/// Sound handle for tracking playing sounds
using SoundHandle = uint32_t;
constexpr SoundHandle INVALID_SOUND_HANDLE = 0;

/// Definition of a registered sound effect
struct SoundDef {
    std::string id;
    std::string filePath;
    float baseVolume = 1.0f;
    float pitchVariance = 0.0f;
    float cooldown = 0.0f;         // Min seconds between plays of this sound
    float lastPlayTime = -1000.0f;  // Timestamp of last play (allows immediate first play)
};

/// Manages loading, caching, and playback of sound effects.
/// Uses Raylib's audio API (backed by miniaudio) for actual playback.
class SoundManager {
public:
    SoundManager() = default;
    ~SoundManager();

    // Non-copyable
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    /// Initialize the sound manager
    bool init(int maxConcurrentSounds = 32);

    /// Shutdown and release all resources
    void shutdown();

    /// Register a sound definition (stores metadata, lazy-loads audio data)
    void registerSound(const std::string& id, const std::string& filePath,
                       float volume = 1.0f, float pitchVariance = 0.0f,
                       float cooldown = 0.0f);

    /// Play a registered sound. Returns a handle for the playing instance.
    SoundHandle play(const std::string& id, float volumeMultiplier = 1.0f,
                     float currentTime = 0.0f);

    /// Play a registered sound with positional audio parameters already computed.
    SoundHandle playWithParams(const std::string& id, float volume, float pitch,
                               float pan, float currentTime = 0.0f);

    /// Stop a specific sound instance
    void stop(SoundHandle handle);

    /// Stop all playing sounds
    void stopAll();

    /// Update active sounds (clean up finished ones)
    void update();

    /// Check if a sound ID is registered
    bool hasSound(const std::string& id) const;

    /// Get the sound definition (nullptr if not found)
    const SoundDef* getSoundDef(const std::string& id) const;

    /// Get number of registered sounds
    size_t registeredCount() const { return m_definitions.size(); }

    /// Get number of active (playing) sounds
    size_t activeCount() const { return m_activeSounds.size(); }

    // ---- Volume ----

    void setSfxVolume(float volume) { m_sfxVolume = std::max(0.0f, std::min(1.0f, volume)); }
    float getSfxVolume() const { return m_sfxVolume; }

    // ---- Static utility functions (testable without audio device) ----

    /// Calculate volume attenuation based on distance from listener
    static float calculateDistanceAttenuation(float sourceX, float sourceY,
                                              float listenerX, float listenerY,
                                              float maxRange);

    /// Calculate stereo pan from horizontal offset (-1.0=left, 0.0=center, 1.0=right)
    static float calculatePan(float sourceX, float listenerX, float maxRange);

private:
    /// Ensure a sound's audio data is loaded from disk
    bool ensureLoaded(const std::string& id);

    struct LoadedSound {
        ::Sound raylibSound{};   // Raylib Sound (base audio data)
        bool loaded = false;
    };

    struct ActiveSoundInstance {
        SoundHandle handle = 0;
        std::string defId;
        ::Sound aliasSound{};    // Raylib Sound alias for concurrent playback
        bool isAlias = false;
    };

    std::unordered_map<std::string, SoundDef> m_definitions;
    std::unordered_map<std::string, LoadedSound> m_loadedSounds;
    std::vector<ActiveSoundInstance> m_activeSounds;

    SoundHandle m_nextHandle = 1;
    int m_maxConcurrentSounds = 32;
    float m_sfxVolume = 1.0f;
    bool m_initialized = false;
};

} // namespace gloaming
