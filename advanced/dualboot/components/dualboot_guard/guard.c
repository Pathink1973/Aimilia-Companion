// Integration layer: original application sources are not changed.
// Each application returns to the factory selector on its next restart.
#include <string.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "nvs_flash.h"

void __real_app_main(void);

void __wrap_app_main(void)
{
    const esp_partition_t *menu = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, "selector");
    if (!menu || menu->address != 0xC00000) {
        ESP_LOGE("dualboot", "Shared partition table missing; stopping");
        return;
    }
    // A selected OTA image may be pending verification. Confirm it before
    // switching the next boot to factory, otherwise IDF rejects the switch.
    esp_err_t valid = esp_ota_mark_app_valid_cancel_rollback();
    if (valid != ESP_OK && valid != ESP_ERR_NOT_FOUND) {
        ESP_LOGW("dualboot", "App confirmation: %s", esp_err_to_name(valid));
    }
    ESP_ERROR_CHECK(esp_ota_set_boot_partition(menu));
    ESP_LOGI("dualboot", "Next restart opens Muse / XiaoZhi selector");
    __real_app_main();
}

// The normal OTA algorithm sees both products' slots. Reject firmware writes
// at the common IDF entry points so neither product can overwrite the other.
esp_err_t __wrap_esp_ota_begin(const esp_partition_t *p, size_t size,
                             esp_ota_handle_t *out)
{
    (void)p; (void)size;
    if (out) *out = 0;
    ESP_LOGW("dualboot", "Firmware update disabled in dual boot");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t __wrap_esp_ota_resume(const esp_partition_t *p, size_t erase_size,
                              size_t offset, esp_ota_handle_t *out)
{
    (void)offset;
    return __wrap_esp_ota_begin(p, erase_size, out);
}

#if CONFIG_DUALBOOT_XIAO_NVS
// Redirect the default NVS APIs, including Wi-Fi driver's nvs_open callback.
// Explicit non-default partitions (e.g. nvsfactory) retain their semantics.
static const char *private_partition(const char *name)
{
    return !name || strcmp(name, "nvs") == 0 ? "xiao_nvs" : name;
}
esp_err_t __real_nvs_open_from_partition(const char *, const char *,
                                       nvs_open_mode_t, nvs_handle_t *);
esp_err_t __real_nvs_flash_init_partition(const char *);
esp_err_t __real_nvs_flash_erase_partition(const char *);
esp_err_t __real_nvs_flash_deinit_partition(const char *);

esp_err_t __wrap_nvs_open_from_partition(const char *p, const char *ns,
                                        nvs_open_mode_t mode, nvs_handle_t *h)
{ return __real_nvs_open_from_partition(private_partition(p), ns, mode, h); }
esp_err_t __wrap_nvs_open(const char *ns, nvs_open_mode_t mode, nvs_handle_t *h)
{ return __real_nvs_open_from_partition("xiao_nvs", ns, mode, h); }
esp_err_t __wrap_nvs_flash_init_partition(const char *p)
{ return __real_nvs_flash_init_partition(private_partition(p)); }
esp_err_t __wrap_nvs_flash_init(void)
{ return __real_nvs_flash_init_partition("xiao_nvs"); }
esp_err_t __wrap_nvs_flash_erase_partition(const char *p)
{ return __real_nvs_flash_erase_partition(private_partition(p)); }
esp_err_t __wrap_nvs_flash_erase(void)
{ return __real_nvs_flash_erase_partition("xiao_nvs"); }
esp_err_t __wrap_nvs_flash_deinit_partition(const char *p)
{ return __real_nvs_flash_deinit_partition(private_partition(p)); }
esp_err_t __wrap_nvs_flash_deinit(void)
{ return __real_nvs_flash_deinit_partition("xiao_nvs"); }
#endif
