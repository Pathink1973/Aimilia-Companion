#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string_view>

// Board-owned, bounded ARGB raster. No heap allocation, LVGL, network or audio calls.
namespace aimilia_formas {
constexpr int kSize = 320;
constexpr size_t kPixels = kSize * kSize;
constexpr uint32_t kColors[] = {0x88D1FF, 0xF6A6C8, 0xB5A0F6, 0xFFCD58, 0x80DBB2, 0xFFAD72};
constexpr const char* kShapes[] = {"Trevo", "Bolinha", "Flor", "Coracao", "Estrela", "Almofadinha"};
constexpr const char* kBackgrounds[] = {"Aurora",  "Preto AMOLED", "Azul noite",
                                        "Pessego", "Jardim",       "Creme"};
constexpr const char* kColorNames[] = {"Azul ceu", "Rosa", "Lilas", "Amarelo", "Menta", "Pessego"};
constexpr uint32_t kBgTop[] = {0x101C3E, 0x000000, 0x071329, 0xFFF0D9, 0x0D312E, 0xF4EDDF};
constexpr uint32_t kBgBottom[] = {0x49396C, 0x000000, 0x142C4A, 0xE8B9CF, 0x48735B, 0xF4EDDF};
inline int Choice(int value) { return std::clamp(value, 0, 5); }
struct Face {
    float w, h, left, right, brow, curve, opening;
};
constexpr Face kFaces[] = {
    {15, 24, 0, 0, 0, 15, 0},       {19, 16, 12, -12, 12, -13, 0}, {21, 6, 0, 0, -3, 30, 23},
    {20, 8, 0, 0, 0, 29, 0},        {16, 18, -7, 7, -10, -21, 0},  {15, 23, -4, 4, -15, -12, 0},
    {16, 13, 2, -2, 3, 0, 0},       {20, 12, 6, -6, 8, -5, 0},     {19, 32, 0, 0, -3, 0, 22},
    {19, 13, 0, -9, -13, -3, 0},    {20, 11, 10, -10, 11, -17, 0}, {21, 10, 0, 0, 0, -1, 0},
    {20, 4, 0, 0, 0, 5, 0},         {20, 10, -6, 6, -4, -5, 0},    {23, 5, 0, 0, 0, 10, 0},
    {22, 14, 18, -18, 17, -22, 12}, {15, 32, -5, 5, -16, 0, 14},   {24, 31, 0, 0, -8, 0, 14}};
constexpr std::string_view kNames[] = {
    "normal",  "angry",      "glee",      "happy",   "sad",        "worried",
    "focused", "annoyed",    "surprised", "skeptic", "frustrated", "unimpressed",
    "sleepy",  "suspicious", "squint",    "furious", "scared",     "awe"};
inline int Emotion(std::string_view e) {
    for (int i = 0; i < 18; ++i)
        if (e == kNames[i])
            return i;
    if (e == "laughing" || e == "funny")
        return 2;
    if (e == "loving" || e == "delicious" || e == "embarrassed" || e == "silly" || e == "winking" ||
        e == "kissy")
        return 3;
    if (e == "crying")
        return 4;
    if (e == "thinking" || e == "confused")
        return 9;
    if (e == "shocked")
        return 8;
    if (e == "relaxed")
        return 12;
    if (e == "confident" || e == "cool")
        return 6;
    return 0;
}
class Raster {
    uint32_t* pixels_;
    float offset_x_ = 0, offset_y_ = 0;

public:
    explicit Raster(uint32_t* pixels) : pixels_(pixels) {}
    void Offset(float x, float y) {
        offset_x_ = x;
        offset_y_ = y;
    }
    void Pixel(int x, int y, uint32_t color) {
        if (x >= 0 && x < kSize && y >= 0 && y < kSize)
            pixels_[y * kSize + x] = 0xFF000000 | color;
    }
    void Ellipse(float x, float y, float rx, float ry, uint32_t color) {
        x = 160 + (x + offset_x_) * .86f;
        y = 150 + (y + offset_y_) * .86f;
        rx = std::max(.6f, rx * .86f);
        ry = std::max(.6f, ry * .86f);
        for (int py = std::max(0, int(y - ry)); py <= std::min(kSize - 1, int(y + ry + 1)); ++py) {
            float d = (py + .5f - y) / ry;
            if (std::abs(d) > 1)
                continue;
            float span = rx * std::sqrt(1 - d * d);
            for (int px = std::max(0, int(std::ceil(x - span)));
                 px <= std::min(kSize - 1, int(x + span)); ++px)
                Pixel(px, py, color);
        }
    }
    void Line(float x1, float y1, float x2, float y2, float width, uint32_t color) {
        int n = std::max(1, int(std::hypot(x2 - x1, y2 - y1) / 2));
        for (int i = 0; i <= n; ++i) {
            float t = float(i) / n;
            Ellipse(x1 + (x2 - x1) * t, y1 + (y2 - y1) * t, width / 2, width / 2, color);
        }
    }
    void Curve(float x, float y, float width, float bend, uint32_t color, float stroke = 8) {
        float px = x - width, py = y;
        for (int i = 1; i <= 24; ++i) {
            float t = float(i) / 24, nx = x - width + 2 * width * t,
                  ny = y + 2 * (1 - t) * t * bend;
            Line(px, py, nx, ny, stroke, color);
            px = nx;
            py = ny;
        }
    }
    void Body(int shape, uint32_t color) {
        std::memset(pixels_, 0, kPixels * 4);
        float xs[128], ys[128];
        for (int i = 0; i < 128; ++i) {
            float a = i * 6.2831853f / 128, r = 147;
            if (shape == 0)
                r = 143 + 25 * std::cos(4 * (a - .785398f));
            if (shape == 2)
                r = 144 + 24 * std::cos(5 * (a + 1.570796f));
            if (shape == 4)
                r = 144 + 24 * std::cos(7 * (a + 1.570796f));
            float x = std::cos(a) * r, y = std::sin(a) * r;
            if (shape == 3) {
                x = 150 * std::pow(std::sin(a), 3);
                y = -(112 * std::cos(a) - 45 * std::cos(2 * a) - 18 * std::cos(3 * a) -
                      9 * std::cos(4 * a));
            }
            if (shape == 5) {
                x = 147 * std::copysign(std::pow(std::abs(std::cos(a)), .5f), std::cos(a));
                y = 137 * std::copysign(std::pow(std::abs(std::sin(a)), .5f), std::sin(a));
            }
            xs[i] = 160 + x * .86f;
            ys[i] = 150 + y * .86f;
        }
        for (int y = 0; y < kSize; ++y) {
            float crossings[128];
            int n = 0;
            for (int i = 0, j = 127; i < 128; j = i++)
                if ((ys[i] > y + .5f) != (ys[j] > y + .5f))
                    crossings[n++] = xs[i] + (y + .5f - ys[i]) * (xs[j] - xs[i]) / (ys[j] - ys[i]);
            std::sort(crossings, crossings + n);
            for (int i = 0; i + 1 < n; i += 2)
                for (int x = std::max(0, int(std::ceil(crossings[i])));
                     x <= std::min(kSize - 1, int(crossings[i + 1])); ++x)
                    Pixel(x, y, color);
        }
    }
    void Draw(int expression, bool blink, float look, float mouth, uint32_t now, bool shy) {
        int e = std::clamp(expression, 0, 17);
        auto f = kFaces[e];
        const uint32_t ink = 0x183142;
        bool cheerful = e == 2 || e == 3 || shy;
        if (cheerful || e == 17) {
            Ellipse(-83, 37, 25, 10, 0xEE99B0);
            Ellipse(83, 37, 25, 10, 0xEE99B0);
        }
        for (int i = 0; i < 2; ++i) {
            float x = i ? 54 : -54, h = (e == 9 && i) ? 25 : f.h, slope = i ? f.right : f.left;
            if (blink || e == 12 || cheerful)
                Curve(x, -7, 19, (blink || e == 12) ? 12 : -22, ink);
            else {
                Ellipse(x + look, -12, f.w, h, ink);
                // Upper lid is cut with body colour by the caller's base pixels.
                for (int py = std::max(0, int(150 + (-12 - h) * .86f) - 2);
                     py < int(150 + (-12 + h) * .86f); ++py) {
                    for (int px = std::max(0, int(160 + (x + look - f.w - 2) * .86f));
                         px < std::min(kSize, int(160 + (x + look + f.w + 2) * .86f)); ++px) {
                        float wx = (px - 160) / .86f - x - look, wy = (py - 150) / .86f;
                        if (wy < -12 - h + std::abs(slope) - wx * slope / std::max(1.f, f.w))
                            pixels_[py * kSize + px] = base_[py * kSize + px];
                    }
                }
                if (h > 18)
                    Ellipse(x + look - 4, -12 - h + 9, 4, 5, 0xFFFFFF);
            }
            if (e != 0 && !cheerful && e != 12) {
                float s = (i ? -1 : 1) * f.brow, y = -12 - h - 14;
                Line(x - 22, y - s, x + 22, y + s, 6, ink);
            }
        }
        float opening = std::max(f.opening, mouth * .24f);
        if (opening > 3) {
            if (cheerful || mouth > 4) {
                Ellipse(0, 52, 29, std::min(28.f, opening), ink);
                Ellipse(0, 57 + opening * .4f, 14, 5, 0xF58C9C);
            } else
                Ellipse(0, 51, e == 17 ? 12 : 16, opening, ink);
        } else
            Curve(0, 46, 31, shy ? 24 : f.curve, ink);
        if (e == 4)
            Ellipse(81, 35 + (now / 80 % 12), 6, 11, 0xE9F8FF);
        if (e == 5 || e == 16)
            Ellipse(91, -44, 6, 11, 0xE9F8FF);
        if (e == 17)
            for (float x : {-85.f, 85.f}) {
                Line(x, -61, x, -45, 4, 0xFFF6C7);
                Line(x - 8, -53, x + 8, -53, 4, 0xFFF6C7);
            }
    }
    const uint32_t* base_ = nullptr;
};
}  // namespace aimilia_formas
