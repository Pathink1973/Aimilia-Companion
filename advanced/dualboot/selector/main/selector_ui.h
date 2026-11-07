#pragma once
#include "lvgl.h"
// Pure LVGL view. Boot selection remains in main.c.
static lv_obj_t *selector_text(lv_obj_t *p, const char *t, const lv_font_t *f,
                               uint32_t c, int x, int y) {
  lv_obj_t *l = lv_label_create(p);
  lv_label_set_text(l, t);
  lv_obj_set_style_text_font(l, f, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(c), 0);
  lv_obj_set_pos(l, x, y);
  return l;
}
static void selector_card(lv_obj_t *p, const char *title, const char *detail,
                          int y, int selection, uint32_t accent,
                          lv_event_cb_t cb) {
  lv_obj_t *b = lv_button_create(p);
  lv_obj_set_size(b, 310, 88);
  lv_obj_set_pos(b, 78, y);
  lv_obj_set_style_bg_color(
      b, lv_color_hex(selection == 2 ? 0x155E8E : 0x513689), 0);
  lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_grad_color(
      b, lv_color_hex(selection == 2 ? 0x13334F : 0x282441), 0);
  lv_obj_set_style_bg_grad_dir(b, LV_GRAD_DIR_HOR, 0);
  lv_obj_set_style_border_width(b, 1, 0);
  lv_obj_set_style_border_color(
      b, lv_color_hex(selection == 2 ? 0x39C5F6 : 0xA78AF4), 0);
  lv_obj_set_style_radius(b, 24, 0);
  lv_obj_set_style_pad_all(b, 0, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_set_style_border_color(b, lv_color_hex(accent), LV_STATE_PRESSED);
  lv_obj_set_style_bg_color(b, lv_color_hex(0x22334A), LV_STATE_PRESSED);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, (void *)(uintptr_t)selection);
  lv_obj_t *icon = lv_obj_create(b);
  lv_obj_remove_style_all(icon);
  lv_obj_set_size(icon, 58, 58);
  lv_obj_set_pos(icon, 13, 15);
  lv_obj_set_style_bg_color(
      icon, lv_color_hex(selection == 2 ? 0x082B45 : 0x221B40), 0);
  lv_obj_set_style_bg_opa(icon, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(icon, 18, 0);
  lv_obj_remove_flag(icon, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(icon, LV_OBJ_FLAG_SCROLLABLE);
  for (int i = 0; i < 2; i++) {
    lv_obj_t *eye = lv_obj_create(icon);
    lv_obj_remove_style_all(eye);
    lv_obj_set_size(eye, 9, 20);
    lv_obj_set_pos(eye, 16 + i * 18, 19);
    lv_obj_set_style_bg_color(eye, lv_color_hex(accent), 0);
    lv_obj_set_style_bg_opa(eye, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(eye, 4, 0);
    lv_obj_remove_flag(eye, LV_OBJ_FLAG_CLICKABLE);
  }
  selector_text(b, title, &lv_font_montserrat_28, 0xEDF5FF, 84, 13);
  selector_text(b, detail, &lv_font_montserrat_14, 0xD0DEEF, 85, 51);
  selector_text(b, LV_SYMBOL_RIGHT, &lv_font_montserrat_20, accent, 278, 33);
}
static void selector_build_ui(lv_obj_t *screen, lv_event_cb_t cb,
                              lv_obj_t **status) {
  lv_obj_set_style_bg_color(screen, lv_color_hex(0x080E18), 0);
  lv_obj_set_style_text_color(screen, lv_color_hex(0xEDF5FF), 0);
  lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *l = selector_text(screen, "O TEU COMPANHEIRO",
                              &lv_font_montserrat_14, 0x80D8FF, 0, 62);
  lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 62);
  l = selector_text(screen, "Vamos conversar?", &lv_font_montserrat_24,
                    0xEDF5FF, 0, 90);
  lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 90);
  selector_card(screen, "Aimilia", "Conversa e companhia", 142, 2, 0x80D8FF,
                cb);
  selector_card(screen, "Muse", "O teu assistente", 244, 1, 0xB7A1F8, cb);
  *status = selector_text(screen, "Toca para entrar", &lv_font_montserrat_14,
                          0x9CAEC6, 0, 365);
  lv_obj_align(*status, LV_ALIGN_TOP_MID, 0, 365);
  l = selector_text(screen, "Dois cliques: voltar ao inicio",
                    &lv_font_montserrat_14, 0x657A97, 0, 395);
  lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 395);
}
