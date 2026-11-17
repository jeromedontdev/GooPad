/*
 * Persisted user settings: volume and backlight brightness.
 *
 * Both are stored in NVS so they survive a reboot. The values are the
 * user-facing 0-100 percentages, not the scaled duty/amplitude, so the
 * power budget in board.h can be changed without invalidating what the
 * user previously chose.
 *
 * Writes are debounced: dragging a slider fires LV_EVENT_VALUE_CHANGED on
 * every pixel, and committing to NVS each time would wear the flash and
 * stall the UI. A dirty flag plus a periodic flush from the refresh timer
 * keeps it to one write per settle.
 */
#include "app.h"
#include "board.h"

#include "esp_log.h"
#include "nvs.h"
#include <stdbool.h>

static const char *TAG = "settings";

#define NVS_NS      "hwtest"
#define KEY_VOLUME  "volume"
#define KEY_BL      "backlight"

/* Last values written to NVS, so flush() can skip no-op writes. Dragging a
 * slider fires an event per pixel; without this the flash would be written
 * several times a second. */
static int s_saved_volume = -1;
static int s_saved_backlight = -1;

void settings_load(void)
{
    nvs_handle_t nvs;
    if (nvs_open(NVS_NS, NVS_READONLY, &nvs) != ESP_OK) {
        ESP_LOGI(TAG, "no saved settings, using defaults");
        s_saved_volume = hw_audio_get_volume();
        s_saved_backlight = board_backlight_get();
        return;
    }

    uint8_t vol = BSP_VOLUME_DEFAULT_PCT;
    uint8_t bl = BSP_BACKLIGHT_DEFAULT_PCT;
    bool have_vol = (nvs_get_u8(nvs, KEY_VOLUME, &vol) == ESP_OK);
    bool have_bl = (nvs_get_u8(nvs, KEY_BL, &bl) == ESP_OK);
    nvs_close(nvs);

    if (have_vol && vol <= 100) {
        hw_audio_set_volume(vol);
        ESP_LOGI(TAG, "restored volume %u%%", vol);
    }
    if (have_bl && bl <= 100) {
        board_backlight_set(bl);
        ESP_LOGI(TAG, "restored backlight %u%%", bl);
    }

    s_saved_volume = hw_audio_get_volume();
    s_saved_backlight = board_backlight_get();
}

static void save_u8(const char *key, uint8_t value)
{
    nvs_handle_t nvs;
    if (nvs_open(NVS_NS, NVS_READWRITE, &nvs) != ESP_OK) {
        return;
    }
    nvs_set_u8(nvs, key, value);
    nvs_commit(nvs);
    nvs_close(nvs);
}

/* Called from the UI refresh timer. Writes only when a value has actually
 * changed since the last save, so a slider drag costs one write when it
 * settles rather than one per pixel. */
void settings_flush(void)
{
    int vol = hw_audio_get_volume();
    int bl = board_backlight_get();

    if (vol != s_saved_volume) {
        save_u8(KEY_VOLUME, (uint8_t)vol);
        s_saved_volume = vol;
        ESP_LOGI(TAG, "saved volume %d%%", vol);
    }
    if (bl != s_saved_backlight) {
        save_u8(KEY_BL, (uint8_t)bl);
        s_saved_backlight = bl;
        ESP_LOGI(TAG, "saved backlight %d%%", bl);
    }
}
