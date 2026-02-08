#pragma once

#include "rendering/IRenderer.hpp"
#include "engine/Input.hpp"
#include "gameplay/InputActions.hpp"

#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <cmath>

namespace gloaming {

/// A single choice in a dialogue
struct DialogueChoice {
    std::string text;
    std::string nextNodeId;      // ID of the next dialogue node (empty = end)
    std::function<void()> onSelect;
};

/// A node in a dialogue tree
struct DialogueNode {
    std::string id;
    std::string speaker;          // Name shown above the text box
    std::string text;             // The dialogue text
    std::string portraitId;       // Optional portrait texture ID
    std::vector<DialogueChoice> choices;
    std::string nextNodeId;       // Auto-advance to this node (if no choices)
    std::function<void()> onShow; // Called when this node is displayed
};

/// Configuration for the dialogue box appearance
struct DialogueBoxConfig {
    // Box dimensions and position
    float boxX = 0.0f;
    float boxY = 0.0f;
    float boxWidth = 0.0f;        // 0 = auto (80% of screen width)
    float boxHeight = 0.0f;       // 0 = auto (25% of screen height)
    float padding = 16.0f;
    float bottomMargin = 32.0f;   // Distance from bottom of screen

    // Colors
    Color backgroundColor{0, 0, 0, 200};
    Color borderColor{255, 255, 255, 255};
    Color textColor = Color::White();
    Color speakerColor{255, 220, 100, 255};
    Color choiceColor{200, 200, 255, 255};
    Color choiceSelectedColor{255, 255, 100, 255};
    float borderWidth = 2.0f;

    // Text
    int fontSize = 20;
    int speakerFontSize = 22;
    float typewriterSpeed = 30.0f;   // Characters per second (0 = instant)
    float lineSpacing = 4.0f;

    // Portrait
    float portraitSize = 64.0f;
    float portraitPadding = 12.0f;

    // Advance key
    Key advanceKey = Key::Z;
    Key cancelKey = Key::X;
};

/// The dialogue system manages displaying text boxes, typewriter effects,
/// choice selection, and dialogue tree navigation.
class DialogueSystem {
public:
    DialogueSystem() = default;

    void setConfig(const DialogueBoxConfig& config) { m_config = config; }
    DialogueBoxConfig& getConfig() { return m_config; }

    /// Set the input action map for respecting mod key rebindings.
    /// If set, choice navigation uses "move_up"/"move_down"/"interact"/"cancel" actions
    /// instead of hard-coded keys.
    void setInputActions(InputActionMap* actions) { m_inputActions = actions; }

    /// Start a dialogue sequence. Adds all nodes and begins at the first one.
    void startDialogue(std::vector<DialogueNode> nodes) {
        m_nodes.clear();
        if (nodes.empty()) return;
        // Capture the first node's ID before moving, to avoid reading moved-from state
        std::string firstId = nodes.front().id;
        for (auto& node : nodes) {
            m_nodes[node.id] = std::move(node);
        }
        showNode(firstId);
        m_active = true;
    }

    /// Start dialogue from a specific node ID
    void startDialogue(std::vector<DialogueNode> nodes, const std::string& startId) {
        m_nodes.clear();
        for (auto& node : nodes) {
            m_nodes[node.id] = std::move(node);
        }
        showNode(startId);
        m_active = true;
    }

    /// Jump to a specific node in the current dialogue
    void jumpToNode(const std::string& nodeId) {
        showNode(nodeId);
    }

    /// Close the dialogue box
    void close() {
        m_active = false;
        m_currentNodeId.clear();
        m_displayedChars = 0;
        m_selectedChoice = 0;
        if (m_onDialogueEnd) m_onDialogueEnd();
    }

    /// Set callback for when dialogue ends
    void setOnDialogueEnd(std::function<void()> callback) {
        m_onDialogueEnd = std::move(callback);
    }

    /// Is the dialogue box currently active?
    bool isActive() const { return m_active; }

    /// Is the dialogue box blocking game input?
    bool isBlocking() const { return m_active; }

