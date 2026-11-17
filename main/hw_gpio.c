/*
 * System information: chip, flash, PSRAM, heap.
 */
#include "app.h"
#include "board.h"

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "nvs.h"

static const char *TAG = "info";

static const char *reset_reason_str(esp_reset_reason_t r)
{
    switch (r) {
        case ESP_RST_POWERON:   return "power-on";
        case ESP_RST_EXT:       return "external pin";
        case ESP_RST_SW:        return "software";
        case ESP_RST_PANIC:     return "panic";
        case ESP_RST_INT_WDT:   return "interrupt watchdog";
        case ESP_RST_TASK_WDT:  return "task watchdog";
        case ESP_RST_WDT:       return "other watchdog";
        case ESP_RST_DEEPSLEEP: return "deep sleep wake";
        case ESP_RST_BROWNOUT:  return "BROWNOUT (supply dipped)";
        case ESP_RST_SDIO:      return "SDIO";
        default:                return "unknown";
    }
}

void hw_info_init(void)
{
    esp_chip_info_t chip;
    esp_chip_info(&chip);

    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);

    size_t psram = esp_psram_is_initialized() ? esp_psram_get_size() : 0;

    ESP_LOGI(TAG, "chip: ESP32-S3 rev %d.%d, %d core(s)",
             chip.revision / 100, chip.revision % 100, chip.cores);
    ESP_LOGI(TAG, "flash: %lu MB", (unsigned long)(flash_size / (1024 * 1024)));
    ESP_LOGI(TAG, "psram: %u KB", (unsigned)(psram / 1024));
    ESP_LOGI(TAG, "heap internal: %u KB free of %u KB",
             (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
             (unsigned)(heap_caps_get_total_size(MALLOC_CAP_INTERNAL) / 1024));
    ESP_LOGI(TAG, "heap psram: %u KB free",
             (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));

    esp_reset_reason_t reason = esp_reset_reason();
    ESP_LOGI(TAG, "reset reason: %s", reset_reason_str(reason));

    /* Count boots in NVS so a reset loop is visible rather than looking
     * like a single unexplained restart. */
    nvs_handle_t nvs;
    uint32_t boots = 0;
    uint32_t brownouts = 0;
    if (nvs_open("hwtest", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_get_u32(nvs, "boots", &boots);
        nvs_get_u32(nvs, "brownouts", &brownouts);
        boots++;
        /* Persist the brownout tally: it lives in NVS because the whole
         * point is to survive the reset it is counting. */
        if (reason == ESP_RST_BROWNOUT) {
            brownouts++;
            nvs_set_u32(nvs, "brownouts", brownouts);
        }
        nvs_set_u32(nvs, "boots", boots);
        nvs_commit(nvs);
        nvs_close(nvs);
    }
    ESP_LOGI(TAG, "boot count: %lu (brownouts: %lu)",
             (unsigned long)boots, (unsigned long)brownouts);

    if (reason == ESP_RST_BROWNOUT) {
        ESP_LOGW(TAG, "  supply dipped below the brownout threshold. With no "
                      "battery fitted this is the USB 5 V rail sagging - "
                      "check the cable and host port.");
    } else if (reason == ESP_RST_PANIC || reason == ESP_RST_TASK_WDT ||
               reason == ESP_RST_INT_WDT) {
        ESP_LOGE(TAG, "  previous boot ended in a crash, not a power event");
    }

    /* Record the reset reason so the Power tab can show a history even on
     * units where the IP5306 cannot be read. Without this the only power
     * evidence is a single line in the boot log that scrolls away. */
    g_app.power.last_reset = reason;
    g_app.power.boot_count = boots;
    g_app.power.brownout_count = brownouts;
}
