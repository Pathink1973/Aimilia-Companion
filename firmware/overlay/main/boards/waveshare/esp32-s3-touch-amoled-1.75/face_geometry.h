#pragma once
#include <cmath>
namespace aimilia_face {
struct Point {
    float x, y;
};
// |x/rx|^n + |y/ry|^n = 1. n=2 gives an ellipse; n=2.7 rounds a square.
inline Point Superellipse(float t, float rx, float ry, float n = 2.7f) {
    return {rx * std::copysign(std::pow(std::abs(std::cos(t)), 2.f / n), std::cos(t)),
            ry * std::copysign(std::pow(std::abs(std::sin(t)), 2.f / n), std::sin(t))};
}
struct Spring {
    float value = 0, velocity = 0;
    void Step(float target, float dt) {
        // Bounded 20fps host step. Physical integration is separate from voice tasks.
        dt = std::fmin(dt, 0.05f);
        velocity += (100.f * (target - value) - 16.f * velocity) * dt;
        value += velocity * dt;
    }
};
}  // namespace aimilia_face