    /// Update the dialogue (typewriter effect, input handling)
    void update(float dt, const Input& input) {
        if (!m_active) return;

        auto it = m_nodes.find(m_currentNodeId);
        if (it == m_nodes.end()) {
            close();
            return;
        }

        const DialogueNode& node = it->second;

        // Typewriter effect
        if (m_config.typewriterSpeed > 0 && m_displayedChars < static_cast<int>(node.text.size())) {
            m_charTimer += dt;
            float charInterval = 1.0f / m_config.typewriterSpeed;
            while (m_charTimer >= charInterval && m_displayedChars < static_cast<int>(node.text.size())) {
                m_charTimer -= charInterval;
                ++m_displayedChars;
            }
        } else {
            m_displayedChars = static_cast<int>(node.text.size());
        }

        // Handle input — use InputActions if available, raw keys as fallback
        bool advancePressed = m_inputActions
            ? m_inputActions->isActionPressed("interact", input)
            : input.isKeyPressed(m_config.advanceKey);

        if (advancePressed) {
            if (m_displayedChars < static_cast<int>(node.text.size())) {
                // Skip typewriter — show all text
                m_displayedChars = static_cast<int>(node.text.size());
            } else if (!node.choices.empty()) {
                // Select current choice
                const auto& choice = node.choices[m_selectedChoice];
                if (choice.onSelect) choice.onSelect();
                if (!choice.nextNodeId.empty()) {
                    showNode(choice.nextNodeId);
                } else {
                    close();
                }
            } else if (!node.nextNodeId.empty()) {
                // Advance to next node
                showNode(node.nextNodeId);
            } else {
                // End of dialogue
                close();
            }
        }

        // Navigate choices — use InputActions if available, raw keys as fallback
        if (!node.choices.empty() && m_displayedChars >= static_cast<int>(node.text.size())) {
            bool navUp = m_inputActions
                ? m_inputActions->isActionPressed("move_up", input)
                : (input.isKeyPressed(Key::Up) || input.isKeyPressed(Key::W));
            bool navDown = m_inputActions
                ? m_inputActions->isActionPressed("move_down", input)
                : (input.isKeyPressed(Key::Down) || input.isKeyPressed(Key::S));

            if (navUp) {
                m_selectedChoice = (m_selectedChoice > 0) ? m_selectedChoice - 1
                                   : static_cast<int>(node.choices.size()) - 1;
            }
            if (navDown) {
                m_selectedChoice = (m_selectedChoice + 1) % static_cast<int>(node.choices.size());
            }
        }
    }

