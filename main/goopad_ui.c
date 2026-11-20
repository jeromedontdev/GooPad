#include "app.h"
#include "board.h"

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_wifi.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>

#define ICON_BYTES (32 * 32 * 2)
#define MAX_MACROS 32

static const char *TAG = "goopad_ui";
static lv_obj_t *s_screen;
static lv_obj_t *s_status;
static lv_obj_t *s_keyboard;
static lv_obj_t *s_ssid;
static lv_obj_t *s_password;
static bool s_password_visible;
static lv_obj_t *s_network_list;
static lv_obj_t *s_scan_button;
static lv_obj_t *s_pages;
static lv_obj_t *s_dots;
static lv_obj_t *s_page_dots[32];
static lv_obj_t *s_info_overlay;
static lv_obj_t *s_theme_dropdown;
static lv_obj_t *s_settings_status;
static size_t s_page_dot_count;
static lv_timer_t *s_timer;
static char *s_config_buffer;
static char *s_rendered_config;
static uint8_t *s_icon_buffers;
static bool s_grid_shown;
static bool s_loading_shown;
static volatile bool s_scan_in_progress;
static bool s_ota_running;
static bool s_ota_armed;

static lv_color_t color(uint32_t value)
{
    return lv_color_hex(value);
}

static const char *symbol_for_icon(const char *icon)
{
    if (!strcmp(icon, "+") || !strcmp(icon, "➕")) return LV_SYMBOL_PLUS;
    if (!strcmp(icon, "🏠")) return LV_SYMBOL_HOME;
    if (!strcmp(icon, "💡") || !strcmp(icon, "🔆")) return LV_SYMBOL_CHARGE;
    if (!strcmp(icon, "🎵") || !strcmp(icon, "🎙️") || !strcmp(icon, "🔊")) return LV_SYMBOL_AUDIO;
    if (!strcmp(icon, "▶️") || !strcmp(icon, "▶")) return LV_SYMBOL_PLAY;
    if (!strcmp(icon, "⏸️")) return LV_SYMBOL_PAUSE;
    if (!strcmp(icon, "⏹️")) return LV_SYMBOL_STOP;
    if (!strcmp(icon, "🌐") || !strcmp(icon, "📶") || !strcmp(icon, "📡")) return LV_SYMBOL_WIFI;
    if (!strcmp(icon, "🔄")) return LV_SYMBOL_REFRESH;
    if (!strcmp(icon, "✅")) return LV_SYMBOL_OK;
    if (!strcmp(icon, "⚠️") || !strcmp(icon, "🚨")) return LV_SYMBOL_WARNING;
    if (!strcmp(icon, "✖️")) return LV_SYMBOL_CLOSE;
    if (!strcmp(icon, "📁") || !strcmp(icon, "📂")) return LV_SYMBOL_DIRECTORY;
    if (!strcmp(icon, "🖼️") || !strcmp(icon, "📷")) return LV_SYMBOL_IMAGE;
    if (!strcmp(icon, "🎥") || !strcmp(icon, "📺")) return LV_SYMBOL_VIDEO;
    if (!strcmp(icon, "🔒") || !strcmp(icon, "🛡️")) return LV_SYMBOL_EYE_OPEN;
    if (!strcmp(icon, "⬆️")) return LV_SYMBOL_UPLOAD;
    if (!strcmp(icon, "⬇️")) return LV_SYMBOL_DOWNLOAD;
    if (!strcmp(icon, "📋")) return LV_SYMBOL_COPY;
    if (!strcmp(icon, "💾")) return LV_SYMBOL_SAVE;
    if (!strcmp(icon, "🔌")) return LV_SYMBOL_USB;
    if (!strcmp(icon, "⚙️")) return LV_SYMBOL_SETTINGS;
    for (const unsigned char *p = (const unsigned char *)icon; *p; p++) {
        if (*p >= 0x80) return NULL;
    }
    return strlen(icon) <= 3 ? icon : NULL;
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, int size, lv_color_t fg)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    lv_obj_set_style_text_font(obj, size >= 20 ? &lv_font_montserrat_20 :
                                      size >= 16 ? &lv_font_montserrat_16 :
                                                   &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(obj, fg, 0);
    return obj;
}

static lv_obj_t *button(lv_obj_t *parent, const char *text, int x, int y, int w, int h,
                        lv_event_cb_t callback, void *data, uint32_t bg)
{
    lv_obj_t *obj = lv_button_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_color(obj, color(bg), 0);
    lv_obj_set_style_radius(obj, 12, 0);
    lv_obj_add_event_cb(obj, callback, LV_EVENT_CLICKED, data);
    lv_obj_t *caption = label(obj, text, 16, lv_color_white());
    lv_obj_center(caption);
    return obj;
}

