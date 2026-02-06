#include <gtest/gtest.h>

#include "audio/AudioSystem.hpp"
#include "audio/SoundManager.hpp"
#include "audio/MusicManager.hpp"

using namespace gloaming;

// ============================================================================
// AudioConfig Tests
// ============================================================================

TEST(AudioConfigTest, Defaults) {
    AudioConfig cfg;
    EXPECT_TRUE(cfg.enabled);
    EXPECT_FLOAT_EQ(cfg.masterVolume, 1.0f);
    EXPECT_FLOAT_EQ(cfg.sfxVolume, 0.8f);
    EXPECT_FLOAT_EQ(cfg.musicVolume, 0.7f);
    EXPECT_FLOAT_EQ(cfg.ambientVolume, 0.8f);
    EXPECT_EQ(cfg.maxConcurrentSounds, 32);
    EXPECT_FLOAT_EQ(cfg.positionalRange, 1000.0f);
}

TEST(AudioConfigTest, CustomValues) {
    AudioConfig cfg;
    cfg.enabled = false;
    cfg.masterVolume = 0.5f;
    cfg.sfxVolume = 0.3f;
    cfg.musicVolume = 0.4f;
    cfg.ambientVolume = 0.6f;
    cfg.maxConcurrentSounds = 16;
    cfg.positionalRange = 500.0f;

    EXPECT_FALSE(cfg.enabled);
    EXPECT_FLOAT_EQ(cfg.masterVolume, 0.5f);
    EXPECT_FLOAT_EQ(cfg.sfxVolume, 0.3f);
    EXPECT_FLOAT_EQ(cfg.musicVolume, 0.4f);
    EXPECT_FLOAT_EQ(cfg.ambientVolume, 0.6f);
    EXPECT_EQ(cfg.maxConcurrentSounds, 16);
    EXPECT_FLOAT_EQ(cfg.positionalRange, 500.0f);
}

// ============================================================================
// AudioStats Tests
// ============================================================================

TEST(AudioStatsTest, Defaults) {
    AudioStats stats;
    EXPECT_EQ(stats.registeredSounds, 0u);
    EXPECT_EQ(stats.activeSounds, 0u);
    EXPECT_FALSE(stats.musicPlaying);
    EXPECT_TRUE(stats.currentMusic.empty());
    EXPECT_FALSE(stats.deviceInitialized);
}

// ============================================================================
// SoundDef Tests
// ============================================================================

TEST(SoundDefTest, Defaults) {
    SoundDef def;
    EXPECT_TRUE(def.id.empty());
    EXPECT_TRUE(def.filePath.empty());
    EXPECT_FLOAT_EQ(def.baseVolume, 1.0f);
    EXPECT_FLOAT_EQ(def.pitchVariance, 0.0f);
    EXPECT_FLOAT_EQ(def.cooldown, 0.0f);
    EXPECT_LT(def.lastPlayTime, 0.0f);  // Allows immediate first play
}

TEST(SoundDefTest, CustomValues) {
    SoundDef def;
    def.id = "test_sound";
    def.filePath = "/sounds/test.ogg";
    def.baseVolume = 0.8f;
    def.pitchVariance = 0.1f;
    def.cooldown = 0.5f;

    EXPECT_EQ(def.id, "test_sound");
    EXPECT_EQ(def.filePath, "/sounds/test.ogg");
    EXPECT_FLOAT_EQ(def.baseVolume, 0.8f);
    EXPECT_FLOAT_EQ(def.pitchVariance, 0.1f);
    EXPECT_FLOAT_EQ(def.cooldown, 0.5f);
}

// ============================================================================
// Distance Attenuation Tests (static math, no audio device needed)
// ============================================================================

TEST(PositionalAudioTest, AttenuationAtOrigin) {
    // Listener and source at same position -> full volume
    float atten = SoundManager::calculateDistanceAttenuation(0, 0, 0, 0, 1000);
    EXPECT_FLOAT_EQ(atten, 1.0f);
}

TEST(PositionalAudioTest, AttenuationAtMaxRange) {
    // Source at max range -> zero volume
    float atten = SoundManager::calculateDistanceAttenuation(1000, 0, 0, 0, 1000);
    EXPECT_FLOAT_EQ(atten, 0.0f);
}

