#include "ui/UISystem.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"

#include <algorithm>

namespace gloaming {

bool UISystem::init(Engine& engine) {
    m_engine = &engine;
    m_renderer = engine.getRenderer();
    m_layout.setRenderer(m_renderer);

    LOG_INFO("UISystem: initialized");
    return true;
}

void UISystem::shutdown() {
    m_screens.clear();
    LOG_INFO("UISystem: shut down");
}

void UISystem::update(float dt) {
    if (!m_config.enabled || !m_engine) return;

    float screenW = static_cast<float>(m_renderer->getScreenWidth());
    float screenH = static_cast<float>(m_renderer->getScreenHeight());

    m_blockingScreenVisible = false;

    // Rebuild dynamic UIs and compute layout for all visible screens
    for (auto& [name, entry] : m_screens) {
        if (!entry.visible) continue;

        // Rebuild dynamic screens only when dirty
        if (entry.builder && entry.dirty) {
            auto newRoot = entry.builder();
            if (newRoot) {
                entry.root = newRoot;
            }
            entry.dirty = false;
        }

        if (entry.root) {
            m_layout.computeLayout(entry.root.get(), screenW, screenH);
        }

        if (entry.blocking) {
            m_blockingScreenVisible = true;
        }
    }

    // Process input for visible screens (top-most first)
    auto visibleScreens = getSortedVisibleScreens();
    // Reverse iterate so top-most screens get input first
    for (auto it = visibleScreens.rbegin(); it != visibleScreens.rend(); ++it) {
        if ((*it)->root) {
            if (m_uiInput.update((*it)->root.get(), m_engine->getInput())) {
                break; // Input consumed by this screen
            }
        }
    }

    // Handle scroll wheel for scroll panels
    float wheel = m_engine->getInput().getMouseWheelDelta();
    if (wheel != 0.0f) {
        float mx = m_engine->getInput().getMouseX();
        float my = m_engine->getInput().getMouseY();
        for (auto it = visibleScreens.rbegin(); it != visibleScreens.rend(); ++it) {
            if ((*it)->root) {
                // Find scroll panels under the mouse
                std::function<bool(UIElement*)> findAndScroll = [&](UIElement* elem) -> bool {
                    if (!elem || !elem->getStyle().visible) return false;
                    if (!elem->getLayout().containsPoint(mx, my)) return false;

                    // Check children first (deeper elements have priority)
                    for (auto childIt = elem->getChildren().rbegin();
                         childIt != elem->getChildren().rend(); ++childIt) {
                        if (findAndScroll(childIt->get())) return true;
                    }

                    if (elem->getType() == UIElementType::ScrollPanel) {
                        static_cast<UIScrollPanel*>(elem)->handleScroll(wheel);
                        return true;
                    }
                    return false;
                };
                if (findAndScroll((*it)->root.get())) break;
            }
        }
    }
}

void UISystem::render() {
    if (!m_config.enabled || !m_renderer) return;

    auto visibleScreens = getSortedVisibleScreens();
    for (auto* entry : visibleScreens) {
        if (entry->root) {
            entry->root->render(m_renderer);
        }
    }
}

void UISystem::registerScreen(const std::string& name, std::shared_ptr<UIElement> root) {
    m_screens[name] = {name, std::move(root), nullptr, false, false, 0};
    LOG_DEBUG("UISystem: registered static screen '{}'", name);
}

void UISystem::registerDynamicScreen(const std::string& name, UIBuilderCallback builder) {
    m_screens[name] = {name, nullptr, std::move(builder), false, false, 0};
    LOG_DEBUG("UISystem: registered dynamic screen '{}'", name);
}

void UISystem::removeScreen(const std::string& name) {
    m_screens.erase(name);
}

void UISystem::showScreen(const std::string& name) {
    auto it = m_screens.find(name);
    if (it != m_screens.end()) {
        it->second.visible = true;
        if (it->second.builder) {
            it->second.dirty = true; // Ensure dynamic screens rebuild on show
        }
        LOG_DEBUG("UISystem: showing screen '{}'", name);
    }
}

void UISystem::hideScreen(const std::string& name) {
    auto it = m_screens.find(name);
    if (it != m_screens.end()) {
        it->second.visible = false;
        LOG_DEBUG("UISystem: hiding screen '{}'", name);
    }
}

void UISystem::setScreenBlocking(const std::string& name, bool blocking) {
    auto it = m_screens.find(name);
    if (it != m_screens.end()) {
        it->second.blocking = blocking;
    }
}

void UISystem::setScreenZOrder(const std::string& name, int zOrder) {
    auto it = m_screens.find(name);
    if (it != m_screens.end()) {
        it->second.zOrder = zOrder;
    }
}

void UISystem::markScreenDirty(const std::string& name) {
    auto it = m_screens.find(name);
    if (it != m_screens.end()) {
        it->second.dirty = true;
    }
}

bool UISystem::isScreenVisible(const std::string& name) const {
    auto it = m_screens.find(name);
    return it != m_screens.end() && it->second.visible;
}

UIElement* UISystem::getScreen(const std::string& name) {
    auto it = m_screens.find(name);
    return (it != m_screens.end() && it->second.root) ? it->second.root.get() : nullptr;
}

UIElement* UISystem::findById(const std::string& id) {
    for (auto& [name, entry] : m_screens) {
        if (!entry.visible || !entry.root) continue;
        UIElement* found = entry.root->findById(id);
        if (found) return found;
    }
    return nullptr;
}

UISystem::Stats UISystem::getStats() const {
    Stats stats;
    stats.screenCount = m_screens.size();
    for (const auto& [name, entry] : m_screens) {
        if (entry.visible) {
            stats.visibleScreenCount++;
            if (entry.root) {
                stats.totalElements += countElements(entry.root.get());
            }
        }
    }
    return stats;
}

size_t UISystem::countElements(const UIElement* element) const {
    if (!element) return 0;
    size_t count = 1;
    for (const auto& child : element->getChildren()) {
        count += countElements(child.get());
    }
    return count;
}

std::vector<UISystem::ScreenEntry*> UISystem::getSortedVisibleScreens() {
    std::vector<ScreenEntry*> result;
    for (auto& [name, entry] : m_screens) {
        if (entry.visible) {
            result.push_back(&entry);
        }
    }
    std::sort(result.begin(), result.end(),
        [](const ScreenEntry* a, const ScreenEntry* b) {
            return a->zOrder < b->zOrder;
        });
    return result;
}

} // namespace gloaming
