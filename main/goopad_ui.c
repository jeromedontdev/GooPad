#include "app.h"
#include "board.h"
#ifdef CONFIG_GOOPAD_HOMEKIT
#include "goopad_homekit.h"
#endif
#include "src/misc/cache/instance/lv_image_cache.h"

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>

#define ICON_BYTES (160 * 160 * 3)
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
static lv_point_t s_menu_press;
static bool s_menu_tracking;
static lv_obj_t *s_settings_status;
static lv_obj_t *s_brightness_slider;
static lv_obj_t *s_volume_slider;
static lv_obj_t *s_brightness_label;
static lv_obj_t *s_volume_label;
static size_t s_page_dot_count;
static lv_timer_t *s_timer;
static char *s_config_buffer;
static char *s_rendered_config;
static uint8_t *s_icon_buffers;
static lv_image_dsc_t s_icon_images[MAX_MACROS];
static uint8_t *s_background_pixels;
static lv_image_dsc_t s_background_image;
static uint8_t *s_gif_buffers[MAX_MACROS];
static lv_image_dsc_t s_gif_images[MAX_MACROS];
static lv_obj_t *s_gif_objects[MAX_MACROS];
static int s_gif_pages[MAX_MACROS];
static bool s_gif_paused[MAX_MACROS];
static lv_obj_t *s_widget_labels[MAX_MACROS];
#ifdef CONFIG_GOOPAD_HOMEKIT
static lv_obj_t *s_homekit_indicators[MAX_MACROS];
static char s_homekit_ids[MAX_MACROS][16];
static int s_homekit_states[MAX_MACROS];
#endif
static unsigned s_widget_types[MAX_MACROS];
static const char *s_widget_formats[MAX_MACROS];
static unsigned s_widget_stats[MAX_MACROS];
static bool s_widget_fahrenheit[MAX_MACROS];
static lv_obj_t *s_widget_graphics[MAX_MACROS], *s_widget_hands[MAX_MACROS][2], *s_widget_days[MAX_MACROS];
static lv_point_precise_t s_widget_points[MAX_MACROS][2][2];
static unsigned s_widget_graphic_modes[MAX_MACROS], s_widget_sizes[MAX_MACROS];
static int s_widget_widths[MAX_MACROS], s_widget_heights[MAX_MACROS], s_widget_minutes[MAX_MACROS], s_widget_sides[MAX_MACROS];
static int s_widget_meter_values[MAX_MACROS];
static int64_t s_countdown_targets[MAX_MACROS], s_stopwatch_elapsed[MAX_MACROS], s_stopwatch_started[MAX_MACROS];
static bool s_stopwatch_running[MAX_MACROS], s_stopwatch_long_press[MAX_MACROS];
static void fit_widget_text(unsigned index, const char *text);
static void update_widget_graphic(unsigned index, const struct tm *now, bool synced);
static int s_time_offset;
static bool s_grid_shown;
static bool s_loading_shown;
static volatile bool s_scan_in_progress;
static bool s_ota_running;
static bool s_ota_armed;

static lv_color_t color(uint32_t value);

static void clear_dynamic_assets(void)
{
    /* Called after LVGL objects are deleted; discard their pointers and free
     * separately owned GIF buffers before rebuilding the screen. */
    s_pages = NULL;
    s_info_overlay = NULL;
    g_app.rendered_tiles = g_app.empty_tiles = g_app.widget_count = g_app.gif_count = g_app.gif_loops = 0;
    g_app.rendered_pages = 0;
    g_app.symbol_fallbacks = 0;
    for (unsigned i = 0; i < MAX_MACROS; i++) {
        free(s_gif_buffers[i]);
        s_gif_buffers[i] = NULL;
        s_gif_objects[i] = NULL;
        s_widget_labels[i] = NULL;
#ifdef CONFIG_GOOPAD_HOMEKIT
        s_homekit_indicators[i] = NULL;
        s_homekit_ids[i][0] = '\0';
        s_homekit_states[i] = -2;
#endif
        s_widget_types[i] = 0;
        s_widget_formats[i] = NULL;
        s_widget_stats[i] = 0;
        s_widget_fahrenheit[i] = false;
        s_widget_graphics[i] = s_widget_days[i] = NULL;
        s_widget_hands[i][0] = s_widget_hands[i][1] = NULL;
        s_widget_graphic_modes[i] = 0;
        s_widget_minutes[i] = -1;
        s_widget_meter_values[i] = -2;
        s_countdown_targets[i] = s_stopwatch_elapsed[i] = s_stopwatch_started[i] = 0;
        s_stopwatch_running[i] = s_stopwatch_long_press[i] = false;
        s_gif_paused[i] = true;
    }
}

static void gif_loop_cb(lv_event_t *event)
{
    (void)event;
    g_app.gif_loops++;
}

static bool macro_configured(cJSON *macro)
{
    if (!macro) return false;
    cJSON *url = cJSON_GetObjectItemCaseSensitive(macro, "url");
    cJSON *image = cJSON_GetObjectItemCaseSensitive(macro, "iconImage");
    cJSON *sound = cJSON_GetObjectItemCaseSensitive(macro, "sound");
    cJSON *mode = cJSON_GetObjectItemCaseSensitive(macro, "actionMode");
    return cJSON_IsString(image) || (cJSON_IsString(url) && url->valuestring[0]) ||
           (cJSON_IsString(sound) && strcmp(sound->valuestring, "none")) ||
           (cJSON_IsString(mode) && (!strcmp(mode->valuestring, "widget") ||
                                    !strcmp(mode->valuestring, "page_navigation")
#ifdef CONFIG_GOOPAD_HOMEKIT
                                    || !strcmp(mode->valuestring, "homekit")
#endif
                                    ));
}

