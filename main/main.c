/*
 * GooPad macro-pad firmware for the Guition ESP32-S3-4848S040.
 */
#include "app.h"
#include "board.h"

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "=== GooPad starting ===");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    if (board_init() != ESP_OK) {
        ESP_LOGE(TAG, "board init failed");
    }

    /* Display first: everything else reports through it. */
    lv_display_t *disp = NULL;
    if (hw_display_init(&disp) != ESP_OK) {
        ESP_LOGE(TAG, "display init failed - halting");
        g_app.results.display = TEST_FAIL;
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    if (hw_touch_init(disp) != ESP_OK) {
        ESP_LOGE(TAG, "touch init failed");
    }
    if (hw_sdcard_init() != ESP_OK) {
        ESP_LOGI(TAG, "no SD card; backup is unavailable");
    }

    if (hw_wifi_init() != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi init failed");
    }
    goopad_init();
    goopad_ui_init(disp);
    esp_ota_mark_app_valid_cancel_rollback();
    ESP_LOGI(TAG, "GooPad ready");
}
