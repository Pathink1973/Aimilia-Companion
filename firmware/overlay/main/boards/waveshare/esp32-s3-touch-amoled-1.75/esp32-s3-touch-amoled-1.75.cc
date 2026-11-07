#include "wifi_board.h"
#include "display/lcd_display.h"
#include "esp_lcd_co5300.h"

#include "application.h"
#include "audio/demuxer/ogg_demuxer.h"
#include "axp2101.h"
#include "backlight.h"
#include "battery_status.h"
#include "button.h"
#include "codecs/box_audio_codec.h"
#include "config.h"
#include "conversation_face.h"
#include "expressive_eyes.h"
#include "formas_renderer.h"
#include "i2c_device.h"
#include "kubi_animation.h"
#include "led/single_led.h"
#include "mcp_server.h"
#include "menu_visual.h"
#include "motion_filter.h"
#include "mouth_audio.h"
#include "portugal_weather.h"
#include "power_save_timer.h"
#include "reaction_sounds.h"
#include "ssid_manager.h"
#include "skin_surfaces.h"
#include "wifi_manager.h"

#include <cmath>

#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <esp_lcd_panel_vendor.h>
#include <driver/i2c_master.h>
#include <driver/spi_master.h>
#include "esp_io_expander_tca9554.h"
#include "settings.h"

#include <esp_lcd_touch_cst9217.h>
#include <esp_lvgl_port.h>
#include <lvgl.h>
#include <src/misc/cache/instance/lv_image_cache.h>

LV_FONT_DECLARE(font_noto_sans_basic_20_4);
LV_FONT_DECLARE(font_noto_sans_basic_30_4);

#define TAG "WaveshareEsp32s3TouchAMOLED1inch75"

class Pmic : public Axp2101 {
public:
    bool TryReadBattery(int& level, bool& charging, bool& discharging) {
        return ReadWaveshareBatteryStatus(i2c_device_, level, charging, discharging);
    }

    Pmic(i2c_master_bus_handle_t i2c_bus, uint8_t addr) : Axp2101(i2c_bus, addr) {
        WriteReg(0x22, 0b110); // PWRON > OFFLEVEL as POWEROFF Source enable
        WriteReg(0x27, 0x10);  // hold 4s to power off

        // Disable All DCs but DC1
        WriteReg(0x80, 0x01);
        // Disable All LDOs
        WriteReg(0x90, 0x00);
        WriteReg(0x91, 0x00);

        // Set DC1 to 3.3V
        WriteReg(0x82, (3300 - 1500) / 100);

        // Set ALDO1 to 3.3V
        WriteReg(0x92, (3300 - 500) / 100);

        // Enable ALDO1(MIC)
        WriteReg(0x90, 0x01);

        WriteReg(0x64, 0x02); // CV charger voltage setting to 4.1V

        WriteReg(0x61, 0x02); // set Main battery precharge current to 50mA
        WriteReg(0x62, 0x08); // set Main battery charger current to 400mA ( 0x08-200mA, 0x09-300mA, 0x0A-400mA )
        WriteReg(0x63, 0x01); // set Main battery term charge current to 25mA
    }
};

#define LCD_OPCODE_WRITE_CMD (0x02ULL)
#define LCD_OPCODE_READ_CMD (0x03ULL)
#define LCD_OPCODE_WRITE_COLOR (0x32ULL)

static const co5300_lcd_init_cmd_t vendor_specific_init[] = {
    // set display to qspi mode
    {0xFE, (uint8_t[]){0x20}, 1, 0},
    {0x19, (uint8_t[]){0x10}, 1, 0},
    {0x1C, (uint8_t[]){0xA0}, 1, 0},

    {0xFE, (uint8_t[]){0x00}, 1, 0},
    {0xC4, (uint8_t[]){0x80}, 1, 0},
    {0x3A, (uint8_t[]){0x55}, 1, 0},
    {0x35, (uint8_t[]){0x00}, 1, 0},
    {0x53, (uint8_t[]){0x20}, 1, 0},
    {0x51, (uint8_t[]){0xFF}, 1, 0},
    {0x63, (uint8_t[]){0xFF}, 1, 0},
    {0x2A, (uint8_t[]){0x00, 0x06, 0x01, 0xD7}, 4, 0},
    {0x2B, (uint8_t[]){0x00, 0x00, 0x01, 0xD1}, 4, 600},
    {0x11, NULL, 0, 600},
    {0x29, NULL, 0, 0},
};

