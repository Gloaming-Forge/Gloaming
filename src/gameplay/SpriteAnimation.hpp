#pragma once

#include "ecs/Systems.hpp"
#include "ecs/Components.hpp"
#include "engine/Log.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace gloaming {

/// Animation playback mode
enum class PlaybackMode {
    Loop,      ///< Repeats indefinitely
    Once,      ///< Plays once, holds last frame
    PingPong   ///< Forward then reverse, repeating
};

/// A single animation clip — a sequence of source rects at a given FPS
struct AnimationClip {
    std::vector<Rect> frames;                    ///< Source rects for each frame
    float fps = 10.0f;                           ///< Frames per second
    PlaybackMode mode = PlaybackMode::Loop;      ///< Playback behavior
};

/// Callback type for animation frame events
using AnimationEventCallback = std::function<void(Entity)>;

/// AnimationController component — frame-based sprite animation with named clips.
///
/// Sits alongside a Sprite component. The AnimationControllerSystem reads this
/// component and writes the current frame's source rect into the Sprite.
struct AnimationController {
    /// Registered animation clips, keyed by name
    std::unordered_map<std::string, AnimationClip> clips;

    /// Currently playing clip name (empty = nothing playing)
    std::string currentClip;

    /// Current frame index within the active clip
    int currentFrame = 0;

    /// Accumulated time since last frame advance
    float frameTimer = 0.0f;

    /// True when a Once-mode animation has reached its last frame
    bool finished = false;

    /// PingPong direction flag (false = forward, true = reverse)
    bool pingPongReverse = false;

    /// Per-clip frame event callbacks: clipName -> (frameIndex -> callbacks)
    std::unordered_map<std::string,
        std::unordered_map<int, std::vector<AnimationEventCallback>>> frameEvents;

    /// Tracks the last frame for which events were fired (-1 = none yet)
    int lastEventFrame = -1;

    // -----------------------------------------------------------------
    // Convenience methods
    // -----------------------------------------------------------------

    /// Register a clip. Overwrites any existing clip with the same name.
    void addClip(const std::string& name, AnimationClip clip) {
        clips[name] = std::move(clip);
    }

    /// Build a clip from sprite-sheet parameters.
    /// @param row         Row index in the sheet (0-based, top to bottom)
    /// @param frameCount  Number of frames in this row
    /// @param frameWidth  Width of one frame in pixels
    /// @param frameHeight Height of one frame in pixels
    /// @param fps         Playback speed
    /// @param mode        Playback mode
    /// @param startCol    Starting column (0-based, default 0)
    /// @param padding     Pixel gap between frames in the sheet (default 0)
    void addClipFromSheet(const std::string& name, int row, int frameCount,
                          int frameWidth, int frameHeight, float fps,
                          PlaybackMode mode = PlaybackMode::Loop,
                          int startCol = 0, int padding = 0) {
        AnimationClip clip;
        clip.fps = fps;
        clip.mode = mode;
        clip.frames.reserve(frameCount);

        const int strideX = frameWidth + padding;
        const int strideY = frameHeight + padding;
        for (int i = 0; i < frameCount; ++i) {
            clip.frames.push_back(Rect(
                static_cast<float>((startCol + i) * strideX),
                static_cast<float>(row * strideY),
                static_cast<float>(frameWidth),
                static_cast<float>(frameHeight)
            ));
        }
        clips[name] = std::move(clip);
    }

    /// Start playing a clip by name. If the clip is already playing and not
    /// finished, this is a no-op (call stop() first to restart).
    /// @return true if the clip exists and playback started
    bool play(const std::string& name) {
        if (clips.find(name) == clips.end()) return false;
        if (name == currentClip && !finished) return true; // already playing
        currentClip = name;
        currentFrame = 0;
        frameTimer = 0.0f;
        finished = false;
        pingPongReverse = false;
        lastEventFrame = -1;
        return true;
    }

    /// Stop the current animation and clear state
    void stop() {
        currentClip.clear();
        currentFrame = 0;
        frameTimer = 0.0f;
        finished = false;
        pingPongReverse = false;
        lastEventFrame = -1;
    }

    /// Direction-aware helper: tries "<baseName>_<direction>" then "<baseName>".
    /// @param direction One of "up", "down", "left", "right"
    bool playDirectional(const std::string& baseName, const std::string& direction) {
        std::string full = baseName + "_" + direction;
        if (clips.find(full) != clips.end()) return play(full);
        return play(baseName); // fallback to non-directional
    }

