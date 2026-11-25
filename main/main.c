/*
 * GooPad macro-pad firmware for the Guition ESP32-S3-4848S040.
 */
#include "app.h"
#include "board.h"
#include "goopad_diagnostics.h"

#include "cJSON.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"

#include <stdlib.h>

static const char *TAG = "main";

static void *json_malloc(size_t size)
{
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== GooPad starting ===");

    cJSON_Hooks json_hooks = {.malloc_fn = json_malloc, .free_fn = free};
    cJSON_InitHooks(&json_hooks);

    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed (%s); preserving stored configuration", esp_err_to_name(err));
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

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

    goopad_ui_boot(disp);
    if (hw_touch_init(disp) != ESP_OK) {
        ESP_LOGE(TAG, "touch init failed");
    }
    if (hw_sdcard_init() != ESP_OK) {
        ESP_LOGI(TAG, "no SD card; backup is unavailable");
    }

    if (hw_wifi_init() != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi init failed");
    }
    if (hw_audio_init() != ESP_OK) {
        ESP_LOGE(TAG, "audio init failed; sound actions unavailable");
    }
    goopad_diag_init();
    goopad_init();
    goopad_diag_serial_init();
    goopad_ui_init(disp);
    esp_ota_mark_app_valid_cancel_rollback();
    ESP_LOGI(TAG, "GooPad ready");
}