// Board-local observation of the existing PCM output; playback remains unchanged.
class ExpressiveAudioCodec : public BoxAudioCodec {
public:
    using BoxAudioCodec::BoxAudioCodec;
    void OutputData(std::vector<int16_t>& data) override {
        mouth_audio::level.store(mouth_audio::Measure(data.data(), data.size()), std::memory_order_relaxed);
        mouth_audio::updated_ms.store(static_cast<uint32_t>(esp_timer_get_time() / 1000), std::memory_order_relaxed);
        BoxAudioCodec::OutputData(data);
    }
};

// 在waveshare_amoled_1_75类之前添加新的显示类
class CustomLcdDisplay : public SpiLcdDisplay {
    lv_obj_t* face_ = nullptr;
    lv_obj_t* eyes_[2] = {};
    lv_obj_t* lids_[2] = {};
    lv_obj_t* mouth_line_ = nullptr;
    lv_obj_t* mouth_open_ = nullptr;
    lv_point_precise_t mouth_points_[9]{};
    float mouth_level_ = 0;
    expressive_eyes::Pose pose_;
    float look_x_ = 0;
    float look_y_ = 0;
    lv_timer_t* face_timer_ = nullptr;
    bool previous_voice_ = false;
    int64_t last_voice_end_ = 0;
    DeviceState previous_state_ = kDeviceStateUnknown;
    std::string emotion_ = "neutral";
    std::string context_emotion_;
    int64_t context_until_ = 0;
    std::string reaction_emotion_;
    std::string requested_emotion_;
    int64_t requested_until_ = 0;
    int64_t reaction_until_ = 0;
    int64_t last_reaction_ = -12000000;
    float tilt_x_ = 0;
    float animation_tilt_ = 0;
    kubi_animation::Sequence kubi_;
    lv_obj_t* sleep_label_ = nullptr;
    lv_obj_t* sparkles_[4] = {};

#include "face_details.inc"
#include "swipe_menu.inc"
#include "formas_skin.inc"