static void theme_screen(void)
{
    lv_obj_set_style_bg_color(s_screen, color(0x111827), 0);
    lv_obj_set_style_text_color(s_screen, lv_color_white(), 0);
    lv_obj_set_style_pad_all(s_screen, 0, 0);
    lv_obj_set_scrollable(s_screen, false);
}

static void connect_cb(lv_event_t *event)
{
    (void)event;
    if (s_scan_in_progress) {
        lv_label_set_text(s_status, "Please wait for the Wi-Fi scan to finish");
        return;
    }
    const char *ssid = lv_textarea_get_text(s_ssid);
    const char *password = lv_textarea_get_text(s_password);
    if (!ssid || !ssid[0]) {
        lv_label_set_text(s_status, "Enter a network name first");
        return;
    }
    lv_label_set_text(s_status, "Connecting... please wait");
    goopad_wifi_join(ssid, password ? password : "");
}

static void hide_keyboard(void)
{
    lv_obj_set_hidden(s_keyboard, true);
    lv_obj_set_hidden(s_network_list, false);
    lv_label_set_text(s_status, g_app.wifi.ap_count
                                    ? "Select a Wi-Fi network or tap a field to type"
                                    : "No networks found. Tap Scan or enter a network name.");
}

static void keyboard_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_CANCEL || code == LV_EVENT_READY) {
        hide_keyboard();
    }
}

static void show_keyboard(lv_obj_t *textarea)
{
    ESP_LOGI(TAG, "opening Wi-Fi keyboard for %s",
             textarea == s_password ? "password" : "network name");
    lv_keyboard_set_textarea(s_keyboard, textarea);
    lv_obj_set_hidden(s_network_list, true);
    lv_obj_set_hidden(s_keyboard, false);
    lv_obj_invalidate(lv_layer_top());
    lv_label_set_text(s_status, "Type here, then tap the keyboard checkmark to finish");
}

static void select_network_cb(lv_event_t *event)
{
    uintptr_t index = (uintptr_t)lv_event_get_user_data(event);
    if (index >= (uintptr_t)g_app.wifi.ap_count) return;
    const char *ssid = g_app.wifi.aps[index].ssid;
    lv_textarea_set_text(s_ssid, ssid);
    lv_textarea_set_text(s_password, "");
    show_keyboard(s_password);
    lv_obj_add_state(s_password, LV_STATE_FOCUSED);
}

static void display_scan_results(void)
{
    if (!s_network_list || !lv_obj_is_valid(s_network_list)) return;
    lv_obj_clean(s_network_list);
    lv_obj_set_flex_flow(s_network_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(s_network_list, 3, 0);
    for (int i = 0; i < g_app.wifi.ap_count; i++) {
        lv_obj_t *item = lv_button_create(s_network_list);
        lv_obj_set_width(item, lv_pct(100));
        lv_obj_set_height(item, 42);
        lv_obj_set_style_bg_color(item, color(0x273449), 0);
        lv_obj_add_event_cb(item, select_network_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)i);
        char network_text[64];
        snprintf(network_text, sizeof(network_text), "%s    %d dBm%s",
                 g_app.wifi.aps[i].ssid, g_app.wifi.aps[i].rssi,
                 g_app.wifi.aps[i].auth == WIFI_AUTH_OPEN ? "" : "  locked");
        lv_obj_t *name = label(item, network_text, 14, lv_color_white());
        lv_obj_set_width(name, lv_pct(100));
        lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
        lv_obj_center(name);
    }
    if (lv_obj_is_hidden(s_keyboard)) {
        lv_label_set_text(s_status, g_app.wifi.ap_count
                                        ? "Select a Wi-Fi network or tap a field to type"
                                        : "No networks found. Tap Scan or enter a network name.");
    }
}

static void scan_task(void *arg)
{
    (void)arg;
    esp_err_t err = hw_wifi_scan();
    if (ui_lock(0)) {
        if (s_scan_button && lv_obj_is_valid(s_scan_button)) {
            lv_obj_set_disabled(s_scan_button, false);
        }
        if (!s_grid_shown) {
            if (err == ESP_OK) {
                display_scan_results();
            } else if (s_status && lv_obj_is_valid(s_status)) {
                char message[80];
                snprintf(message, sizeof(message), "Scan failed: %s",
                         esp_err_to_name(err));
                lv_label_set_text(s_status, message);
            }
        }
        s_scan_in_progress = false;
        ui_unlock();
    } else {
        ESP_LOGE(TAG, "could not reacquire UI lock after Wi-Fi scan");
        s_scan_in_progress = false;
    }
    vTaskDelete(NULL);
}

