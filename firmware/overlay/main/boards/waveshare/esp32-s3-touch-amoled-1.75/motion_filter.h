#pragma once
#include <cstdint>

class MotionFilter {
    float last_[3] = {};
    bool initialized_ = false;
    unsigned hits_ = 0;
    int64_t window_ = 0;
    int64_t last_reaction_ = -12000;

public:
    bool Observe(float x, float y, float z, int64_t now_ms) {
        if (!initialized_) {
            last_[0] = x;
            last_[1] = y;
            last_[2] = z;
            initialized_ = true;
            return false;
        }
        float dx = x - last_[0], dy = y - last_[1], dz = z - last_[2];
        last_[0] = x;
        last_[1] = y;
        last_[2] = z;
        if (now_ms - window_ > 900) {
            hits_ = 0;
            window_ = now_ms;
        }
        if (dx * dx + dy * dy + dz * dz > 1.44f)
            ++hits_;
        if (hits_ >= 3 && now_ms - last_reaction_ >= 12000) {
            hits_ = 0;
            last_reaction_ = now_ms;
            return true;
        }
        return false;
    }
};
