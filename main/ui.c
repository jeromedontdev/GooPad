/*
 * LVGL test UI.
 *
 * One tab per subsystem so each piece of hardware can be exercised and
 * judged independently. All LVGL calls happen either from the LVGL task
 * (timers, event callbacks) or under ui_lock().
 */
#include "app.h"
#include "board.h"

#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "ui";

#define COL_BG        lv_color_hex(0x101418)
#define COL_CARD      lv_color_hex(0x1C2228)
#define COL_TEXT      lv_color_hex(0xE8EDF2)
#define COL_DIM       lv_color_hex(0x8A97A3)
#define COL_PASS      lv_color_hex(0x2ECC71)
#define COL_FAIL      lv_color_hex(0xE74C3C)
#define COL_UNKNOWN   lv_color_hex(0x7F8C8D)
#define COL_ACCENT    lv_color_hex(0x3498DB)

/* Widgets refreshed by the periodic timer. */
static lv_obj_t *s_lbl_status[8];
static lv_obj_t *s_lbl_wifi_info;
static lv_obj_t *s_lbl_ble_info;
static lv_obj_t *s_slider_volume;
static lv_obj_t *s_slider_backlight;
static lv_obj_t *s_lbl_power_state;
static lv_obj_t *s_lbl_power_level;
static lv_obj_t *s_lbl_power_regs;
static lv_obj_t *s_lbl_power_hist;
static lv_obj_t *s_lbl_touch_coords;
static lv_obj_t *s_lbl_touch_count;
static lv_obj_t *s_lbl_touch_diag;
static lv_obj_t *s_lbl_audio_diag;
static lv_obj_t *s_lbl_sd_info;
static lv_obj_t *s_lbl_i2c;
static lv_obj_t *s_lbl_health;
static lv_obj_t *s_bar_power;

/* Touch drawing canvas. */
static lv_obj_t *s_canvas;
static uint8_t *s_canvas_buf;
static int s_last_x = -1;
static int s_last_y = -1;

#define CANVAS_W 440
#define CANVAS_H 240

/* ------------------------------------------------------------------ */

static lv_color_t status_color(test_status_t st)
{
    switch (st) {
        case TEST_PASS: return COL_PASS;
        case TEST_FAIL: return COL_FAIL;
        default:        return COL_UNKNOWN;
    }
}

static const char *status_text(test_status_t st)
{
    switch (st) {
        case TEST_PASS: return "PASS";
        case TEST_FAIL: return "FAIL";
        case TEST_SKIP: return "SKIP";
        default:        return "--";
    }
}

static lv_obj_t *make_card(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, COL_CARD, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_set_style_pad_row(card, 6, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollable(card, false);
    return card;
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text, const lv_font_t *font,
                            lv_color_t color)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    return lbl;
}

/* A label for multi-line body text.
 *
 * A plain lv_label has no width constraint, so long text runs off the
 * right edge of the card instead of wrapping. Constraining the width to
 * the parent and enabling wrap mode fixes that. Use this for any text
 * that is more than a short caption - not for labels inside flex rows,
 * where a 100% width would squeeze the siblings out. */
static lv_obj_t *make_body_label(lv_obj_t *parent, const char *text,
                                 const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *lbl = make_label(parent, text, font, color);
    lv_obj_set_width(lbl, lv_pct(100));
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    return lbl;
}

/* Common setup for a tab page.
 *
 * Every page is pinned to the content area and allowed to scroll only
 * vertically. Without this a page whose content is wider than the panel
 * (a long label, a wide button row) makes the whole tabview scroll
 * sideways, which is what let part of the neighbouring tab show through. */
static void tab_page_init(lv_obj_t *tab)
{
    lv_obj_set_style_bg_color(tab, COL_BG, 0);
    lv_obj_set_style_pad_all(tab, 10, 0);
    lv_obj_set_style_pad_row(tab, 8, 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(tab, lv_pct(100));
    lv_obj_set_scroll_dir(tab, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(tab, LV_SCROLLBAR_MODE_AUTO);
}

/* A "name ......... STATUS" row. */
static lv_obj_t *make_status_row(lv_obj_t *parent, const char *name)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 2, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollable(row, false);

    make_label(row, name, &lv_font_montserrat_16, COL_TEXT);
    lv_obj_t *val = make_label(row, "--", &lv_font_montserrat_16, COL_UNKNOWN);
    return val;
}

static lv_obj_t *make_button(lv_obj_t *parent, const char *text, lv_event_cb_t cb,
                             void *user_data, lv_color_t color)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_height(btn, 52);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_style_bg_color(btn, color, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);

    lv_obj_t *lbl = make_label(btn, text, &lv_font_montserrat_16, lv_color_white());
    lv_obj_center(lbl);
    return btn;
}

/* ------------------------------------------------------------------ */
/* Tab: Status                                                        */
/* ------------------------------------------------------------------ */

static void build_status_tab(lv_obj_t *tab)
{
    tab_page_init(tab);

    lv_obj_t *card = make_card(tab);
    make_label(card, "ESP32-S3-4848S040", &lv_font_montserrat_20, COL_ACCENT);
    make_label(card, "Hardware self-test", &lv_font_montserrat_14, COL_DIM);

    lv_obj_t *list = make_card(tab);
    static const char *names[8] = {
        "Display (ST7701S)", "Touch (GT911)", "Power (IP5306)",
        "microSD", "Audio (NS4168)", "I2C bus",
        "WiFi", "Bluetooth LE",
    };
    for (int i = 0; i < 8; i++) {
        s_lbl_status[i] = make_status_row(list, names[i]);
    }

    lv_obj_t *i2c_card = make_card(tab);
    make_label(i2c_card, "I2C devices", &lv_font_montserrat_14, COL_DIM);
    s_lbl_i2c = make_body_label(i2c_card, "scanning...", &lv_font_montserrat_14, COL_TEXT);

    lv_obj_t *health_card = make_card(tab);
    s_lbl_health = make_label(health_card, "uptime --", &lv_font_montserrat_14, COL_DIM);
}