static void scan_cb(lv_event_t *event)
{
    (void)event;
    if (s_scan_in_progress) return;
    s_scan_in_progress = true;
    lv_label_set_text(s_status, "Scanning nearby Wi-Fi...");
    lv_obj_clean(s_network_list);
    lv_obj_set_disabled(s_scan_button, true);
    if (xTaskCreate(scan_task, "wifi_scan", 8192, NULL, 4, NULL) != pdPASS) {
        s_scan_in_progress = false;
        lv_obj_set_disabled(s_scan_button, false);
        lv_label_set_text(s_status, "Could not start scan - tap a field to enter network details");
    }
}

static void textarea_pressed_cb(lv_event_t *event)
{
    lv_obj_t *textarea = lv_event_get_current_target(event);
    ESP_LOGI(TAG, "Wi-Fi text box pressed at (%d,%d)",
             (int)lv_obj_get_x(textarea), (int)lv_obj_get_y(textarea));
    show_keyboard(textarea);
}

static void build_info_panel(bool settings);

static void close_info_cb(lv_event_t *event)
{
    (void)event;
    if (s_info_overlay && lv_obj_is_valid(s_info_overlay)) {
        lv_obj_del(s_info_overlay);
    }
    s_info_overlay = NULL;
    s_theme_dropdown = NULL;
    s_settings_status = NULL;
    s_ota_armed = false;
}

static void show_settings_cb(lv_event_t *event)
{
    (void)event;
    s_ota_armed = false;
    build_info_panel(true);
}

static void back_to_info_cb(lv_event_t *event)
{
    (void)event;
    s_ota_armed = false;
    build_info_panel(false);
}

static void save_device_theme_cb(lv_event_t *event)
{
    (void)event;
    static const char *themes[] = {"midnight", "ocean", "forest", "light"};
    if (!s_theme_dropdown || !lv_obj_is_valid(s_theme_dropdown)) return;
    uint16_t selected = lv_dropdown_get_selected(s_theme_dropdown);
    if (selected >= sizeof(themes) / sizeof(themes[0])) return;
    esp_err_t err = goopad_set_theme(themes[selected]);
    if (err != ESP_OK && s_settings_status && lv_obj_is_valid(s_settings_status)) {
        char message[64];
        snprintf(message, sizeof(message), "Could not save theme: %s",
                 esp_err_to_name(err));
        lv_label_set_text(s_settings_status, message);
    } else if (s_settings_status && lv_obj_is_valid(s_settings_status)) {
        lv_label_set_text(s_settings_status, "Theme saved.");
    }
}

static void install_sd_task(void *arg)
{
    (void)arg;
    esp_err_t err = goopad_install_ota_from_sd();
    if (err == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(1200));
        esp_restart();
    }
    ESP_LOGE(TAG, "SD firmware update failed: %s", esp_err_to_name(err));

    if (ui_lock(1000)) {
        if (s_settings_status && lv_obj_is_valid(s_settings_status)) {
            char message[72];
            snprintf(message, sizeof(message), "Update failed: %s",
                     esp_err_to_name(err));
            lv_label_set_text(s_settings_status, message);
        }
        ui_unlock();
    }
    s_ota_running = false;
    s_ota_armed = false;
    vTaskDelete(NULL);
}

static void install_sd_ota_cb(lv_event_t *event)
{
    lv_obj_t *button_obj = lv_event_get_target(event);
    if (s_ota_running) return;
    if (!s_ota_armed) {
        s_ota_armed = true;
        lv_label_set_text(lv_obj_get_child(button_obj, 0), "Confirm install & reboot");
        if (s_settings_status && lv_obj_is_valid(s_settings_status)) {
            lv_label_set_text(s_settings_status,
                              "Tap again to install /goopad.bin from SD.");
        }
        return;
    }

    s_ota_armed = false;
    s_ota_running = true;
    lv_label_set_text(lv_obj_get_child(button_obj, 0), "Installing...");
    if (s_settings_status && lv_obj_is_valid(s_settings_status)) {
        lv_label_set_text(s_settings_status, "Installing firmware. Do not power off.");
    }
    if (xTaskCreate(install_sd_task, "sd_ota", 6144, NULL, 5, NULL) != pdPASS) {
        s_ota_running = false;
        lv_label_set_text(lv_obj_get_child(button_obj, 0), "Install from SD");
        if (s_settings_status && lv_obj_is_valid(s_settings_status)) {
            lv_label_set_text(s_settings_status, "Could not start firmware update.");
        }
    }
}

