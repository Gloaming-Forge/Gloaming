#include "engine/DiagnosticOverlay.hpp"
#include "engine/Profiler.hpp"
#include "engine/ResourceManager.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "ecs/Components.hpp"

#include <cstdio>
#include <algorithm>
#include <cmath>

namespace gloaming {

void DiagnosticOverlay::cycle() {
    switch (m_mode) {
        case DiagnosticMode::Off:     m_mode = DiagnosticMode::Minimal; break;
        case DiagnosticMode::Minimal: m_mode = DiagnosticMode::Full;    break;
        case DiagnosticMode::Full:    m_mode = DiagnosticMode::Off;     break;
    }
}

void DiagnosticOverlay::render(IRenderer* renderer, const Profiler& profiler,
                                const ResourceManager& resources, Engine& engine) {
    if (m_mode == DiagnosticMode::Off || !renderer) return;

    if (m_mode == DiagnosticMode::Minimal) {
        renderMinimal(renderer, profiler);
    } else {
        renderFull(renderer, profiler, resources, engine);
    }
}

// =============================================================================
// Minimal mode: top-right corner FPS + budget bar
// =============================================================================

void DiagnosticOverlay::renderMinimal(IRenderer* renderer, const Profiler& profiler) {
    int screenW = renderer->getScreenWidth();
    float x = static_cast<float>(screenW) - kBarWidth - kPadding * 2 - 80;
    float y = static_cast<float>(kPadding);

    // Background panel
    renderer->drawRectangle(
        Rect(x - kPadding, y - 4, kBarWidth + kPadding * 2 + 90,
             kLineHeight * 2 + kBarHeight + 12),
        Color(0, 0, 0, 180));

    // FPS + frame time
    char fpsText[64];
    snprintf(fpsText, sizeof(fpsText), "FPS: %.0f  (%.2f ms)",
             profiler.avgFrameTimeMs() > 0.0 ? 1000.0 / profiler.avgFrameTimeMs() : 0.0,
             profiler.frameTimeMs());

    Color fpsColor = Color::Green();
    if (profiler.frameBudgetUsage() > 0.9) fpsColor = Color(255, 200, 0, 255);
    if (profiler.frameBudgetUsage() > 1.0) fpsColor = Color(255, 80, 80, 255);

    renderer->drawText(fpsText, {x, y}, kFontSize, fpsColor);
    y += kLineHeight;

    // Budget bar
    drawBudgetBar(renderer, x, y, profiler.frameBudgetUsage());
    y += kBarHeight + 4;

    // Frame budget text
    char budgetText[64];
    snprintf(budgetText, sizeof(budgetText), "Budget: %.0f%% of %.1f ms",
             profiler.frameBudgetUsage() * 100.0, profiler.frameBudgetMs());
    renderer->drawText(budgetText, {x, y}, kFontSize - 2, Color(180, 180, 180, 255));
}

// =============================================================================
// Full mode: comprehensive diagnostics
// =============================================================================

void DiagnosticOverlay::renderFull(IRenderer* renderer, const Profiler& profiler,
                                    const ResourceManager& resources, Engine& engine) {
    float x = static_cast<float>(kPadding);
    float y = static_cast<float>(kPadding);

    // Calculate panel height based on content
    const auto& zones = profiler.getAllZoneStats();
    int zoneLines = static_cast<int>(zones.size());
    float panelHeight = kLineHeight * (14 + zoneLines) + kGraphHeight + kPadding * 4;

    // Background panel
    renderer->drawRectangle(
        Rect(x - 4, y - 4, 420, panelHeight),
        Color(0, 0, 0, 200));

    // ---- Header ----
    char headerBuf[128];
    snprintf(headerBuf, sizeof(headerBuf), "Gloaming Engine v%s - Diagnostics", kEngineVersion);
    y = drawLine(renderer, x, y, headerBuf,
                 Color(255, 200, 100, 255));

    // ---- Frame timing ----
    {
        char buf[128];
        snprintf(buf, sizeof(buf), "Frame: %.2f ms (avg %.2f / min %.2f / max %.2f)",
                 profiler.frameTimeMs(), profiler.avgFrameTimeMs(),
                 profiler.minFrameTimeMs(), profiler.maxFrameTimeMs());
        y = drawLine(renderer, x, y, buf, Color::Green());

        double fps = profiler.avgFrameTimeMs() > 0.0
                   ? 1000.0 / profiler.avgFrameTimeMs() : 0.0;
        snprintf(buf, sizeof(buf), "FPS: %.1f | Frames: %llu",
                 fps, static_cast<unsigned long long>(profiler.frameCount()));
        y = drawLine(renderer, x, y, buf, Color::Green());
    }

    // Budget bar
    drawBudgetBar(renderer, x, y, profiler.frameBudgetUsage());
    y += kBarHeight + 6;

    // ---- Profiler zones ----
    if (!zones.empty()) {
        y = drawLine(renderer, x, y, "-- Profiler Zones --",
                     Color(200, 200, 255, 255));
        for (const auto& zone : zones) {
            char buf[128];
            snprintf(buf, sizeof(buf), "  %-16s %6.2f ms (avg %5.2f)",
                     zone.name.c_str(), zone.lastTimeMs, zone.avgTimeMs);

            Color color = Color::White();
            if (zone.lastTimeMs > profiler.frameBudgetMs() * 0.5) {
                color = Color(255, 200, 0, 255);
            }
            if (zone.lastTimeMs > profiler.frameBudgetMs() * 0.8) {
                color = Color(255, 80, 80, 255);
            }
            y = drawLine(renderer, x, y, buf, color);
        }
    }

    // ---- Frame graph ----
    y += 4;
    drawFrameGraph(renderer, x, y, profiler);
    y += kGraphHeight + 8;

    // ---- Camera ----
    {
        Vec2 camPos = engine.getCamera().getPosition();
        char buf[128];
        snprintf(buf, sizeof(buf), "Camera: (%.1f, %.1f) Zoom: %.2f",
                 camPos.x, camPos.y, engine.getCamera().getZoom());
        y = drawLine(renderer, x, y, buf, Color(100, 200, 255, 255));
    }

    // ---- World ----
    if (engine.getTileMap().isWorldLoaded()) {
        const auto& stats = engine.getTileMap().getStats();
        char buf[128];
        snprintf(buf, sizeof(buf), "Chunks: %zu loaded | %zu dirty",
                 stats.loadedChunks, stats.dirtyChunks);
        y = drawLine(renderer, x, y, buf, Color(200, 200, 100, 255));
    }

    // ---- ECS ----
    {
        auto entityCount = engine.getRegistry().alive();
        char buf[128];
        snprintf(buf, sizeof(buf), "Entities: %zu",
                 static_cast<size_t>(entityCount));
        y = drawLine(renderer, x, y, buf, Color(200, 255, 200, 255));
    }

    // ---- Resources ----
    {
        auto rStats = resources.getStats();
        char buf[192];
        snprintf(buf, sizeof(buf),
                 "Resources: %zu (%s) | Tex:%zu Snd:%zu Mus:%zu Lua:%zu",
                 rStats.totalCount, formatBytes(rStats.totalBytes).c_str(),
                 rStats.textureCount, rStats.soundCount,
                 rStats.musicCount, rStats.scriptCount);
        y = drawLine(renderer, x, y, buf, Color(200, 150, 255, 255));
    }

    // ---- Mods ----
    {
        char buf[128];
        snprintf(buf, sizeof(buf), "Mods: %zu loaded",
                 engine.getModLoader().loadedCount());
        y = drawLine(renderer, x, y, buf, Color(220, 180, 255, 255));
    }

    // ---- Particles & Tweens ----
    {
        auto pStats = engine.getParticleSystem()
                    ? engine.getParticleSystem()->getStats()
                    : ParticleSystem::Stats{};
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "Particles: %zu emitters, %zu alive | Tweens: %zu",
                 pStats.activeEmitters, pStats.activeParticles,
                 engine.getTweenSystem().activeCount());
        y = drawLine(renderer, x, y, buf, Color(255, 200, 150, 255));
    }
}