static void update_widgets(void)
{
    /* This runs on the LVGL thread. Workers expose snapshots rather than
     * touching LVGL objects directly. */
    time_t epoch = time(NULL);
    bool synced = epoch >= 1577836800;
    epoch += s_time_offset * 60;
    struct tm now;
    gmtime_r(&epoch, &now);
    int scroll_x = s_pages ? lv_obj_get_scroll_x(s_pages) : 0;
    for (unsigned i = 0; i < MAX_MACROS; i++) {
#ifdef CONFIG_GOOPAD_HOMEKIT
        if (s_homekit_indicators[i]) {
            bool on = false;
            esp_err_t err = goopad_homekit_state(s_homekit_ids[i], &on);
            int state = err == ESP_OK ? (on ? 1 : 0) : -1;
            if (err == ESP_ERR_TIMEOUT && s_homekit_states[i] != -2) state = s_homekit_states[i];
            if (state != s_homekit_states[i]) {
                lv_obj_set_style_border_color(s_homekit_indicators[i],
                    color(state == 1 ? 0x34D399 : state == 0 ? 0xFFFFFF : 0x94A3B8), 0);
                lv_obj_set_style_bg_color(s_homekit_indicators[i], color(0x34D399), 0);
                lv_obj_set_style_bg_opa(s_homekit_indicators[i], state == 1 ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
                s_homekit_states[i] = state;
            }
        }
#endif
        if (s_widget_labels[i]) {
            char text[64];
            if (s_widget_types[i] == 4) {
                if (!synced) strlcpy(text, "Sync time\nin the website", sizeof(text));
                else {
                    int64_t remaining = s_countdown_targets[i] - (int64_t)time(NULL);
                    if (remaining < 0) remaining = 0;
                    if (remaining >= 86400) snprintf(text, sizeof(text), "%lldd\n%02lld:%02lld",
                        (long long)(remaining / 86400), (long long)(remaining / 3600 % 24), (long long)(remaining / 60 % 60));
                    else snprintf(text, sizeof(text), "%02lld:%02lld:%02lld",
                        (long long)(remaining / 3600), (long long)(remaining / 60 % 60), (long long)(remaining % 60));
                }
            } else if (s_widget_types[i] == 5) {
                int64_t elapsed = s_stopwatch_elapsed[i] + (s_stopwatch_running[i] ? esp_timer_get_time() - s_stopwatch_started[i] : 0);
                int64_t seconds = elapsed / 1000000;
                snprintf(text, sizeof(text), "%02lld:%02lld:%02lld",
                    (long long)(seconds / 3600), (long long)(seconds / 60 % 60), (long long)(seconds % 60));
            } else if (s_widget_types[i] >= 6)
                goopad_live_widget_text(i, text, sizeof(text));
            else if (s_widget_types[i] == 3)
                goopad_pc_stat_text(s_widget_stats[i], s_widget_fahrenheit[i], text, sizeof(text));
            else if (!synced) strlcpy(text, "Sync time\nin the website", sizeof(text));
            else strftime(text, sizeof(text), s_widget_formats[i], &now);
            if (strcmp(lv_label_get_text(s_widget_labels[i]), text)) {
                fit_widget_text(i, text);
                lv_label_set_text(s_widget_labels[i], text);
            }
            update_widget_graphic(i, &now, synced);
        }
        if (s_gif_objects[i]) {
            int left = s_gif_pages[i] * 480;
            bool pause = left + 480 <= scroll_x || left >= scroll_x + 480 || s_info_overlay != NULL;
            if (pause != s_gif_paused[i]) {
                if (pause) lv_gif_pause(s_gif_objects[i]);
                else lv_gif_resume(s_gif_objects[i]);
                s_gif_paused[i] = pause;
            }
        }
    }
}

static lv_color_t color(uint32_t value)
{
    return lv_color_hex(value);
}

static const char *symbol_for_icon(const char *icon)
{
    const char *original = icon;
    char normalized[32];
    size_t used = 0;
    for (const unsigned char *p = (const unsigned char *)icon; *p;) {
        if (p[0] == 0xEF && p[1] == 0xB8 && (p[2] == 0x8E || p[2] == 0x8F)) {
            p += 3;
            continue;
        }
        if (used + 1 >= sizeof(normalized)) return NULL;
        normalized[used++] = (char)*p++;
    }
    normalized[used] = '\0';
    icon = normalized;
    if (!strcmp(icon, "+") || !strcmp(icon, "➕")) return LV_SYMBOL_PLUS;
    if (!strcmp(icon, "🏠")) return LV_SYMBOL_HOME;
    if (!strcmp(icon, "💡") || !strcmp(icon, "🔆")) return LV_SYMBOL_CHARGE;
    if (!strcmp(icon, "🎵") || !strcmp(icon, "🎙") || !strcmp(icon, "🔊")) return LV_SYMBOL_AUDIO;
    if (!strcmp(icon, "▶️") || !strcmp(icon, "▶")) return LV_SYMBOL_PLAY;
    if (!strcmp(icon, "⏸")) return LV_SYMBOL_PAUSE;
    if (!strcmp(icon, "⏹")) return LV_SYMBOL_STOP;
    if (!strcmp(icon, "🌐") || !strcmp(icon, "📶") || !strcmp(icon, "📡")) return LV_SYMBOL_WIFI;
    if (!strcmp(icon, "🔄")) return LV_SYMBOL_REFRESH;
    if (!strcmp(icon, "✅")) return LV_SYMBOL_OK;
    if (!strcmp(icon, "⚠") || !strcmp(icon, "🚨")) return LV_SYMBOL_WARNING;
    if (!strcmp(icon, "✖")) return LV_SYMBOL_CLOSE;
    if (!strcmp(icon, "📁") || !strcmp(icon, "📂")) return LV_SYMBOL_DIRECTORY;
    if (!strcmp(icon, "🖼") || !strcmp(icon, "📷")) return LV_SYMBOL_IMAGE;
    if (!strcmp(icon, "🎥") || !strcmp(icon, "📺")) return LV_SYMBOL_VIDEO;
    if (!strcmp(icon, "🔒") || !strcmp(icon, "🛡")) return LV_SYMBOL_EYE_OPEN;
    if (!strcmp(icon, "⬆")) return LV_SYMBOL_UPLOAD;
    if (!strcmp(icon, "⬇")) return LV_SYMBOL_DOWNLOAD;
    if (!strcmp(icon, "📋")) return LV_SYMBOL_COPY;
    if (!strcmp(icon, "💾")) return LV_SYMBOL_SAVE;
    if (!strcmp(icon, "🔌")) return LV_SYMBOL_USB;
    if (!strcmp(icon, "⚙")) return LV_SYMBOL_SETTINGS;
    for (const unsigned char *p = (const unsigned char *)icon; *p; p++) {
        if (*p >= 0x80) return NULL;
    }
    return strlen(icon) <= 3 ? original : NULL;
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, int size, lv_color_t fg)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    lv_obj_set_style_text_font(obj, size >= 28 ? &lv_font_montserrat_28 :
                                      size >= 20 ? &lv_font_montserrat_20 :
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

static void fit_widget_text(unsigned index, const char *text)
{
    static const lv_font_t *fonts[] = {&lv_font_montserrat_40, &lv_font_montserrat_28,
        &lv_font_montserrat_20, &lv_font_montserrat_16, &lv_font_montserrat_14,
        &lv_font_montserrat_12, &lv_font_montserrat_10, &lv_font_montserrat_8};
    static const unsigned sizes[] = {40, 28, 20, 16, 14, 12, 10, 8};
    const lv_font_t *chosen = &lv_font_montserrat_8;
    for (unsigned i = 0; i < sizeof(fonts) / sizeof(fonts[0]); i++) {
        if (sizes[i] > s_widget_sizes[index]) continue;
        lv_point_t bounds;
        lv_text_get_size(&bounds, text, fonts[i], 0, 0,
                        (s_widget_types[index] == 3 || s_widget_types[index] >= 6) ? s_widget_widths[index] : LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        if (bounds.x <= s_widget_widths[index] && bounds.y <= s_widget_heights[index]) {
            chosen = fonts[i];
            break;
        }
    }
    lv_obj_set_style_text_font(s_widget_labels[index], chosen, 0);
    lv_obj_set_style_text_line_space(s_widget_labels[index], 0, 0);
    lv_label_set_long_mode(s_widget_labels[index], (s_widget_types[index] == 3 || s_widget_types[index] >= 6) ? LV_LABEL_LONG_WRAP : LV_LABEL_LONG_CLIP);
    lv_obj_set_height(s_widget_labels[index], LV_SIZE_CONTENT);
    lv_obj_set_style_max_height(s_widget_labels[index], s_widget_heights[index], 0);
}

static lv_obj_t *graphic_box(lv_obj_t *parent, int w, int h, uint32_t accent)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_clickable(box, false);
    lv_obj_set_scrollable(box, false);
    lv_obj_set_size(box, w, h);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_set_style_shadow_width(box, 0, 0);
    lv_obj_set_style_radius(box, 4, 0);
    lv_obj_set_style_border_width(box, 2, 0);
    lv_obj_set_style_border_color(box, color(accent), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    return box;
}

static void set_clock_hands(unsigned index, int hour, int minute)
{
    int side = s_widget_sides[index], center = side / 2;
    int angles[] = {(hour % 12) * 30 + minute / 2, minute * 6};
    for (unsigned hand = 0; hand < 2; hand++) {
        int length = side * (hand ? 36 : 25) / 100;
        s_widget_points[index][hand][0] = (lv_point_precise_t){center, center};
        s_widget_points[index][hand][1] = (lv_point_precise_t){
            center + lv_trigo_sin(angles[hand]) * length / 32768,
            center - lv_trigo_cos(angles[hand]) * length / 32768};
        lv_line_set_points(s_widget_hands[index][hand], s_widget_points[index][hand], 2);
    }
}

static void create_widget_graphic(unsigned index, lv_obj_t *tile, const char *graphic,
                                  unsigned type, uint32_t accent, int width, int height, bool caption, int header)
{
    if (!strcmp(graphic, "text")) return;
    unsigned mode = !strcmp(graphic, "analog") ? 2 : !strcmp(graphic, "calendar") ? 3 :
                    !strcmp(graphic, "bar") ? 4 : !strcmp(graphic, "ring") ? 5 : 1;
    s_widget_graphic_modes[index] = mode;
    int content_height = height - (caption ? 28 : 12) - header;
    if (content_height < (type == 3 ? 50 : 38)) {
        s_widget_graphic_modes[index] = 0;
        return;
    }
    if (mode == 4) {
        lv_obj_t *bar = lv_bar_create(tile);
        s_widget_graphics[index] = bar;
        lv_obj_set_clickable(bar, false);
        lv_obj_set_size(bar, width - 24, 8);
        lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, caption ? -24 : -8);
        lv_obj_set_style_bg_color(bar, color(accent), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bar, LV_OPA_20, LV_PART_MAIN);
        lv_obj_set_style_bg_color(bar, color(accent), LV_PART_INDICATOR);
        lv_bar_set_range(bar, 0, 100);
        s_widget_heights[index] = content_height - 18;
        return;
    }
    int side = content_height * 35 / 100;
    if (side > width - 20) side = width - 20;
    if (side < 16) side = 16;
    s_widget_sides[index] = side;
    s_widget_heights[index] = content_height - side - 6;
    lv_obj_align(s_widget_labels[index], LV_ALIGN_CENTER, 0, side / 2 + 2 - (caption ? 10 : 0) + header / 2);
    if (mode == 5) {
        lv_obj_t *arc = lv_arc_create(tile);
        s_widget_graphics[index] = arc;
        lv_obj_set_size(arc, side, side);
        lv_obj_align(arc, LV_ALIGN_TOP_MID, 0, 8 + header);
        lv_obj_set_clickable(arc, false);
        lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
        lv_obj_set_style_arc_width(arc, side < 40 ? 3 : 6, LV_PART_MAIN);
        lv_obj_set_style_arc_width(arc, side < 40 ? 3 : 6, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(arc, color(accent), LV_PART_MAIN);
        lv_obj_set_style_arc_opa(arc, LV_OPA_20, LV_PART_MAIN);
        lv_obj_set_style_arc_color(arc, color(accent), LV_PART_INDICATOR);
        lv_arc_set_bg_angles(arc, 135, 45);
        lv_arc_set_range(arc, 0, 100);
        return;
    }
    lv_obj_t *box = graphic_box(tile, side, side, accent);
    s_widget_graphics[index] = box;
    lv_obj_align(box, LV_ALIGN_TOP_MID, 0, 8 + header);
    if (type == 1) {
        lv_obj_set_style_radius(box, LV_RADIUS_CIRCLE, 0);
        for (unsigned hand = 0; hand < 2; hand++) {
            s_widget_hands[index][hand] = lv_line_create(box);
            lv_obj_set_style_line_color(s_widget_hands[index][hand], color(accent), 0);
            lv_obj_set_style_line_width(s_widget_hands[index][hand], side < 40 ? 1 : 2, 0);
            lv_obj_set_pos(s_widget_hands[index][hand], -2, -2);
        }
        set_clock_hands(index, 10, 10);
    } else if (type == 2) {
        lv_obj_t *header = graphic_box(box, side, side < 30 ? 4 : 8, accent);
        lv_obj_set_pos(header, -2, -2);
        lv_obj_set_style_bg_color(header, color(accent), 0);
        lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
        s_widget_days[index] = label(box, mode == 3 ? "--" : "#", side >= 60 ? 28 : side >= 34 ? 20 : 14, color(accent));
        if (side < 30) lv_obj_set_style_text_font(s_widget_days[index], &lv_font_montserrat_12, 0);
        if (side < 24) lv_obj_set_style_text_font(s_widget_days[index], &lv_font_montserrat_8, 0);
        lv_obj_align(s_widget_days[index], LV_ALIGN_CENTER, 0, 2);
    } else {
        for (int bar = 0; bar < 3; bar++) {
            lv_obj_t *stripe = graphic_box(box, side / 8, side / 2, accent);
            lv_obj_set_pos(stripe, side * (bar * 2 + 2) / 8 - 2, side / 4 - 2);
            lv_obj_set_style_bg_color(stripe, color(accent), 0);
            lv_obj_set_style_bg_opa(stripe, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(stripe, 0, 0);
        }
    }
}

static void update_widget_graphic(unsigned index, const struct tm *now, bool synced)
{
    unsigned mode = s_widget_graphic_modes[index];
    if (mode == 2 && synced) {
        int minute = now->tm_hour * 60 + now->tm_min;
        if (s_widget_minutes[index] != minute) {
            set_clock_hands(index, now->tm_hour, now->tm_min);
            s_widget_minutes[index] = minute;
        }
    } else if (mode == 3 && synced) {
        char day[4];
        strftime(day, sizeof(day), "%d", now);
        if (strcmp(lv_label_get_text(s_widget_days[index]), day))
            lv_label_set_text(s_widget_days[index], day);
    } else if (mode == 4 || mode == 5) {
        double reading;
        bool valid = goopad_pc_stat_value(s_widget_stats[index], &reading);
        int value = valid ? (int)(reading + 0.5) : 0;
        if (value > 100) value = 100;
        if (value < 0) value = 0;
        int state = valid ? value : -1;
        if (s_widget_meter_values[index] != state) {
            lv_obj_set_style_opa(s_widget_graphics[index], valid ? LV_OPA_COVER : LV_OPA_30, 0);
            if (mode == 4) lv_bar_set_value(s_widget_graphics[index], value, LV_ANIM_OFF);
            else lv_arc_set_value(s_widget_graphics[index], value);
            s_widget_meter_values[index] = state;
        }
    }
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
    s_menu_tracking = false;
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

static void save_device_controls_cb(lv_event_t *event)
{
    (void)event;
    esp_err_t err = goopad_set_controls(
        lv_slider_get_value(s_brightness_slider), lv_slider_get_value(s_volume_slider));
    if (err != ESP_OK && s_settings_status && lv_obj_is_valid(s_settings_status)) {
        char message[64];
        snprintf(message, sizeof(message), "Could not save controls: %s",
                 esp_err_to_name(err));
        lv_label_set_text(s_settings_status, message);
    } else if (s_settings_status && lv_obj_is_valid(s_settings_status)) {
        lv_label_set_text(s_settings_status, "Settings saved.");
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

static void power_slider_cb(lv_event_t *event)
{
    lv_obj_t *slider = lv_event_get_target(event);
    int value = lv_slider_get_value(slider);
    char text[40];
    if (slider == s_brightness_slider) {
        board_backlight_set(value);
        snprintf(text, sizeof(text), "Display brightness: %d%%", value);
        lv_label_set_text(s_brightness_label, text);
    } else {
        hw_audio_set_volume(value);
        snprintf(text, sizeof(text), "Sound volume: %d%%", value);
        lv_label_set_text(s_volume_label, text);
    }
}

static void build_info_panel(bool settings)
{
    if (!s_info_overlay || !lv_obj_is_valid(s_info_overlay)) return;
    lv_obj_clean(s_info_overlay);
    s_settings_status = NULL;
    s_brightness_slider = s_volume_slider = NULL;
    s_brightness_label = s_volume_label = NULL;

    lv_obj_t *panel = lv_obj_create(s_info_overlay);
    lv_obj_set_size(panel, 420, settings ? 420 : 300);
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
        const char *initial = goopad_initial_password();
        snprintf(details, sizeof(details), "Web editor: https://%s\n%s%s",
                 address, initial ? "Initial password: " : "Original password unavailable on this upgrade.",
                 initial ? initial : "");
        lv_obj_t *info = label(panel, details, 14, lv_color_white());
        lv_obj_set_width(info, 380);
        lv_label_set_long_mode(info, LV_LABEL_LONG_WRAP);
        lv_obj_align(info, LV_ALIGN_TOP_LEFT, 0, 40);
        lv_obj_t *hint = label(panel, "Changed passwords are never displayed here.\nUse your existing admin password to sign in.",
                               14, color(0xCBD5E1));
        lv_obj_set_width(hint, 380);
        lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
        lv_obj_align(hint, LV_ALIGN_TOP_LEFT, 0, 120);
        button(panel, "Settings", 150, 220, 112, 42, show_settings_cb, NULL, 0x4F46E5);
        button(panel, "Close", 278, 220, 112, 42, close_info_cb, NULL, 0x475569);
        return;
    }

    lv_obj_t *appearance_hint = label(panel, "Change appearance from the web editor.",
                                      14, color(0xCBD5E1));
    lv_obj_align(appearance_hint, LV_ALIGN_TOP_LEFT, 0, 42);
    button(panel, "Save settings", 224, 68, 150, 42,
           save_device_controls_cb, NULL, 0x4F46E5);

    char text[40];
    snprintf(text, sizeof(text), "Display brightness: %d%%", board_backlight_get());
    s_brightness_label = label(panel, text, 14, lv_color_white());
    lv_obj_set_pos(s_brightness_label, 0, 122);
    s_brightness_slider = lv_slider_create(panel);
    lv_obj_set_pos(s_brightness_slider, 10, 151);
    lv_obj_set_size(s_brightness_slider, 354, 14);
    lv_slider_set_range(s_brightness_slider, 0, 100);
    lv_slider_set_value(s_brightness_slider, board_backlight_get(), LV_ANIM_OFF);
    lv_obj_add_event_cb(s_brightness_slider, power_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
    snprintf(text, sizeof(text), "Sound volume: %d%%", hw_audio_get_volume());
    s_volume_label = label(panel, text, 14, lv_color_white());
    lv_obj_set_pos(s_volume_label, 0, 181);
    s_volume_slider = lv_slider_create(panel);
    lv_obj_set_pos(s_volume_slider, 10, 210);
    lv_obj_set_size(s_volume_slider, 354, 14);
    lv_slider_set_range(s_volume_slider, 0, 100);
    lv_slider_set_value(s_volume_slider, hw_audio_get_volume(), LV_ANIM_OFF);
    lv_obj_add_event_cb(s_volume_slider, power_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_t *power_hint = label(panel, "Both controls retain the board's safe power caps.",
                                14, color(0x94A3B8));
    lv_obj_set_width(power_hint, 374);
    lv_obj_set_pos(power_hint, 0, 241);
    button(panel, "Install SD /goopad.bin", 0, 278, 250, 38,
           install_sd_ota_cb, NULL, 0x4F46E5);
    s_settings_status = label(panel, "", 12, color(0xCBD5E1));
    lv_obj_set_width(s_settings_status, 380);
    lv_label_set_long_mode(s_settings_status, LV_LABEL_LONG_DOT);
    lv_obj_align(s_settings_status, LV_ALIGN_TOP_LEFT, 0, 320);
    button(panel, "Back", 140, 344, 110, 38, back_to_info_cb, NULL, 0x475569);
    button(panel, "Close", 264, 344, 110, 38, close_info_cb, NULL, 0x475569);
}

static void menu_slide(void *panel, int32_t y)
{
    lv_obj_set_y(panel, y);
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
    lv_obj_t *panel = lv_obj_get_child(s_info_overlay, 0);
    lv_obj_update_layout(panel);
    int target_y = lv_obj_get_y(panel);
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, panel);
    lv_anim_set_exec_cb(&animation, menu_slide);
    /* Animation positions are offsets from the panel's centered alignment. */
    int start_y = -lv_obj_get_height(panel) - target_y;
    lv_anim_set_values(&animation, start_y, 0);
    lv_anim_set_duration(&animation, 220);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_out);
    lv_obj_set_y(panel, start_y);
    lv_anim_start(&animation);
}

static void menu_swipe_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING &&
        code != LV_EVENT_RELEASED && code != LV_EVENT_PRESS_LOST) return;
    lv_indev_t *indev = lv_event_get_indev(event);
    if (!indev) return;
    lv_point_t point;
    lv_indev_get_point(indev, &point);
    if (code == LV_EVENT_PRESSED) {
        s_menu_press = point;
        s_menu_tracking = point.y <= 32;
    } else if (code == LV_EVENT_PRESSING && s_menu_tracking) {
        int dy = point.y - s_menu_press.y;
        int dx = abs(point.x - s_menu_press.x);
        if (dy >= 64 && dy > dx) {
            s_menu_tracking = false;
            lv_indev_wait_release(indev);
            show_info_cb(NULL);
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        s_menu_tracking = false;
    }
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
    clear_dynamic_assets();
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
    clear_dynamic_assets();
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
    esp_err_t err = goopad_trigger(index);
    if (err != ESP_OK) ESP_LOGW(TAG, "button %u: %s", index, esp_err_to_name(err));
}

static void page_navigation_cb(lv_event_t *event)
{
    uintptr_t data = (uintptr_t)lv_event_get_user_data(event);
    unsigned index = data & 0xff;
    unsigned direction = data >> 8;
    if (!s_pages) return;
    esp_err_t err = goopad_navigation_sound(index);
    if (err != ESP_OK) ESP_LOGW(TAG, "button %u sound: %s", index, esp_err_to_name(err));
    int width = lv_obj_get_width(s_pages);
    int count = lv_obj_get_child_count(s_pages);
    if (width > 0 && count > 0) {
        int page = (lv_obj_get_scroll_x(s_pages) + width / 2) / width;
        if (direction == 3) page = 0;
        else if (direction == 1 && page < count - 1) page++;
        else if (direction == 2 && page > 0) page--;
        lv_obj_scroll_to_x(s_pages, page * width, LV_ANIM_ON);
    }
}

static void stopwatch_cb(lv_event_t *event)
{
    unsigned index = (unsigned)(uintptr_t)lv_event_get_user_data(event);
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESSED) s_stopwatch_long_press[index] = false;
    else if (code == LV_EVENT_LONG_PRESSED) {
        s_stopwatch_elapsed[index] = 0;
        s_stopwatch_running[index] = false;
        s_stopwatch_long_press[index] = true;
        update_widgets();
    } else if (code == LV_EVENT_CLICKED && !s_stopwatch_long_press[index]) {
        int64_t now = esp_timer_get_time();
        if (s_stopwatch_running[index]) s_stopwatch_elapsed[index] += now - s_stopwatch_started[index];
        else s_stopwatch_started[index] = now;
        s_stopwatch_running[index] = !s_stopwatch_running[index];
        update_widgets();
    }
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
    s_menu_tracking = false;
    s_settings_status = NULL;
    s_ota_armed = false;
    s_dots = NULL;
    s_page_dot_count = 0;
    memset(s_page_dots, 0, sizeof(s_page_dots));
    goopad_get_config(s_config_buffer, 12001);
    strlcpy(s_rendered_config, s_config_buffer, 12001);
    s_grid_shown = true;
    lv_obj_clean(s_screen);
    clear_dynamic_assets();
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
    cJSON *hide_item = root ? cJSON_GetObjectItemCaseSensitive(root, "hideUnconfigured") : NULL;
    cJSON *offset_item = root ? cJSON_GetObjectItemCaseSensitive(root, "timeOffsetMinutes") : NULL;
    bool hide = cJSON_IsTrue(hide_item);
    bool hide_empty = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "hideEmptyTiles"));
    s_time_offset = cJSON_IsNumber(offset_item) ? offset_item->valueint : 0;
    /* Visible positions map back to original macro indices; compacting the
     * grid must never redirect a tile press to a different saved action. */
    int indices[MAX_MACROS], visible = 0;
    for (int i = 0; i < total && i < MAX_MACROS; i++) {
        if (!hide || macro_configured(cJSON_GetArrayItem(macros, i))) indices[visible++] = i;
    }
    int per_page = cols * rows;
    int page_count = (visible + per_page - 1) / per_page;
    cJSON *pages_item = cJSON_GetObjectItemCaseSensitive(root, "pageCount");
    int minimum_pages = hide ? 1 : cJSON_IsNumber(pages_item) ? pages_item->valueint : 2;
    if (page_count < minimum_pages) page_count = minimum_pages;
    cJSON *bg = root ? cJSON_GetObjectItemCaseSensitive(root, "background") : NULL;
    cJSON *device_theme = root ? cJSON_GetObjectItemCaseSensitive(root, "theme") : NULL;
    const char *theme_name = cJSON_IsString(device_theme)
                                 ? device_theme->valuestring : "midnight";
    bool light_theme = cJSON_IsString(device_theme) &&
                       strcmp(device_theme->valuestring, "light") == 0;
    unsigned theme_background = 0x111827;
    uint32_t accent_color = 0xA5B4FC;
    if (!strcmp(theme_name, "ocean")) {
        accent_color = 0x67E8F9;
        theme_background = 0x0B1B2A;
    } else if (!strcmp(theme_name, "forest")) {
        accent_color = 0x6EE7B7;
        theme_background = 0x10251D;
    } else if (light_theme) {
        accent_color = 0x334155;
        theme_background = 0xF1F5F9;
    } else if (!strcmp(theme_name, "slate")) {
        accent_color = 0xCBD5E1;
        theme_background = 0x202B3A;
    } else if (!strcmp(theme_name, "violet")) {
        accent_color = 0xD8B4FE;
        theme_background = 0x241938;
    } else if (!strcmp(theme_name, "ember")) {
        accent_color = 0xFDBA74;
        theme_background = 0x2F1B1A;
    } else if (!strcmp(theme_name, "berry")) {
        accent_color = 0xFDA4C4;
        theme_background = 0x301729;
    } else if (!strcmp(theme_name, "teal")) {
        accent_color = 0x5EEAD4;
        theme_background = 0x102F31;
    } else if (!strcmp(theme_name, "black")) {
        accent_color = 0xE2E8F0;
        theme_background = 0x000000;
    }
    unsigned background = !strcmp(theme_name, "custom") && cJSON_IsString(bg)
        ? (unsigned)strtoul(bg->valuestring, NULL, 16) : theme_background;
    lv_obj_set_style_bg_color(s_screen, color(background), 0);
    lv_obj_set_style_text_color(s_screen, light_theme ? color(0x172033) : lv_color_white(), 0);
    lv_image_cache_drop(&s_background_image);
    cJSON *background_image = root ? cJSON_GetObjectItemCaseSensitive(root, "backgroundImage") : NULL;
    if (!strcmp(theme_name, "custom") && cJSON_IsString(background_image)) {
        if (!s_background_pixels) {
            s_background_pixels = heap_caps_malloc(480 * 480 * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        }
        char path[64];
        snprintf(path, sizeof(path), BSP_SD_MOUNT_POINT "/goopad/icon-%s.480.rgb565",
                 background_image->valuestring);
        FILE *file = g_app.sd.mounted ? fopen(path, "rb") : NULL;
        bool loaded = file && s_background_pixels &&
                      fread(s_background_pixels, 1, 480 * 480 * 2, file) == 480 * 480 * 2;
        if (file) fclose(file);
        if (loaded) {
            s_background_image = (lv_image_dsc_t) {
                .header = {.magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RGB565,
                           .w = 480, .h = 480, .stride = 960},
                .data_size = 480 * 480 * 2, .data = s_background_pixels,
            };
            lv_obj_t *image = lv_image_create(s_screen);
            lv_image_set_src(image, &s_background_image);
            lv_obj_set_pos(image, 0, 0);
            lv_obj_set_clickable(image, false);
        } else {
            ESP_LOGE(TAG, "Could not load custom background from SD");
        }
    } else {
        free(s_background_pixels);
        s_background_pixels = NULL;
    }

    int screen_width = lv_obj_get_width(s_screen);
    int screen_height = lv_obj_get_height(s_screen);
    if (screen_width <= 0 || screen_height <= 0) {
        ESP_LOGE(TAG, "invalid screen dimensions: %d x %d", screen_width, screen_height);
        cJSON_Delete(root);
        return;
    }
    lv_obj_t *menu_edge = lv_obj_create(s_screen);
    lv_obj_set_pos(menu_edge, 0, 0);
    lv_obj_set_size(menu_edge, screen_width, 48);
    lv_obj_set_style_bg_opa(menu_edge, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(menu_edge, 0, 0);
    lv_obj_set_style_pad_all(menu_edge, 0, 0);
    lv_obj_set_scrollable(menu_edge, false);
    lv_obj_set_clickable(menu_edge, true);
    lv_obj_add_flag(menu_edge, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_event_cb(menu_edge, menu_swipe_cb, LV_EVENT_ALL, NULL);

    s_pages = lv_obj_create(s_screen);
    const int page_top = 48;
    const int footer_height = 36;
    int page_height = screen_height - page_top - footer_height;
    if (page_height < 1) page_height = 1;
    lv_obj_set_pos(s_pages, 0, page_top);
    lv_obj_set_size(s_pages, screen_width, page_height);
    lv_obj_set_style_bg_opa(s_pages, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_pages, 0, 0);
    lv_obj_set_style_pad_all(s_pages, 0, 0);
    lv_obj_set_style_pad_column(s_pages, 0, 0);
    lv_obj_set_flex_flow(s_pages, LV_FLEX_FLOW_ROW);
    lv_obj_set_scroll_dir(s_pages, LV_DIR_HOR);
    lv_obj_set_scroll_snap_x(s_pages, LV_SCROLL_SNAP_START);
    lv_obj_set_scrollbar_mode(s_pages, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(s_pages, page_scroll_cb, LV_EVENT_SCROLL_END, NULL);

    const int tile_padding = 10;
    const int tile_gap = 8;
    int cell_width = (screen_width - 2 * tile_padding - (cols - 1) * tile_gap) / cols;
    int cell_height = (page_height - 2 * tile_padding - (rows - 1) * tile_gap) / rows;
    if (cell_width < 1 || cell_height < 1) {
        ESP_LOGE(TAG, "screen too small for %dx%d grid in %dx%d viewport",
                 cols, rows, screen_width, page_height);
        cJSON_Delete(root);
        return;
    }
    ESP_LOGI(TAG, "render grid %dx%d on %dx%d display", cols, rows,
             screen_width, screen_height);
    for (int page = 0; page < page_count; page++) {
        lv_obj_t *surface = lv_obj_create(s_pages);
        lv_obj_set_size(surface, screen_width, page_height);
        lv_obj_set_style_bg_opa(surface, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(surface, 0, 0);
        lv_obj_set_style_pad_all(surface, 0, 0);
        lv_obj_set_scrollable(surface, false);
        for (int slot = 0; slot < per_page; slot++) {
            int position = page * per_page + slot;
            if (hide && position >= visible) continue;
            int index = position < visible ? indices[position] : MAX_MACROS;
            cJSON *macro = index < MAX_MACROS ? cJSON_GetArrayItem(macros, index) : NULL;
            if (hide_empty && !macro_configured(macro)) continue;
            lv_obj_t *tile = lv_button_create(surface);
            int width = cell_width;
            int height = cell_height;
            int column = slot % cols;
            int row = slot / cols;
            int x = tile_padding + column * (cell_width + tile_gap);
            int y = tile_padding + row * (cell_height + tile_gap);
            cJSON *shape = macro
                ? cJSON_GetObjectItemCaseSensitive(macro, "shape") : NULL;
            bool circle = cJSON_IsString(shape) &&
                          !strcmp(shape->valuestring, "circle");
            bool soft = cJSON_IsString(shape) && !strcmp(shape->valuestring, "soft");
            bool pill = cJSON_IsString(shape) && !strcmp(shape->valuestring, "pill");
            if (circle) {
                width = height = cell_width < cell_height ? cell_width : cell_height;
                x += (cell_width - width) / 2;
                y += (cell_height - height) / 2;
            }
            if (pill) {
                height = cell_height * 3 / 4;
                y += (cell_height - height) / 2;
            }
            lv_obj_set_size(tile, width, height);
            lv_obj_set_pos(tile, x, y);
            lv_obj_set_style_bg_color(tile, color(0x273449), 0);
            int radius = circle ? LV_RADIUS_CIRCLE : pill ? height / 2 :
                         soft ? (width < height ? width : height) / 3 :
                         cJSON_IsString(shape) && !strcmp(shape->valuestring, "square")
                             ? 4 : 18;
            cJSON *outline = macro ? cJSON_GetObjectItemCaseSensitive(macro, "outline") : NULL;
            cJSON *outline_color = macro ? cJSON_GetObjectItemCaseSensitive(macro, "outlineColor") : NULL;
            lv_obj_set_style_border_width(tile, cJSON_IsFalse(outline) ? 0 : 2, 0);
            lv_obj_set_style_border_color(tile, cJSON_IsString(outline_color)
                ? color((unsigned)strtoul(outline_color->valuestring, NULL, 16)) : color(accent_color), 0);
            lv_obj_set_style_radius(tile, radius, 0);
            lv_obj_set_style_pad_all(tile, 0, 0);
            lv_obj_set_style_shadow_width(tile, 0, 0);
            lv_obj_set_style_clip_corner(tile, true, 0);
            if (macro_configured(macro)) {
                g_app.rendered_tiles++;
                cJSON *name = cJSON_GetObjectItemCaseSensitive(macro, "name");
                cJSON *icon = cJSON_GetObjectItemCaseSensitive(macro, "icon");
                cJSON *accent = cJSON_GetObjectItemCaseSensitive(macro, "color");
                cJSON *font_color = cJSON_GetObjectItemCaseSensitive(macro, "textColor");
                cJSON *icon_color = cJSON_GetObjectItemCaseSensitive(macro, "iconColor");
                const char *caption = cJSON_IsString(name) ? name->valuestring : "";
                const char *glyph = cJSON_IsString(icon) ? icon->valuestring : "•";
                unsigned accent_value = cJSON_IsString(accent)
                    ? (unsigned)strtoul(accent->valuestring, NULL, 16) : 0x4F46E5;
                lv_color_t text_color = cJSON_IsString(font_color)
                    ? color((unsigned)strtoul(font_color->valuestring, NULL, 16))
                    : light_theme ? color(0x172033) : lv_color_white();
                lv_color_t icon_tint = cJSON_IsString(icon_color)
                    ? color((unsigned)strtoul(icon_color->valuestring, NULL, 16)) : text_color;
                lv_obj_set_style_bg_color(tile, color(accent_value), 0);
                cJSON *mode = cJSON_GetObjectItemCaseSensitive(macro, "actionMode");
                cJSON *widget = cJSON_GetObjectItemCaseSensitive(macro, "widget");
                bool is_widget = cJSON_IsString(mode) && !strcmp(mode->valuestring, "widget");
                bool is_navigation = cJSON_IsString(mode) && !strcmp(mode->valuestring, "page_navigation");
#ifdef CONFIG_GOOPAD_HOMEKIT
                bool is_homekit = cJSON_IsString(mode) && !strcmp(mode->valuestring, "homekit");
#else
                bool is_homekit = false;
#endif
                cJSON *tile_gradient = cJSON_GetObjectItemCaseSensitive(macro, "tileGradient");
                if (!is_widget && cJSON_IsString(tile_gradient)) {
                    lv_obj_set_style_bg_grad_color(tile, color((unsigned)strtoul(tile_gradient->valuestring, NULL, 16)), 0);
                    lv_obj_set_style_bg_grad_dir(tile, LV_GRAD_DIR_VER, 0);
                }
                cJSON *url = cJSON_GetObjectItemCaseSensitive(macro, "url");
                cJSON *sound = cJSON_GetObjectItemCaseSensitive(macro, "sound");
                bool actionable = !is_widget && (is_homekit || (cJSON_IsString(url) && url->valuestring[0]) ||
                                  (cJSON_IsString(sound) && strcmp(sound->valuestring, "none")));
                if (is_navigation) {
                    cJSON *direction = cJSON_GetObjectItemCaseSensitive(macro, "pageDirection");
                    unsigned value = cJSON_IsString(direction) && !strcmp(direction->valuestring, "previous") ? 2 :
                                     cJSON_IsString(direction) && !strcmp(direction->valuestring, "first") ? 3 : 1;
                    lv_obj_add_event_cb(tile, page_navigation_cb, LV_EVENT_CLICKED, (void *)((uintptr_t)index | (value << 8)));
                }
                else if (actionable) lv_obj_add_event_cb(tile, tile_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)index);
                else lv_obj_set_clickable(tile, false);
#ifdef CONFIG_GOOPAD_HOMEKIT
                if (is_homekit) {
                    cJSON *id = cJSON_GetObjectItemCaseSensitive(macro, "homekitId");
                    if (cJSON_IsString(id)) {
                        strlcpy(s_homekit_ids[index], id->valuestring, sizeof(s_homekit_ids[index]));
                        lv_obj_t *badge = lv_obj_create(tile);
                        lv_obj_set_size(badge, 18, 18);
                        if (circle) lv_obj_align(badge, LV_ALIGN_TOP_MID, 0, 5);
                        else lv_obj_align(badge, LV_ALIGN_TOP_RIGHT, pill ? -height / 3 : soft ? -width / 8 : -5, 5);
                        lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0);
                        lv_obj_set_style_bg_color(badge, color(0x0F172A), 0);
                        lv_obj_set_style_bg_opa(badge, LV_OPA_70, 0);
                        lv_obj_set_style_border_width(badge, 0, 0);
                        lv_obj_set_style_shadow_width(badge, 0, 0);
                        lv_obj_set_style_pad_all(badge, 0, 0);
                        lv_obj_set_scrollable(badge, false);
                        lv_obj_set_clickable(badge, false);
                        s_homekit_indicators[index] = lv_obj_create(badge);
                        lv_obj_set_size(s_homekit_indicators[index], 9, 9);
                        lv_obj_center(s_homekit_indicators[index]);
                        lv_obj_set_style_radius(s_homekit_indicators[index], LV_RADIUS_CIRCLE, 0);
                        lv_obj_set_style_border_width(s_homekit_indicators[index], 2, 0);
                        lv_obj_set_style_shadow_width(s_homekit_indicators[index], 0, 0);
                        lv_obj_set_style_pad_all(s_homekit_indicators[index], 0, 0);
                        lv_obj_set_scrollable(s_homekit_indicators[index], false);
                        lv_obj_set_clickable(s_homekit_indicators[index], false);
                    }
                }
#endif
                cJSON *custom_image = cJSON_GetObjectItemCaseSensitive(macro, "iconImage");
                cJSON *image_format = cJSON_GetObjectItemCaseSensitive(macro, "iconFormat");
                bool alpha = cJSON_IsString(image_format) &&
                             !strcmp(image_format->valuestring, "rgb565a8");
                cJSON *size_item = cJSON_GetObjectItemCaseSensitive(macro, "iconSize");
                int side = alpha && cJSON_IsNumber(size_item) && size_item->valueint == 160 ? 160 : 32;
                int icon_size = (width < height ? width : height) - (caption[0] ? 32 : 20);
                cJSON *visual_scale = cJSON_GetObjectItemCaseSensitive(macro, "visualScale");
                if (cJSON_IsNumber(visual_scale)) icon_size = icon_size * visual_scale->valueint / 100;
                if (icon_size < 16) icon_size = 16;
                int icon_y = caption[0] ? -10 : 0;
                bool image_drawn = false;
                if (is_widget) {
                    g_app.widget_count++;
                    unsigned type = cJSON_IsString(widget) && !strcmp(widget->valuestring, "weather") ? 6 :
                                    cJSON_IsString(widget) && !strcmp(widget->valuestring, "stock") ? 7 :
                                    cJSON_IsString(widget) && !strcmp(widget->valuestring, "crypto") ? 8 :
                                    cJSON_IsString(widget) && !strcmp(widget->valuestring, "stopwatch") ? 5 :
                                    cJSON_IsString(widget) && !strcmp(widget->valuestring, "countdown") ? 4 :
                                    cJSON_IsString(widget) && !strcmp(widget->valuestring, "pc_monitor") ? 3 :
                                    cJSON_IsString(widget) && !strcmp(widget->valuestring, "date") ? 2 : 1;
                    s_widget_types[index] = type;
                    if (type == 4) {
                        cJSON *target = cJSON_GetObjectItemCaseSensitive(macro, "countdownAt");
                        s_countdown_targets[index] = cJSON_IsNumber(target) ? (int64_t)target->valuedouble : 0;
                    } else if (type == 5) {
                        lv_obj_set_clickable(tile, true);
                        lv_obj_add_event_cb(tile, stopwatch_cb, LV_EVENT_ALL, (void *)(uintptr_t)index);
                    }
                    cJSON *format = cJSON_GetObjectItemCaseSensitive(macro, type == 2 ? "dateFormat" : "clockFormat");
                    s_widget_formats[index] = goopad_widget_time_format(type == 2,
                        cJSON_IsString(format) ? format->valuestring : type == 2 ? "iso" : "24h");
                    if (!s_widget_formats[index]) {
                        ESP_LOGW(TAG, "Invalid time format on tile %d; using default", index);
                        s_widget_formats[index] = type == 2 ? "%Y-%m-%d" : "%H:%M";
                    }
                    cJSON *stat = cJSON_GetObjectItemCaseSensitive(macro, "pcStat");
                    int stat_index = goopad_pc_stat_index(cJSON_IsString(stat) ? stat->valuestring : "summary");
                    s_widget_stats[index] = stat_index >= 0 ? (unsigned)stat_index : 0;
                    cJSON *unit = cJSON_GetObjectItemCaseSensitive(macro, "temperatureUnit");
                    s_widget_fahrenheit[index] = cJSON_IsString(unit) && !strcmp(unit->valuestring, "F");
                    bool long_clock = type == 1 && strlen(s_widget_formats[index]) > 5;
                    bool long_stat = type == 3 && s_widget_stats[index] != 0;
                    cJSON *widget_theme = cJSON_GetObjectItemCaseSensitive(macro, "widgetTheme");
                    const goopad_widget_style_t *style = goopad_widget_style(cJSON_IsString(widget_theme) ? widget_theme->valuestring : NULL);
                    cJSON *widget_accent = cJSON_GetObjectItemCaseSensitive(macro, "widgetAccent");
                    unsigned graphic_accent = cJSON_IsString(widget_accent) ?
                        (unsigned)strtoul(widget_accent->valuestring, NULL, 16) : 0xA5B4FC;
                    if (style) {
                        lv_obj_set_style_bg_color(tile, color(style->background), 0);
                        lv_obj_set_style_bg_grad_color(tile, color(style->gradient), 0);
                        lv_obj_set_style_bg_grad_dir(tile, style->background == style->gradient ? LV_GRAD_DIR_NONE : LV_GRAD_DIR_VER, 0);
                        text_color = color(style->text);
                        graphic_accent = style->accent;
                        lv_obj_set_style_border_color(tile, color(style->accent), 0);
                    } else {
                        cJSON *gradient = cJSON_GetObjectItemCaseSensitive(macro, "widgetGradient");
                        if (cJSON_IsString(gradient)) {
                            lv_obj_set_style_bg_grad_color(tile, color((unsigned)strtoul(gradient->valuestring, NULL, 16)), 0);
                            lv_obj_set_style_bg_grad_dir(tile, LV_GRAD_DIR_VER, 0);
                        }
                    }
                    cJSON *widget_layout = cJSON_GetObjectItemCaseSensitive(macro, "widgetLayout");
                    bool panel = cJSON_IsString(widget_layout) && !strcmp(widget_layout->valuestring, "panel") &&
                                 width >= 110 && height >= 100 && !circle && !pill && !soft;
                    int header = panel ? 20 : 0;
                    if (panel) {
                        static const char *stat_titles[] = {"PC STATS", "CPU", "RAM", "RAM USED", "GPU",
                                                             "GPU RAM", "CPU TEMP", "GPU TEMP",
                                                             "DOWNLOAD", "UPLOAD", "FAN"};
                        const char *title = type == 1 ? "CLOCK" : type == 2 ? "DATE" : type == 4 ? "COUNTDOWN" :
                                            type == 5 ? "STOPWATCH" : type == 6 ? "WEATHER" : type == 7 ? "STOCK" :
                                            type == 8 ? "CRYPTO" : stat_titles[s_widget_stats[index]];
                        lv_obj_t *heading = label(tile, title, 14, color(graphic_accent));
                        lv_obj_set_style_text_font(heading, &lv_font_montserrat_10, 0);
                        lv_obj_set_style_text_letter_space(heading, 1, 0);
                        lv_obj_align(heading, LV_ALIGN_TOP_LEFT, 12, 9);
                        lv_obj_t *rule = lv_obj_create(tile);
                        lv_obj_set_size(rule, 22, 3);
                        lv_obj_align(rule, LV_ALIGN_TOP_RIGHT, -12, 12);
                        lv_obj_set_style_bg_color(rule, color(graphic_accent), 0);
                        lv_obj_set_style_border_width(rule, 0, 0);
                        lv_obj_set_style_radius(rule, LV_RADIUS_CIRCLE, 0);
                        lv_obj_set_scrollable(rule, false);
                        lv_obj_set_clickable(rule, false);
                    }
                    cJSON *widget_size = cJSON_GetObjectItemCaseSensitive(macro, "widgetTextSize");
                    s_widget_sizes[index] = cJSON_IsNumber(widget_size) && widget_size->valueint ? (unsigned)widget_size->valueint :
                        width < 140 || ((long_clock || long_stat) && width < 200) ? 14 : 28;
                    s_widget_widths[index] = width - 16;
                    s_widget_heights[index] = height - (caption[0] ? 36 : 16) - header;
                    s_widget_labels[index] = label(tile, "", width < 140 || ((long_clock || long_stat) && width < 200) ? 14 : 28, text_color);
                    lv_obj_set_width(s_widget_labels[index], width - 16);
                    lv_obj_set_style_text_align(s_widget_labels[index], LV_TEXT_ALIGN_CENTER, 0);
                    lv_obj_align(s_widget_labels[index], LV_ALIGN_CENTER, 0, icon_y + header / 2);
                    cJSON *graphic = cJSON_GetObjectItemCaseSensitive(macro, "widgetGraphic");
                    create_widget_graphic(index, tile, cJSON_IsString(graphic) ? graphic->valuestring : "text",
                        type, graphic_accent, width, height, caption[0], header);
                    if (circle && s_widget_graphic_modes[index]) {
                        s_widget_widths[index] = width * 3 / 4;
                        lv_obj_set_width(s_widget_labels[index], s_widget_widths[index]);
                    }
                    image_drawn = true;
                } else if (cJSON_IsString(custom_image) && cJSON_IsString(image_format) &&
                           !strcmp(image_format->valuestring, "gif") && g_app.sd.mounted) {
                    char path[64];
                    snprintf(path, sizeof(path), BSP_SD_MOUNT_POINT "/goopad/icon-%s.gif", custom_image->valuestring);
                    struct stat info;
                    if (stat(path, &info) == 0 && info.st_size >= 14 && info.st_size <= 128 * 1024) {
                        s_gif_buffers[index] = heap_caps_malloc(info.st_size + 240 * 8, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                        FILE *file = fopen(path, "rb");
                        bool loaded = file && s_gif_buffers[index] &&
                                      fread(s_gif_buffers[index], 1, info.st_size, file) == (size_t)info.st_size;
                        if (file) fclose(file);
                        size_t gif_bytes = info.st_size;
                        if (loaded) loaded = goopad_prepare_gif(s_gif_buffers[index], &gif_bytes, info.st_size + 240 * 8);
                        if (loaded) {
                            uint8_t *data = s_gif_buffers[index];
                            int w = data[6] | data[7] << 8, h = data[8] | data[9] << 8;
                            s_gif_images[index] = (lv_image_dsc_t) {
                                .header = {.magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RAW, .w = w, .h = h},
                                .data_size = gif_bytes, .data = data,
                            };
                            lv_obj_t *gif = lv_gif_create(tile);
                            lv_gif_set_src(gif, &s_gif_images[index]);
                            image_drawn = lv_gif_is_loaded(gif);
                            if (image_drawn) {
                                g_app.gif_count++;
                                lv_obj_add_event_cb(gif, gif_loop_cb, LV_EVENT_READY, NULL);
                                cJSON *gif_fit = cJSON_GetObjectItemCaseSensitive(macro, "gifFit");
                                bool fill = cJSON_IsString(gif_fit) && !strcmp(gif_fit->valuestring, "cover");
                                int scale = icon_size * 256 / (w > h ? w : h);
                                if (fill) {
                                    int scale_x = (width * 256 + w - 1) / w;
                                    int scale_y = (height * 256 + h - 1) / h;
                                    scale = scale_x > scale_y ? scale_x : scale_y;
                                    lv_obj_set_style_border_post(tile, true, 0);
                                }
                                lv_image_set_scale(gif, scale);
                                lv_obj_align(gif, LV_ALIGN_CENTER, 0, fill ? 0 : icon_y);
                                s_gif_objects[index] = gif;
                                s_gif_pages[index] = page;
                                lv_gif_pause(gif);
                            } else {
                                lv_obj_delete(gif);
                                ESP_LOGE(TAG, "Could not decode GIF for tile %d", index);
                            }
                        } else ESP_LOGE(TAG, "Could not read GIF for tile %d", index);
                    } else ESP_LOGE(TAG, "GIF missing or invalid for tile %d", index);
                } else if (cJSON_IsString(custom_image) && g_app.sd.mounted && s_icon_buffers) {
                    char path[64];
                    snprintf(path, sizeof(path), BSP_SD_MOUNT_POINT "/goopad/icon-%s.%s",
                             custom_image->valuestring, side == 160 ? "160.rgb565a8" : alpha ? "rgb565a8" : "rgb565");
                    FILE *image = fopen(path, "rb");
                    if (image) {
                        uint8_t *pixels = s_icon_buffers + (size_t)index * ICON_BYTES;
                        size_t bytes = (size_t)side * side * (alpha ? 3 : 2);
                        image_drawn = fread(pixels, 1, bytes, image) == bytes;
                        fclose(image);
                        if (image_drawn) {
                            lv_image_cache_drop(&s_icon_images[index]);
                            s_icon_images[index] = (lv_image_dsc_t) {
                                .header = {
                                    .magic = LV_IMAGE_HEADER_MAGIC,
                                    .cf = alpha ? LV_COLOR_FORMAT_RGB565A8 : LV_COLOR_FORMAT_RGB565,
                                    .w = side, .h = side, .stride = side * 2,
                                },
                                .data_size = bytes,
                                .data = pixels,
                            };
                            lv_obj_t *image_obj = lv_image_create(tile);
                            lv_image_set_src(image_obj, &s_icon_images[index]);
                            cJSON *fa_icon = cJSON_GetObjectItemCaseSensitive(macro, "faIcon");
                            if (cJSON_IsString(fa_icon) && cJSON_IsString(icon_color)) {
                                lv_obj_set_style_image_recolor(image_obj, icon_tint, 0);
                                lv_obj_set_style_image_recolor_opa(image_obj, LV_OPA_COVER, 0);
                            }
                            lv_image_set_scale(image_obj, icon_size * 256 / side);
                            lv_obj_align(image_obj, LV_ALIGN_CENTER, 0, icon_y);
                        }
                    } else {
                        ESP_LOGW(TAG, "could not load icon for macro %d", index);
                    }
                }
                cJSON *visual_source = cJSON_GetObjectItemCaseSensitive(macro, "iconSource");
                if (!image_drawn && (cJSON_IsString(custom_image) ||
                    (cJSON_IsString(visual_source) && strcmp(visual_source->valuestring, "emoji")))) {
                    bool gif_missing = (cJSON_IsString(image_format) && !strcmp(image_format->valuestring, "gif")) ||
                                       (cJSON_IsString(visual_source) && !strcmp(visual_source->valuestring, "gif"));
                    lv_obj_t *notice = label(tile, gif_missing ? "GIF unavailable" : "Image unavailable",
                                             14, text_color);
                    lv_obj_set_width(notice, width - 16);
                    lv_obj_set_style_text_align(notice, LV_TEXT_ALIGN_CENTER, 0);
                    lv_obj_align(notice, LV_ALIGN_CENTER, 0, icon_y);
                    ESP_LOGW(TAG, "Tile %d image unavailable; upload it in the website", index);
                    image_drawn = true;
                }
                if (!image_drawn) {
                    const char *drawn_icon = symbol_for_icon(glyph);
                    if (!drawn_icon) {
                        g_app.symbol_fallbacks++;
                        ESP_LOGW(TAG, "Tile %d unsupported symbol; choose full-color emoji or Font Awesome", index);
                    }
                    char initial[2] = {caption[0] ? caption[0] : '?', '\0'};
                    lv_obj_t *symbol = label(tile, drawn_icon ? drawn_icon : initial,
                                             28, icon_tint);
                    lv_obj_update_layout(symbol);
                    int largest = lv_obj_get_width(symbol) > lv_obj_get_height(symbol)
                        ? lv_obj_get_width(symbol) : lv_obj_get_height(symbol);
                    int scale = icon_size * 256 / (largest > 0 ? largest : 28);
                    lv_obj_set_style_transform_scale_x(symbol, scale, 0);
                    lv_obj_set_style_transform_scale_y(symbol, scale, 0);
                    lv_obj_align(symbol, LV_ALIGN_CENTER, 0, icon_y);
                }
                if (caption[0]) {
                    lv_obj_t *text = label(tile, caption, 14, text_color);
                    lv_obj_set_width(text, width - 16);
                    lv_label_set_long_mode(text, LV_LABEL_LONG_DOT);
                    lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
                    lv_obj_align(text, LV_ALIGN_BOTTOM_MID, 0, -6);
                }
#ifdef CONFIG_GOOPAD_HOMEKIT
                if (s_homekit_indicators[index]) lv_obj_move_foreground(lv_obj_get_parent(s_homekit_indicators[index]));
#endif
            } else {
                g_app.empty_tiles++;
                lv_obj_set_style_bg_color(tile, color(0x1F2937), 0);
                lv_obj_set_clickable(tile, false);
            }
        }
    }
    update_widgets();
    s_dots = lv_obj_create(s_screen);
    s_page_dot_count = (size_t)page_count;
    g_app.rendered_pages = (uint32_t)page_count;
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
        update_widgets();
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
