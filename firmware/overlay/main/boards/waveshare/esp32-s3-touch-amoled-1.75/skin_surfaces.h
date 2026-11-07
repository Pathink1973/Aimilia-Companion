#pragma once
#include <lvgl.h>
#include <initializer_list>

namespace aimilia_skin {
inline lv_color_t HeaderInk(lv_color_t background) {
    return lv_color_luminance(background) < 128 ? lv_color_white() : lv_color_hex(0x183142);
}
inline void ApplyHeaderInk(lv_color_t background, std::initializer_list<lv_obj_t*> labels) {
    auto ink = HeaderInk(background);
    for (auto label : labels)
        if (label)
            lv_obj_set_style_text_color(label, ink, LV_PART_MAIN);
}
inline int Next(int skin) { return (skin + 1) % 3; }
inline bool IsFormas(int skin) { return skin == 1; }
inline const char* Name(int skin) {
    return skin == 1 ? "Formas" : skin == 2 ? "Classica escura" : "Classica original";
}
// Save the resolved original styles before any skin modifies them. Restore the
// screen AND its opaque children; restoring just the screen loses the head contrast.
class Surfaces {
    struct Saved {
        lv_obj_t* obj = nullptr;
        lv_color_t background{}, gradient{}, text{};
        lv_opa_t opacity = 0;
        lv_grad_dir_t direction = LV_GRAD_DIR_NONE;
        void Capture(lv_obj_t* object) {
            obj = object;
            if (!obj)
                return;
            background = lv_obj_get_style_bg_color(obj, LV_PART_MAIN);
            gradient = lv_obj_get_style_bg_grad_color(obj, LV_PART_MAIN);
            direction = lv_obj_get_style_bg_grad_dir(obj, LV_PART_MAIN);
            opacity = lv_obj_get_style_bg_opa(obj, LV_PART_MAIN);
            text = lv_obj_get_style_text_color(obj, LV_PART_MAIN);
        }
        void Restore() const {
            if (!obj)
                return;
            lv_obj_set_style_bg_color(obj, background, LV_PART_MAIN);
            lv_obj_set_style_bg_grad_color(obj, gradient, LV_PART_MAIN);
            lv_obj_set_style_bg_grad_dir(obj, direction, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(obj, opacity, LV_PART_MAIN);
            lv_obj_set_style_text_color(obj, text, LV_PART_MAIN);
        }
    } saved_[10];

public:
    void Capture(std::initializer_list<lv_obj_t*> objects) {
        int i = 0;
        for (auto obj : objects) {
            if (i == 10)
                break;
            saved_[i++].Capture(obj);
        }
    }
    void Restore() const {
        for (const auto& item : saved_)
            item.Restore();
    }
};
}  // namespace aimilia_skin
