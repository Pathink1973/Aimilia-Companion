// Board-local controls; included inside CustomLcdDisplay.
lv_obj_t* menu_ = nullptr;
lv_obj_t* menu_body_ = nullptr;
lv_obj_t* menu_home_ = nullptr;
std::atomic<bool> menu_open_{false};
int menu_page_ = 0;
static constexpr int kMenuPages = 11;
lv_obj_t* menu_dots_[kMenuPages] = {};
lv_point_t touch_start_{};
lv_obj_t* clock_value_ = nullptr;
lv_obj_t* clock_segments_[4][7] = {};
uint32_t clock_last_update_ = 0;
int64_t weather_displayed_update_ = -1;
lv_obj_t* analog_hour_ = nullptr;
lv_obj_t* analog_minute_ = nullptr;
lv_point_precise_t analog_hour_points_[2] = {};
lv_point_precise_t analog_minute_points_[2] = {};

static void MenuTouch(lv_event_t* event) {
    auto self = static_cast<CustomLcdDisplay*>(lv_event_get_user_data(event));
    auto input = lv_indev_active();
    if (!input || (self->menu_home_ && !lv_obj_has_flag(self->menu_home_, LV_OBJ_FLAG_HIDDEN)))
        return;
    lv_point_t point;
    lv_indev_get_point(input, &point);
    auto code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESSED)
        self->touch_start_ = point;
    if (code != LV_EVENT_RELEASED)
        return;
    int dx = point.x - self->touch_start_.x;
    int dy = point.y - self->touch_start_.y;
    if (std::abs(dx) > 65 && std::abs(dx) > std::abs(dy) * 2) {
        if (!self->menu_open_)
            self->OpenMenu();
        else {
            self->menu_page_ = (self->menu_page_ + (dx < 0 ? 1 : kMenuPages - 1)) % kMenuPages;
            self->RenderMenu();
        }
    } else if (!self->menu_open_ && std::abs(dx) < 15 && std::abs(dy) < 15) {
        Application::GetInstance().Schedule([self]() { self->React(false); });
    }
}

lv_obj_t* MenuLabel(lv_obj_t* parent, const std::string& text, int y, int width = 300) {
    auto label = lv_label_create(parent);
    lv_label_set_text(label, text.c_str());
    lv_obj_set_width(label, width);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, y);
    return label;
}

void MenuButton(lv_obj_t* parent, const char* title, int x, int y, int width, int action) {
    auto button = lv_button_create(parent);
    lv_obj_set_size(button, width, 48);
    aimilia_button_style(button);
    lv_obj_align(button, LV_ALIGN_TOP_MID, x, y);
    lv_obj_set_user_data(button, reinterpret_cast<void*>(static_cast<intptr_t>(action)));
    auto label = lv_label_create(button);
    lv_label_set_text(label, title);
    lv_obj_center(label);
    lv_obj_add_event_cb(
        button,
        [](lv_event_t* event) {
            auto self = static_cast<CustomLcdDisplay*>(lv_event_get_user_data(event));
            auto button = static_cast<lv_obj_t*>(lv_event_get_target(event));
            int action = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(button)));
            if (action == 0) {
                ESP_LOGI("SwipeMenu", "Closed");
                lv_obj_remove_flag(self->menu_home_, LV_OBJ_FLAG_HIDDEN);
                return;
            }
            if (action == 1 || action == 2) {
                self->menu_page_ =
                    (self->menu_page_ + (action == 1 ? kMenuPages - 1 : 1)) % kMenuPages;
                self->RenderMenu();
                return;
            }
            Application::GetInstance().Schedule([self, action]() {
                auto& board = Board::GetInstance();
                if (action == 3 || action == 4) {
                    auto codec = board.GetAudioCodec();
                    codec->SetOutputVolume(
                        std::clamp(codec->output_volume() + (action == 3 ? -10 : 10), 10, 100));
                } else if (action == 5 || action == 6) {
                    auto light = board.GetBacklight();
                    if (light)
                        light->SetBrightness(std::clamp(static_cast<int>(light->brightness()) +
                                                            (action == 5 ? -10 : 10),
                                                        10, 100),
                                             true);
                } else if (action == 7) {
                    // Open the portal for new networks without erasing saved credentials.
                    {
                        DisplayLockGuard lock(self);
                        self->menu_open_ = false;
                        lv_obj_add_flag(self->menu_, LV_OBJ_FLAG_HIDDEN);
                    }
                    static_cast<WifiBoard&>(board).EnterWifiConfigMode();
                    return;
                } else if (action >= 8 && action <= 14) {
                    DisplayLockGuard lock(self);
                    if (action == 8)
                        self->skin_ = aimilia_skin::Next(self->skin_);
                    else if (action == 9 || action == 10)
                        self->shape_ = (self->shape_ + (action == 9 ? 5 : 1)) % 6;
                    else if (action == 11 || action == 12)
                        self->background_ = (self->background_ + (action == 11 ? 5 : 1)) % 6;
                    else
                        self->skin_color_ = (self->skin_color_ + (action == 13 ? 5 : 1)) % 6;
                    self->ApplySkin();
                    self->StoreAppearance();
                }
                DisplayLockGuard lock(self);
                if (self->menu_open_)
                    self->RenderMenu();
            });
        },
        LV_EVENT_CLICKED, this);
}

