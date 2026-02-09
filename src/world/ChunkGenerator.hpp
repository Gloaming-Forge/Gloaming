#pragma once

#include "world/Chunk.hpp"
#include <cstdint>
#include <functional>
#include <cmath>

namespace gloaming {

/// Callback type for chunk generation
/// @param chunk The chunk to populate with tiles
/// @param seed The world seed for deterministic generation
using ChunkGeneratorCallback = std::function<void(Chunk& chunk, uint64_t seed)>;

/// Simple noise generation utilities for procedural world generation
/// These are placeholder implementations - mods will provide more sophisticated worldgen
class Noise {
public:
    /// Simple 1D noise based on position and seed
    /// @return Value between 0.0 and 1.0
    static float noise1D(int x, uint64_t seed);

    /// Simple 2D noise based on position and seed
    /// @return Value between 0.0 and 1.0
    static float noise2D(int x, int y, uint64_t seed);

    /// Smoothed 1D noise (interpolated)
    /// @param scale Controls frequency (smaller = smoother)
    /// @return Value between 0.0 and 1.0
    static float smoothNoise1D(float x, uint64_t seed, float scale = 1.0f);

    /// Smoothed 2D noise (interpolated)
    /// @param scale Controls frequency (smaller = smoother)
    /// @return Value between 0.0 and 1.0
    static float smoothNoise2D(float x, float y, uint64_t seed, float scale = 1.0f);

    /// Fractal noise (multiple octaves of noise combined)
    /// @param octaves Number of noise layers to combine
    /// @param persistence How much each octave contributes (0.5 is common)
    /// @return Value between 0.0 and 1.0
    static float fractalNoise1D(float x, uint64_t seed, int octaves = 4, float persistence = 0.5f);

    /// Fractal noise in 2D
    static float fractalNoise2D(float x, float y, uint64_t seed, int octaves = 4, float persistence = 0.5f);

    /// Public hash accessors for seeding sub-generators
    static uint32_t hash_public(int x, uint64_t seed) { return hash(x, seed); }
    static uint32_t hash2D_public(int x, int y, uint64_t seed) { return hash2D(x, y, seed); }

private:
    /// Hash function for noise generation
    static uint32_t hash(int x, uint64_t seed);
    static uint32_t hash2D(int x, int y, uint64_t seed);

    /// Linear interpolation
    static float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    /// Smooth step for interpolation (3t^2 - 2t^3)
    static float smoothStep(float t) {
        return t * t * (3.0f - 2.0f * t);
    }
};

/// Chunk generator that uses callbacks to generate chunk content
/// This is the hook system for mod-provided world generation
class ChunkGenerator {
public:
    ChunkGenerator() = default;
    explicit ChunkGenerator(uint64_t seed) : m_seed(seed) {}

    /// Set the world seed
    void setSeed(uint64_t seed) { m_seed = seed; }

    /// Get the world seed
    uint64_t getSeed() const { return m_seed; }

    /// Set the generation callback
    void setGeneratorCallback(ChunkGeneratorCallback callback) {
        m_callback = std::move(callback);
    }

    /// Check if a generator callback is set
    bool hasGeneratorCallback() const { return m_callback != nullptr; }

    /// Generate a chunk at the given position
    /// If no callback is set, uses the default placeholder generator
    void generate(Chunk& chunk) const;

    /// Default placeholder generator that creates basic terrain
    /// This is used when no mod provides world generation
    static void defaultGenerator(Chunk& chunk, uint64_t seed);

    /// Flat world generator - creates flat terrain at a fixed height
    static void flatGenerator(Chunk& chunk, uint64_t seed, int surfaceY = 100);

    /// Empty chunk generator - leaves chunk empty
    static void emptyGenerator(Chunk& chunk, uint64_t seed);

private:
    uint64_t m_seed = 12345;
    ChunkGeneratorCallback m_callback = nullptr;
};

} // namespace gloaming