    /// Register a callback to fire when a specific frame is reached.
    void addFrameEvent(const std::string& clipName, int frame,
                       AnimationEventCallback callback) {
        frameEvents[clipName][frame].push_back(std::move(callback));
    }

    /// Get the name of the current clip (empty if none)
    const std::string& getCurrentClipName() const { return currentClip; }

    /// Is the current Once-mode animation finished?
    bool isFinished() const { return finished; }

    /// Get the source rect for the current frame, or nullptr if nothing is playing
    const Rect* getCurrentFrameRect() const {
        auto it = clips.find(currentClip);
        if (it == clips.end() || it->second.frames.empty()) return nullptr;
        if (currentFrame >= 0 &&
            currentFrame < static_cast<int>(it->second.frames.size())) {
            return &it->second.frames[currentFrame];
        }
        return nullptr;
    }

    // -----------------------------------------------------------------
    // Tick — advances frame state and writes to sprite
    // -----------------------------------------------------------------

    /// Advance animation state by dt seconds, update sprite sourceRect, and
    /// fire any registered frame-event callbacks.  Called once per frame by
    /// AnimationControllerSystem (or directly in tests).
    void tick(float dt, Entity entity, Sprite& sprite) {
        if (currentClip.empty()) return;

        auto clipIt = clips.find(currentClip);
        if (clipIt == clips.end() || clipIt->second.frames.empty()) return;

        const auto& clip = clipIt->second;

        // Fire events for the initial frame (frame 0) on the first tick
        if (lastEventFrame == -1) {
            fireFrameEvents(entity);
            lastEventFrame = currentFrame;
        }

        if (finished) {
            // Still update the sprite to the held frame
            applyFrame(clip, sprite);
            return;
        }

        float frameDuration = (clip.fps > 0.0f) ? (1.0f / clip.fps) : 1.0f;

        frameTimer += dt;

        while (frameTimer >= frameDuration && !finished) {
            frameTimer -= frameDuration;

            int prevFrame = currentFrame;
            advanceFrame(clip);

            // Fire events when the frame changes
            if (currentFrame != prevFrame) {
                fireFrameEvents(entity);
                lastEventFrame = currentFrame;
            }
        }

        applyFrame(clip, sprite);
    }

private:
    /// Apply the current frame's source rect to the sprite
    void applyFrame(const AnimationClip& clip, Sprite& sprite) {
        if (currentFrame >= 0 &&
            currentFrame < static_cast<int>(clip.frames.size())) {
            sprite.sourceRect = clip.frames[currentFrame];
        }
    }

    /// Advance to the next frame according to the clip's playback mode
    void advanceFrame(const AnimationClip& clip) {
        int frameCount = static_cast<int>(clip.frames.size());
        if (frameCount <= 1) {
            finished = (clip.mode == PlaybackMode::Once);
            return;
        }

        switch (clip.mode) {
            case PlaybackMode::Loop:
                currentFrame = (currentFrame + 1) % frameCount;
                break;

            case PlaybackMode::Once:
                if (currentFrame < frameCount - 1) {
                    currentFrame++;
                } else {
                    finished = true;
                }
                break;

            case PlaybackMode::PingPong:
                if (!pingPongReverse) {
                    if (currentFrame < frameCount - 1) {
                        currentFrame++;
                    } else {
                        pingPongReverse = true;
                        currentFrame--;
                    }
                } else {
                    if (currentFrame > 0) {
                        currentFrame--;
                    } else {
                        pingPongReverse = false;
                        currentFrame++;
                    }
                }
                break;
        }
    }

    /// Fire any registered callbacks for the current frame
    void fireFrameEvents(Entity entity) {
        auto eventMapIt = frameEvents.find(currentClip);
        if (eventMapIt == frameEvents.end()) return;

        auto frameIt = eventMapIt->second.find(currentFrame);
        if (frameIt == eventMapIt->second.end()) return;

        for (auto& callback : frameIt->second) {
            callback(entity);
        }
    }
};

// =========================================================================
// AnimationControllerSystem
// =========================================================================

/// System that advances AnimationController state and updates the paired Sprite.
/// Runs in the Update phase, before rendering.
class AnimationControllerSystem : public System {
public:
    AnimationControllerSystem() : System("AnimationControllerSystem", 5) {}

    void update(float dt) override {
        getRegistry().each<AnimationController, Sprite>(
            [dt](Entity entity, AnimationController& ctrl, Sprite& sprite) {
                ctrl.tick(dt, entity, sprite);
            }
        );
    }
};

} // namespace gloaming