// A full-screen home menu. Each large target opens an existing, functional view.
void HomeIcon(lv_obj_t* parent, const char* symbol, const char* caption, int x, int y, int page) {
    auto button = lv_button_create(parent);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, 112, 112);
    aimilia_button_style(button);
    lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(button, 2, 0);
    lv_obj_set_user_data(button, reinterpret_cast<void*>(static_cast<intptr_t>(page)));
    (void)symbol;
    static const lv_point_precise_t settings_lines[][2] = {{{0,0},{36,0}},{{0,12},{36,12}},{{0,24},{36,24}}};
    static const lv_point_precise_t clock_hand1[] = {{18,18},{18,5}};
    static const lv_point_precise_t clock_hand2[] = {{18,18},{28,23}};
    auto draw_line = [&](const lv_point_precise_t* points, int px, int py) {
        auto line = lv_line_create(button);
        lv_line_set_points(line, points, 2);
        lv_obj_set_pos(line, px, py);
        lv_obj_set_style_line_color(line, lv_color_hex(0x80D8FF), 0);
        lv_obj_set_style_line_width(line, 3, 0);
        lv_obj_set_style_line_rounded(line, true, 0);
        lv_obj_remove_flag(line, LV_OBJ_FLAG_CLICKABLE);
    };
    if (page == 0) {
        for (auto& line : settings_lines) draw_line(line, 38, 39);
        for (int i=0; i<3; ++i) {
            auto knob=lv_obj_create(button); lv_obj_remove_style_all(knob);
            lv_obj_set_size(knob, 8, 8); lv_obj_set_pos(knob, i==1 ? 62 : 47, 35+i*12);
            lv_obj_set_style_radius(knob, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_color(knob, lv_color_hex(0x80D8FF), 0);
            lv_obj_set_style_bg_opa(knob, LV_OPA_COVER, 0);
            lv_obj_remove_flag(knob, LV_OBJ_FLAG_CLICKABLE);
        }
    } else if (page == 8) {
        auto dial=lv_obj_create(button); lv_obj_remove_style_all(dial);
        lv_obj_set_size(dial, 42, 42); lv_obj_set_pos(dial, 35, 32);
        lv_obj_set_style_radius(dial, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(dial, 3, 0);
        lv_obj_set_style_border_color(dial, lv_color_hex(0x80D8FF), 0);
        lv_obj_remove_flag(dial, LV_OBJ_FLAG_CLICKABLE);
        draw_line(clock_hand1, 38, 35); draw_line(clock_hand2, 38, 35);
    } else {
        vivid_icon(button, 1, 43, 40);
    }
    (void)caption;
    lv_obj_add_event_cb(button, [](lv_event_t* event) {
        auto self = static_cast<CustomLcdDisplay*>(lv_event_get_user_data(event));
        auto target = static_cast<lv_obj_t*>(lv_event_get_target(event));
        self->menu_page_ = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(target)));
        lv_obj_add_flag(self->menu_home_, LV_OBJ_FLAG_HIDDEN);
        self->RenderMenu();
    }, LV_EVENT_CLICKED, this);
}

