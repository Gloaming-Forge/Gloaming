#pragma once

#include "rendering/IRenderer.hpp"

#include <string>

namespace gloaming {

// Forward declarations
class Engine;
class Profiler;
class ResourceManager;

/// Display mode for the diagnostic overlay.
enum class DiagnosticMode {
    Off,        ///< No overlay displayed
    Minimal,    ///< FPS counter, frame time, budget bar only
    Full        ///< All system stats, profiler zones, frame graph, resources
};

/// Renders performance diagnostics as a screen-space overlay.
///
/// Three modes cycled by F2:
///   Off     → nothing
///   Minimal → compact FPS + frame budget bar in the top-right corner
///   Full    → detailed breakdown with per-zone profiler times, frame time
///             graph, resource stats, entity counts, and system states
///
/// The overlay replaces the previous hard-coded HUD text in Engine::render()
/// when enabled, providing a cleaner separation of concerns.
class DiagnosticOverlay {
public:
    /// Cycle through modes: Off → Minimal → Full → Off.
    void cycle();

    /// Set a specific mode.
    void setMode(DiagnosticMode mode) { m_mode = mode; }

    /// Get the current mode.
    DiagnosticMode getMode() const { return m_mode; }

    /// Whether any overlay is being displayed.
    bool isVisible() const { return m_mode != DiagnosticMode::Off; }

    /// Render the overlay. Call after all game rendering is complete,
    /// before the renderer's endFrame().
    void render(IRenderer* renderer, const Profiler& profiler,
                const ResourceManager& resources, Engine& engine);

private:
    DiagnosticMode m_mode = DiagnosticMode::Off;

    // Layout constants
    static constexpr int kFontSize     = 14;
    static constexpr int kLineHeight   = 18;
    static constexpr int kPadding      = 10;
    static constexpr int kGraphHeight  = 60;
    static constexpr int kGraphWidth   = 240;
    static constexpr int kBarHeight    = 8;
    static constexpr int kBarWidth     = 200;

    // Section renderers
    void renderMinimal(IRenderer* renderer, const Profiler& profiler);
    void renderFull(IRenderer* renderer, const Profiler& profiler,
                    const ResourceManager& resources, Engine& engine);

    /// Draw the frame time graph from the profiler history.
    void drawFrameGraph(IRenderer* renderer, float x, float y,
                        const Profiler& profiler);

    /// Draw a horizontal bar showing budget usage (green → yellow → red).
    void drawBudgetBar(IRenderer* renderer, float x, float y,
                       double usage);

    /// Draw a labeled text line and return the Y position for the next line.
    float drawLine(IRenderer* renderer, float x, float y,
                   const std::string& text, Color color = Color::White());

    /// Format bytes into a human-readable string (B / KB / MB / GB).
    static std::string formatBytes(size_t bytes);
};

} // namespace gloaming
