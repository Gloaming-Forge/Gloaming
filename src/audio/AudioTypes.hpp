#pragma once

#include <cstdint>
#include <random>

namespace gloaming {

/// Handle for tracking a playing sound instance. Zero is invalid.
using SoundHandle = uint32_t;
constexpr SoundHandle INVALID_SOUND_HANDLE = 0;

/// Shared RNG for pitch variance across the audio subsystem.
/// Uses a single thread_local instance to avoid duplicate RNGs.
inline float randomPitchOffset(float pitchVariance) {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(-pitchVariance, pitchVariance);
    return dist(rng);
}

} // namespace gloaming
