#pragma once

#include <cmath>

namespace gloaming {

/// 2D vector for positions, sizes, etc.
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2() = default;
    constexpr Vec2(float x, float y) : x(x), y(y) {}

    // Arithmetic operators
    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
    Vec2 operator/(float scalar) const { return {x / scalar, y / scalar}; }
    Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
    Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
    Vec2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }

    // Comparison operators
    bool operator==(const Vec2& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Vec2& other) const { return !(*this == other); }

    // Utility functions
    float length() const { return std::sqrt(x * x + y * y); }
    float lengthSquared() const { return x * x + y * y; }
    Vec2 normalized() const {
        float len = length();
        return len > 0.0f ? Vec2(x / len, y / len) : Vec2();
    }
    static float dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
    static float distance(const Vec2& a, const Vec2& b) { return (b - a).length(); }
};

} // namespace gloaming