TEST(PositionalAudioTest, AttenuationBeyondRange) {
    // Source beyond max range -> zero volume
    float atten = SoundManager::calculateDistanceAttenuation(2000, 0, 0, 0, 1000);
    EXPECT_FLOAT_EQ(atten, 0.0f);
}

TEST(PositionalAudioTest, AttenuationHalfRange) {
    // At half the max range
    float atten = SoundManager::calculateDistanceAttenuation(500, 0, 0, 0, 1000);
    // With quadratic falloff: 1 - (0.5)^2 = 0.75
    EXPECT_FLOAT_EQ(atten, 0.75f);
}

TEST(PositionalAudioTest, AttenuationQuarterRange) {
    // At 25% of max range
    float atten = SoundManager::calculateDistanceAttenuation(250, 0, 0, 0, 1000);
    // With quadratic falloff: 1 - (0.25)^2 = 1 - 0.0625 = 0.9375
    EXPECT_FLOAT_EQ(atten, 0.9375f);
}

TEST(PositionalAudioTest, AttenuationVerticalDistance) {
    // Vertical distance also attenuates
    float atten = SoundManager::calculateDistanceAttenuation(0, 500, 0, 0, 1000);
    EXPECT_FLOAT_EQ(atten, 0.75f);
}

TEST(PositionalAudioTest, AttenuationDiagonalDistance) {
    // Diagonal distance (300, 400 from origin = 500 total)
    float atten = SoundManager::calculateDistanceAttenuation(300, 400, 0, 0, 1000);
    EXPECT_FLOAT_EQ(atten, 0.75f);
}

TEST(PositionalAudioTest, AttenuationZeroRange) {
    // Zero range always returns 0
    float atten = SoundManager::calculateDistanceAttenuation(0, 0, 0, 0, 0);
    EXPECT_FLOAT_EQ(atten, 0.0f);
}

TEST(PositionalAudioTest, AttenuationNegativeRange) {
    float atten = SoundManager::calculateDistanceAttenuation(0, 0, 0, 0, -100);
    EXPECT_FLOAT_EQ(atten, 0.0f);
}

TEST(PositionalAudioTest, AttenuationSymmetric) {
    // Same distance in opposite directions should give same attenuation
    float left  = SoundManager::calculateDistanceAttenuation(-400, 0, 0, 0, 1000);
    float right = SoundManager::calculateDistanceAttenuation(400, 0, 0, 0, 1000);
    EXPECT_FLOAT_EQ(left, right);
}

TEST(PositionalAudioTest, AttenuationMonotonicallyDecreasing) {
    // Attenuation should decrease as distance increases
    float prev = 1.0f;
    for (int d = 0; d <= 1000; d += 100) {
        float atten = SoundManager::calculateDistanceAttenuation(
            static_cast<float>(d), 0, 0, 0, 1000);
        EXPECT_LE(atten, prev);
        prev = atten;
    }
}

// ============================================================================
// Pan Calculation Tests
// ============================================================================

TEST(PanTest, CenterWhenSamePosition) {
    float pan = SoundManager::calculatePan(100, 100, 1000);
    EXPECT_FLOAT_EQ(pan, 0.5f);
}

TEST(PanTest, RightWhenSourceRight) {
    // Source to the right -> pan > 0.5
    float pan = SoundManager::calculatePan(600, 100, 1000);
    EXPECT_GT(pan, 0.5f);
    EXPECT_LE(pan, 1.0f);
}

TEST(PanTest, LeftWhenSourceLeft) {
    // Source to the left -> pan < 0.5
    float pan = SoundManager::calculatePan(-400, 100, 1000);
    EXPECT_LT(pan, 0.5f);
    EXPECT_GE(pan, 0.0f);
}

TEST(PanTest, MaxRight) {
    // Source at max range to the right
    float pan = SoundManager::calculatePan(1100, 100, 1000);
    EXPECT_FLOAT_EQ(pan, 1.0f);
}

