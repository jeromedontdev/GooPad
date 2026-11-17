/*
 * ESP32-S3-4848S040 hardware test firmware.
 *
 * Brings up every peripheral on the board and presents an interactive
 * LVGL test UI so each one can be exercised and judged by eye.
 */
#include "app.h"
#include "board.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

static const char *TAG = "main";

static void power_poll_task(void *arg)
{
    (void)arg;
    while (1) {
        hw_power_poll();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static const char *result_str(test_status_t st)
{
    switch (st) {
        case TEST_PASS: return "PASS";
        case TEST_FAIL: return "FAIL";
        case TEST_SKIP: return "SKIP";
        default:        return "----";
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32-S3-4848S040 hardware test ===");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    hw_info_init();

    if (board_init() != ESP_OK) {
        ESP_LOGE(TAG, "board init failed");
    }

    board_i2c_scan();
    g_app.results.i2c = (g_app.i2c_found_count > 0) ? TEST_PASS : TEST_FAIL;

    /* Display first: everything else reports through it. */
    lv_display_t *disp = NULL;
    if (hw_display_init(&disp) != ESP_OK) {
        ESP_LOGE(TAG, "display init failed - halting");
        g_app.results.display = TEST_FAIL;
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    ui_init(disp);

    /* Power before touch: the GT911 driver polls continuously once started,
     * and the IP5306 is sensitive to a busy bus during its first probe. */
    if (hw_power_init() != ESP_OK) {
        ESP_LOGW(TAG, "power init failed");
    }
    if (hw_touch_init(disp) != ESP_OK) {
        ESP_LOGW(TAG, "touch init failed");
    }
    if (hw_sdcard_init() != ESP_OK) {
        ESP_LOGW(TAG, "no SD card");
    }
    if (hw_audio_init() != ESP_OK) {
        ESP_LOGW(TAG, "audio init failed");
    }

    /* Restore saved volume and brightness before the UI is built, so the
     * sliders are created already showing the right values. */
    settings_load();

    /* Radios are initialised but left idle: they are the largest current
     * consumers after the backlight, and this board has no bulk
     * capacitance to absorb a burst. Scanning is on demand from the UI or
     * the console, and the interface is stopped again afterwards. */
    if (hw_wifi_init() != ESP_OK) {
        ESP_LOGW(TAG, "wifi init failed");
    }
    if (hw_ble_init() != ESP_OK) {
        ESP_LOGW(TAG, "ble init failed");
    }

    xTaskCreate(power_poll_task, "power_poll", 3072, NULL, 3, NULL);

    ui_refresh();

    /* Serial console, so audio and slot width can be exercised without
     * depending on the touch panel or the LVGL task. */
    console_init();

    ESP_LOGI(TAG, "=== all subsystems initialised ===");
    ESP_LOGI(TAG, "display=%s touch=%s power=%s sd=%s audio=%s i2c=%s wifi=%s ble=%s",
             result_str(g_app.results.display),
             result_str(g_app.results.touch),
             result_str(g_app.results.power),
             result_str(g_app.results.sdcard),
             result_str(g_app.results.audio),
             result_str(g_app.results.i2c),
             result_str(g_app.results.wifi),
             result_str(g_app.results.ble));

    /* Play a short confirmation chirp. */
    hw_audio_play_tone(880, 150);
}