    /// Render the dialogue box
    void render(IRenderer* renderer, int screenWidth, int screenHeight) {
        if (!m_active || !renderer) return;

        auto it = m_nodes.find(m_currentNodeId);
        if (it == m_nodes.end()) return;

        const DialogueNode& node = it->second;

        // Calculate box dimensions
        float boxW = m_config.boxWidth > 0 ? m_config.boxWidth : screenWidth * 0.8f;
        float boxH = m_config.boxHeight > 0 ? m_config.boxHeight : screenHeight * 0.25f;
        float boxX = m_config.boxX > 0 ? m_config.boxX : (screenWidth - boxW) * 0.5f;
        float boxY = m_config.boxY > 0 ? m_config.boxY : (screenHeight - boxH - m_config.bottomMargin);

        // Draw background
        Rect boxRect{boxX, boxY, boxW, boxH};
        renderer->drawRectangle(boxRect, m_config.backgroundColor);

        // Draw border
        if (m_config.borderWidth > 0) {
            renderer->drawRectangleOutline(boxRect, m_config.borderColor, m_config.borderWidth);
        }

        float textX = boxX + m_config.padding;
        float textY = boxY + m_config.padding;
        float textMaxW = boxW - m_config.padding * 2.0f;

        // Account for portrait
        if (!node.portraitId.empty()) {
            // TODO: Portrait rendering is a stub — draws an outline rectangle as a placeholder.
            // Actual texture rendering requires TextureManager integration (planned).
            // The Lua API accepts a "portrait" field for forward compatibility.
            Rect portraitRect{
                boxX + m_config.padding,
                boxY + m_config.padding,
                m_config.portraitSize,
                m_config.portraitSize
            };
            renderer->drawRectangleOutline(portraitRect, m_config.borderColor, 1.0f);

            textX += m_config.portraitSize + m_config.portraitPadding;
            textMaxW -= m_config.portraitSize + m_config.portraitPadding;
        }

        // Draw speaker name
        if (!node.speaker.empty()) {
            renderer->drawText(node.speaker, {textX, textY},
                             m_config.speakerFontSize, m_config.speakerColor);
            textY += m_config.speakerFontSize + m_config.lineSpacing;
        }

        // Draw text (with typewriter effect)
        std::string displayText = node.text.substr(0, m_displayedChars);
        // Simple word-wrap rendering
        renderWrappedText(renderer, displayText, textX, textY, textMaxW,
                         m_config.fontSize, m_config.textColor, m_config.lineSpacing);

        // Draw choices (only when text is fully displayed)
        if (!node.choices.empty() && m_displayedChars >= static_cast<int>(node.text.size())) {
            float choiceY = boxY + boxH - m_config.padding
                          - static_cast<float>(node.choices.size()) * (m_config.fontSize + m_config.lineSpacing);

            for (int i = 0; i < static_cast<int>(node.choices.size()); ++i) {
                bool selected = (i == m_selectedChoice);
                Color color = selected ? m_config.choiceSelectedColor : m_config.choiceColor;
                std::string prefix = selected ? "> " : "  ";
                renderer->drawText(prefix + node.choices[i].text,
                                 {textX, choiceY}, m_config.fontSize, color);
                choiceY += m_config.fontSize + m_config.lineSpacing;
            }
        }
    }

    /// Get the current node being displayed
    const std::string& getCurrentNodeId() const { return m_currentNodeId; }

private:
    void showNode(const std::string& nodeId) {
        m_currentNodeId = nodeId;
        m_displayedChars = 0;
        m_charTimer = 0.0f;
        m_selectedChoice = 0;

        auto it = m_nodes.find(nodeId);
        if (it != m_nodes.end() && it->second.onShow) {
            it->second.onShow();
        }

        // If typewriter is disabled, show all text immediately
        if (m_config.typewriterSpeed <= 0 && it != m_nodes.end()) {
            m_displayedChars = static_cast<int>(it->second.text.size());
        }
    }

    void renderWrappedText(IRenderer* renderer, const std::string& text,
                           float x, float y, float maxWidth,
                           int fontSize, const Color& color, float lineSpacing) {
        // Simple word-wrap: break on spaces
        std::string line;
        float lineY = y;

        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '\n') {
                renderer->drawText(line, {x, lineY}, fontSize, color);
                lineY += fontSize + lineSpacing;
                line.clear();
                continue;
            }

            line += text[i];
            float lineWidth = static_cast<float>(renderer->measureTextWidth(line, fontSize));

            if (lineWidth > maxWidth && !line.empty()) {
                // Find last space to break at
                size_t lastSpace = line.rfind(' ');
                if (lastSpace != std::string::npos && lastSpace > 0) {
                    std::string printLine = line.substr(0, lastSpace);
                    renderer->drawText(printLine, {x, lineY}, fontSize, color);
                    lineY += fontSize + lineSpacing;
                    line = line.substr(lastSpace + 1);
                } else {
                    // No space found, force break
                    renderer->drawText(line, {x, lineY}, fontSize, color);
                    lineY += fontSize + lineSpacing;
                    line.clear();
                }
            }
        }

        // Draw remaining text
        if (!line.empty()) {
            renderer->drawText(line, {x, lineY}, fontSize, color);
        }
    }

    DialogueBoxConfig m_config;
    InputActionMap* m_inputActions = nullptr;  // Optional: respects mod key rebindings
    std::unordered_map<std::string, DialogueNode> m_nodes;
    std::string m_currentNodeId;
    int m_displayedChars = 0;
    float m_charTimer = 0.0f;
    int m_selectedChoice = 0;
    bool m_active = false;
    std::function<void()> m_onDialogueEnd;
};

} // namespace gloaming
