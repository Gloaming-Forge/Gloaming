#pragma once

#include "rendering/IRenderer.hpp"

#include <vector>
#include <queue>
#include <unordered_set>
#include <functional>
#include <cmath>
#include <algorithm>

namespace gloaming {

/// A position on the tile grid
struct TilePos {
    int x = 0;
    int y = 0;

    TilePos() = default;
    TilePos(int x, int y) : x(x), y(y) {}

    bool operator==(const TilePos& other) const { return x == other.x && y == other.y; }
    bool operator!=(const TilePos& other) const { return !(*this == other); }
};

struct TilePosHash {
    std::size_t operator()(const TilePos& pos) const {
        std::size_t h1 = std::hash<int>{}(pos.x);
        std::size_t h2 = std::hash<int>{}(pos.y);
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

/// Result of a pathfinding query
struct PathResult {
    bool found = false;
    std::vector<TilePos> path;     // Sequence of tile positions from start to goal
    int nodesExplored = 0;
};

/// Callback to check if a tile is walkable.
/// Receives tile coordinates, returns true if the tile can be traversed.
using WalkableFunc = std::function<bool(int x, int y)>;

/// Callback to get the movement cost for a tile (for weighted pathfinding).
/// Returns the cost to enter this tile. Default is 1.0 for all tiles.
using TileCostFunc = std::function<float(int x, int y)>;

/// A* pathfinder operating on a 2D tile grid.
/// Supports 4-directional and 8-directional movement.
class Pathfinder {
public:
    /// Configure whether diagonal movement is allowed
    void setAllowDiagonals(bool allow) { m_allowDiagonals = allow; }
    bool getAllowDiagonals() const { return m_allowDiagonals; }

    /// Set the maximum number of nodes to explore before giving up (0 = unlimited)
    void setMaxNodes(int max) { m_maxNodes = max; }
    int getMaxNodes() const { return m_maxNodes; }

    /// Find a path from start to goal using A*.
    /// @param start Starting tile position
    /// @param goal  Target tile position
    /// @param isWalkable Callback that returns true if a tile is passable
    /// @param tileCost Optional callback for weighted tiles (default: all cost 1.0)
    PathResult findPath(TilePos start, TilePos goal,
                        const WalkableFunc& isWalkable,
                        const TileCostFunc& tileCost = nullptr) const {
        PathResult result;

        if (start == goal) {
            result.found = true;
            result.path.push_back(start);
            return result;
        }

        if (!isWalkable(goal.x, goal.y)) {
            return result;  // Goal is unreachable
        }

        // A* open set (priority queue: lowest f-score first)
        struct Node {
            TilePos pos;
            float fScore;
            bool operator>(const Node& other) const { return fScore > other.fScore; }
        };

        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
        std::unordered_map<TilePos, TilePos, TilePosHash> cameFrom;
        std::unordered_map<TilePos, float, TilePosHash> gScore;
        std::unordered_set<TilePos, TilePosHash> closedSet;

        gScore[start] = 0.0f;
        openSet.push({start, heuristic(start, goal)});

        // Direction offsets: 4-directional
        static const int dx4[] = {0, 1, 0, -1};
        static const int dy4[] = {-1, 0, 1, 0};

        // Direction offsets: 8-directional (4 cardinal + 4 diagonal)
        static const int dx8[] = {0, 1, 1, 1, 0, -1, -1, -1};
        static const int dy8[] = {-1, -1, 0, 1, 1, 1, 0, -1};

        const int* dx = m_allowDiagonals ? dx8 : dx4;
        const int* dy = m_allowDiagonals ? dy8 : dy4;
        int dirCount = m_allowDiagonals ? 8 : 4;

        while (!openSet.empty()) {
            Node current = openSet.top();
            openSet.pop();

            if (current.pos == goal) {
                result.found = true;
                result.nodesExplored = static_cast<int>(closedSet.size());
                result.path = reconstructPath(cameFrom, current.pos);
                return result;
            }

            if (closedSet.count(current.pos)) continue;
            closedSet.insert(current.pos);

            if (m_maxNodes > 0 && static_cast<int>(closedSet.size()) > m_maxNodes) {
                break;  // Exceeded search budget
            }

            for (int i = 0; i < dirCount; ++i) {
                TilePos neighbor{current.pos.x + dx[i], current.pos.y + dy[i]};

                if (closedSet.count(neighbor)) continue;
                if (!isWalkable(neighbor.x, neighbor.y)) continue;

                // For diagonal movement, check that both adjacent cardinal tiles are walkable
                // (prevents cutting corners through walls)
                if (m_allowDiagonals && i >= 4) {
                    bool cardinalXWalkable = isWalkable(current.pos.x + dx[i], current.pos.y);
                    bool cardinalYWalkable = isWalkable(current.pos.x, current.pos.y + dy[i]);
                    if (!cardinalXWalkable || !cardinalYWalkable) continue;
                }

                float moveCost = (m_allowDiagonals && i >= 4) ? 1.414f : 1.0f;
                if (tileCost) {
                    moveCost *= tileCost(neighbor.x, neighbor.y);
                }

                float tentativeG = gScore[current.pos] + moveCost;

                auto it = gScore.find(neighbor);
                if (it != gScore.end() && tentativeG >= it->second) continue;

                cameFrom[neighbor] = current.pos;
                gScore[neighbor] = tentativeG;
                float f = tentativeG + heuristic(neighbor, goal);
                openSet.push({neighbor, f});
            }
        }

        result.nodesExplored = static_cast<int>(closedSet.size());
        return result;
    }

    /// Quick reachability check — is there any path between two points?
    /// Uses BFS with a limited search radius for efficiency.
    bool isReachable(TilePos start, TilePos goal,
                     const WalkableFunc& isWalkable, int maxDistance = 100) const {
        if (start == goal) return true;
        if (!isWalkable(goal.x, goal.y)) return false;

        std::queue<TilePos> queue;
        std::unordered_set<TilePos, TilePosHash> visited;

        queue.push(start);
        visited.insert(start);

        while (!queue.empty()) {
            TilePos current = queue.front();
            queue.pop();

            // Manhattan distance cutoff
            if (std::abs(current.x - start.x) + std::abs(current.y - start.y) > maxDistance) {
                continue;
            }

            static const int dx[] = {0, 1, 0, -1};
            static const int dy[] = {-1, 0, 1, 0};

            for (int i = 0; i < 4; ++i) {
                TilePos neighbor{current.x + dx[i], current.y + dy[i]};
                if (neighbor == goal) return true;
                if (visited.count(neighbor)) continue;
                if (!isWalkable(neighbor.x, neighbor.y)) continue;
                visited.insert(neighbor);
                queue.push(neighbor);
            }
        }
        return false;
    }

private:
    float heuristic(TilePos a, TilePos b) const {
        if (m_allowDiagonals) {
            // Octile distance (diagonal heuristic)
            float dx = static_cast<float>(std::abs(a.x - b.x));
            float dy = static_cast<float>(std::abs(a.y - b.y));
            return std::max(dx, dy) + 0.414f * std::min(dx, dy);
        }
        // Manhattan distance
        return static_cast<float>(std::abs(a.x - b.x) + std::abs(a.y - b.y));
    }

    std::vector<TilePos> reconstructPath(
        const std::unordered_map<TilePos, TilePos, TilePosHash>& cameFrom,
        TilePos current) const {
        std::vector<TilePos> path;
        path.push_back(current);
        while (cameFrom.count(current)) {
            current = cameFrom.at(current);
            path.push_back(current);
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

    bool m_allowDiagonals = false;
    int m_maxNodes = 5000;
};

} // namespace gloaming
