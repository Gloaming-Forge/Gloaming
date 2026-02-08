#include "gameplay/EntitySpawning.hpp"

#include <algorithm>

namespace gloaming {

std::vector<EntityQueryResult> EntitySpawning::findInRadius(
        float cx, float cy, float radius,
        const EntityQueryFilter& filter) const {

    if (!m_registry) return {};

    float radiusSq = radius * radius;
    std::vector<EntityQueryResult> results;

    m_registry->each<Transform>([&](Entity entity, const Transform& transform) {
        // Type filter
        if (!filter.type.empty()) {
            if (!m_registry->has<Name>(entity)) return;
            if (m_registry->get<Name>(entity).type != filter.type) return;
        }

        // Layer filter
        if (filter.requiredLayer != 0) {
            if (!m_registry->has<Collider>(entity)) return;
            if ((m_registry->get<Collider>(entity).layer & filter.requiredLayer) == 0) return;
        }

        // Dead filter
        if (filter.excludeDead && m_registry->has<Health>(entity)) {
            if (m_registry->get<Health>(entity).isDead()) return;
        }

        float dx = transform.position.x - cx;
        float dy = transform.position.y - cy;
        float distSq = dx * dx + dy * dy;

        if (distSq <= radiusSq) {
            EntityQueryResult result;
            result.entity = entity;
            result.distance = std::sqrt(distSq);
            result.position = transform.position;
            results.push_back(result);
        }
    });

    // Sort by distance (nearest first)
    std::sort(results.begin(), results.end(),
        [](const EntityQueryResult& a, const EntityQueryResult& b) {
            return a.distance < b.distance;
        });

    return results;
}

EntityQueryResult EntitySpawning::findNearest(
        float cx, float cy, float maxRadius,
        const EntityQueryFilter& filter) const {

    if (!m_registry) return EntityQueryResult{};

    float maxRadiusSq = maxRadius * maxRadius;
    float bestDistSq = maxRadiusSq + 1.0f;
    EntityQueryResult best;

    m_registry->each<Transform>([&](Entity entity, const Transform& transform) {
        if (!filter.type.empty()) {
            if (!m_registry->has<Name>(entity)) return;
            if (m_registry->get<Name>(entity).type != filter.type) return;
        }
        if (filter.requiredLayer != 0) {
            if (!m_registry->has<Collider>(entity)) return;
            if ((m_registry->get<Collider>(entity).layer & filter.requiredLayer) == 0) return;
        }
        if (filter.excludeDead && m_registry->has<Health>(entity)) {
            if (m_registry->get<Health>(entity).isDead()) return;
        }

        float dx = transform.position.x - cx;
        float dy = transform.position.y - cy;
        float distSq = dx * dx + dy * dy;

        if (distSq <= maxRadiusSq && distSq < bestDistSq) {
            bestDistSq = distSq;
            best.entity = entity;
            best.distance = std::sqrt(distSq);
            best.position = transform.position;
        }
    });

    return best;
}

size_t EntitySpawning::countByType(const std::string& type) const {
    if (!m_registry) return 0;

    size_t count = 0;
    m_registry->each<Name>([&](Entity, const Name& name) {
        if (name.type == type) count++;
    });
    return count;
}

} // namespace gloaming
