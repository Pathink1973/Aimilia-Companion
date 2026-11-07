#pragma once
#include <string.h>
#include "lvgl.h"
// Shared by the board view and the host screenshot harness; no application state.
static void aimilia_button_style(lv_obj_t* b) {
    lv_obj_set_style_bg_color(b, lv_color_hex(0x195A80), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_grad_color(b, lv_color_hex(0x14354D), 0);
    lv_obj_set_style_bg_grad_dir(b, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(0x368EB1), 0);
    lv_obj_set_style_radius(b, 18, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_style_pad_all(b, 0, 0);
    lv_obj_set_style_text_color(b, lv_color_hex(0xF0FAFF), 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x31516D), LV_STATE_PRESSED);
}
static lv_obj_t* aimilia_menu_text(lv_obj_t* p, const char* t, int y, int width, const lv_font_t* f,
                                   uint32_t c) {
    lv_obj_t* l = lv_label_create(p);
    lv_label_set_text(l, t);
    lv_obj_set_width(l, width);
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(l, f, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(c), 0);
    lv_obj_align(l, LV_ALIGN_TOP_MID, 0, y);
    return l;
}

static void vivid_icon(lv_obj_t* p, int kind, int x, int y) {
    static const lv_point_precise_t speaker[] = {{0, 8},  {6, 8},  {13, 1}, {13, 25},
                                                 {6, 18}, {0, 18}, {0, 8}};
    static const lv_point_precise_t rays[][2] = {
        {{13, 0}, {13, 4}}, {{13, 22}, {13, 26}}, {{0, 13}, {4, 13}}, {{22, 13}, {26, 13}},
        {{3, 3}, {6, 6}},   {{20, 20}, {23, 23}}, {{3, 23}, {6, 20}}, {{20, 6}, {23, 3}}};
    if (kind == 0) {
        auto l = lv_line_create(p);
        lv_line_set_points(l, speaker, 7);
        lv_obj_set_pos(l, x, y);
        lv_obj_set_style_line_color(l, lv_color_hex(0x8CEBFF), 0);
        lv_obj_set_style_line_width(l, 2, 0);
    }
    if (kind == 1) {
        auto c = lv_obj_create(p);
        lv_obj_remove_style_all(c);
        lv_obj_set_size(c, 12, 12);
        lv_obj_remove_flag(c, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(c, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_pos(c, x + 7, y + 7);
        lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(c, 2, 0);
        lv_obj_set_style_border_color(c, lv_color_hex(0x8CEBFF), 0);
        for (int i = 0; i < 8; i++) {
            auto l = lv_line_create(p);
            lv_line_set_points(l, rays[i], 2);
            lv_obj_set_pos(l, x, y);
            lv_obj_set_style_line_color(l, lv_color_hex(0x8CEBFF), 0);
            lv_obj_set_style_line_width(l, 2, 0);
        }
    }
    if (kind == 2) {
        for (int i = 0; i < 3; i++) {
            auto a = lv_arc_create(p);
            lv_obj_remove_style_all(a);
            lv_obj_set_size(a, 12 + i * 12, 12 + i * 12);
            lv_obj_set_pos(a, x + 18 - i * 6, y + 18 - i * 6);
            lv_arc_set_angles(a, 220, 320);
            lv_obj_set_style_arc_width(a, 2, LV_PART_INDICATOR);
            lv_obj_set_style_arc_color(a, lv_color_hex(0x8CEBFF), LV_PART_INDICATOR);
            lv_obj_remove_style(a, nullptr, LV_PART_KNOB);
            lv_obj_remove_flag(a, LV_OBJ_FLAG_CLICKABLE);
        }
    }
}
static void aimilia_meter(lv_obj_t* p, const char* title, int value, const lv_font_t* normal,
                          const lv_font_t* large) {
    auto label = aimilia_menu_text(p, title, 0, 280, normal, 0xBFD7EB);
    lv_obj_set_width(label, 200);
    lv_obj_set_align(label, LV_ALIGN_TOP_LEFT);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_pos(label, strcmp(title, "Som") == 0 ? 160 : 102, 0);
    vivid_icon(p, strcmp(title, "Som") == 0 ? 0 : 1, strcmp(title, "Som") == 0 ? 124 : 62, 0);
    char text[12];
    lv_snprintf(text, sizeof(text), "%d%%", value);
    aimilia_menu_text(p, text, 35, 280, large, 0xEDF5FF);
    lv_obj_t* bar = lv_obj_create(p);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, 220, 12);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 90);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x24354A), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bar, 4, 0);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t* fill = lv_obj_create(bar);
    lv_obj_remove_style_all(fill);
    lv_obj_set_size(fill, 220 * value / 100, 12);
    lv_obj_set_style_bg_color(fill, lv_color_hex(0x25BEF4), 0);
    lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_grad_color(fill, lv_color_hex(0x92F8E0), 0);
    lv_obj_set_style_bg_grad_dir(fill, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(fill, 4, 0);
    lv_obj_remove_flag(fill, LV_OBJ_FLAG_CLICKABLE);
}