TEST(PanTest, MaxLeft) {
    // Source at max range to the left
    float pan = SoundManager::calculatePan(-900, 100, 1000);
    EXPECT_FLOAT_EQ(pan, 0.0f);
}

TEST(PanTest, ClampsBeyondRange) {
    // Source far beyond range -> clamped
    float pan = SoundManager::calculatePan(5000, 0, 1000);
    EXPECT_FLOAT_EQ(pan, 1.0f);
}

TEST(PanTest, ZeroRange) {
    float pan = SoundManager::calculatePan(100, 0, 0);
    EXPECT_FLOAT_EQ(pan, 0.5f);  // Center when range is zero
}

TEST(PanTest, SymmetricPan) {
    // Equal offsets in opposite directions should be symmetric around 0.5
    float right = SoundManager::calculatePan(300, 0, 1000);
    float left  = SoundManager::calculatePan(-300, 0, 1000);
    EXPECT_NEAR(right - 0.5f, 0.5f - left, 0.001f);
}

// ============================================================================
// Crossfade Math Tests
// ============================================================================

TEST(CrossfadeTest, ProgressAtStart) {
    float progress = MusicManager::calculateFadeProgress(0.0f, 2.0f);
    EXPECT_FLOAT_EQ(progress, 0.0f);
}

TEST(CrossfadeTest, ProgressAtEnd) {
    float progress = MusicManager::calculateFadeProgress(2.0f, 2.0f);
    EXPECT_FLOAT_EQ(progress, 1.0f);
}

TEST(CrossfadeTest, ProgressAtMiddle) {
    float progress = MusicManager::calculateFadeProgress(1.0f, 2.0f);
    // Smoothstep at t=0.5: 0.5*0.5*(3-2*0.5) = 0.25*2 = 0.5
    EXPECT_FLOAT_EQ(progress, 0.5f);
}

TEST(CrossfadeTest, ProgressQuarter) {
    float progress = MusicManager::calculateFadeProgress(0.5f, 2.0f);
    // Smoothstep at t=0.25: 0.25*0.25*(3-2*0.25) = 0.0625*2.5 = 0.15625
    EXPECT_FLOAT_EQ(progress, 0.15625f);
}

TEST(CrossfadeTest, ProgressZeroDuration) {
    // Zero duration -> instant (100%)
    float progress = MusicManager::calculateFadeProgress(0.0f, 0.0f);
    EXPECT_FLOAT_EQ(progress, 1.0f);
}

TEST(CrossfadeTest, ProgressNegativeDuration) {
    float progress = MusicManager::calculateFadeProgress(1.0f, -1.0f);
    EXPECT_FLOAT_EQ(progress, 1.0f);
}

TEST(CrossfadeTest, ProgressBeyondDuration) {
    // Elapsed exceeds duration -> clamped to 1.0
    float progress = MusicManager::calculateFadeProgress(5.0f, 2.0f);
    EXPECT_FLOAT_EQ(progress, 1.0f);
}

TEST(CrossfadeTest, ProgressNegativeElapsed) {
    float progress = MusicManager::calculateFadeProgress(-1.0f, 2.0f);
    EXPECT_FLOAT_EQ(progress, 0.0f);
}

TEST(CrossfadeTest, SmoothstepMonotonic) {
    // Smoothstep should be monotonically increasing
    float prev = 0.0f;
    for (int i = 0; i <= 100; i++) {
        float t = static_cast<float>(i) / 100.0f;
        float progress = MusicManager::calculateFadeProgress(t, 1.0f);
        EXPECT_GE(progress, prev);
        prev = progress;
    }
}

TEST(CrossfadeTest, SmoothstepBoundaries) {
    // Smoothstep: zero derivative at t=0 and t=1
    // Check progress near start and end is very close to boundaries
    float nearStart = MusicManager::calculateFadeProgress(0.01f, 1.0f);
    float nearEnd = MusicManager::calculateFadeProgress(0.99f, 1.0f);
    EXPECT_NEAR(nearStart, 0.0f, 0.001f);
    EXPECT_NEAR(nearEnd, 1.0f, 0.001f);
}

// ============================================================================
// AudioSystem Construction Tests (no device needed)
// ============================================================================

