#pragma once

#include <raylib.h>

#include <string>

namespace gloaming {

/// Manages music streaming with crossfade support.
/// Uses Raylib's Music streaming API (backed by miniaudio).
class MusicManager {
public:
    MusicManager() = default;
    ~MusicManager();

    // Non-copyable
    MusicManager(const MusicManager&) = delete;
    MusicManager& operator=(const MusicManager&) = delete;

    /// Initialize the music manager
    bool init();

    /// Shutdown and release all music resources
    void shutdown();

    /// Play a music track. If another track is playing, crossfades to the new one.
    /// @param filePath Path to the music file (OGG, WAV, etc.)
    /// @param fadeInSeconds Duration of fade-in (0 = instant)
    /// @param loop Whether to loop the track
    void play(const std::string& filePath, float fadeInSeconds = 0.0f, bool loop = true);

    /// Stop the current music with optional fade-out
    void stop(float fadeOutSeconds = 0.0f);

    /// Pause or resume the current music
    void setPaused(bool paused);

    /// Update music streams and process crossfade transitions.
    /// Must be called every frame.
    void update(float dt);

    // ---- State queries ----

    bool isPlaying() const;
    bool isPaused() const { return m_paused; }
    const std::string& getCurrentTrack() const { return m_currentPath; }

    /// Get playback progress (0.0 to 1.0)
    float getProgress() const;

    /// Get time played in seconds
    float getTimePlayed() const;

    /// Get total track length in seconds
    float getTimeLength() const;

    // ---- Volume ----

    void setVolume(float volume) { m_musicVolume = std::max(0.0f, std::min(1.0f, volume)); }
    float getVolume() const { return m_musicVolume; }

    /// Set the minimum crossfade duration in seconds (0 = allow instant switch)
    void setMinCrossfade(float seconds) { m_minCrossfade = std::max(0.0f, seconds); }
    float getMinCrossfade() const { return m_minCrossfade; }

    // ---- Crossfade math (static, testable without device) ----

    /// Calculate fade progress given elapsed time and fade duration
    static float calculateFadeProgress(float elapsed, float duration);

private:
    void unloadCurrent();
    void unloadPrevious();
    void applyVolume();

    // Current track
    ::Music m_current{};
    bool m_currentLoaded = false;
    std::string m_currentPath;
    float m_currentFadeVolume = 1.0f;

    // Previous track (fading out during crossfade)
    ::Music m_previous{};
    bool m_previousLoaded = false;
    float m_previousFadeVolume = 1.0f;

    // Fade state
    float m_fadeInDuration = 0.0f;
    float m_fadeInElapsed = 0.0f;
    bool m_fadingIn = false;

    float m_fadeOutDuration = 0.0f;
    float m_fadeOutElapsed = 0.0f;
    bool m_fadingOut = false;      // Crossfade: previous track fading out
    bool m_stoppingFade = false;   // Simple stop with fade

    float m_musicVolume = 1.0f;
    float m_minCrossfade = 0.5f;
    bool m_initialized = false;
    bool m_paused = false;
};

} // namespace gloaming
