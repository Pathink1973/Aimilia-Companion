// Board-local controls; included inside CustomLcdDisplay.
lv_obj_t* menu_ = nullptr;
lv_obj_t* menu_body_ = nullptr;
std::atomic<bool> menu_open_{false};
int menu_page_ = 0;
static constexpr int kMenuPages = 8;
lv_obj_t* menu_dots_[kMenuPages] = {};
lv_point_t touch_start_{};

static void MenuTouch(lv_event_t* event) {
    auto self = static_cast<CustomLcdDisplay*>(lv_event_get_user_data(event));
    auto input = lv_indev_active();
    if (!input)
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
                self->menu_open_ = false;
                lv_obj_add_flag(self->menu_, LV_OBJ_FLAG_HIDDEN);
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
                    // Keep active calls intact. Retry stored networks only while idle.
                    if (Application::GetInstance().GetDeviceState() == kDeviceStateIdle) {
                        auto& wifi = WifiManager::GetInstance();
                        wifi.StopStation();
                        wifi.StartStation();
                    }
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

void OpenMenu() {
    if (!menu_) {
        menu_ = lv_obj_create(lv_layer_top());
        lv_obj_set_size(menu_, 466, 466);
        lv_obj_center(menu_);
        lv_obj_set_style_bg_color(menu_, lv_color_hex(0x080E18), 0);
        lv_obj_set_style_bg_opa(menu_, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(menu_, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_text_font(menu_, &font_noto_sans_basic_20_4, 0);
        lv_obj_set_style_border_width(menu_, 0, 0);
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
            lv_obj_align(menu_dots_[i], LV_ALIGN_TOP_MID, i * 16 - 56, 368);
            lv_obj_set_style_radius(menu_dots_[i], LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_opa(menu_dots_[i], LV_OPA_COVER, 0);
            lv_obj_remove_flag(menu_dots_[i], LV_OBJ_FLAG_CLICKABLE);
        }
    }
    ESP_LOGI("SwipeMenu", "Open page %d", menu_page_ + 1);
    menu_open_ = true;
    lv_obj_remove_flag(menu_, LV_OBJ_FLAG_HIDDEN);
    RenderMenu();
}

void RenderMenu() {
    battery_value_ = battery_state_ = nullptr;
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
        MenuButton(menu_body_, "Procurar redes", 0, 171, 230, 7);
        aimilia_menu_text(menu_body_, "Fora da conversa", 225, 290, &font_noto_sans_basic_20_4,
                          0x9CAEC6);
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