static void build_info_panel(bool settings)
{
    if (!s_info_overlay || !lv_obj_is_valid(s_info_overlay)) return;
    lv_obj_clean(s_info_overlay);

    lv_obj_t *panel = lv_obj_create(s_info_overlay);
    lv_obj_set_size(panel, 420, 300);
    lv_obj_center(panel);
    lv_obj_set_style_bg_color(panel, color(0x1E293B), 0);
    lv_obj_set_style_border_color(panel, color(0x475569), 0);
    lv_obj_set_style_radius(panel, 18, 0);
    lv_obj_set_style_pad_all(panel, 18, 0);
    lv_obj_set_scrollable(panel, false);

    lv_obj_t *title = label(panel, settings ? "Device settings" : "Device information",
                            20, color(0xA5B4FC));
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);
    if (!settings) {
        char details[160];
        char address[16];
        goopad_get_address(address, sizeof(address));
        snprintf(details, sizeof(details), "Web editor: http://%s\nAdmin password: %s",
                 address, goopad_web_password());
        lv_obj_t *info = label(panel, details, 14, lv_color_white());
        lv_obj_set_width(info, 380);
        lv_label_set_long_mode(info, LV_LABEL_LONG_WRAP);
        lv_obj_align(info, LV_ALIGN_TOP_LEFT, 0, 40);
        lv_obj_t *hint = label(panel, "Use Settings for device themes or firmware updates.",
                               14, color(0xCBD5E1));
        lv_obj_set_width(hint, 380);
        lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
        lv_obj_align(hint, LV_ALIGN_TOP_LEFT, 0, 120);
        button(panel, "Settings", 150, 220, 112, 42, show_settings_cb, NULL, 0x4F46E5);
        button(panel, "Close", 278, 220, 112, 42, close_info_cb, NULL, 0x475569);
        return;
    }

    lv_obj_t *theme_label = label(panel, "Device color theme", 15, color(0xCBD5E1));
    lv_obj_align(theme_label, LV_ALIGN_TOP_LEFT, 0, 42);
    s_theme_dropdown = lv_dropdown_create(panel);
    lv_dropdown_set_options(s_theme_dropdown, "Midnight\nOcean\nForest\nLight");
    lv_obj_set_pos(s_theme_dropdown, 0, 68);
    lv_obj_set_size(s_theme_dropdown, 210, 42);
    char config[12001];
    goopad_get_config(config, sizeof(config));
    cJSON *root = cJSON_Parse(config);
    cJSON *theme = root ? cJSON_GetObjectItemCaseSensitive(root, "theme") : NULL;
    if (cJSON_IsString(theme)) {
        if (!strcmp(theme->valuestring, "ocean")) lv_dropdown_set_selected(s_theme_dropdown, 1);
        else if (!strcmp(theme->valuestring, "forest")) lv_dropdown_set_selected(s_theme_dropdown, 2);
        else if (!strcmp(theme->valuestring, "light")) lv_dropdown_set_selected(s_theme_dropdown, 3);
    }
    cJSON_Delete(root);
    button(panel, "Save theme", 230, 68, 160, 42,
           save_device_theme_cb, NULL, 0x4F46E5);

    lv_obj_t *firmware_label = label(panel, "Firmware update", 15, color(0xCBD5E1));
    lv_obj_align(firmware_label, LV_ALIGN_TOP_LEFT, 0, 122);
    button(panel, "Install from SD", 0, 148, 210, 42,
           install_sd_ota_cb, NULL, 0x4F46E5);
    lv_obj_t *firmware_hint = label(panel, "Place the OTA .bin at /goopad.bin on the SD card.",
                                    12, color(0x94A3B8));
    lv_obj_set_width(firmware_hint, 380);
    lv_label_set_long_mode(firmware_hint, LV_LABEL_LONG_WRAP);
    lv_obj_align(firmware_hint, LV_ALIGN_TOP_LEFT, 0, 195);
    s_settings_status = label(panel, "", 12, color(0xCBD5E1));
    lv_obj_set_width(s_settings_status, 380);
    lv_label_set_long_mode(s_settings_status, LV_LABEL_LONG_DOT);
    lv_obj_align(s_settings_status, LV_ALIGN_TOP_LEFT, 0, 215);
    button(panel, "Back", 150, 250, 112, 38, back_to_info_cb, NULL, 0x475569);
    button(panel, "Close", 278, 250, 112, 38, close_info_cb, NULL, 0x475569);
}