// =============================================================================
// Drawing helpers
// =============================================================================

void DiagnosticOverlay::drawFrameGraph(IRenderer* renderer, float x, float y,
                                        const Profiler& profiler) {
    const auto& history = profiler.frameTimeHistory();
    size_t histIdx = profiler.historyIndex();

    // Background
    renderer->drawRectangle(
        Rect(x, y, static_cast<float>(kGraphWidth), static_cast<float>(kGraphHeight)),
        Color(20, 20, 30, 200));

    // Target frame time line (green dashed)
    float budgetMs = static_cast<float>(profiler.frameBudgetMs());
    float maxMs = budgetMs * 2.5f; // Graph scale tops out at 2.5x budget

    float budgetY = y + kGraphHeight - (budgetMs / maxMs) * kGraphHeight;
    renderer->drawLine({x, budgetY}, {x + kGraphWidth, budgetY},
                       Color(0, 100, 0, 150), 1.0f);

    // Frame time bars
    float barWidth = static_cast<float>(kGraphWidth) / static_cast<float>(Profiler::kHistorySize);

    for (size_t i = 0; i < Profiler::kHistorySize; ++i) {
        size_t idx = (histIdx + i) % Profiler::kHistorySize;
        float ms = history[idx];
        if (ms <= 0.0f) continue;

        float barHeight = std::min((ms / maxMs) * kGraphHeight,
                                    static_cast<float>(kGraphHeight));
        float barX = x + static_cast<float>(i) * barWidth;
        float barY = y + kGraphHeight - barHeight;

        // Color based on performance
        Color barColor = Color(60, 200, 60, 200);        // green = good
        if (ms > budgetMs * 0.9f) barColor = Color(220, 200, 0, 200);  // yellow = warning
        if (ms > budgetMs)         barColor = Color(220, 60, 60, 200);  // red = over budget

        renderer->drawRectangle(Rect(barX, barY, barWidth, barHeight), barColor);
    }

    // Label
    char label[32];
    snprintf(label, sizeof(label), "%.1f ms", budgetMs);
    renderer->drawText(label, {x + kGraphWidth + 4, budgetY - 6},
                       kFontSize - 2, Color(100, 200, 100, 180));
}

