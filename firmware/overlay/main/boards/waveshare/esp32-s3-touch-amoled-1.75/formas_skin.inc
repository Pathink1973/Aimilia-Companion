// Included inside CustomLcdDisplay. A fixed pair of PSRAM buffers is allocated once.
int skin_ = 1, shape_ = 5, background_ = 5, skin_color_ = 5;
aimilia_skin::Surfaces classic_surfaces_;
uint32_t* formas_base_ = nullptr;
uint32_t* formas_frame_ = nullptr;
lv_image_dsc_t formas_image_{};
lv_obj_t* formas_view_ = nullptr;
uint32_t formas_last_ = 0;
uint32_t formas_frames_ = 0;
int formas_expression_ = -1;
bool formas_shy_ = false;
int battery_level_ = -1;
bool battery_charging_ = false, battery_discharging_ = false;
bool battery_valid_ = false;
std::atomic<bool> battery_pending_{false};
uint32_t battery_last_ = 0;
lv_obj_t* battery_value_ = nullptr;
lv_obj_t* battery_state_ = nullptr;

bool InitFormas() {
    classic_surfaces_.Capture({lv_screen_active(), container_, content_, emoji_box_, bottom_bar_,
                               top_bar_, status_bar_, chat_message_label_});
    Settings s("aimilia_face");
    skin_ = std::clamp(static_cast<int>(s.GetInt("skin", 1)), 0, 2);
    shape_ = aimilia_formas::Choice(s.GetInt("shape", 5));
    background_ = aimilia_formas::Choice(s.GetInt("background", 5));
    skin_color_ = aimilia_formas::Choice(s.GetInt("color", 5));
    formas_base_ = static_cast<uint32_t*>(
        heap_caps_malloc(aimilia_formas::kPixels * 4, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    formas_frame_ = static_cast<uint32_t*>(
        heap_caps_malloc(aimilia_formas::kPixels * 4, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!formas_base_ || !formas_frame_) {
        heap_caps_free(formas_base_);
        heap_caps_free(formas_frame_);
        formas_base_ = formas_frame_ = nullptr;
        skin_ = 0;
        ESP_LOGW("Formas", "PSRAM allocation unavailable; using Classic skin");
        return false;
    }
    formas_image_.header.magic = LV_IMAGE_HEADER_MAGIC;
    formas_image_.header.cf = LV_COLOR_FORMAT_ARGB8888;
    formas_image_.header.w = aimilia_formas::kSize;
    formas_image_.header.h = aimilia_formas::kSize;
    formas_image_.header.stride = aimilia_formas::kSize * 4;
    formas_image_.data_size = aimilia_formas::kPixels * 4;
    formas_image_.data = reinterpret_cast<const uint8_t*>(formas_frame_);
    formas_view_ = lv_image_create(face_);
    lv_obj_center(formas_view_);
    lv_obj_remove_flag(formas_view_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(formas_view_, LV_OBJ_FLAG_SCROLLABLE);
    return true;
}
void ApplySkin() {
    if (aimilia_skin::IsFormas(skin_) && !formas_view_)
        skin_ = 0;
    bool formas = aimilia_skin::IsFormas(skin_);
    for (uint32_t i = 0; i < lv_obj_get_child_count(face_); ++i) {
        auto child = lv_obj_get_child(face_, i);
        if (child != formas_view_) {
            if (formas)
                lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
            else
                lv_obj_remove_flag(child, LV_OBJ_FLAG_HIDDEN);
        }
    }
    lv_obj_set_size(face_, formas ? 320 : 280, formas ? 320 : 260);
    lv_obj_set_style_bg_opa(face_, formas ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    auto screen = lv_screen_active();
    uint32_t top = formas ? aimilia_formas::kBgTop[background_] : 0x101625;
    uint32_t bottom = formas ? aimilia_formas::kBgBottom[background_] : 0x101625;
    lv_obj_set_style_bg_color(screen, lv_color_hex(top), 0);
    lv_obj_set_style_bg_grad_color(screen, lv_color_hex(bottom), 0);
    lv_obj_set_style_bg_grad_dir(screen, LV_GRAD_DIR_VER, 0);
    // Stock content stays transparent so the selected background reaches the captions.
    for (auto obj : {container_, content_, emoji_box_, bottom_bar_, top_bar_, status_bar_})
        if (obj)
            lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    uint32_t text = formas && (background_ == 3 || background_ == 5) ? 0x183142 : 0xEDF5FF;
    lv_obj_set_style_text_color(chat_message_label_, lv_color_hex(text), 0);
    lv_obj_set_style_text_color(status_bar_, lv_color_hex(text), 0);
    lv_obj_set_style_text_color(top_bar_, lv_color_hex(text), 0);
    if (skin_ == 0)
        classic_surfaces_.Restore();
    // Labels have their own explicit theme colours; recolouring their parent
    // does not update the clock, listening state, notifications or status icons.
    auto header_background = skin_ == 0 && container_
                                 ? lv_obj_get_style_bg_color(container_, LV_PART_MAIN)
                                 : lv_color_hex(top);
    aimilia_skin::ApplyHeaderInk(header_background, {status_label_, notification_label_,
                                                     network_label_, mute_label_, battery_label_});
    if (formas_view_) {
        if (formas) {
            aimilia_formas::Raster base(formas_base_);
            base.Body(shape_, aimilia_formas::kColors[skin_color_]);
            std::memcpy(formas_frame_, formas_base_, aimilia_formas::kPixels * 4);
            lv_image_cache_drop(&formas_image_);
            lv_image_set_src(formas_view_, &formas_image_);
            lv_obj_remove_flag(formas_view_, LV_OBJ_FLAG_HIDDEN);
        } else
            lv_obj_add_flag(formas_view_, LV_OBJ_FLAG_HIDDEN);
    }
    formas_expression_ = -1;
    formas_last_ = 0;
    ESP_LOGI("Formas", "Skin %d shape %d background %d color %d; free PSRAM %u", skin_, shape_,
             background_, skin_color_,
             static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
}
void StoreAppearance() {
    Settings s("aimilia_face", true);
    s.SetInt("skin", skin_);
    s.SetInt("shape", shape_);
    s.SetInt("background", background_);
    s.SetInt("color", skin_color_);
}
void DrawFormas(std::string_view emotion, bool listening, bool thinking) {
    if (!formas_frame_ || menu_open_)
        return;
    uint32_t now = lv_tick_get();
    int expression = aimilia_formas::Emotion(emotion);
    if (conversation_face::Neutral(emotion) && thinking)
        expression = 9;
    bool shy = emotion == "embarrassed";
    if (now - formas_last_ < 100 && expression == formas_expression_ && shy == formas_shy_)
        return;
    formas_last_ = now;
    auto render_start = esp_timer_get_time();
    formas_expression_ = expression;
    formas_shy_ = shy;
    auto energy = mouth_audio::Read(static_cast<uint32_t>(esp_timer_get_time() / 1000));
    mouth_level_ += (static_cast<float>(energy) - mouth_level_) * .55f;
    std::memcpy(formas_frame_, formas_base_, aimilia_formas::kPixels * 4);
    aimilia_formas::Raster draw(formas_frame_);
    draw.base_ = formas_base_;
    bool blink = now % 4700 < 110 || now % 17300 < 100;
    float look = listening ? 0 : std::sin(now * .0007f) * 3 + tilt_x_ * .2f;
    draw.Draw(expression, blink, look, mouth_level_, now, shy);
    lv_image_cache_drop(&formas_image_);
    lv_obj_invalidate(formas_view_);
    if (++formas_frames_ % 300 == 1)
        ESP_LOGI("Formas", "Render %lld us, 10 fps limit", esp_timer_get_time() - render_start);
}
void UpdateBatteryPanel() {
    if (!menu_open_ || menu_page_ != 7 || !battery_value_ || !battery_state_)
        return;
    if (battery_valid_) {
        lv_label_set_text_fmt(battery_value_, "%d%%", battery_level_);
        lv_label_set_text(battery_state_, battery_charging_      ? "A carregar"
                                          : battery_discharging_ ? "A usar bateria"
                                                                 : "Ligada / em repouso");
    } else {
        lv_label_set_text(battery_value_, "--%");
        lv_label_set_text(battery_state_, "Leitura indisponivel");
    }
}
void RequestBattery() {
    if (battery_pending_.exchange(true))
        return;
    Application::GetInstance().Schedule([this]() {
        int level = 0;
        bool charging = false, discharging = false;
        bool valid = Board::GetInstance().GetBatteryLevel(level, charging, discharging);
        DisplayLockGuard lock(this);
        battery_valid_ = valid && level >= 0 && level <= 100;
        if (battery_valid_) {
            battery_level_ = level;
            battery_charging_ = charging;
            battery_discharging_ = discharging;
        }
        battery_last_ = lv_tick_get();
        battery_pending_ = false;
        UpdateBatteryPanel();
        ESP_LOGI("BatteryPanel", "valid=%d level=%d charging=%d discharging=%d", battery_valid_,
                 battery_valid_ ? battery_level_ : -1, charging, discharging);
    });
}