/* ------------------------------------------------------------------ */
/* Tab: Display                                                       */
/* ------------------------------------------------------------------ */

static void brightness_slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    board_backlight_set((int)lv_slider_get_value(slider));
}

static void build_display_tab(lv_obj_t *tab)
{
    tab_page_init(tab);

    /* Brightness first: the backlight is the largest load on the 3.3 V
     * rail, so being able to turn it down is the quickest way to stop a
     * marginal supply from collapsing under an audio burst. */
    lv_obj_t *bl_card = make_card(tab);
    make_label(bl_card, "Backlight", &lv_font_montserrat_14, COL_DIM);
    lv_obj_t *bl_row = lv_obj_create(bl_card);
    lv_obj_set_width(bl_row, lv_pct(100));
    lv_obj_set_height(bl_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(bl_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bl_row, 0, 0);
    lv_obj_set_style_pad_all(bl_row, 0, 0);
    lv_obj_set_style_pad_column(bl_row, 10, 0);
    lv_obj_set_flex_flow(bl_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bl_row, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollable(bl_row, false);

    lv_obj_t *bl_slider = lv_slider_create(bl_row);
    lv_obj_set_flex_grow(bl_slider, 1);
    lv_obj_set_width(bl_slider, 0);
    lv_obj_set_height(bl_slider, 20);
    lv_slider_set_range(bl_slider, 0, 100);
    lv_slider_set_value(bl_slider, board_backlight_get(), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bl_slider, COL_ACCENT, LV_PART_INDICATOR);
    lv_obj_add_event_cb(bl_slider, brightness_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
    s_slider_backlight = bl_slider;

    lv_obj_t *card = make_card(tab);
    make_label(card, "Colour bars", &lv_font_montserrat_14, COL_DIM);

    lv_obj_t *bars = lv_obj_create(card);
    lv_obj_set_width(bars, lv_pct(100));
    lv_obj_set_height(bars, 90);
    lv_obj_set_style_border_width(bars, 0, 0);
    lv_obj_set_style_pad_all(bars, 0, 0);
    lv_obj_set_style_radius(bars, 6, 0);
    lv_obj_set_flex_flow(bars, LV_FLEX_FLOW_ROW);
    lv_obj_set_scrollable(bars, false);

    static const uint32_t colors[8] = {
        0xFFFFFF, 0xFFFF00, 0x00FFFF, 0x00FF00,
        0xFF00FF, 0xFF0000, 0x0000FF, 0x000000,
    };
    for (int i = 0; i < 8; i++) {
        lv_obj_t *b = lv_obj_create(bars);
        lv_obj_set_height(b, lv_pct(100));
        lv_obj_set_flex_grow(b, 1);
        lv_obj_set_style_bg_color(b, lv_color_hex(colors[i]), 0);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(b, 0, 0);
        lv_obj_set_style_radius(b, 0, 0);
        lv_obj_set_style_pad_all(b, 0, 0);
    }

    lv_obj_t *grad_card = make_card(tab);
    make_label(grad_card, "Gradient (banding check)", &lv_font_montserrat_14, COL_DIM);
    lv_obj_t *grad = lv_obj_create(grad_card);
    lv_obj_set_width(grad, lv_pct(100));
    lv_obj_set_height(grad, 70);
    lv_obj_set_style_border_width(grad, 0, 0);
    lv_obj_set_style_radius(grad, 6, 0);
    lv_obj_set_style_bg_color(grad, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_grad_color(grad, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_grad_dir(grad, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(grad, LV_OPA_COVER, 0);

    lv_obj_t *text_card = make_card(tab);
    make_label(text_card, "Text sharpness", &lv_font_montserrat_14, COL_DIM);
    make_label(text_card, "The quick brown fox jumps over 13 lazy dogs.",
               &lv_font_montserrat_16, COL_TEXT);
    make_label(text_card, "0123456789  !@#$%^&*()  []{}<>",
               &lv_font_montserrat_14, COL_DIM);
}

/* ------------------------------------------------------------------ */
/* Tab: Touch                                                         */
/* ------------------------------------------------------------------ */

static void canvas_clear(void)
{
    if (!s_canvas) {
        return;
    }
    lv_canvas_fill_bg(s_canvas, lv_color_hex(0x0A0E12), LV_OPA_COVER);
    lv_obj_invalidate(s_canvas);
}

/* The touch driver reports screen coordinates, but the canvas is nested
 * inside a card inside a tab, so its own origin is somewhere else on the
 * display. Convert before drawing or every dot lands in the wrong place. */
static void canvas_dot(int screen_x, int screen_y)
{
    if (!s_canvas) {
        return;
    }

    lv_area_t coords;
    lv_obj_get_coords(s_canvas, &coords);
    int x = screen_x - coords.x1;
    int y = screen_y - coords.y1;

    if (x < 0 || y < 0 || x >= CANVAS_W || y >= CANVAS_H) {
        return;
    }

    lv_layer_t layer;
    lv_canvas_init_layer(s_canvas, &layer);

    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = COL_ACCENT;
    dsc.bg_opa = LV_OPA_COVER;
    dsc.radius = LV_RADIUS_CIRCLE;
    lv_area_t area = {x - 6, y - 6, x + 6, y + 6};
    lv_draw_rect(&layer, &dsc, &area);

    lv_canvas_finish_layer(s_canvas, &layer);
    lv_obj_invalidate(s_canvas);
}

static void clear_btn_cb(lv_event_t *e)
{
    (void)e;
    canvas_clear();
    s_last_x = -1;
    s_last_y = -1;
}

static void build_touch_tab(lv_obj_t *tab)
{
    tab_page_init(tab);

    lv_obj_t *card = make_card(tab);
    make_label(card, "Draw on the panel to test touch", &lv_font_montserrat_14, COL_DIM);

    s_canvas_buf = heap_caps_malloc(CANVAS_W * CANVAS_H * 2, MALLOC_CAP_SPIRAM);
    if (s_canvas_buf) {
        s_canvas = lv_canvas_create(card);
        lv_canvas_set_buffer(s_canvas, s_canvas_buf, CANVAS_W, CANVAS_H,
                             LV_COLOR_FORMAT_RGB565);
        lv_obj_set_style_radius(s_canvas, 6, 0);
        canvas_clear();
    } else {
        make_label(card, "canvas alloc failed", &lv_font_montserrat_14, COL_FAIL);
    }

    lv_obj_t *row = lv_obj_create(card);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 8, 0);
    lv_obj_set_scrollable(row, false);

    s_lbl_touch_coords = make_label(row, "x: --  y: --", &lv_font_montserrat_16, COL_TEXT);
    s_lbl_touch_count = make_label(row, "events: 0", &lv_font_montserrat_14, COL_DIM);
    make_button(row, "Clear", clear_btn_cb, NULL, COL_UNKNOWN);

    lv_obj_t *diag = make_card(tab);
    make_label(diag, "Controller", &lv_font_montserrat_14, COL_DIM);
    s_lbl_touch_diag = make_body_label(diag, "--", &lv_font_montserrat_14, COL_TEXT);
}

/* ------------------------------------------------------------------ */
/* Tab: Power                                                         */
/* ------------------------------------------------------------------ */

static void power_rescan_cb(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "power rescan requested");
    board_i2c_scan();
    hw_power_init();
}

static void build_power_tab(lv_obj_t *tab)
{
    tab_page_init(tab);

    lv_obj_t *card = make_card(tab);
    make_label(card, "IP5306 PMIC", &lv_font_montserrat_20, COL_ACCENT);

    s_lbl_power_state = make_label(card, "state: --", &lv_font_montserrat_16, COL_TEXT);
    s_lbl_power_level = make_label(card, "level: --", &lv_font_montserrat_16, COL_TEXT);

    s_bar_power = lv_bar_create(card);
    lv_obj_set_width(s_bar_power, lv_pct(100));
    lv_obj_set_height(s_bar_power, 22);
    lv_bar_set_range(s_bar_power, 0, 100);
    lv_bar_set_value(s_bar_power, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar_power, COL_CARD, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar_power, COL_PASS, LV_PART_INDICATOR);

    lv_obj_t *reg_card = make_card(tab);
    make_label(reg_card, "Raw registers", &lv_font_montserrat_14, COL_DIM);
    s_lbl_power_regs = make_body_label(reg_card, "--", &lv_font_montserrat_14, COL_TEXT);

    /* Power history is the only power evidence available on units where
     * the IP5306 does not answer on I2C. */
    lv_obj_t *hist_card = make_card(tab);
    make_label(hist_card, "Power history", &lv_font_montserrat_14, COL_DIM);
    s_lbl_power_hist = make_body_label(hist_card, "--", &lv_font_montserrat_14, COL_TEXT);

    lv_obj_t *row = lv_obj_create(tab);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_scrollable(row, false);
    make_button(row, "Rescan I2C", power_rescan_cb, NULL, COL_ACCENT);

    lv_obj_t *note = make_card(tab);
    make_label(note, "Why there is no battery reading", &lv_font_montserrat_14, COL_DIM);
    make_body_label(note,
               "U1 is the plain IP5306 (ESOP-8), not the IP5306-I2C "
               "variant. Its pins are:\n\n"
               "  1 VIN   2 LED1   3 LED2   4 LED3\n"
               "  5 KEY   6 BAT    7 SW     8 VOUT\n\n"
               "There is no SDA and no SCL, so the chip has no I2C "
               "interface at all. It charges and boosts on its own; the "
               "ESP32 cannot read charge state or battery level from it, "
               "and no bodge wire can change that.\n\n"
               "Charge state is shown only by the board's own LEDs. The "
               "history above is the only power telemetry available.",
               &lv_font_montserrat_14, COL_TEXT);
}

/* ------------------------------------------------------------------ */
/* Tab: SD card                                                       */
/* ------------------------------------------------------------------ */

static void sd_remount_cb(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "remount requested");
    hw_sdcard_remount();
}

static void build_sd_tab(lv_obj_t *tab)
{
    tab_page_init(tab);

    lv_obj_t *card = make_card(tab);
    make_label(card, "microSD (SPI2)", &lv_font_montserrat_20, COL_ACCENT);
    s_lbl_sd_info = make_body_label(card, "not mounted", &lv_font_montserrat_14, COL_TEXT);

    lv_obj_t *row = lv_obj_create(tab);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_scrollable(row, false);
    make_button(row, "Remount card", sd_remount_cb, NULL, COL_ACCENT);

    lv_obj_t *note = make_card(tab);
    make_label(note, "Wiring", &lv_font_montserrat_14, COL_DIM);
    make_body_label(note,
               "CS 42   MISO 41   MOSI 47   SCLK 48\n"
               "MOSI/SCLK are shared with the LCD command bus.",
               &lv_font_montserrat_14, COL_TEXT);
}

/* ------------------------------------------------------------------ */
/* Tab: Audio                                                         */
/* ------------------------------------------------------------------ */

static void tone_btn_cb(lv_event_t *e)
{
    int freq = (int)(intptr_t)lv_event_get_user_data(e);
    hw_audio_play_tone(freq, 500);
}

static void square_btn_cb(lv_event_t *e)
{
    int freq = (int)(intptr_t)lv_event_get_user_data(e);
    hw_audio_play_square(freq, 500);
}

static void volume_slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    hw_audio_set_volume((int)lv_slider_get_value(slider));
}

static lv_obj_t *s_bits_btns[3];   /* 16, 24, 32-bit buttons */

/* Repaint the slot-width buttons so the active one is highlighted.
 *
 * The buttons always worked, but nothing showed which width was selected,
 * so pressing them looked like a no-op. The pending value is used when a
 * change is in flight, otherwise the highlight would lag behind the tap by
 * however long the audio task takes to get to it. */
static void bits_highlight(void)
{
    static const int widths[3] = {16, 24, 32};
    int pending = hw_audio_get_pending_bits();
    int cur = pending ? pending : hw_audio_get_bits();
    for (int i = 0; i < 3; i++) {
        if (!s_bits_btns[i]) {
            continue;
        }
        bool on = (widths[i] == cur);
        lv_obj_set_style_bg_color(s_bits_btns[i], on ? COL_ACCENT : COL_CARD, 0);
        lv_obj_set_style_border_width(s_bits_btns[i], on ? 2 : 0, 0);
        lv_obj_set_style_border_color(s_bits_btns[i], COL_TEXT, 0);
    }
}

static void bits_btn_cb(lv_event_t *e)
{
    int bits = (int)(intptr_t)lv_event_get_user_data(e);
    esp_err_t err = hw_audio_set_bits(bits);
    ESP_LOGI(TAG, "slot width request %d-bit -> %s", bits, esp_err_to_name(err));
    /* Do NOT call ui_refresh() here: this runs inside an LVGL event
     * handler, which already holds the (non-recursive) LVGL mutex, so
     * taking it again self-deadlocks the UI. Updating the button styles
     * directly is safe - we are already inside the LVGL context. */
    bits_highlight();
}

static void selftest_btn_cb(lv_event_t *e)
{
    (void)e;
    hw_audio_selftest();
}

static void sweep_btn_cb(lv_event_t *e)
{
    (void)e;
    hw_audio_play_sweep();
}

/* GPIO 1/2/40 are shared between the I2S amp and the relays. Toggling
 * them as plain outputs makes a relay board click audibly, which tells
 * the two variants apart when the speaker stays silent.
 *
 * This deliberately does NOT call gpio_reset_pin(): that clears the GPIO
 * matrix output selector, which is what routes the I2S peripheral onto
 * these pins. Resetting them leaves the amp permanently silent until the
 * routing is restored, so the I2S pins are re-routed afterwards. */
static void relay_click_cb(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "relay click test on GPIO 1/2/40");

    const gpio_num_t pins[3] = {GPIO_NUM_1, GPIO_NUM_2, GPIO_NUM_40};

    /* Detach the I2S peripheral first so the two are not fighting. */
    hw_audio_suspend();

    for (int i = 0; i < 3; i++) {
        gpio_set_direction(pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(pins[i], 0);
    }
    for (int n = 0; n < 3; n++) {
        for (int i = 0; i < 3; i++) {
            gpio_set_level(pins[i], 1);
        }
        vTaskDelay(pdMS_TO_TICKS(120));
        for (int i = 0; i < 3; i++) {
            gpio_set_level(pins[i], 0);
        }
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    /* Hand the pins back to I2S. */
    hw_audio_restore_pins();
    ESP_LOGI(TAG, "relay click test done, I2S routing restored");
}
static void build_audio_tab(lv_obj_t *tab)
{
    tab_page_init(tab);

    lv_obj_t *card = make_card(tab);
    make_label(card, "NS4168 I2S amplifier", &lv_font_montserrat_20, COL_ACCENT);
    make_label(card, "Tap a tone to play it through the speaker.",
               &lv_font_montserrat_14, COL_DIM);

    /* Slot width sits at the top of the tab, not the bottom: it used to be
     * the last row in a card that overflows a 480 px screen, so the
     * buttons were below the fold and could not be tapped at all. */
    lv_obj_t *bits_row = lv_obj_create(card);
    lv_obj_set_width(bits_row, lv_pct(100));
    lv_obj_set_height(bits_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(bits_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bits_row, 0, 0);
    lv_obj_set_style_pad_all(bits_row, 0, 0);
    lv_obj_set_style_pad_column(bits_row, 8, 0);
    lv_obj_set_flex_flow(bits_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_scrollable(bits_row, false);
    make_label(bits_row, "Slot", &lv_font_montserrat_14, COL_DIM);
    s_bits_btns[0] = make_button(bits_row, "16-bit", bits_btn_cb, (void *)(intptr_t)16, COL_CARD);
    s_bits_btns[1] = make_button(bits_row, "24-bit", bits_btn_cb, (void *)(intptr_t)24, COL_CARD);
    s_bits_btns[2] = make_button(bits_row, "32-bit", bits_btn_cb, (void *)(intptr_t)32, COL_CARD);
    bits_highlight();

    lv_obj_t *r1 = lv_obj_create(card);
    lv_obj_set_width(r1, lv_pct(100));
    lv_obj_set_height(r1, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(r1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(r1, 0, 0);
    lv_obj_set_style_pad_all(r1, 0, 0);
    lv_obj_set_style_pad_column(r1, 8, 0);
    lv_obj_set_flex_flow(r1, LV_FLEX_FLOW_ROW);
    lv_obj_set_scrollable(r1, false);
    make_button(r1, "440 Hz", tone_btn_cb, (void *)(intptr_t)440, COL_ACCENT);
    make_button(r1, "880 Hz", tone_btn_cb, (void *)(intptr_t)880, COL_ACCENT);
    make_button(r1, "1.5 kHz", tone_btn_cb, (void *)(intptr_t)1500, COL_ACCENT);

    lv_obj_t *r2 = lv_obj_create(card);
    lv_obj_set_width(r2, lv_pct(100));
    lv_obj_set_height(r2, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(r2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(r2, 0, 0);
    lv_obj_set_style_pad_all(r2, 0, 0);
    lv_obj_set_style_pad_column(r2, 8, 0);
    lv_obj_set_flex_flow(r2, LV_FLEX_FLOW_ROW);
    lv_obj_set_scrollable(r2, false);
    make_button(r2, "Sweep 200 Hz - 3 kHz", sweep_btn_cb, NULL, COL_PASS);
    make_button(r2, "Relay click", relay_click_cb, NULL, COL_UNKNOWN);

    /* Square waves carry ~2.1x the RMS of a sine at the same peak, so they
     * are the fastest way to tell "too quiet" from "not working". */
    lv_obj_t *r3 = lv_obj_create(card);
    lv_obj_set_width(r3, lv_pct(100));
    lv_obj_set_height(r3, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(r3, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(r3, 0, 0);
    lv_obj_set_style_pad_all(r3, 0, 0);
    lv_obj_set_style_pad_column(r3, 8, 0);
    lv_obj_set_flex_flow(r3, LV_FLEX_FLOW_ROW);
    lv_obj_set_scrollable(r3, false);
    make_button(r3, "Square 440 Hz", square_btn_cb, (void *)(intptr_t)440, COL_FAIL);
    make_button(r3, "Square 1 kHz", square_btn_cb, (void *)(intptr_t)1000, COL_FAIL);

    lv_obj_t *vol_row = lv_obj_create(card);
    lv_obj_set_width(vol_row, lv_pct(100));
    lv_obj_set_height(vol_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(vol_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(vol_row, 0, 0);
    lv_obj_set_style_pad_all(vol_row, 0, 0);
    lv_obj_set_style_pad_column(vol_row, 10, 0);
    lv_obj_set_flex_flow(vol_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(vol_row, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollable(vol_row, false);

    /* Fixed-width caption, then the slider takes the rest. Using
     * SPACE_BETWEEN with flex_grow made the slider overflow the card by a
     * few pixels, which is why its right edge sat off screen. */
    lv_obj_t *vol_lbl = make_label(vol_row, "Volume", &lv_font_montserrat_14, COL_DIM);
    lv_obj_set_width(vol_lbl, 70);

    lv_obj_t *slider = lv_slider_create(vol_row);
    lv_obj_set_flex_grow(slider, 1);
    lv_obj_set_width(slider, 0);
    lv_obj_set_height(slider, 20);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, hw_audio_get_volume(), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, COL_ACCENT, LV_PART_INDICATOR);
    lv_obj_add_event_cb(slider, volume_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
    s_slider_volume = slider;

    /* The NS4168 reads samples MSB-first, so the slot width has to match
     * what it expects. The buttons live at the top of this tab. */

    lv_obj_t *st_row = lv_obj_create(card);
    lv_obj_set_width(st_row, lv_pct(100));
    lv_obj_set_height(st_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(st_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(st_row, 0, 0);
    lv_obj_set_style_pad_all(st_row, 0, 0);
    lv_obj_set_flex_flow(st_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_scrollable(st_row, false);
    make_button(st_row, "Auto-test all slot widths", selftest_btn_cb, NULL, COL_PASS);

    lv_obj_t *diag = make_card(tab);
    make_label(diag, "Diagnostics", &lv_font_montserrat_14, COL_DIM);
    s_lbl_audio_diag = make_body_label(diag, "--", &lv_font_montserrat_14, COL_TEXT);

    lv_obj_t *note = make_card(tab);
    make_label(note, "Audio is working", &lv_font_montserrat_14, COL_PASS);
    make_body_label(note,
               "Per the schematic, IO40/IO2/IO1 each fan out to TWO 0-ohm "
               "links:\n\n"
               "  IO40 -> R25 (relay1)  or  R21 (DIN)\n"
               "  IO2  -> R26 (relay2)  or  R22 (LRCLK)\n"
               "  IO1  -> R27 (relay3)  or  R23 (BCLK)\n\n"
               "Only one of each pair is fitted. If the relay side is "
               "fitted the amp inputs float and you hear almost nothing. "
               "16-bit is the default and the widest-tolerance setting.",
               &lv_font_montserrat_14, COL_TEXT);
}

/* ------------------------------------------------------------------ */
/* Tab: Info                                                          */
/* ------------------------------------------------------------------ */

static void build_info_tab(lv_obj_t *tab)
{
    tab_page_init(tab);

    lv_obj_t *card = make_card(tab);
    make_label(card, "Pin map", &lv_font_montserrat_20, COL_ACCENT);
    make_body_label(card,
               "LCD RGB   DE 18  VS 17  HS 16  PCLK 21\n"
               "LCD data  B0-4 4,5,6,7,15  G0-5 8,20,3,46,9,10\n"
               "          R0-4 11,12,13,14,0\n"
               "LCD cmd   CS 39  SCLK 48  MOSI 47\n"
               "Backlight 38 (PWM)\n"
               "Touch     SDA 19  SCL 45  (GT911 @ 0x5D)\n"
               "PMIC      IP5306 @ 0x75\n"
               "microSD   CS 42  MISO 41  MOSI 47  SCLK 48\n"
               "Audio     BCLK 1  LRCLK 2  DOUT 40",
               &lv_font_montserrat_14, COL_TEXT);

    lv_obj_t *card2 = make_card(tab);
    make_label(card2, "Build", &lv_font_montserrat_14, COL_DIM);
    make_label(card2, "ESP-IDF " IDF_VER, &lv_font_montserrat_14, COL_TEXT);
    make_label(card2, "LVGL 9 + esp_lvgl_port", &lv_font_montserrat_14, COL_TEXT);
}

/* ------------------------------------------------------------------ */
/* Periodic refresh                                                   */
/* ------------------------------------------------------------------ */

/* Only touch a label when its text actually changes: lv_label_set_text
 * invalidates the widget even when the string is identical, which would
 * otherwise force a full-screen redraw on every timer tick. */
static void set_label_if_changed(lv_obj_t *lbl, const char *text)
{
    if (!lbl) {
        return;
    }
    const char *cur = lv_label_get_text(lbl);
    if (cur && strcmp(cur, text) == 0) {
        return;
    }
    lv_label_set_text(lbl, text);
}

static void refresh_timer_cb(lv_timer_t *t)
{
    (void)t;

    /* This timer only ever runs on the LVGL task, so the formatting
     * buffers are static rather than ~900 bytes of stack per call. */
    static char buf[256];

    test_status_t st[8] = {
        g_app.results.display,
        g_app.results.touch,
        g_app.results.power,
        g_app.results.sdcard,
        g_app.results.audio,
        g_app.results.i2c,
        g_app.results.wifi,
        g_app.results.ble,
    };
    for (int i = 0; i < 8; i++) {
        if (!s_lbl_status[i]) {
            continue;
        }
        set_label_if_changed(s_lbl_status[i], status_text(st[i]));
        /* Setting a style invalidates the widget even when the value is
         * unchanged, so only touch it when the colour actually differs. */
        lv_color_t want = status_color(st[i]);
        if (!lv_color_eq(lv_obj_get_style_text_color(s_lbl_status[i], 0), want)) {
            lv_obj_set_style_text_color(s_lbl_status[i], want, 0);
        }
    }

    if (s_lbl_i2c) {
        int n = 0;
        n += snprintf(buf + n, sizeof(buf) - n, "%d device(s):", g_app.i2c_found_count);
        for (int i = 0; i < g_app.i2c_found_count && n < (int)sizeof(buf) - 8; i++) {
            n += snprintf(buf + n, sizeof(buf) - n, " %02X", g_app.i2c_found[i]);
        }
        set_label_if_changed(s_lbl_i2c, buf);
    }

    if (s_lbl_power_state) {
        if (!g_app.power.present) {
            snprintf(buf, sizeof(buf), "state: no I2C interface");
        } else if (g_app.power.charge_full) {
            snprintf(buf, sizeof(buf), "state: charged (full)");
        } else if (g_app.power.charging) {
            snprintf(buf, sizeof(buf), "state: charging");
        } else {
            snprintf(buf, sizeof(buf), "state: on battery");
        }
        set_label_if_changed(s_lbl_power_state, buf);
        lv_color_t want = g_app.power.charging ? COL_PASS : COL_TEXT;
        if (!lv_color_eq(lv_obj_get_style_text_color(s_lbl_power_state, 0), want)) {
            lv_obj_set_style_text_color(s_lbl_power_state, want, 0);
        }
    }

    if (s_lbl_power_level) {
        if (g_app.power.level_pct < 0) {
            snprintf(buf, sizeof(buf), "level: unknown");
        } else {
            snprintf(buf, sizeof(buf), "level: %d %%", g_app.power.level_pct);
        }
        set_label_if_changed(s_lbl_power_level, buf);
        if (s_bar_power && g_app.power.level_pct >= 0) {
            lv_bar_set_value(s_bar_power, g_app.power.level_pct, LV_ANIM_ON);
        }
    }

    if (s_lbl_power_regs) {
        snprintf(buf, sizeof(buf),
                 "70=%02X  71=%02X  72=%02X\n77=%02X  78=%02X",
                 g_app.power.reg70, g_app.power.reg71, g_app.power.reg72,
                 g_app.power.reg77, g_app.power.reg78);
        set_label_if_changed(s_lbl_power_regs, buf);
    }

    if (s_lbl_power_hist) {
        static const char *reasons[] = {
            "unknown", "power-on", "external pin", "software", "panic",
            "int watchdog", "task watchdog", "other watchdog",
            "deep sleep", "brownout", "sdio",
        };
        int r = g_app.power.last_reset;
        const char *rs = (r >= 0 && r < (int)(sizeof(reasons) / sizeof(reasons[0])))
                             ? reasons[r] : "unknown";
        snprintf(buf, sizeof(buf),
                 "last reset: %s\nboots: %lu   brownouts: %lu",
                 rs,
                 (unsigned long)g_app.power.boot_count,
                 (unsigned long)g_app.power.brownout_count);
        set_label_if_changed(s_lbl_power_hist, buf);
    }

    if (s_lbl_touch_coords) {
        if (g_app.touch_pressed) {
            snprintf(buf, sizeof(buf), "x: %d  y: %d", g_app.touch_x, g_app.touch_y);
        } else {
            snprintf(buf, sizeof(buf), "x: --  y: --");
        }
        set_label_if_changed(s_lbl_touch_coords, buf);
    }
    if (s_lbl_touch_count) {
        snprintf(buf, sizeof(buf), "events: %lu",
                 (unsigned long)g_app.touch_events);
        set_label_if_changed(s_lbl_touch_count, buf);
    }

    if (s_lbl_touch_diag) {
        if (!g_app.touch_present) {
            snprintf(buf, sizeof(buf),
                     "NOT DETECTED\n"
                     "No GT911 answered at 0x5D or 0x14.\n"
                     "Check the FPC connector is seated.");
        } else if (g_app.touch_events == 0) {
            snprintf(buf, sizeof(buf),
                     "present, ID \"%s\"\n"
                     "polling OK (%lu reads, %lu errors)\n"
                     "No touches yet - tap the canvas.",
                     g_app.touch_product_id,
                     (unsigned long)g_app.touch_polls,
                     (unsigned long)g_app.touch_read_errors);
        } else {
            snprintf(buf, sizeof(buf),
                     "present, ID \"%s\"\n"
                     "%lu touch event(s), %lu reads, %lu errors",
                     g_app.touch_product_id,
                     (unsigned long)g_app.touch_events,
                     (unsigned long)g_app.touch_polls,
                     (unsigned long)g_app.touch_read_errors);
        }
        set_label_if_changed(s_lbl_touch_diag, buf);
    }

    /* Draw a dot while a finger is down. */
    if (g_app.touch_pressed && s_canvas) {
        int x = g_app.touch_x;
        int y = g_app.touch_y;
        if (x != s_last_x || y != s_last_y) {
            canvas_dot(x, y);
            s_last_x = x;
            s_last_y = y;
        }
    } else {
        s_last_x = -1;
        s_last_y = -1;
    }

    if (s_lbl_sd_info) {
        if (g_app.sd.mounted) {
            snprintf(buf, sizeof(buf),
                     "card: %s\n"
                     "capacity: %llu MB\n"
                     "sector: %lu B\n"
                     "files in root: %d\n"
                     "write test: %s",
                     g_app.sd.card_name,
                     (unsigned long long)(g_app.sd.capacity_bytes / (1024 * 1024)),
                     (unsigned long)g_app.sd.sector_size,
                     g_app.sd.file_count,
                     g_app.sd.write_ok ? "OK" : "FAILED");
        } else {
            snprintf(buf, sizeof(buf), "not mounted\n%s", g_app.sd.last_error);
        }
        set_label_if_changed(s_lbl_sd_info, buf);
    }

    if (s_lbl_audio_diag) {
        int pending = hw_audio_get_pending_bits();
        snprintf(buf, sizeof(buf),
                 "I2S %d Hz, stereo %d-bit%s\n"
                 "BCLK %d Hz, vol %d%%\n"
                 "%lu tone(s), %lu bytes to DMA",
                 BSP_I2S_SAMPLE_RATE, hw_audio_get_bits(),
                 pending ? " (switching...)" : "",
                 BSP_I2S_SAMPLE_RATE * hw_audio_get_bits() * 2,
                 hw_audio_get_volume(),
                 (unsigned long)g_app.audio_tones_played,
                 (unsigned long)g_app.audio_bytes_written);
        set_label_if_changed(s_lbl_audio_diag, buf);
        /* Keep the buttons in sync when the width is changed from the
         * serial console rather than by tapping. */
        bits_highlight();
    }

    if (s_lbl_wifi_info) {
        if (g_app.wifi.scanning) {
            snprintf(buf, sizeof(buf), "scanning...");
        } else if (g_app.wifi.scan_count == 0) {
            snprintf(buf, sizeof(buf), "not scanned yet");
        } else {
            int n = snprintf(buf, sizeof(buf), "%d network(s):\n",
                             g_app.wifi.ap_count);
            for (int i = 0; i < g_app.wifi.ap_count && n < (int)sizeof(buf) - 40; i++) {
                n += snprintf(buf + n, sizeof(buf) - n, "  %s  %ddBm  ch%d\n",
                              g_app.wifi.aps[i].ssid[0] ? g_app.wifi.aps[i].ssid : "(hidden)",
                              g_app.wifi.aps[i].rssi, g_app.wifi.aps[i].channel);
            }
        }
        set_label_if_changed(s_lbl_wifi_info, buf);
    }

    if (s_lbl_ble_info) {
        if (g_app.ble.scanning) {
            snprintf(buf, sizeof(buf), "scanning...");
        } else if (g_app.ble.scan_count == 0) {
            snprintf(buf, sizeof(buf), "not scanned yet");
        } else {
            int n = snprintf(buf, sizeof(buf), "%d device(s):\n",
                             g_app.ble.device_count);
            for (int i = 0; i < g_app.ble.device_count && n < (int)sizeof(buf) - 40; i++) {
                n += snprintf(buf + n, sizeof(buf) - n, "  %s  %ddBm\n",
                              g_app.ble.devices[i].name, g_app.ble.devices[i].rssi);
            }
        }
        set_label_if_changed(s_lbl_ble_info, buf);
    }

    /* Persist slider changes once they settle, and keep the sliders in
     * sync when the value is changed from the console. */
    settings_flush();
    if (s_slider_volume &&
        lv_slider_get_value(s_slider_volume) != hw_audio_get_volume()) {
        lv_slider_set_value(s_slider_volume, hw_audio_get_volume(), LV_ANIM_OFF);
    }
    if (s_slider_backlight &&
        lv_slider_get_value(s_slider_backlight) != board_backlight_get()) {
        lv_slider_set_value(s_slider_backlight, board_backlight_get(), LV_ANIM_OFF);
    }

    /* Uptime + heap, so a frozen UI is obvious at a glance. */
    if (s_lbl_health) {
        snprintf(buf, sizeof(buf),
                 "uptime %lus   heap %uK   psram %uK",
                 (unsigned long)(esp_timer_get_time() / 1000000),
                 (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
                 (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
        set_label_if_changed(s_lbl_health, buf);
    }
}

/* ------------------------------------------------------------------ */
/* Tab: Radio (WiFi + BLE)                                            */
/* ------------------------------------------------------------------ */

static void wifi_scan_cb(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "WiFi scan requested");
    hw_wifi_scan();
}

static void ble_scan_cb(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "BLE scan requested");
    hw_ble_scan(3000);
}

static void build_radio_tab(lv_obj_t *tab)
{
    tab_page_init(tab);

    lv_obj_t *wifi_card = make_card(tab);
    make_label(wifi_card, "WiFi", &lv_font_montserrat_20, COL_ACCENT);
    make_body_label(wifi_card,
               "A scan proves the radio works without needing credentials.",
               &lv_font_montserrat_14, COL_DIM);

    lv_obj_t *wrow = lv_obj_create(wifi_card);
    lv_obj_set_width(wrow, lv_pct(100));
    lv_obj_set_height(wrow, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(wrow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wrow, 0, 0);
    lv_obj_set_style_pad_all(wrow, 0, 0);
    lv_obj_set_flex_flow(wrow, LV_FLEX_FLOW_ROW);
    lv_obj_set_scrollable(wrow, false);
    make_button(wrow, "Scan WiFi", wifi_scan_cb, NULL, COL_ACCENT);

    s_lbl_wifi_info = make_body_label(wifi_card, "--", &lv_font_montserrat_14, COL_TEXT);

    lv_obj_t *ble_card = make_card(tab);
    make_label(ble_card, "Bluetooth LE", &lv_font_montserrat_20, COL_ACCENT);
    make_body_label(ble_card,
               "NimBLE. Advertises as \"4848S040-test\" and scans for "
               "nearby devices.",
               &lv_font_montserrat_14, COL_DIM);

    lv_obj_t *brow = lv_obj_create(ble_card);
    lv_obj_set_width(brow, lv_pct(100));
    lv_obj_set_height(brow, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(brow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(brow, 0, 0);
    lv_obj_set_style_pad_all(brow, 0, 0);
    lv_obj_set_flex_flow(brow, LV_FLEX_FLOW_ROW);
    lv_obj_set_scrollable(brow, false);
    make_button(brow, "Scan BLE", ble_scan_cb, NULL, COL_ACCENT);

    s_lbl_ble_info = make_body_label(ble_card, "--", &lv_font_montserrat_14, COL_TEXT);

    lv_obj_t *note = make_card(tab);
    make_label(note, "Power note", &lv_font_montserrat_14, COL_DIM);
    make_body_label(note,
               "The radios are the largest current consumers after the "
               "backlight. They are initialised but left idle, and the "
               "interface is stopped again after each scan, because this "
               "board has no bulk capacitance on its 5 V rail to absorb "
               "a radio burst.",
               &lv_font_montserrat_14, COL_TEXT);
}

/* ------------------------------------------------------------------ */

void ui_init(lv_display_t *disp)
{
    if (!ui_lock(1000)) {
        ESP_LOGE(TAG, "could not lock LVGL");
        return;
    }

    lv_obj_t *scr = lv_display_get_screen_active(disp);
    lv_obj_set_style_bg_color(scr, COL_BG, 0);
    lv_obj_set_style_text_color(scr, COL_TEXT, 0);

    lv_obj_t *tv = lv_tabview_create(scr);
    /* Pin the tabview to the screen. Without an explicit size it can grow
     * to fit its tab bar, and with eight tabs that bar is wider than the
     * 480 px panel - which pushed the content area sideways so part of the
     * neighbouring tab stayed visible. */
    lv_obj_set_size(tv, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(tv, 0, 0);
    lv_tabview_set_tab_bar_size(tv, 46);
    lv_obj_set_style_bg_color(tv, COL_BG, 0);
    lv_obj_set_style_bg_color(tv, COL_CARD, LV_PART_MAIN);

    /* The tab bar itself must scroll rather than stretch the tabview. */
    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(tv);
    if (tab_bar) {
        lv_obj_set_width(tab_bar, lv_pct(100));
        lv_obj_set_scroll_dir(tab_bar, LV_DIR_HOR);
        lv_obj_set_scrollbar_mode(tab_bar, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_pad_all(tab_bar, 0, 0);
        lv_obj_set_style_pad_column(tab_bar, 0, 0);
    }

    build_status_tab(lv_tabview_add_tab(tv, "Status"));
    build_display_tab(lv_tabview_add_tab(tv, "Display"));
    build_touch_tab(lv_tabview_add_tab(tv, "Touch"));
    build_power_tab(lv_tabview_add_tab(tv, "Power"));
    build_sd_tab(lv_tabview_add_tab(tv, "SD"));
    build_audio_tab(lv_tabview_add_tab(tv, "Audio"));
    build_radio_tab(lv_tabview_add_tab(tv, "Radio"));
    build_info_tab(lv_tabview_add_tab(tv, "Info"));

    /* Eight tabs across 480 px is 60 px each. The default tab button
     * padding makes them wider than that, so shrink the text and padding
     * to fit them all without horizontal scrolling. */
    if (tab_bar) {
        uint32_t n = lv_obj_get_child_count(tab_bar);
        for (uint32_t i = 0; i < n; i++) {
            lv_obj_t *btn = lv_obj_get_child(tab_bar, i);
            if (!btn) {
                continue;
            }
            lv_obj_set_style_pad_left(btn, 2, 0);
            lv_obj_set_style_pad_right(btn, 2, 0);
            lv_obj_set_style_pad_top(btn, 4, 0);
            lv_obj_set_style_pad_bottom(btn, 4, 0);
            lv_obj_set_style_text_font(btn, &lv_font_montserrat_14, 0);
        }
    }

    lv_timer_create(refresh_timer_cb, 200, NULL);

    ui_unlock();
    ESP_LOGI(TAG, "UI built");
}

void ui_refresh(void)
{
    if (ui_lock(100)) {
        refresh_timer_cb(NULL);
        ui_unlock();
    }
}