    void AnimateFace() {
        if (menu_open_ && menu_page_ == 7 && lv_tick_get() - battery_last_ > 5000)
            RequestBattery();
        auto state = Application::GetInstance().GetDeviceState();
        bool listening = state == kDeviceStateListening;
        bool voice = Application::GetInstance().IsVoiceDetected();
        if (listening && voice != previous_voice_) {
            ESP_LOGI("VoiceLatency", "local VAD %s", voice ? "speech" : "silence");
            if (!voice)
                last_voice_end_ = esp_timer_get_time();
        }
        if (state == kDeviceStateSpeaking && previous_state_ != state && last_voice_end_) {
            ESP_LOGI("VoiceLatency", "last local silence to speaking state: %lld ms",
                     (esp_timer_get_time() - last_voice_end_) / 1000);
            last_voice_end_ = 0;
        }
        if (!listening && state != kDeviceStateSpeaking)
            last_voice_end_ = 0;
        previous_voice_ = voice;
        previous_state_ = state;
        bool speaking = state == kDeviceStateSpeaking;
        bool thinking = emotion_ == "thinking" || state == kDeviceStateConnecting ||
                        (listening && last_voice_end_ && !voice);
        auto now_us = esp_timer_get_time();
        bool reacting = reaction_until_ > now_us && state == kDeviceStateIdle;
        if (reaction_until_ && !reacting) {
            reaction_until_ = 0;
            if (state == kDeviceStateIdle)
                lv_label_set_text(chat_message_label_, "");
        }
        bool requested = requested_until_ > now_us;
        bool moved = std::abs(tilt_x_ - animation_tilt_) > 2.0f;
        animation_tilt_ = tilt_x_;
        auto animation = kubi_.Update(static_cast<uint32_t>(now_us / 1000),
            state == kDeviceStateIdle, menu_open_, requested, moved);
        std::string_view face_emotion = requested ? requested_emotion_ :
            (reacting ? reaction_emotion_ : emotion_);
        if (!requested && !reacting && conversation_face::Neutral(face_emotion) &&
            context_until_ > now_us && !context_emotion_.empty())
            face_emotion = context_emotion_;
        if (!animation.emotion.empty()) face_emotion = animation.emotion;
        if (aimilia_skin::IsFormas(skin_)) {
            DrawFormas(face_emotion, listening, thinking);
            lv_obj_add_flag(emoji_label_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(emoji_image_, LV_OBJ_FLAG_HIDDEN);
            return;
        }
        auto target = expressive_eyes::ForEmotion(face_emotion);
        target.y += animation.jump;
        if (sleep_label_) {
            if (animation.sleep) {
                lv_obj_remove_flag(sleep_label_, LV_OBJ_FLAG_HIDDEN);
                lv_obj_align(sleep_label_, LV_ALIGN_CENTER, 85, -75 - (lv_tick_get() / 180 % 18));
            } else lv_obj_add_flag(sleep_label_, LV_OBJ_FLAG_HIDDEN);
            for (int i = 0; i < 4; ++i) {
                if (animation.sparkle) {
                    lv_obj_remove_flag(sparkles_[i], LV_OBJ_FLAG_HIDDEN);
                    lv_obj_align(sparkles_[i], LV_ALIGN_CENTER, (i % 2 ? 112 : -112),
                        -80 + (i / 2) * 125 - (lv_tick_get() / 160 + i) % 4 * 2);
                } else lv_obj_add_flag(sparkles_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        if (!requested && !reacting && conversation_face::Neutral(face_emotion)) {
            if (thinking)
                target = expressive_eyes::ForEmotion("thinking");
            else if (listening)
                target.height = 100;
        }
        expressive_eyes::Approach(pose_, target);
        UpdateMouth(face_emotion);
        const uint32_t now = lv_tick_get();
        // Different periods avoid mechanically synchronized gaze and blinks.
        const uint32_t phase = now % 4700;
        bool blink = phase < 110 || (now % 17300 < 100);
        int direction = static_cast<int>((now / 2900) % 5) - 2;
        look_x_ =
            expressive_eyes::Approach(look_x_, listening && voice ? 0 : direction * 8 + tilt_x_);
        look_y_ = expressive_eyes::Approach(look_y_, thinking ? -10 : 0);
        uint32_t color = 0x80D8FF;
        for (int i = 0; i < 2; ++i) {
            int height = expressive_eyes::EyeHeight(pose_, i, blink);
            // Small rhythmic movement while speaking; independent of audio tasks.
            if (speaking && !blink)
                height += (now / 160 % 3) * 2;
            lv_obj_set_size(eyes_[i], static_cast<int>(pose_.width), height);
            lv_obj_align(eyes_[i], LV_ALIGN_CENTER, (i ? 65 : -65) + static_cast<int>(look_x_),
                         static_cast<int>(pose_.y + look_y_));
            lv_obj_set_style_radius(eyes_[i], height < 40 ? 14 : 25, 0);
            lv_obj_set_style_bg_color(eyes_[i], lv_color_hex(color), 0);
            lv_obj_set_pos(lids_[i], -20, -60 + static_cast<int>(pose_.lid));
            lv_obj_set_style_transform_rotation(lids_[i],
                                                static_cast<int>(pose_.slope) * (i ? -1 : 1), 0);
        }
        UpdateFaceDetails(face_emotion, now, blink);
        // Base display methods can restore the stock emoji; keep our face exclusive.
        lv_obj_add_flag(emoji_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(emoji_image_, LV_OBJ_FLAG_HIDDEN);
    }

    void UpdateMouth(std::string_view emotion) {
        if (!mouth_line_) return;
        auto energy = mouth_audio::Read(static_cast<uint32_t>(esp_timer_get_time() / 1000));
        mouth_level_ += (static_cast<float>(energy) - mouth_level_) * 0.55f;
        bool open = mouth_level_ > 4;
        float curve = 5;
        if (emotion == "happy" || emotion == "glee" || emotion == "laughing" ||
            emotion == "loving" || emotion == "embarrassed") curve = 10;
        if (emotion == "sad" || emotion == "crying" || emotion == "worried" ||
            emotion == "angry" || emotion == "furious" || emotion == "frustrated") curve = -6;
        if (emotion == "surprised" || emotion == "shocked" || emotion == "scared" || emotion == "awe") {
            open = true;
            mouth_level_ = std::max(mouth_level_, 18.0f);
        }
        if (open) {
            lv_obj_add_flag(mouth_line_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(mouth_open_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_size(mouth_open_, 22 + static_cast<int>(mouth_level_ * 0.25f),
                            7 + static_cast<int>(mouth_level_ * 0.25f));
            lv_obj_align(mouth_open_, LV_ALIGN_CENTER, 0, 85);
        } else {
            lv_obj_add_flag(mouth_open_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(mouth_line_, LV_OBJ_FLAG_HIDDEN);
            for (int i = 0; i < 9; ++i) {
                float x = (i - 4) / 4.0f;
                mouth_points_[i] = {static_cast<lv_value_precise_t>(i * 6),
                    static_cast<lv_value_precise_t>(12 + curve * (1 - x * x))};
            }
            lv_line_set_points(mouth_line_, mouth_points_, 9);
            lv_obj_align(mouth_line_, LV_ALIGN_CENTER, 0, 78);
        }
    }

public:
    bool ShowExpression(const std::string& name, int seconds) {
        if (!expressive_eyes::IsCanonical(name))
            return false;
        DisplayLockGuard lock(this);
        requested_emotion_ = name;
        requested_until_ = esp_timer_get_time() + seconds * 1000000LL;
        return true;
    }

    void SetTilt(float x) {
        DisplayLockGuard lock(this);
        tilt_x_ = std::clamp(x * 16.0f, -18.0f, 18.0f);
    }

    void React(bool shake) {
        auto& app = Application::GetInstance();
        auto now = esp_timer_get_time();
        if (menu_open_ || app.GetDeviceState() != kDeviceStateIdle || now - last_reaction_ < 12000000)
            return;
        {
            DisplayLockGuard lock(this);
            last_reaction_ = now;
            kubi_.React(static_cast<uint32_t>(now / 1000), shake);
            reaction_until_ = now + 4000000;
            reaction_emotion_ = shake ? "angry" : "embarrassed";
            lv_label_set_text(chat_message_label_,
                              shake ? "Ei, com calma!" : "Fiquei envergonhada!");
        }
        // Enable the existing codec/power timer without filling queues on the main task.
        app.PlaySound({});
        auto sound = new std::string_view(shake ? reaction_sounds::shake_sound()
                                                : reaction_sounds::touch_sound());
        if (xTaskCreate(
                [](void* context) {
                    std::unique_ptr<std::string_view> clip(static_cast<std::string_view*>(context));
                    auto demuxer = std::make_unique<OggDemuxer>();
                    demuxer->OnPacket([](const uint8_t* data, int rate, int duration, size_t size) {
                        auto& app = Application::GetInstance();
                        if (app.GetDeviceState() != kDeviceStateIdle)
                            return;
                        auto packet = std::make_unique<AudioStreamPacket>();
                        packet->sample_rate = rate;
                        packet->frame_duration = duration;
                        packet->payload.assign(data, data + size);
                        app.GetAudioService().PushPacketToDecodeQueue(std::move(packet), false);
                        // Pace frames so starting a conversation can stop the reaction promptly.
                        vTaskDelay(pdMS_TO_TICKS(duration));
                    });
                    demuxer->Process(reinterpret_cast<const uint8_t*>(clip->data()), clip->size());
                    clip.reset();
                    demuxer.reset();
                    vTaskDelete(nullptr);
                },
                "reaction", 4096, sound, 1, nullptr) != pdPASS)
            delete sound;
        ESP_LOGI("Personality", "Reaction: %s", shake ? "shake" : "touch");
    }

    ~CustomLcdDisplay() override {
        DisplayLockGuard lock(this);
        if (face_timer_)
            lv_timer_delete(face_timer_);
        if (face_)
            lv_obj_delete(face_);
        heap_caps_free(formas_base_);
        heap_caps_free(formas_frame_);
    }

    void SetChatMessage(const char* role, const char* text) override {
        // Parent owns captions. Only a bounded visual hint is retained here.
        SpiLcdDisplay::SetChatMessage(role, text);
        if (!role || !text || !*text)
            return;
        bool user = !strcmp(role, "user");
        if (!user && strcmp(role, "assistant"))
            return;
        auto hint = conversation_face::Hint(text, user);
        DisplayLockGuard lock(this);
        if (!hint.empty()) {
            context_emotion_ = hint;
            context_until_ = esp_timer_get_time() + 10000000;
            ESP_LOGI("Personality", "Context visual hint: %s", context_emotion_.c_str());
        } else if (!user && context_until_ <= esp_timer_get_time()) {
            context_emotion_.clear();
        }
    }

    void SetEmotion(const char* emotion) override {
        DisplayLockGuard lock(this);
        emotion_ = emotion ? emotion : "neutral";
        if (face_)
            AnimateFace();
    }

    static void rounder_event_cb(lv_event_t* e) {
        lv_area_t* area = (lv_area_t*)lv_event_get_param(e);
        uint16_t x1 = area->x1;
        uint16_t x2 = area->x2;

        uint16_t y1 = area->y1;
        uint16_t y2 = area->y2;

        // round the start of coordinate down to the nearest 2M number
        area->x1 = (x1 >> 1) << 1;
        area->y1 = (y1 >> 1) << 1;
        // round the end of coordinate up to the nearest 2N+1 number
        area->x2 = ((x2 >> 1) << 1) + 1;
        area->y2 = ((y2 >> 1) << 1) + 1;
    }

    CustomLcdDisplay(esp_lcd_panel_io_handle_t io_handle, esp_lcd_panel_handle_t panel_handle,
                     int width, int height, int offset_x, int offset_y, bool mirror_x,
                     bool mirror_y, bool swap_xy)
        : SpiLcdDisplay(io_handle, panel_handle, width, height, offset_x, offset_y, mirror_x,
                        mirror_y, swap_xy) {
        // Note: UI customization should be done in SetupUI(), not in constructor
        // to ensure lvgl objects are created before accessing them
    }

    virtual void SetupUI() override {
        if (face_)
            return;
        // Call parent SetupUI() first to create all lvgl objects
        SpiLcdDisplay::SetupUI();

        DisplayLockGuard lock(this);
        lv_obj_set_style_pad_left(status_bar_, LV_HOR_RES * 0.1, 0);
        lv_obj_set_style_pad_right(status_bar_, LV_HOR_RES * 0.1, 0);
        lv_display_add_event_cb(display_, rounder_event_cb, LV_EVENT_INVALIDATE_AREA, NULL);
        // Board-local 280px vector face: no enlarged low-resolution bitmaps.
        face_ = lv_obj_create(lv_screen_active());
        lv_obj_set_size(face_, 280, 260);
        lv_obj_align(face_, LV_ALIGN_CENTER, 0, -8);
        lv_obj_set_style_bg_color(face_, lv_color_hex(0x101625), 0);
        lv_obj_set_style_bg_opa(face_, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(face_, 90, 0);
        lv_obj_set_style_border_width(face_, 0, 0);
        lv_obj_set_style_pad_all(face_, 0, 0);
        lv_obj_remove_flag(face_, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(face_, MenuTouch, LV_EVENT_ALL, this);
        InitFaceDetails();
        for (int i = 0; i < 2; ++i) {
            auto& eye = eyes_[i];
            eye = lv_obj_create(face_);
            lv_obj_set_style_radius(eye, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_border_width(eye, 0, 0);
            lv_obj_remove_flag(eye, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_remove_flag(eye, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_pad_all(eye, 0, 0);
            lids_[i] = lv_obj_create(eye);
            lv_obj_set_size(lids_[i], 120, 60);
            lv_obj_set_style_bg_color(lids_[i], lv_color_hex(0x101625), 0);
            lv_obj_set_style_bg_opa(lids_[i], LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(lids_[i], 0, 0);
            lv_obj_set_style_radius(lids_[i], 0, 0);
            lv_obj_set_style_transform_pivot_x(lids_[i], 60, 0);
            lv_obj_set_style_transform_pivot_y(lids_[i], 60, 0);
            lv_obj_remove_flag(lids_[i], LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_remove_flag(lids_[i], LV_OBJ_FLAG_CLICKABLE);
        }
        sleep_label_ = lv_label_create(face_);
        lv_label_set_text(sleep_label_, "Zzz");
        lv_obj_set_style_text_font(sleep_label_, &font_noto_sans_basic_20_4, 0);
        lv_obj_set_style_text_color(sleep_label_, lv_color_hex(0x80D8FF), 0);
        lv_obj_add_flag(sleep_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(sleep_label_, LV_OBJ_FLAG_CLICKABLE);
        for (int i = 0; i < 4; ++i) {
            sparkles_[i] = lv_label_create(face_);
            lv_label_set_text(sparkles_[i], "+");
            lv_obj_set_style_text_font(sparkles_[i], &font_noto_sans_basic_20_4, 0);
            lv_obj_set_style_text_color(sparkles_[i], lv_color_hex(0xFFE7A0), 0);
            lv_obj_add_flag(sparkles_[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(sparkles_[i], LV_OBJ_FLAG_CLICKABLE);
        }
        mouth_line_ = lv_line_create(face_);
        lv_obj_set_pos(mouth_line_, 116, 198);
        lv_obj_set_style_line_width(mouth_line_, 4, 0);
        lv_obj_set_style_line_rounded(mouth_line_, true, 0);
        lv_obj_set_style_line_color(mouth_line_, lv_color_hex(0x80D8FF), 0);
        lv_obj_remove_flag(mouth_line_, LV_OBJ_FLAG_CLICKABLE);
        mouth_open_ = lv_obj_create(face_);
        lv_obj_set_style_bg_color(mouth_open_, lv_color_hex(0x80D8FF), 0);
        lv_obj_set_style_bg_opa(mouth_open_, LV_OPA_TRANSP, 0);
        lv_obj_set_style_radius(mouth_open_, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(mouth_open_, 3, 0);
        lv_obj_set_style_border_color(mouth_open_, lv_color_hex(0x80D8FF), 0);
        lv_obj_remove_flag(mouth_open_, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(mouth_open_, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(mouth_open_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_width(bottom_bar_, 330);
        lv_obj_align(bottom_bar_, LV_ALIGN_BOTTOM_MID, 0, -38);
        lv_obj_set_width(chat_message_label_, 310);
        InitFormas();
        ApplySkin();
        AnimateFace();
        face_timer_ = lv_timer_create(
            [](lv_timer_t* timer) {
                static_cast<CustomLcdDisplay*>(lv_timer_get_user_data(timer))->AnimateFace();
            },
            50, this);
    }
};

class CustomBacklight : public Backlight {
public:
    CustomBacklight(esp_lcd_panel_io_handle_t panel_io) : Backlight(), panel_io_(panel_io) {}

protected:
    esp_lcd_panel_io_handle_t panel_io_;

    virtual void SetBrightnessImpl(uint8_t brightness) override {
        auto display = Board::GetInstance().GetDisplay();
        DisplayLockGuard lock(display);
        uint8_t data[1] = {((uint8_t)((255*  brightness) / 100))};
        int lcd_cmd = 0x51;
        lcd_cmd &= 0xff;
        lcd_cmd <<= 8;
        lcd_cmd |= LCD_OPCODE_WRITE_CMD << 24;
        esp_lcd_panel_io_tx_param(panel_io_, lcd_cmd, &data, sizeof(data));
    }
};

class WaveshareEsp32s3TouchAMOLED1inch75 : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_;
    Pmic* pmic_ = nullptr;
    PortugalWeather weather_;
    Button boot_button_;
    CustomLcdDisplay* display_;
    CustomBacklight* backlight_;
    esp_io_expander_handle_t io_expander = NULL;
    PowerSaveTimer* power_save_timer_;

    void InitializePowerSaveTimer() {
        power_save_timer_ = new PowerSaveTimer(-1, 60, 300);
        power_save_timer_->OnEnterSleepMode([this]() {
            GetDisplay()->SetPowerSaveMode(true);
            GetBacklight()->SetBrightness(20); });
        power_save_timer_->OnExitSleepMode([this]() {
            GetDisplay()->SetPowerSaveMode(false);
            GetBacklight()->RestoreBrightness(); });
        power_save_timer_->OnShutdownRequest([this](){ 
            pmic_->PowerOff(); });
        power_save_timer_->SetEnabled(true);
    }

    void InitializeCodecI2c() {
        // Initialize I2C peripheral
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_));
    }

    void InitializeTca9554(void) {
        esp_err_t ret = esp_io_expander_new_i2c_tca9554(i2c_bus_, I2C_ADDRESS, &io_expander);
        if (ret != ESP_OK)
            ESP_LOGE(TAG, "TCA9554 create returned error");
        ret = esp_io_expander_set_dir(io_expander, IO_EXPANDER_PIN_NUM_4, IO_EXPANDER_INPUT);
        ESP_ERROR_CHECK(ret);
    }

    void InitializeAxp2101() {
        ESP_LOGI(TAG, "Init AXP2101");
        pmic_ = new Pmic(i2c_bus_, 0x34);
    }

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.sclk_io_num = EXAMPLE_PIN_NUM_LCD_PCLK;
        buscfg.data0_io_num = EXAMPLE_PIN_NUM_LCD_DATA0;
        buscfg.data1_io_num = EXAMPLE_PIN_NUM_LCD_DATA1;
        buscfg.data2_io_num = EXAMPLE_PIN_NUM_LCD_DATA2;
        buscfg.data3_io_num = EXAMPLE_PIN_NUM_LCD_DATA3;
        buscfg.max_transfer_sz = DISPLAY_WIDTH*  DISPLAY_HEIGHT*  sizeof(uint16_t);
        buscfg.flags = SPICOMMON_BUSFLAG_QUAD;
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });

#if CONFIG_DUALBOOT_XIAO_NVS
        boot_button_.OnDoubleClick([]() {
            Application::GetInstance().Schedule([]() {
                const auto* menu = esp_partition_find_first(
                    ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, "selector");
                if (!menu || menu->address != 0xC00000) {
                    ESP_LOGE(TAG, "Menu partition unavailable; staying in XiaoZhi");
                    return;
                }
                esp_err_t err = esp_ota_set_boot_partition(menu);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Cannot select menu: %s", esp_err_to_name(err));
                    return;
                }
                ESP_LOGI(TAG, "Double click: returning to Muse / XiaoZhi menu");
                esp_restart();
            });
        });
#elif CONFIG_USE_DEVICE_AEC
        boot_button_.OnDoubleClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateIdle) {
                app.SetAecMode(app.GetAecMode() == kAecOff ? kAecOnDeviceSide : kAecOff);
            }
        });
#endif
    }

    void InitializeDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        // 液晶屏控制IO初始化
        ESP_LOGD(TAG, "Install panel IO");
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS;
        io_config.dc_gpio_num = GPIO_NUM_NC;
        io_config.spi_mode = 0;
        io_config.pclk_hz = 40 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 32;
        io_config.lcd_param_bits = 8;
        io_config.flags.quad_mode = true;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &panel_io));

        // 初始化液晶屏驱动芯片
        ESP_LOGD(TAG, "Install LCD driver");
        const co5300_vendor_config_t vendor_config = {
            .init_cmds = &vendor_specific_init[0],
            .init_cmds_size = sizeof(vendor_specific_init) / sizeof(co5300_lcd_init_cmd_t),
            .flags = {
                .use_qspi_interface = 1,
            }};

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST;
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.bits_per_pixel = 16;
        panel_config.vendor_config = (void* )&vendor_config;
        ESP_ERROR_CHECK(esp_lcd_new_panel_co5300(panel_io, &panel_config, &panel));
        esp_lcd_panel_set_gap(panel, 0x06, 0);
        esp_lcd_panel_reset(panel);
        esp_lcd_panel_init(panel);
        esp_lcd_panel_invert_color(panel, false);
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
        esp_lcd_panel_disp_on_off(panel, true);
        display_ = new CustomLcdDisplay(panel_io, panel,
                                        DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
        backlight_ = new CustomBacklight(panel_io);
        backlight_->RestoreBrightness();
    }

    void InitializeTouch() {
        esp_lcd_touch_handle_t tp;
        esp_lcd_touch_config_t tp_cfg = {
            .x_max = DISPLAY_WIDTH - 1,
            .y_max = DISPLAY_HEIGHT - 1,
            .rst_gpio_num = PIN_NUM_TOUCH_RST,
            .int_gpio_num = PIN_NUM_TOUCH_INT,
            .levels = {
                .reset = 0,
                .interrupt = 0,
            },
            .flags = {
                .swap_xy = 0,
                .mirror_x = 1,
                .mirror_y = 1,
            },
        };
        esp_lcd_panel_io_handle_t tp_io_handle = NULL;
        esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_CST9217_CONFIG();
        tp_io_config.scl_speed_hz = 400*  1000;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus_, &tp_io_config, &tp_io_handle));
        ESP_LOGI(TAG, "Initialize touch controller");
        ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_cst9217(tp_io_handle, &tp_cfg, &tp));
        const lvgl_port_touch_cfg_t touch_cfg = {
            .disp = lv_display_get_default(),
            .handle = tp,
        };
        lvgl_port_add_touch(&touch_cfg);
        ESP_LOGI(TAG, "Touch panel initialized successfully");
    }

void InitializeMotion() {
    xTaskCreate(
        [](void* arg) {
            auto self = static_cast<WaveshareEsp32s3TouchAMOLED1inch75*>(arg);
            i2c_master_dev_handle_t device = nullptr;
            i2c_device_config_t config = {};
            config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
            config.scl_speed_hz = 100000;
            uint8_t identity = 0, reg = 0;
            for (uint8_t address : {0x6a, 0x6b}) {
                config.device_address = address;
                if (i2c_master_bus_add_device(self->i2c_bus_, &config, &device) != ESP_OK)
                    continue;
                if (i2c_master_transmit_receive(device, &reg, 1, &identity, 1, 100) == ESP_OK &&
                    identity == 5)
                    break;
                i2c_master_bus_rm_device(device);
                device = nullptr;
            }
            if (!device) {
                ESP_LOGW("Personality", "QMI8658 not found; motion disabled");
                vTaskDelete(nullptr);
                return;
            }
            const uint8_t setup[][2] = {{0x08, 0x00}, {0x02, 0x40}, {0x03, 0x27}, {0x08, 0x01}};
            for (auto& command : setup) {
                if (i2c_master_transmit(device, command, 2, 100) != ESP_OK) {
                    ESP_LOGW("Personality", "IMU configuration failed");
                    i2c_master_bus_rm_device(device);
                    vTaskDelete(nullptr);
                    return;
                }
            }
            ESP_LOGI("Personality", "QMI8658 ready, accelerometer 8g / 62.5Hz");
            MotionFilter filter;
            unsigned errors = 0;
            while (true) {
                vTaskDelay(pdMS_TO_TICKS(50));
                // Personality reactions only run at rest; leave the shared bus free
                // while connecting, updating or handling a voice conversation.
                if (Application::GetInstance().GetDeviceState() != kDeviceStateIdle) continue;
                uint8_t data[6];
                reg = 0x35;
                if (i2c_master_transmit_receive(device, &reg, 1, data, 6, 30) != ESP_OK) {
                    if (++errors >= 10)
                        break;
                    continue;
                }
                errors = 0;
                float x = static_cast<int16_t>(data[0] | data[1] << 8) / 4096.0f;
                float y = static_cast<int16_t>(data[2] | data[3] << 8) / 4096.0f;
                float z = static_cast<int16_t>(data[4] | data[5] << 8) / 4096.0f;
                self->display_->SetTilt(x);
                if (filter.Observe(x, y, z, esp_timer_get_time() / 1000)) {
                    Application::GetInstance().Schedule([self]() { self->display_->React(true); });
                }
            }
            ESP_LOGW("Personality", "IMU read failures; motion disabled");
            i2c_master_bus_rm_device(device);
            vTaskDelete(nullptr);
        },
        "personality", 4096, this, 1, nullptr);
}

// 初始化工具
void InitializeTools() {
    auto& mcp_server = McpServer::GetInstance();
    mcp_server.AddTool(
        "self.face.show_expression",
        "Show a requested eye expression for 1-10 seconds without interrupting speech. "
        "Use when the user asks to see an emotion. expression must be one of: "
        "normal, angry, glee, happy, sad, worried, focused, annoyed, surprised, skeptic, "
        "frustrated, unimpressed, sleepy, suspicious, squint, furious, scared, awe. "
        "For Portuguese requests translate the emotion to this name. Do not read the code aloud.",
        PropertyList({Property("expression", kPropertyTypeString),
                      Property("seconds", kPropertyTypeInteger, 4, 1, 10)}),
        [this](const PropertyList& args) -> ToolResult {
            if (!display_->ShowExpression(args["expression"].value<std::string>(),
                                          args["seconds"].value<int>()))
                return std::unexpected("Expressão desconhecida. Usa um dos 18 nomes indicados.");
            return true;
        });
    mcp_server.AddTool(
        "self.weather.portugal",
        "Get real IPMA daily forecasts for Portugal. Prefer this tool for Portuguese weather. "
        "city: Portugal for a regional sample, or district capital/island such as Lisboa, Porto, "
        "Faro, Funchal, Ponta Delgada. "
        "day: 0 today, 1 tomorrow, 2 day after. State the forecast date and place. Do not invent "
        "current temperature.",
        PropertyList({Property("city", kPropertyTypeString, std::string("Portugal")),
                      Property("day", kPropertyTypeInteger, 0, 0, 2)}),
        [this](const PropertyList& args) -> ToolResult { return weather_.Get(args); });
    mcp_server.AddTool("self.system.reconfigure_wifi",
                       "End this conversation and enter WiFi configuration mode.\n"
                       "**CAUTION** You must ask the user to confirm this action.",
                       PropertyList(), [this](const PropertyList& properties) {
                           EnterWifiConfigMode();
                           return true;
                       });
}

public:
    WaveshareEsp32s3TouchAMOLED1inch75() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializePowerSaveTimer();
        InitializeCodecI2c();
#if CONFIG_BOARD_TYPE_WAVESHARE_ESP32_S3_TOUCH_AMOLED_1_75
        InitializeTca9554();
#endif
        InitializeAxp2101();
        InitializeSpi();
        InitializeDisplay();
        InitializeTouch();
        InitializeButtons();
        InitializeTools();
        InitializeMotion();
        weather_.Start();
#if CONFIG_DUALBOOT_XIAO_NVS
        // Avoid acoustic self-interruption on this enclosure. AutoStop waits
        // for reply playback to drain before reopening voice capture.
        Application::GetInstance().SetAecMode(kAecOff);
        ESP_LOGI(TAG, "Conversation uses turn-taking to avoid speaker echo");
#endif
    }

    virtual AudioCodec* GetAudioCodec() override {
        static ExpressiveAudioCodec audio_codec(
            i2c_bus_, 
            AUDIO_INPUT_SAMPLE_RATE, 
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK, 
            AUDIO_I2S_GPIO_BCLK, 
            AUDIO_I2S_GPIO_WS, 
            AUDIO_I2S_GPIO_DOUT, 
            AUDIO_I2S_GPIO_DIN,
            AUDIO_CODEC_PA_PIN, 
            AUDIO_CODEC_ES8311_ADDR, 
            AUDIO_CODEC_ES7210_ADDR, 
            AUDIO_INPUT_REFERENCE);
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }

    virtual Backlight* GetBacklight() override {
        return backlight_;
    }

    virtual bool GetBatteryLevel(int &level, bool &charging, bool &discharging) override {
        static bool last_discharging = false;
        if (!pmic_->TryReadBattery(level, charging, discharging)) {
            static int64_t last_warning = -30000000;
            auto now = esp_timer_get_time();
            if (now - last_warning >= 30000000) {
                ESP_LOGW(TAG, "Battery status temporarily unavailable; keeping application running");
                last_warning = now;
            }
            return false;
        }
        if (discharging != last_discharging)
        {
            power_save_timer_->SetEnabled(discharging);
            last_discharging = discharging;
        }

        return true;
    }

    virtual void SetPowerSaveLevel(PowerSaveLevel level) override {
        if (level != PowerSaveLevel::LOW_POWER) {
            power_save_timer_->WakeUp();
        }
        WifiBoard::SetPowerSaveLevel(level);
    }
};

DECLARE_BOARD(WaveshareEsp32s3TouchAMOLED1inch75);