void BuildMenuHome() {
    menu_home_ = lv_obj_create(menu_);
    lv_obj_set_size(menu_home_, 462, 462);
    lv_obj_center(menu_home_);
    lv_obj_set_style_radius(menu_home_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(menu_home_, lv_color_hex(0x0D1729), 0);
    lv_obj_set_style_bg_grad_color(menu_home_, lv_color_hex(0x030711), 0);
    lv_obj_set_style_bg_grad_dir(menu_home_, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(menu_home_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(menu_home_, 0, 0);
    lv_obj_set_style_pad_all(menu_home_, 0, 0);
    lv_obj_remove_flag(menu_home_, LV_OBJ_FLAG_SCROLLABLE);

    auto face = lv_obj_create(menu_home_);
    lv_obj_set_size(face, 142, 142);
    lv_obj_align(face, LV_ALIGN_CENTER, 0, -18);
    lv_obj_set_style_radius(face, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(face, lv_color_hex(0x101E35), 0);
    lv_obj_set_style_bg_opa(face, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(face, 2, 0);
    lv_obj_set_style_border_color(face, lv_color_hex(0x4CA9CF), 0);
    lv_obj_set_style_shadow_width(face, 24, 0);
    lv_obj_set_style_shadow_color(face, lv_color_hex(0x36BCEB), 0);
    lv_obj_set_style_shadow_opa(face, LV_OPA_30, 0);
    lv_obj_remove_flag(face, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(face, LV_OBJ_FLAG_SCROLLABLE);
    for (int x : {-25, 25}) {
        auto eye = lv_obj_create(face);
        lv_obj_remove_style_all(eye);
        lv_obj_set_size(eye, 14, 26);
        lv_obj_set_style_radius(eye, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(eye, lv_color_hex(0x76DDFF), 0);
        lv_obj_set_style_bg_opa(eye, LV_OPA_COVER, 0);
        lv_obj_align(eye, LV_ALIGN_CENTER, x, -9);
        lv_obj_remove_flag(eye, LV_OBJ_FLAG_CLICKABLE);
    }
    auto smile = lv_arc_create(face);
    lv_obj_remove_style_all(smile);
    lv_obj_set_size(smile, 42, 42);
    lv_obj_align(smile, LV_ALIGN_CENTER, 0, 19);
    lv_arc_set_angles(smile, 35, 145);
    lv_obj_set_style_arc_color(smile, lv_color_hex(0x76DDFF), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(smile, 4, LV_PART_INDICATOR);
    lv_obj_remove_flag(smile, LV_OBJ_FLAG_CLICKABLE);

    HomeIcon(menu_home_, "+", "Definicoes", 13, 160, 0);
    HomeIcon(menu_home_, "12:00", "Relogio", 337, 160, 8);
    HomeIcon(menu_home_, "*", "Tempo", 175, 325, 10);
    auto close = lv_button_create(menu_home_);
    lv_obj_set_size(close, 72, 46);
    lv_obj_align(close, LV_ALIGN_TOP_MID, 0, 17);
    aimilia_button_style(close);
    auto text = lv_label_create(close);
    lv_label_set_text(text, "X");
    lv_obj_center(text);
    lv_obj_add_event_cb(close, [](lv_event_t* event) {
        auto self = static_cast<CustomLcdDisplay*>(lv_event_get_user_data(event));
        self->menu_open_ = false;
        lv_obj_add_flag(self->menu_, LV_OBJ_FLAG_HIDDEN);
    }, LV_EVENT_CLICKED, this);
}

void OpenMenu() {
    if (!menu_) {
        menu_ = lv_obj_create(lv_layer_top());
        lv_obj_set_size(menu_, 466, 466);
        lv_obj_center(menu_);
        lv_obj_set_style_bg_color(menu_, lv_color_hex(0x101B31), 0);
        lv_obj_set_style_bg_grad_color(menu_, lv_color_hex(0x060B16), 0);
        lv_obj_set_style_bg_grad_dir(menu_, LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_bg_opa(menu_, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(menu_, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_text_font(menu_, &font_noto_sans_basic_20_4, 0);
        lv_obj_set_style_border_width(menu_, 2, 0);
        lv_obj_set_style_border_color(menu_, lv_color_hex(0x294766), 0);
        lv_obj_set_style_radius(menu_, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_pad_all(menu_, 0, 0);
        lv_obj_remove_flag(menu_, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(menu_, MenuTouch, LV_EVENT_ALL, this);
        menu_body_ = lv_obj_create(menu_);
        lv_obj_set_size(menu_body_, 330, 250);
        lv_obj_align(menu_body_, LV_ALIGN_TOP_MID, 0, 112);
        lv_obj_set_style_bg_opa(menu_body_, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(menu_body_, 0, 0);
        lv_obj_set_style_pad_all(menu_body_, 0, 0);
        lv_obj_set_style_text_color(menu_body_, lv_color_hex(0xffffff), 0);
        lv_obj_remove_flag(menu_body_, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(menu_body_, MenuTouch, LV_EVENT_ALL, this);
        MenuButton(menu_, "<", -115, 60, 50, 1);
        MenuButton(menu_, ">", 115, 60, 50, 2);
        MenuButton(menu_, "Voltar", 0, 386, 130, 0);
        aimilia_menu_text(menu_, "DEFINICOES", 72, 170, &font_noto_sans_basic_20_4, 0x80D8FF);
        for (int i = 0; i < kMenuPages; ++i) {
            menu_dots_[i] = lv_obj_create(menu_);
            lv_obj_remove_style_all(menu_dots_[i]);
            lv_obj_set_size(menu_dots_[i], 7, 7);
            lv_obj_align(menu_dots_[i], LV_ALIGN_TOP_MID, i * 16 - 80, 368);
            lv_obj_set_style_radius(menu_dots_[i], LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_opa(menu_dots_[i], LV_OPA_COVER, 0);
            lv_obj_remove_flag(menu_dots_[i], LV_OBJ_FLAG_CLICKABLE);
        }
        BuildMenuHome();
    }
    ESP_LOGI("SwipeMenu", "Open page %d", menu_page_ + 1);
    menu_open_ = true;
    lv_obj_remove_flag(menu_, LV_OBJ_FLAG_HIDDEN);
    RenderMenu();
    lv_obj_remove_flag(menu_home_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(menu_home_);
}

void RenderMenu() {
    battery_value_ = battery_state_ = nullptr;
    clock_value_ = nullptr;
    for (auto& digit : clock_segments_) for (auto& segment : digit) segment = nullptr;
    analog_hour_ = analog_minute_ = nullptr;
    lv_obj_clean(menu_body_);
    for (int i = 0; i < kMenuPages; ++i)
        lv_obj_set_style_bg_color(menu_dots_[i],
                                  lv_color_hex(i == menu_page_ ? 0x80D8FF : 0x304156), 0);
    if (menu_page_ == 0 || menu_page_ == 1) {
        bool sound = menu_page_ == 0;
        auto light = Board::GetInstance().GetBacklight();
        int value = sound ? Board::GetInstance().GetAudioCodec()->output_volume()
                          : (light ? light->brightness() : 0);
        aimilia_meter(menu_body_, sound ? "Som" : "Luminosidade", value, &font_noto_sans_basic_20_4,
                      &font_noto_sans_basic_30_4);
        MenuButton(menu_body_, "-", -80, 122, 90, sound ? 3 : 5);
        MenuButton(menu_body_, "+", 80, 122, 90, sound ? 4 : 6);
        aimilia_menu_text(menu_body_, "Ajustes guardados", 195, 290, &font_noto_sans_basic_20_4,
                          0x9CAEC6);
    } else if (menu_page_ == 2) {
        auto& wifi = WifiManager::GetInstance();
        vivid_icon(menu_body_, 2, 51, -6);
        aimilia_menu_text(menu_body_, wifi.IsConnected() ? "WI-FI LIGADO" : "SEM LIGACAO", 0, 290,
                          &font_noto_sans_basic_20_4, 0x80D8FF);
        aimilia_menu_text(menu_body_,
                          wifi.IsConnected() ? wifi.GetSsid().c_str() : "Tenta uma rede guardada",
                          32, 290, &font_noto_sans_basic_20_4, 0xEDF5FF);
        std::string networks = "Redes guardadas";
        for (const auto& item : SsidManager::GetInstance().GetSsidList())
            networks += "\n" + item.ssid;
        auto list = aimilia_menu_text(menu_body_, networks.c_str(), 65, 290,
                                      &font_noto_sans_basic_20_4, 0x9CAEC6);
        lv_obj_set_height(list, 100);
        lv_label_set_long_mode(list, LV_LABEL_LONG_DOT);
        MenuButton(menu_body_, "Adicionar rede", 0, 171, 230, 7);
        aimilia_menu_text(menu_body_, "Abre a configuracao Wi-Fi", 225, 290, &font_noto_sans_basic_20_4,
                          0x9CAEC6);
    } else if (menu_page_ == 8) {
        aimilia_menu_text(menu_body_, "HORA LOCAL", 0, 290, &font_noto_sans_basic_20_4, 0x9CAEC6);
        // Seven-segment digits drawn directly by LVGL; no bitmap font required.
        static const int sx[] = {8, 42, 42, 8, 0, 0, 8};
        static const int sy[] = {0, 8, 49, 83, 49, 8, 41};
        for (int digit=0; digit<4; ++digit) {
            int x = 31 + digit*65 + (digit>=2 ? 12 : 0);
            for (int segment=0; segment<7; ++segment) {
                auto part = lv_obj_create(menu_body_); lv_obj_remove_style_all(part);
                bool horizontal = segment==0 || segment==3 || segment==6;
                lv_obj_set_size(part, horizontal ? 34 : 8, horizontal ? 8 : 34);
                lv_obj_set_pos(part, x+sx[segment], 62+sy[segment]);
                lv_obj_set_style_radius(part, 4, 0);
                lv_obj_set_style_bg_color(part, lv_color_hex(0x74DDFF), 0);
                lv_obj_set_style_bg_opa(part, LV_OPA_20, 0);
                lv_obj_set_style_shadow_color(part, lv_color_hex(0x40BEF4), 0);
                lv_obj_set_style_shadow_width(part, 10, 0);
                lv_obj_remove_flag(part, LV_OBJ_FLAG_CLICKABLE);
                clock_segments_[digit][segment] = part;
            }
        }
        for (int y : {91, 126}) {
            auto dot=lv_obj_create(menu_body_); lv_obj_remove_style_all(dot);
            lv_obj_set_size(dot, 9, 9); lv_obj_set_pos(dot, 157, y);
            lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_color(dot, lv_color_hex(0x74DDFF), 0);
            lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
            lv_obj_remove_flag(dot, LV_OBJ_FLAG_CLICKABLE);
        }
        clock_last_update_ = 0;
        aimilia_menu_text(menu_body_, "HORA DO DISPOSITIVO", 185, 300, &font_noto_sans_basic_20_4, 0x9CAEC6);
    } else if (menu_page_ == 9) {
        aimilia_menu_text(menu_body_, "RELOGIO ANALOGICO", 0, 290, &font_noto_sans_basic_20_4, 0x80D8FF);
        auto dial = lv_obj_create(menu_body_);
        lv_obj_remove_style_all(dial);
        lv_obj_set_size(dial, 150, 150);
        lv_obj_align(dial, LV_ALIGN_TOP_MID, 0, 45);
        lv_obj_set_style_radius(dial, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(dial, 2, 0);
        lv_obj_set_style_border_color(dial, lv_color_hex(0x80D8FF), 0);
        lv_obj_set_style_shadow_width(dial, 18, 0);
        lv_obj_set_style_shadow_color(dial, lv_color_hex(0x288AC0), 0);
        lv_obj_set_style_shadow_opa(dial, LV_OPA_30, 0);
        lv_obj_remove_flag(dial, LV_OBJ_FLAG_CLICKABLE);
        for (int tick = 0; tick < 12; ++tick) {
            float angle = tick * 3.14159265f / 6.0f;
            auto dot = lv_obj_create(dial);
            lv_obj_remove_style_all(dot);
            lv_obj_set_size(dot, tick % 3 == 0 ? 6 : 3, tick % 3 == 0 ? 6 : 3);
            lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_color(dot, lv_color_hex(0x80D8FF), 0);
            lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
            lv_obj_set_pos(dot, 72 + static_cast<int>(sinf(angle) * 64), 72 - static_cast<int>(cosf(angle) * 64));
            lv_obj_remove_flag(dot, LV_OBJ_FLAG_CLICKABLE);
        }
        analog_hour_ = lv_line_create(menu_body_);
        analog_minute_ = lv_line_create(menu_body_);
        for (auto hand : {analog_hour_, analog_minute_}) {
            lv_obj_set_style_line_color(hand, lv_color_hex(0x80D8FF), 0);
            lv_obj_set_style_line_width(hand, hand == analog_hour_ ? 5 : 3, 0);
            lv_obj_set_style_line_rounded(hand, true, 0);
            lv_obj_remove_flag(hand, LV_OBJ_FLAG_CLICKABLE);
        }
        clock_last_update_ = 0;
        aimilia_menu_text(menu_body_, "Hora do dispositivo", 204, 290, &font_noto_sans_basic_20_4, 0x9CAEC6);
    } else if (menu_page_ == 10) {
        auto updated = valenca_updated_us.load();
        weather_displayed_update_ = updated;
        auto city = WeatherLocationName();
        aimilia_menu_text(menu_body_, city.empty() ? "LOCALIZACAO AUTOMATICA" : city.c_str(), 0, 300, &font_noto_sans_basic_20_4, 0x80D8FF);
        bool fresh = updated > 0 && esp_timer_get_time() - updated < 1800000000LL;
        int code = valenca_weather_code.load();
        if (fresh) {
            // Simple scalable weather illustration, matching the retrieved condition.
            bool clear = code <= 3;
            bool cloudy = code >= 1;
            bool rain = code >= 51 && code <= 82;
            if (clear) {
                auto sun=lv_obj_create(menu_body_); lv_obj_remove_style_all(sun);
                lv_obj_set_size(sun, 52, 52); lv_obj_set_pos(sun, cloudy ? 110 : 139, 42);
                lv_obj_set_style_radius(sun, LV_RADIUS_CIRCLE, 0);
                lv_obj_set_style_bg_color(sun, lv_color_hex(0xFFD06F), 0);
                lv_obj_set_style_bg_opa(sun, LV_OPA_COVER, 0);
                lv_obj_set_style_shadow_color(sun, lv_color_hex(0xFFA63D), 0);
                lv_obj_set_style_shadow_width(sun, 24, 0);
                lv_obj_remove_flag(sun, LV_OBJ_FLAG_CLICKABLE);
            }
            if (cloudy) {
                for (int i=0;i<3;++i) {
                    auto cloud=lv_obj_create(menu_body_); lv_obj_remove_style_all(cloud);
                    int w=i==1?67:50; lv_obj_set_size(cloud,w,w);
                    lv_obj_set_pos(cloud, 100+i*32, i==1?50:66);
                    lv_obj_set_style_radius(cloud,LV_RADIUS_CIRCLE,0);
                    lv_obj_set_style_bg_color(cloud,lv_color_hex(0xDAE5F8),0);
                    lv_obj_set_style_bg_opa(cloud,LV_OPA_COVER,0);
                    lv_obj_remove_flag(cloud,LV_OBJ_FLAG_CLICKABLE);
                }
            }
            if (rain) {
                for (int i=0;i<3;++i) {
                    auto drop=lv_obj_create(menu_body_);lv_obj_remove_style_all(drop);
                    lv_obj_set_size(drop,5,15);lv_obj_set_pos(drop,125+i*30,122);
                    lv_obj_set_style_radius(drop,3,0);lv_obj_set_style_bg_color(drop,lv_color_hex(0x70D4FF),0);
                    lv_obj_set_style_bg_opa(drop,LV_OPA_COVER,0);lv_obj_remove_flag(drop,LV_OBJ_FLAG_CLICKABLE);
                }
            }
        }
        char temp[32] = "-- C";
        if (fresh) snprintf(temp, sizeof(temp), "%d C", valenca_temperature_tenths.load() / 10);
        aimilia_menu_text(menu_body_, temp, 152, 290, &font_noto_sans_basic_30_4, 0xEDF5FF);
        aimilia_menu_text(menu_body_, fresh ? "Local aproximado · Open-Meteo" : "A aguardar dados", 202, 290, &font_noto_sans_basic_20_4, 0x9CAEC6);
    } else if (menu_page_ >= 3 && menu_page_ <= 6) {
        const char* titles[] = {"Aparencia", "Forma", "Fundo", "Cor"};
        const char* value = menu_page_ == 3   ? aimilia_skin::Name(skin_)
                            : menu_page_ == 4 ? aimilia_formas::kShapes[shape_]
                            : menu_page_ == 5 ? aimilia_formas::kBackgrounds[background_]
                                              : aimilia_formas::kColorNames[skin_color_];
        aimilia_menu_text(menu_body_, titles[menu_page_ - 3], 10, 290, &font_noto_sans_basic_20_4,
                          0x80D8FF);
        aimilia_menu_text(menu_body_, value, 56, 290, &font_noto_sans_basic_30_4, 0xEDF5FF);
        if (menu_page_ == 3)
            MenuButton(menu_body_, "Mudar skin", 0, 122, 230, 8);
        else {
            int action = 9 + (menu_page_ - 4) * 2;
            MenuButton(menu_body_, "<", -80, 122, 90, action);
            MenuButton(menu_body_, ">", 80, 122, 90, action + 1);
        }
        aimilia_menu_text(menu_body_,
                          menu_page_ == 3                 ? "Tres estilos, a mesma Aimilia"
                          : aimilia_skin::IsFormas(skin_) ? "Escolha guardada"
                                                          : "Ativa a skin Formas",
                          200, 310, &font_noto_sans_basic_20_4, 0x9CAEC6);
    } else {
        aimilia_menu_text(menu_body_, "Bateria", 8, 290, &font_noto_sans_basic_20_4, 0x80D8FF);
        battery_value_ =
            aimilia_menu_text(menu_body_, "--%", 52, 290, &font_noto_sans_basic_30_4, 0xEDF5FF);
        battery_state_ = aimilia_menu_text(menu_body_, "A obter leitura...", 105, 310,
                                           &font_noto_sans_basic_20_4, 0xEDF5FF);
        aimilia_menu_text(menu_body_, "Carga estimada pelo sensor", 163, 310,
                          &font_noto_sans_basic_20_4, 0x9CAEC6);
        aimilia_menu_text(menu_body_, "Atualiza a cada 5 segundos", 197, 310,
                          &font_noto_sans_basic_20_4, 0x9CAEC6);
        UpdateBatteryPanel();
        if (lv_tick_get() - battery_last_ > 1000 || !battery_last_)
            RequestBattery();
    }
}
