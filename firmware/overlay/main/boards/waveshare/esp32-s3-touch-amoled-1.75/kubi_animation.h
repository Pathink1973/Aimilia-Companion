#pragma once
#include <cstdint>
#include <string_view>

// Nonblocking LVGL adaptation of sleep/wakeup/excited behaviour from
// UNIT-Electronics-MX/unit_kubi_pulsar_desk_pet, software/emo_bmi_rtc_ino.
// Upstream copyright (c) 2025 UNIT-Electronics-MX, MIT; see KUBI_LICENSE.txt.
namespace kubi_animation {
struct Frame {
    std::string_view emotion;
    bool sleep = false;
    bool sparkle = false;
    int jump = 0;
};
class Sequence {
    uint32_t activity_ = 0;
    uint32_t started_ = 0;
    enum Kind { None, Wake, Touch, Shake } kind_ = None;
    bool sleeping_ = false;
public:
    void React(uint32_t now, bool shake) {
        activity_ = started_ = now;
        kind_ = shake ? Shake : (sleeping_ ? Wake : Touch);
        sleeping_ = false;
    }
    Frame Update(uint32_t now, bool idle, bool menu, bool requested, bool moved) {
        if (!idle || menu || requested) {
            activity_ = now;
            sleeping_ = false;
            kind_ = None;
            return {};
        }
        if (moved) {
            activity_ = now;
            if (sleeping_) {
                kind_ = Wake;
                started_ = now;
                sleeping_ = false;
            }
        }
        const uint32_t elapsed = now - started_;
        if (kind_ != None && elapsed >= 4000) kind_ = None;
        if (kind_ == Wake) {
            if (elapsed < 600) return {"surprised"};
            return {"happy", false, true, -static_cast<int>((elapsed / 180) % 3) * 2};
        }
        if (kind_ == Touch) {
            if (elapsed < 1800) return {"embarrassed"};
            return {"happy", false, true, -static_cast<int>((elapsed / 180) % 3) * 2};
        }
        if (kind_ == Shake) {
            if (elapsed < 450) return {"surprised"};
            if (elapsed < 3000) return {"angry"};
            return {"normal"};
        }
        if (now - activity_ >= 60000) {
            sleeping_ = true;
            return {"sleepy", true};
        }
        return {};
    }
};
}
