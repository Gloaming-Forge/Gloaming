#pragma once

#include <entt/entt.hpp>

#include <vector>
#include <functional>
#include <string>
#include <unordered_map>

namespace gloaming {

/// Entity handle - wrapper around EnTT entity for convenience
using Entity = entt::entity;

/// Null entity constant
constexpr Entity NullEntity = entt::null;

/// Entity registry wrapper providing convenient access to EnTT functionality
class Registry {
public:
    Registry() = default;
    ~Registry() = default;

    // Non-copyable, movable
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) = default;
    Registry& operator=(Registry&&) = default;

    /// Create a new entity
    Entity create() {
        return m_registry.create();
    }

    /// Create a new entity with given components
    template<typename... Components>
    Entity create(Components&&... components) {
        Entity entity = m_registry.create();
        (m_registry.emplace<std::decay_t<Components>>(entity, std::forward<Components>(components)), ...);
        return entity;
    }

    /// Destroy an entity
    void destroy(Entity entity) {
        if (valid(entity)) {
            m_registry.destroy(entity);
        }
    }

    /// Destroy all entities matching a condition
    template<typename Func>
    void destroyIf(Func&& predicate) {
        std::vector<Entity> toDestroy;
        auto& storage = m_registry.storage<entt::entity>();
        for (auto it = storage.begin(); it != storage.end(); ++it) {
            Entity entity = *it;
            if (predicate(entity)) {
                toDestroy.push_back(entity);
            }
        }
        for (Entity entity : toDestroy) {
            destroy(entity);
        }
    }

    /// Check if entity is valid
    bool valid(Entity entity) const {
        return m_registry.valid(entity);
    }

    /// Add a component to an entity
    template<typename Component, typename... Args>
    Component& add(Entity entity, Args&&... args) {
        return m_registry.emplace<Component>(entity, std::forward<Args>(args)...);
    }

    /// Add or replace a component on an entity
    template<typename Component, typename... Args>
    Component& addOrReplace(Entity entity, Args&&... args) {
        return m_registry.emplace_or_replace<Component>(entity, std::forward<Args>(args)...);
    }

    /// Remove a component from an entity
    template<typename Component>
    void remove(Entity entity) {
        if (has<Component>(entity)) {
            m_registry.remove<Component>(entity);
        }
    }

    /// Get a component from an entity (returns nullptr if not present)
    template<typename Component>
    Component* tryGet(Entity entity) {
        return m_registry.try_get<Component>(entity);
    }

    /// Get a component from an entity (const version)
    template<typename Component>
    const Component* tryGet(Entity entity) const {
        return m_registry.try_get<Component>(entity);
    }

    /// Get a component from an entity (assumes it exists)
    template<typename Component>
    Component& get(Entity entity) {
        return m_registry.get<Component>(entity);
    }

    /// Get a component from an entity (const version, assumes it exists)
    template<typename Component>
    const Component& get(Entity entity) const {
        return m_registry.get<Component>(entity);
    }

    /// Get multiple components from an entity
    template<typename... Components>
    std::tuple<Components&...> getMultiple(Entity entity) {
        return m_registry.get<Components...>(entity);
    }

    /// Check if entity has a component
    template<typename Component>
    bool has(Entity entity) const {
        return m_registry.all_of<Component>(entity);
    }

    /// Check if entity has all specified components
    template<typename... Components>
    bool hasAll(Entity entity) const {
        return m_registry.all_of<Components...>(entity);
    }

    /// Check if entity has any of specified components
    template<typename... Components>
    bool hasAny(Entity entity) const {
        return m_registry.any_of<Components...>(entity);
    }

    /// Get a view of entities with specified components
    template<typename... Components>
    auto view() {
        return m_registry.view<Components...>();
    }

    /// Get a view of entities with specified components (const)
    template<typename... Components>
    auto view() const {
        return m_registry.view<Components...>();
    }

    /// Iterate over all entities with specified components
    template<typename... Components, typename Func>
    void each(Func&& func) {
        m_registry.view<Components...>().each(std::forward<Func>(func));
    }

    /// Iterate over all entities
    template<typename Func>
    void eachEntity(Func&& func) {
        auto& storage = m_registry.storage<entt::entity>();
        for (auto it = storage.begin(); it != storage.end(); ++it) {
            func(*it);
        }
    }

    /// Get count of entities with specific components
    template<typename... Components>
    size_t count() const {
        size_t n = 0;
        for ([[maybe_unused]] auto _ : m_registry.view<Components...>()) {
            ++n;
        }
        return n;
    }

    /// Get total entity count (includes destroyed but not yet recycled slots)
    size_t size() const {
        auto* storage = m_registry.storage<entt::entity>();
        return storage ? storage->size() : 0;
    }

    /// Get alive entity count (not counting destroyed slots)
    size_t alive() const {
        auto* storage = m_registry.storage<entt::entity>();
        return storage ? storage->free_list() : 0;
    }

    /// Check if registry is empty (no alive entities)
    bool empty() const {
        auto* storage = m_registry.storage<entt::entity>();
        return !storage || storage->free_list() == 0;
    }

    /// Clear all entities and components
    void clear() {
        m_registry.clear();
    }

    /// Sort entities with a component
    template<typename Component, typename Compare>
    void sort(Compare compare) {
        m_registry.sort<Component>(compare);
    }

    /// Get the underlying EnTT registry (for advanced usage)
    entt::registry& raw() { return m_registry; }
    const entt::registry& raw() const { return m_registry; }

    /// Find first entity matching a predicate
    template<typename... Components, typename Func>
    Entity findFirst(Func&& predicate) {
        for (auto entity : m_registry.view<Components...>()) {
            if (predicate(entity)) {
                return entity;
            }
        }
        return NullEntity;
    }

    /// Find all entities matching a predicate
    template<typename... Components, typename Func>
    std::vector<Entity> findAll(Func&& predicate) {
        std::vector<Entity> results;
        for (auto entity : m_registry.view<Components...>()) {
            if (predicate(entity)) {
                results.push_back(entity);
            }
        }
        return results;
    }

    /// Collect all entities with specified components
    template<typename... Components>
    std::vector<Entity> collect() {
        std::vector<Entity> results;
        for (auto entity : m_registry.view<Components...>()) {
            results.push_back(entity);
        }
        return results;
    }

private:
    entt::registry m_registry;
};

} // namespace gloaming