TEST(AudioSystemTest, DefaultConstruction) {
    AudioSystem sys;
    EXPECT_EQ(sys.getName(), "AudioSystem");
    EXPECT_FALSE(sys.isDeviceReady());
    EXPECT_EQ(sys.getRegisteredSoundCount(), 0u);
    EXPECT_EQ(sys.getActiveSoundCount(), 0u);
}

TEST(AudioSystemTest, ConstructionWithConfig) {
    AudioConfig cfg;
    cfg.masterVolume = 0.5f;
    cfg.sfxVolume = 0.3f;
    cfg.musicVolume = 0.4f;
    cfg.maxConcurrentSounds = 16;

    AudioSystem sys(cfg);
    EXPECT_FLOAT_EQ(sys.getMasterVolume(), 0.5f);
    EXPECT_FLOAT_EQ(sys.getSfxVolume(), 0.3f);
    EXPECT_FLOAT_EQ(sys.getMusicVolume(), 0.4f);
    EXPECT_EQ(sys.getConfig().maxConcurrentSounds, 16);
}

TEST(AudioSystemTest, DefaultConfigValues) {
    AudioSystem sys;
    EXPECT_FLOAT_EQ(sys.getMasterVolume(), 1.0f);
    EXPECT_FLOAT_EQ(sys.getSfxVolume(), 0.8f);
    EXPECT_FLOAT_EQ(sys.getMusicVolume(), 0.7f);
    EXPECT_FLOAT_EQ(sys.getAmbientVolume(), 0.8f);
}

TEST(AudioSystemTest, ListenerPosition) {
    AudioSystem sys;
    EXPECT_FLOAT_EQ(sys.getListenerPosition().x, 0.0f);
    EXPECT_FLOAT_EQ(sys.getListenerPosition().y, 0.0f);

    sys.setListenerPosition(Vec2(100.0f, 200.0f));
    EXPECT_FLOAT_EQ(sys.getListenerPosition().x, 100.0f);
    EXPECT_FLOAT_EQ(sys.getListenerPosition().y, 200.0f);
}

TEST(AudioSystemTest, StatsWithoutDevice) {
    AudioSystem sys;
    AudioStats stats = sys.getStats();
    EXPECT_EQ(stats.registeredSounds, 0u);
    EXPECT_EQ(stats.activeSounds, 0u);
    EXPECT_FALSE(stats.musicPlaying);
    EXPECT_TRUE(stats.currentMusic.empty());
    EXPECT_FALSE(stats.deviceInitialized);
}

TEST(AudioSystemTest, RegisterSoundWithoutDevice) {
    // Registration should work even without audio device (stores metadata)
    AudioSystem sys;
    sys.registerSound("test", "/path/to/test.ogg", 0.8f, 0.1f, 0.5f);
    EXPECT_EQ(sys.getRegisteredSoundCount(), 1u);
}

TEST(AudioSystemTest, RegisterMultipleSounds) {
    AudioSystem sys;
    sys.registerSound("sound1", "/path/a.ogg");
    sys.registerSound("sound2", "/path/b.ogg");
    sys.registerSound("sound3", "/path/c.ogg");
    EXPECT_EQ(sys.getRegisteredSoundCount(), 3u);
}

TEST(AudioSystemTest, PlaySoundWithoutDevice) {
    // Play should return 0 (invalid handle) when device isn't ready
    AudioSystem sys;
    sys.registerSound("test", "/path/to/test.ogg");
    SoundHandle handle = sys.playSound("test");
    EXPECT_EQ(handle, 0u);
}

TEST(AudioSystemTest, IsMusicPlayingWithoutDevice) {
    AudioSystem sys;
    EXPECT_FALSE(sys.isMusicPlaying());
}

TEST(AudioSystemTest, GetCurrentMusicWithoutDevice) {
    AudioSystem sys;
    EXPECT_TRUE(sys.getCurrentMusic().empty());
}

// ============================================================================
// SoundHandle Constants
// ============================================================================

TEST(SoundHandleTest, InvalidHandleIsZero) {
    EXPECT_EQ(INVALID_SOUND_HANDLE, 0u);
}
