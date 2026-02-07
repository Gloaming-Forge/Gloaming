#include "audio/MusicManager.hpp"
#include "engine/Log.hpp"

#include <algorithm>

namespace gloaming {

MusicManager::~MusicManager() {
    if (m_initialized) {
        shutdown();
    }
}

bool MusicManager::init() {
    m_initialized = true;
    LOG_DEBUG("MusicManager: initialized");
    return true;
}

void MusicManager::shutdown() {
    unloadPrevious();
    unloadCurrent();
    m_initialized = false;
    LOG_DEBUG("MusicManager: shut down");
}

void MusicManager::play(const std::string& filePath, float fadeInSeconds, bool loop) {
    if (!m_initialized) return;

    // If the same track is already playing, do nothing
    if (m_currentLoaded && m_currentPath == filePath && !m_stoppingFade) {
        return;
    }

    // If there's a current track, move it to "previous" for crossfade
    if (m_currentLoaded) {
        unloadPrevious();  // Clean up any existing previous track
        m_previous = m_current;
        m_previousLoaded = true;
        m_previousFadeVolume = m_currentFadeVolume;

        // Start fading out previous track
        m_fadingOut = true;
        m_fadeOutDuration = std::max(fadeInSeconds, m_minCrossfade);
        m_fadeOutElapsed = 0.0f;
    }

    // Load new track
    m_current = LoadMusicStream(filePath.c_str());
    if (m_current.frameCount == 0) {
        LOG_WARN("MusicManager: failed to load music '{}'", filePath);
        m_currentLoaded = false;
        return;
    }

    m_current.looping = loop;
    m_currentLoaded = true;
    m_currentPath = filePath;
    m_stoppingFade = false;
    m_paused = false;

    // Set up fade-in
    if (fadeInSeconds > 0.0f) {
        m_currentFadeVolume = 0.0f;
        m_fadingIn = true;
        m_fadeInDuration = fadeInSeconds;
        m_fadeInElapsed = 0.0f;
    } else {
        m_currentFadeVolume = 1.0f;
        m_fadingIn = false;
    }

    applyVolume();
    PlayMusicStream(m_current);

    LOG_INFO("MusicManager: playing '{}' (fade_in={}s, loop={})",
             filePath, fadeInSeconds, loop);
}

void MusicManager::stop(float fadeOutSeconds) {
    if (!m_currentLoaded) return;

    if (fadeOutSeconds > 0.0f) {
        // Fade out then stop
        m_stoppingFade = true;
        m_fadingIn = false;  // Cancel any fade-in
        m_fadeOutDuration = fadeOutSeconds;
        m_fadeOutElapsed = 0.0f;
    } else {
        // Immediate stop
        StopMusicStream(m_current);
        unloadCurrent();
    }
}

void MusicManager::setPaused(bool paused) {
    if (!m_currentLoaded) return;

    if (paused && !m_paused) {
        PauseMusicStream(m_current);
        if (m_previousLoaded) PauseMusicStream(m_previous);
        m_paused = true;
    } else if (!paused && m_paused) {
        ResumeMusicStream(m_current);
        if (m_previousLoaded) ResumeMusicStream(m_previous);
        m_paused = false;
    }
}

void MusicManager::update(float dt) {
    if (!m_initialized || m_paused) return;

    // Update current track stream
    if (m_currentLoaded) {
        UpdateMusicStream(m_current);
    }

    // Update previous track stream (during crossfade)
    if (m_previousLoaded) {
        UpdateMusicStream(m_previous);
    }

    // Process fade-in on current track
    if (m_fadingIn && m_currentLoaded) {
        m_fadeInElapsed += dt;
        m_currentFadeVolume = calculateFadeProgress(m_fadeInElapsed, m_fadeInDuration);
        if (m_fadeInElapsed >= m_fadeInDuration) {
            m_currentFadeVolume = 1.0f;
            m_fadingIn = false;
        }
        applyVolume();
    }

    // Process fade-out on previous track (crossfade)
    if (m_fadingOut && m_previousLoaded) {
        m_fadeOutElapsed += dt;
        float progress = calculateFadeProgress(m_fadeOutElapsed, m_fadeOutDuration);
        m_previousFadeVolume = 1.0f - progress;
        SetMusicVolume(m_previous, m_previousFadeVolume * m_musicVolume);

        if (m_fadeOutElapsed >= m_fadeOutDuration) {
            m_fadingOut = false;
            unloadPrevious();
        }
    }

    // Process stopping fade (fade out current track to silence, then stop)
    if (m_stoppingFade && m_currentLoaded) {
        m_fadeOutElapsed += dt;
        float progress = calculateFadeProgress(m_fadeOutElapsed, m_fadeOutDuration);
        m_currentFadeVolume = 1.0f - progress;
        applyVolume();

        if (m_fadeOutElapsed >= m_fadeOutDuration) {
            m_stoppingFade = false;
            StopMusicStream(m_current);
            unloadCurrent();
        }
    }
}

bool MusicManager::isPlaying() const {
    return m_currentLoaded && !m_paused && !m_stoppingFade;
}

float MusicManager::getProgress() const {
    if (!m_currentLoaded) return 0.0f;
    float length = GetMusicTimeLength(m_current);
    if (length <= 0.0f) return 0.0f;
    return GetMusicTimePlayed(m_current) / length;
}

float MusicManager::getTimePlayed() const {
    if (!m_currentLoaded) return 0.0f;
    return GetMusicTimePlayed(m_current);
}

float MusicManager::getTimeLength() const {
    if (!m_currentLoaded) return 0.0f;
    return GetMusicTimeLength(m_current);
}

float MusicManager::calculateFadeProgress(float elapsed, float duration) {
    if (duration <= 0.0f) return 1.0f;
    float t = elapsed / duration;
    t = std::max(0.0f, std::min(1.0f, t));
    // Smooth step for natural-sounding fade
    return t * t * (3.0f - 2.0f * t);
}

void MusicManager::unloadCurrent() {
    if (m_currentLoaded) {
        StopMusicStream(m_current);
        UnloadMusicStream(m_current);
        m_currentLoaded = false;
        m_currentPath.clear();
        m_currentFadeVolume = 1.0f;
        m_fadingIn = false;
        m_stoppingFade = false;
    }
}

void MusicManager::unloadPrevious() {
    if (m_previousLoaded) {
        StopMusicStream(m_previous);
        UnloadMusicStream(m_previous);
        m_previousLoaded = false;
        m_previousFadeVolume = 1.0f;
        m_fadingOut = false;
    }
}

void MusicManager::applyVolume() {
    if (m_currentLoaded) {
        SetMusicVolume(m_current, m_currentFadeVolume * m_musicVolume);
    }
}

} // namespace gloaming
