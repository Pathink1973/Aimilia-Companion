// Pin mapping and display/touch initialization: Waveshare's 1.75C BSP 3.0.0,
// also used by the vendor's examples/esp-idf/02_lvgl_demo_v9/main/main.c.
#include "bsp/display.h"
#include "bsp/esp-bsp.h"
#include "bsp/touch.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "selector_ui.h"

static TaskHandle_t main_task;
static lv_obj_t *status_label;

static void round_area(lv_event_t *event) {
  lv_area_t *area = lv_event_get_param(event);
  area->x1 &= ~1;
  area->y1 &= ~1;
  area->x2 |= 1;
  area->y2 |= 1;
}

static lv_display_t *start_display(void) {
  // The vendor helper sends directly from PSRAM, which produced SPI DMA
  // underflows on this unit. A small internal-RAM pair needs no bounce copy.
  // Keep panel initialization and the LVGL task on CPU0 for SPI completion.
  esp_lv_adapter_config_t adapter = ESP_LV_ADAPTER_DEFAULT_CONFIG();
  adapter.task_core_id = 0;
  ESP_ERROR_CHECK(esp_lv_adapter_init(&adapter));
  esp_lcd_panel_handle_t panel;
  esp_lcd_panel_io_handle_t io;
  const bsp_display_config_t panel_config = {.max_transfer_sz = 466 * 20 * 2};
  ESP_ERROR_CHECK(bsp_display_new(&panel_config, &panel, &io));
  const esp_lv_adapter_display_config_t config = {
      .panel = panel,
      .panel_io = io,
      .profile =
          {
              .interface = ESP_LV_ADAPTER_PANEL_IF_OTHER,
              .rotation = ESP_LV_ADAPTER_ROTATE_0,
              .hor_res = 466,
              .ver_res = 466,
              .buffer_height = 20,
              .use_psram = false,
              .require_double_buffer = true,
          },
      .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_NONE,
  };
  lv_display_t *display = esp_lv_adapter_register_display(&config);
  if (!display)
    return NULL;
  lv_display_add_event_cb(display, round_area, LV_EVENT_INVALIDATE_AREA, NULL);
  const bsp_display_cfg_t touch_config = {
      .touch_flags = {.mirror_x = 1, .mirror_y = 1},
  };
  esp_lcd_touch_handle_t touch;
  ESP_ERROR_CHECK(bsp_touch_new(&touch_config, &touch));
  const esp_lv_adapter_touch_config_t input =
      ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(display, touch);
  if (!esp_lv_adapter_register_touch(&input))
    return NULL;
  ESP_ERROR_CHECK(esp_lv_adapter_start());
  return display;
}

static void choose(lv_event_t *event) {
  uint32_t selection = (uint32_t)(uintptr_t)lv_event_get_user_data(event);
  xTaskNotify(main_task, selection, eSetValueWithOverwrite);
}

void app_main(void) {
  main_task = xTaskGetCurrentTaskHandle();
  if (!start_display()) {
    ESP_LOGE("selector", "Display failed; USB recovery is available");
    return;
  }
  bsp_display_lock(-1);
  lv_obj_t *screen = lv_screen_active();
  selector_build_ui(screen, choose, &status_label);
  bsp_display_unlock();
  ESP_LOGI("selector", "Ready: Muse / XiaoZhi");

  for (;;) {
    uint32_t selection;
    if (!xTaskNotifyWait(0, UINT32_MAX, &selection, portMAX_DELAY))
      continue;
    if (selection != 1 && selection != 2)
      continue;
    const char *label = selection == 1 ? "ota_0" : "xiaozhi";
    const esp_partition_t *app = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, label);
    esp_err_t err = app ? esp_ota_set_boot_partition(app) : ESP_ERR_NOT_FOUND;
    if (err != ESP_OK) {
      ESP_LOGE("selector", "Cannot start %s: %s", label, esp_err_to_name(err));
      bsp_display_lock(-1);
      lv_label_set_text(status_label, "Sistema indisponivel");
      bsp_display_unlock();
      continue;
    }
    ESP_LOGI("selector", "Starting %s", label);
    vTaskDelay(pdMS_TO_TICKS(150));
    esp_restart();
  }
}
