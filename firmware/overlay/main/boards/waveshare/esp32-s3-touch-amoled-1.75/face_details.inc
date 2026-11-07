// Board-owned vector details. Persistent buffers; no audio/network changes.
lv_obj_t* face_brows_[2] = {};
lv_obj_t* face_blush_[2] = {};
lv_obj_t* face_tears_[2] = {};
lv_point_precise_t brow_points_[2][2]{};

void InitFaceDetails() {
    for (int i = 0; i < 2; ++i) {
        face_brows_[i] = lv_line_create(face_);
        lv_obj_set_style_line_width(face_brows_[i], 3, 0);
        lv_obj_set_style_line_color(face_brows_[i], lv_color_hex(0x80D8FF), 0);
        lv_obj_set_style_line_rounded(face_brows_[i], true, 0);
        lv_obj_remove_flag(face_brows_[i], LV_OBJ_FLAG_CLICKABLE);
        for (auto slot : {&face_blush_[i], &face_tears_[i]}) {
            *slot = lv_obj_create(face_);
            lv_obj_remove_style_all(*slot);
            lv_obj_remove_flag(*slot, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_remove_flag(*slot, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_radius(*slot, 5, 0);
        }
        lv_obj_set_size(face_blush_[i], 18, 6);
        lv_obj_set_style_bg_color(face_blush_[i], lv_color_hex(0xFF96B4), 0);
        lv_obj_set_size(face_tears_[i], 4, 9);
        lv_obj_set_style_bg_color(face_tears_[i], lv_color_hex(0x80D8FF), 0);
    }
}

void UpdateFaceDetails(std::string_view emotion, uint32_t now, bool blink) {
    if (menu_open_)
        return;
    bool happy = emotion == "happy" || emotion == "glee" || emotion == "loving" ||
                 emotion == "embarrassed" || emotion == "laughing";
    bool sad = emotion == "sad" || emotion == "crying";
    float angle = std::clamp(pose_.slope / 10.f, -22.f, 22.f);
    for (int i = 0; i < 2; ++i) {
        int height = expressive_eyes::EyeHeight(pose_, i, blink);
        int x = (i ? 65 : -65) + static_cast<int>(look_x_);
        int y = static_cast<int>(pose_.y + look_y_);
        int delta = static_cast<int>(std::sin(angle * .0174533f) * 10) * (i ? -1 : 1);
        brow_points_[i][0] = {0, static_cast<lv_value_precise_t>(12 + delta)};
        brow_points_[i][1] = {40, static_cast<lv_value_precise_t>(12 - delta)};
        lv_line_set_points(face_brows_[i], brow_points_[i], 2);
        lv_obj_align(face_brows_[i], LV_ALIGN_CENTER, x, y - height / 2 - 15);
        lv_obj_align(face_blush_[i], LV_ALIGN_CENTER, x, y + height / 2 + 12);
        lv_obj_set_style_bg_opa(face_blush_[i], happy ? 140 : 0, 0);
        lv_obj_align(face_tears_[i], LV_ALIGN_CENTER, x + (i ? 25 : -25),
                     y + height / 2 + 8 + (now / 45 + i * 9) % 19);
        lv_obj_set_style_bg_opa(face_tears_[i], sad ? 180 : 0, 0);
    }
}