static void show_info_cb(lv_event_t *event)
{
    (void)event;
    if (s_info_overlay && lv_obj_is_valid(s_info_overlay)) return;

    s_info_overlay = lv_obj_create(s_screen);
    lv_obj_set_size(s_info_overlay, 480, 480);
    lv_obj_set_style_bg_color(s_info_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_info_overlay, LV_OPA_70, 0);
    lv_obj_set_style_border_width(s_info_overlay, 0, 0);
    lv_obj_set_style_pad_all(s_info_overlay, 0, 0);
    lv_obj_set_scrollable(s_info_overlay, false);
    build_info_panel(false);
}

static void password_visibility_cb(lv_event_t *event)
{
    s_password_visible = !s_password_visible;
    lv_textarea_set_password_mode(s_password, !s_password_visible);
    lv_obj_t *caption = lv_obj_get_child(lv_event_get_target(event), 0);
    if (caption) {
        lv_label_set_text(caption, s_password_visible ? LV_SYMBOL_EYE_CLOSE
                                                       : LV_SYMBOL_EYE_OPEN);
    }
}

static void build_setup_screen(void)
{
    s_grid_shown = false;
    s_loading_shown = false;
    s_password_visible = false;
    lv_obj_clean(s_screen);
    theme_screen();
    lv_obj_t *title = label(s_screen, "GooPad", 28, color(0xA5B4FC));
    lv_obj_set_pos(title, 18, 8);
    s_status = label(s_screen, "Connect GooPad to Wi-Fi", 14, color(0xCBD5E1));
    lv_obj_set_pos(s_status, 20, 45);

    s_ssid = lv_textarea_create(s_screen);
    lv_obj_set_pos(s_ssid, 14, 76);
    lv_obj_set_size(s_ssid, 452, 38);
    lv_obj_set_clickable(s_ssid, true);
    lv_textarea_set_one_line(s_ssid, true);
    lv_textarea_set_max_length(s_ssid, 32);
    lv_textarea_set_placeholder_text(s_ssid, "Wi-Fi network name");
    lv_obj_add_event_cb(s_ssid, textarea_pressed_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_ssid, textarea_pressed_cb, LV_EVENT_CLICKED, NULL);

    s_password = lv_textarea_create(s_screen);
    lv_obj_set_pos(s_password, 14, 119);
    lv_obj_set_size(s_password, 452, 38);
    lv_obj_set_clickable(s_password, true);
    lv_obj_set_style_pad_right(s_password, 42, 0);
    lv_textarea_set_one_line(s_password, true);
    lv_textarea_set_max_length(s_password, 64);
    lv_textarea_set_password_mode(s_password, true);
    lv_textarea_set_placeholder_text(s_password, "Wi-Fi password");
    lv_obj_add_event_cb(s_password, textarea_pressed_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_password, textarea_pressed_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *password_eye = lv_button_create(s_screen);
    lv_obj_set_pos(password_eye, 424, 119);
    lv_obj_set_size(password_eye, 38, 38);
    lv_obj_set_style_bg_opa(password_eye, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(password_eye, 0, 0);
    lv_obj_set_style_shadow_width(password_eye, 0, 0);
    lv_obj_add_event_cb(password_eye, password_visibility_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *eye_icon = label(password_eye, LV_SYMBOL_EYE_OPEN, 18, color(0xCBD5E1));
    lv_obj_center(eye_icon);
    s_scan_button = button(s_screen, "Scan networks", 15, 162, 162, 38,
                           scan_cb, NULL, 0x334155);
    button(s_screen, "Join Wi-Fi", 184, 162, 281, 38,
           connect_cb, NULL, 0x4F46E5);

    s_network_list = lv_obj_create(s_screen);
    lv_obj_set_pos(s_network_list, 14, 230);
    lv_obj_set_size(s_network_list, 452, 250);
    lv_obj_set_style_bg_opa(s_network_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_network_list, 0, 0);
    lv_obj_set_style_pad_all(s_network_list, 0, 0);
    lv_obj_set_style_pad_row(s_network_list, 3, 0);
    lv_obj_set_flex_flow(s_network_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(s_network_list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_network_list, LV_SCROLLBAR_MODE_AUTO);
    s_status = label(s_screen, "Tap a Wi-Fi field to open the keyboard", 14,
                     color(0x94A3B8));
    lv_obj_set_pos(s_status, 16, 205);
    lv_obj_set_width(s_status, 448);
    lv_label_set_long_mode(s_status, LV_LABEL_LONG_DOT);

    if (!s_keyboard || !lv_obj_is_valid(s_keyboard)) {
        s_keyboard = lv_keyboard_create(lv_layer_top());
        lv_obj_add_event_cb(s_keyboard, keyboard_event_cb, LV_EVENT_CANCEL, NULL);
        lv_obj_add_event_cb(s_keyboard, keyboard_event_cb, LV_EVENT_READY, NULL);
    }
    lv_obj_set_size(s_keyboard, 480, 228);
    lv_obj_align(s_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_mode(s_keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(s_keyboard, s_ssid);
    lv_obj_set_hidden(s_keyboard, true);
    lv_obj_set_hidden(s_network_list, false);
    s_scan_in_progress = false;
}

static void build_loading_screen(void)
{
    s_grid_shown = false;
    s_loading_shown = true;
    if (s_keyboard && lv_obj_is_valid(s_keyboard)) {
        lv_obj_set_hidden(s_keyboard, true);
    }
    lv_obj_clean(s_screen);
    theme_screen();

    lv_obj_t *title = label(s_screen, "GooPad", 28, color(0xA5B4FC));
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -72);
    lv_obj_t *spinner = lv_spinner_create(s_screen);
    lv_obj_set_size(spinner, 56, 56);
    lv_spinner_set_anim_params(spinner, 1000, 90);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t *status = label(s_screen, "Connecting to Wi-Fi...", 18,
                             color(0xCBD5E1));
    lv_obj_align(status, LV_ALIGN_CENTER, 0, 58);
    lv_obj_t *hint = label(s_screen, "Please wait", 14, color(0x94A3B8));
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, 86);
}

static void tile_cb(lv_event_t *event)
{
    unsigned index = (unsigned)(uintptr_t)lv_event_get_user_data(event);
    goopad_trigger(index);
}

static void page_scroll_cb(lv_event_t *event)
{
    (void)event;
    if (!s_pages || !s_dots) return;
    int page = (lv_obj_get_scroll_x(s_pages) + 240) / 480;
    int count = lv_obj_get_child_count(s_pages);
    if (page < 0) page = 0;
    if (page >= count) page = count - 1;
    for (size_t i = 0; i < s_page_dot_count; i++) {
        bool active = (int)i == page;
        lv_obj_set_size(s_page_dots[i], active ? 8 : 6, active ? 8 : 6);
        lv_obj_set_style_bg_color(s_page_dots[i], color(active ? 0xA5B4FC : 0x64748B), 0);
        lv_obj_set_style_bg_opa(s_page_dots[i], active ? LV_OPA_COVER : LV_OPA_70, 0);
    }
}

static void build_grid_screen(void)
{
    if (!s_config_buffer || !s_rendered_config) return;
    s_loading_shown = false;
    if (s_keyboard && lv_obj_is_valid(s_keyboard)) {
        lv_obj_set_hidden(s_keyboard, true);
    }
    s_info_overlay = NULL;
    s_theme_dropdown = NULL;
    s_settings_status = NULL;
    s_ota_armed = false;
    s_dots = NULL;
    s_page_dot_count = 0;
    memset(s_page_dots, 0, sizeof(s_page_dots));
    goopad_get_config(s_config_buffer, 12001);
    strlcpy(s_rendered_config, s_config_buffer, 12001);
    s_grid_shown = true;
    lv_obj_clean(s_screen);
    theme_screen();

    cJSON *root = cJSON_Parse(s_config_buffer);
    cJSON *macros = root ? cJSON_GetObjectItemCaseSensitive(root, "macros") : NULL;
    cJSON *cols_item = root ? cJSON_GetObjectItemCaseSensitive(root, "cols") : NULL;
    cJSON *rows_item = root ? cJSON_GetObjectItemCaseSensitive(root, "rows") : NULL;
    int cols = cJSON_IsNumber(cols_item) ? cols_item->valueint : 3;
    int rows = cJSON_IsNumber(rows_item) ? rows_item->valueint : 3;
    if (cols < 2 || cols > 4) cols = 3;
    if (rows < 2 || rows > 4) rows = 3;
    int total = cJSON_IsArray(macros) ? cJSON_GetArraySize(macros) : 0;
    int per_page = cols * rows;
    int page_count = (total + per_page - 1) / per_page;
    if (page_count < 2) page_count = 2;
    cJSON *bg = root ? cJSON_GetObjectItemCaseSensitive(root, "background") : NULL;
    unsigned background = cJSON_IsString(bg) ? (unsigned)strtoul(bg->valuestring, NULL, 16)
                                               : 0x111827;
    cJSON *shape = root ? cJSON_GetObjectItemCaseSensitive(root, "shape") : NULL;
    cJSON *device_theme = root ? cJSON_GetObjectItemCaseSensitive(root, "theme") : NULL;
    bool light_theme = cJSON_IsString(device_theme) &&
                       strcmp(device_theme->valuestring, "light") == 0;
    uint32_t accent_color = 0xA5B4FC;
    if (cJSON_IsString(device_theme)) {
        if (!strcmp(device_theme->valuestring, "ocean")) accent_color = 0x67E8F9;
        if (!strcmp(device_theme->valuestring, "forest")) accent_color = 0x6EE7B7;
        if (light_theme) accent_color = 0x334155;
    }
    lv_obj_set_style_bg_color(s_screen, color(background), 0);
    lv_obj_set_style_text_color(s_screen, light_theme ? color(0x172033) : lv_color_white(), 0);

    lv_obj_t *info_button = lv_button_create(s_screen);
    lv_obj_set_pos(info_button, 432, 6);
    lv_obj_set_size(info_button, 40, 36);
    lv_obj_set_style_bg_opa(info_button, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(info_button, 1, 0);
    lv_obj_set_style_border_color(info_button, color(accent_color), 0);
    lv_obj_set_style_shadow_width(info_button, 0, 0);
    lv_obj_set_style_radius(info_button, LV_RADIUS_CIRCLE, 0);
    lv_obj_add_event_cb(info_button, show_info_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *info_icon = label(info_button, "i", 20, color(accent_color));
    lv_obj_center(info_icon);

    s_pages = lv_obj_create(s_screen);
    lv_obj_set_pos(s_pages, 0, 48);
    lv_obj_set_size(s_pages, 480, 393);
    lv_obj_set_style_bg_opa(s_pages, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_pages, 0, 0);
    lv_obj_set_style_pad_all(s_pages, 0, 0);
    lv_obj_set_style_pad_column(s_pages, 0, 0);
    lv_obj_set_flex_flow(s_pages, LV_FLEX_FLOW_ROW);
    lv_obj_set_scroll_dir(s_pages, LV_DIR_HOR);
    lv_obj_set_scroll_snap_x(s_pages, LV_SCROLL_SNAP_START);
    lv_obj_set_scrollbar_mode(s_pages, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(s_pages, page_scroll_cb, LV_EVENT_SCROLL_END, NULL);

    for (int page = 0; page < page_count; page++) {
        lv_obj_t *surface = lv_obj_create(s_pages);
        lv_obj_set_size(surface, 480, 393);
        lv_obj_set_style_bg_opa(surface, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(surface, 0, 0);
        lv_obj_set_style_pad_all(surface, 10, 0);
        lv_obj_set_style_pad_row(surface, 8, 0);
        lv_obj_set_style_pad_column(surface, 8, 0);
        lv_obj_set_flex_flow(surface, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_scrollable(surface, false);
        for (int slot = 0; slot < per_page; slot++) {
            int index = page * per_page + slot;
            cJSON *macro = index < total ? cJSON_GetArrayItem(macros, index) : NULL;
            lv_obj_t *tile = lv_button_create(surface);
            int width = (460 - (cols - 1) * 8) / cols;
            int height = (373 - (rows - 1) * 8) / rows;
            lv_obj_set_size(tile, width, height);
            lv_obj_set_style_bg_color(tile, color(0x273449), 0);
            int radius = cJSON_IsString(shape) && !strcmp(shape->valuestring, "square")
                             ? 4 : cJSON_IsString(shape) &&
                                   !strcmp(shape->valuestring, "circle")
                                       ? LV_RADIUS_CIRCLE : 18;
            lv_obj_set_style_radius(tile, radius, 0);
            if (macro) {
                cJSON *name = cJSON_GetObjectItemCaseSensitive(macro, "name");
                cJSON *icon = cJSON_GetObjectItemCaseSensitive(macro, "icon");
                cJSON *accent = cJSON_GetObjectItemCaseSensitive(macro, "color");
                const char *caption = cJSON_IsString(name) ? name->valuestring : "Macro";
                const char *glyph = cJSON_IsString(icon) ? icon->valuestring : "•";
                unsigned accent_value = cJSON_IsString(accent)
                    ? (unsigned)strtoul(accent->valuestring, NULL, 16) : 0x4F46E5;
                lv_obj_set_style_bg_color(tile, color(accent_value), 0);
                lv_obj_add_event_cb(tile, tile_cb, LV_EVENT_CLICKED,
                                    (void *)(uintptr_t)index);
                cJSON *custom_image = cJSON_GetObjectItemCaseSensitive(macro, "iconImage");
                bool image_drawn = false;
                if (cJSON_IsString(custom_image) && g_app.sd.mounted && s_icon_buffers) {
                    char path[64];
                    snprintf(path, sizeof(path), BSP_SD_MOUNT_POINT "/goopad/icon-%s.rgb565",
                             custom_image->valuestring);
                    FILE *image = fopen(path, "rb");
                    if (image) {
                        uint8_t *pixels = s_icon_buffers + (size_t)index * ICON_BYTES;
                        image_drawn = fread(pixels, 1, ICON_BYTES, image) == ICON_BYTES;
                        fclose(image);
                        if (image_drawn) {
                            lv_obj_t *canvas = lv_canvas_create(tile);
                            lv_canvas_set_buffer(canvas, pixels, 32, 32, LV_COLOR_FORMAT_RGB565);
                            lv_obj_align(canvas, LV_ALIGN_CENTER, 0, -13);
                        }
                    }
                }
                if (!image_drawn) {
                    const char *drawn_icon = symbol_for_icon(glyph);
                    char initial[2] = {caption[0], '\0'};
                    lv_obj_t *symbol = label(tile, drawn_icon ? drawn_icon : initial,
                                             24, lv_color_white());
                    lv_obj_align(symbol, LV_ALIGN_CENTER, 0, -13);
                }
                lv_obj_t *text = label(tile, caption, 14, lv_color_white());
                lv_obj_align(text, LV_ALIGN_CENTER, 0, 23);
            } else {
                lv_obj_set_style_bg_color(tile, color(0x1F2937), 0);
                bool swipe_test = page > 0 && slot == 0 && index >= total;
                lv_obj_t *empty = label(tile, swipe_test ? "Swipe test" : "+",
                                        swipe_test ? 16 : 24,
                                        color(swipe_test ? 0xA5B4FC : 0x64748B));
                lv_obj_center(empty);
                if (swipe_test) lv_obj_set_clickable(tile, false);
            }
        }
    }
    s_dots = lv_obj_create(s_screen);
    s_page_dot_count = (size_t)page_count;
    lv_obj_set_size(s_dots, page_count * 18, 14);
    lv_obj_align(s_dots, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_flex_flow(s_dots, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_dots, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(s_dots, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_dots, 0, 0);
    lv_obj_set_style_pad_all(s_dots, 0, 0);
    lv_obj_set_style_pad_column(s_dots, 7, 0);
    lv_obj_set_scrollable(s_dots, false);
    for (size_t i = 0; i < s_page_dot_count && i < 32; i++) {
        s_page_dots[i] = lv_obj_create(s_dots);
        lv_obj_set_size(s_page_dots[i], 6, 6);
        lv_obj_set_style_radius(s_page_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(s_page_dots[i], 0, 0);
        lv_obj_set_style_bg_color(s_page_dots[i], color(0x64748B), 0);
        lv_obj_set_style_bg_opa(s_page_dots[i], LV_OPA_70, 0);
        lv_obj_set_scrollable(s_page_dots[i], false);
    }
    page_scroll_cb(NULL);
    cJSON_Delete(root);
}

static void refresh_cb(lv_timer_t *timer)
{
    (void)timer;
    if (goopad_wifi_connected()) {
        goopad_get_config(s_config_buffer, 12001);
        if (!s_grid_shown || strcmp(s_config_buffer, s_rendered_config) != 0) {
            build_grid_screen();
        }
    } else if (goopad_wifi_connecting()) {
        if (!s_loading_shown) build_loading_screen();
    } else if (s_grid_shown || s_loading_shown) {
        build_setup_screen();
    }
}

void goopad_ui_init(lv_display_t *disp)
{
    if (!ui_lock(1000)) {
        ESP_LOGE(TAG, "LVGL lock timeout");
        return;
    }
    s_config_buffer = heap_caps_malloc(12001, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s_rendered_config = heap_caps_malloc(12001, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s_icon_buffers = heap_caps_calloc(MAX_MACROS, ICON_BYTES,
                                      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_config_buffer || !s_rendered_config || !s_icon_buffers) {
        ESP_LOGE(TAG, "could not allocate config buffers in PSRAM");
        ui_unlock();
        return;
    }
    s_screen = lv_display_get_screen_active(disp);
    if (goopad_wifi_connected()) {
        build_grid_screen();
    } else if (goopad_wifi_connecting()) {
        build_loading_screen();
    } else {
        build_setup_screen();
    }
    s_timer = lv_timer_create(refresh_cb, 500, NULL);
    (void)s_timer;
    ui_unlock();
}
