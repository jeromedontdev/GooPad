/*
 * Shared application state for the 4848S040 hardware test firmware.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "lvgl.h"

typedef enum {
    TEST_UNKNOWN = 0,
    TEST_PASS,
    TEST_FAIL,
    TEST_SKIP,
} test_status_t;

typedef struct {
    bool present;
    bool charging;
    bool charge_full;
    int level_pct;      /* -1 when unknown */
    uint8_t reg70;
    uint8_t reg71;
    uint8_t reg72;
    uint8_t reg77;
    uint8_t reg78;

    /* Power history, kept even when the PMIC cannot be read. */
    int last_reset;         /* esp_reset_reason_t of the previous boot */
    uint32_t boot_count;
    uint32_t brownout_count;
} power_info_t;

typedef struct {
    bool mounted;
    char card_name[16];
    uint64_t capacity_bytes;
    uint32_t sector_size;
    int file_count;
    bool write_ok;
    char last_error[64];
} sd_info_t;

typedef struct {
    test_status_t display;
    test_status_t touch;
    test_status_t power;
    test_status_t sdcard;
    test_status_t audio;
    test_status_t i2c;
    test_status_t wifi;
    test_status_t ble;
} test_results_t;

/* WiFi scan results. */
#define HW_WIFI_MAX_APS 12

typedef struct {
    char ssid[33];
    int rssi;
    int channel;
    int auth;
} wifi_ap_t;

typedef struct {
    bool ready;
    volatile bool scanning;
    int ap_count;
    uint32_t scan_count;
    wifi_ap_t aps[HW_WIFI_MAX_APS];
} wifi_info_t;

/* BLE scan results. */
#define HW_BLE_MAX_DEVICES 12

typedef struct {
    char name[24];
    int rssi;
    int addr_type;
} ble_dev_t;

typedef struct {
    bool ready;
    volatile bool scanning;
    int device_count;
    uint32_t scan_count;
    ble_dev_t devices[HW_BLE_MAX_DEVICES];
} ble_info_t;

typedef struct {
    test_results_t results;
    power_info_t power;
    sd_info_t sd;
    wifi_info_t wifi;
    ble_info_t ble;

    /* Touch diagnostics, updated by the LVGL indev read callback. */
    bool touch_present;             /* controller answered on I2C */
    char touch_product_id[5];       /* "911" when a GT911 is fitted */
    volatile int touch_x;
    volatile int touch_y;
    volatile bool touch_pressed;
    volatile uint32_t touch_events;
    volatile uint32_t touch_polls;        /* read attempts, proves the path runs */
    volatile uint32_t touch_read_errors;  /* I2C failures from the controller */

    /* I2C scan results. */
    uint8_t i2c_found[16];
    int i2c_found_count;

    /* Audio diagnostics. */
    volatile uint32_t audio_bytes_written;
    volatile uint32_t audio_tones_played;

    i2c_master_bus_handle_t i2c_bus;
    lv_display_t *disp;
} app_state_t;

extern app_state_t g_app;

/* board.c */
esp_err_t board_init(void);
void board_backlight_set(int percent);
int board_backlight_get(void);
void board_i2c_scan(void);

/* settings.c */
void settings_load(void);
void settings_flush(void);

/* ui.c */
void ui_init(lv_display_t *disp);
bool ui_lock(uint32_t timeout_ms);
void ui_unlock(void);
void ui_refresh(void);

/* GooPad product UI and macro service. */
void goopad_ui_init(lv_display_t *disp);
void goopad_init(void);
void goopad_wifi_join(const char *ssid, const char *password);
bool goopad_wifi_connected(void);
bool goopad_configured(void);
void goopad_get_config(char *out, size_t capacity);
void goopad_trigger(unsigned index);
const char *goopad_web_password(void);
void goopad_get_address(char *out, size_t capacity);

/* hw_*.c */
esp_err_t hw_display_init(lv_display_t **out_disp);
esp_err_t hw_touch_init(lv_display_t *disp);
esp_err_t hw_power_init(void);
void hw_power_poll(void);
esp_err_t hw_sdcard_init(void);
esp_err_t hw_sdcard_remount(void);
esp_err_t hw_audio_init(void);
void hw_audio_play_tone(int freq_hz, int duration_ms);
void hw_audio_play_square(int freq_hz, int duration_ms);
void hw_audio_play_sweep(void);
void hw_audio_set_volume(int percent);
int hw_audio_get_volume(void);
esp_err_t hw_audio_set_bits(int bits);
int hw_audio_get_bits(void);
int hw_audio_get_pending_bits(void);
void hw_audio_set_channel(int ch);
int hw_audio_get_channel(void);
void hw_audio_suspend(void);
void hw_audio_restore_pins(void);
void hw_audio_selftest(void);
void hw_info_init(void);
void console_init(void);

/* hw_wifi.c */
esp_err_t hw_wifi_init(void);
esp_err_t hw_wifi_scan(void);
void hw_wifi_stop(void);

/* hw_ble.c */
esp_err_t hw_ble_init(void);
esp_err_t hw_ble_scan(int duration_ms);
void hw_ble_stop(void);
