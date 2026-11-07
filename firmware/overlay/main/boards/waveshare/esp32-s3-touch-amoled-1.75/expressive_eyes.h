#pragma once

#include <algorithm>
#include <string_view>

// Native LVGL eye geometry, inspired by the expressive-eye concept in
// https://github.com/playfultechnology/esp32-eyes. No Arduino renderer dependency.
namespace expressive_eyes {
struct Pose {
    float width = 70;
    float height = 82;
    float lid = 0;
    float slope = 0;
    float asymmetry = 0;
    float y = 0;
};

struct Expression {
    std::string_view name;
    Pose pose;
};
// All 18 canonical emotions are individually designed for the 466px display.
inline constexpr Expression kExpressions[] = {
    {"normal", {70, 82, 0, 0, 0, 0}},         {"angry", {77, 69, 26, 190, 0, 3}},
    {"glee", {80, 18, 0, 0, 0, -16}},         {"happy", {76, 32, 0, -35, 0, -10}},
    {"sad", {72, 65, 20, -180, 0, 10}},       {"worried", {65, 91, 15, -110, 0, 4}},
    {"focused", {78, 49, 12, 90, 0, -4}},     {"annoyed", {76, 40, 17, 0, 8, 5}},
    {"surprised", {84, 108, 0, 0, 0, -4}},    {"skeptic", {68, 69, 15, -80, 26, -2}},
    {"frustrated", {80, 58, 20, 110, 12, 6}}, {"unimpressed", {76, 35, 17, 0, 0, 8}},
    {"sleepy", {70, 23, 9, -30, 7, 14}},      {"suspicious", {65, 52, 19, 50, -19, 0}},
    {"squint", {82, 17, 4, 60, 0, -2}},       {"furious", {82, 49, 27, 290, 0, -5}},
    {"scared", {67, 104, 12, -160, 5, 7}},    {"awe", {78, 98, 0, -40, 13, -13}},
};
inline bool IsCanonical(std::string_view name) {
    for (const auto& expression : kExpressions)
        if (expression.name == name)
            return true;
    return false;
}
inline Pose ForEmotion(std::string_view emotion) {
    if (emotion == "neutral" || emotion == "robot_2")
        emotion = "normal";
    if (emotion == "laughing" || emotion == "funny")
        emotion = "glee";
    if (emotion == "loving" || emotion == "delicious")
        emotion = "happy";
    if (emotion == "crying")
        emotion = "sad";
    if (emotion == "shocked")
        emotion = "surprised";
    if (emotion == "thinking" || emotion == "confused")
        emotion = "skeptic";
    if (emotion == "relaxed")
        emotion = "sleepy";
    if (emotion == "confident" || emotion == "cool")
        emotion = "focused";
    if (emotion == "winking" || emotion == "kissy" || emotion == "silly" ||
        emotion == "embarrassed")
        return {73, 66, 0, 0, 45, -3};
    for (const auto& expression : kExpressions)
        if (expression.name == emotion)
            return expression.pose;
    return {};
}

inline float Approach(float value, float target) { return value + (target - value) * 0.28f; }
inline void Approach(Pose& value, const Pose& target) {
    value.width = Approach(value.width, target.width);
    value.height = Approach(value.height, target.height);
    value.lid = Approach(value.lid, target.lid);
    value.slope = Approach(value.slope, target.slope);
    value.asymmetry = Approach(value.asymmetry, target.asymmetry);
    value.y = Approach(value.y, target.y);
}
inline int EyeHeight(const Pose& pose, int eye, bool blink) {
    if (blink)
        return 7;
    return std::clamp(static_cast<int>(pose.height + (eye ? pose.asymmetry : -pose.asymmetry)), 9,
                      120);
}
}  // namespace expressive_eyes