void DiagnosticOverlay::drawBudgetBar(IRenderer* renderer, float x, float y,
                                       double usage) {
    // Background
    renderer->drawRectangle(
        Rect(x, y, static_cast<float>(kBarWidth), static_cast<float>(kBarHeight)),
        Color(40, 40, 40, 200));

    // Fill
    float fillWidth = static_cast<float>(std::min(usage, 1.5) / 1.5) * kBarWidth;
    Color fillColor = Color(60, 200, 60, 255);
    if (usage > 0.75) fillColor = Color(220, 200, 0, 255);
    if (usage > 1.0)  fillColor = Color(220, 60, 60, 255);

    renderer->drawRectangle(
        Rect(x, y, fillWidth, static_cast<float>(kBarHeight)),
        fillColor);

    // 100% mark
    float markX = x + static_cast<float>(kBarWidth) * (1.0f / 1.5f);
    renderer->drawLine(
        {markX, y}, {markX, y + kBarHeight},
        Color(255, 255, 255, 150), 1.0f);
}

float DiagnosticOverlay::drawLine(IRenderer* renderer, float x, float y,
                                   const std::string& text, Color color) {
    renderer->drawText(text, {x, y}, kFontSize, color);
    return y + kLineHeight;
}

std::string DiagnosticOverlay::formatBytes(size_t bytes) {
    char buf[32];
    if (bytes < 1024) {
        snprintf(buf, sizeof(buf), "%zu B", bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
    } else if (bytes < 1024ULL * 1024 * 1024) {
        snprintf(buf, sizeof(buf), "%.1f MB", bytes / (1024.0 * 1024.0));
    } else {
        snprintf(buf, sizeof(buf), "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    }
    return buf;
}

} // namespace gloaming
