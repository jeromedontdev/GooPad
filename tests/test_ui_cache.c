#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_CACHE_SLOTS 3
#define PAGE_CACHE_RESERVE 1024
#define MALLOC_CAP_SPIRAM 1
#define MALLOC_CAP_8BIT 2
#define LV_IMAGE_HEADER_MAGIC 1
#define LV_COLOR_FORMAT_RGB565 2
#define ESP_LOGW(...) ((void)0)
typedef struct { int x1, y1, x2, y2; } lv_area_t;
typedef struct {
    struct { int magic, cf, w, h, stride; } header;
    size_t data_size;
    const uint8_t *data;
} lv_image_dsc_t;
typedef struct {
    uint8_t *pixels;
    lv_image_dsc_t image;
    int page;
    bool valid;
    int64_t used;
} page_cache_t;
static page_cache_t s_page_cache[PAGE_CACHE_SLOTS];
static bool s_page_cache_enabled = true, s_page_scrolling;
static void *s_pages = (void *)1, *s_dots = (void *)1, *s_info_overlay;
static int s_page_width = 8, s_page_height = 4, s_page_top = 1, s_page_count = 2;
static struct { bool touch_pressed; } g_app;
static int scroll_x;
static size_t copied_bytes;
static const char *TAG = "test";
static int lv_obj_get_scroll_x(void *object) { (void)object; return scroll_x; }
static int64_t esp_timer_get_time(void) { return 1; }
static size_t heap_caps_get_free_size(int caps) { (void)caps; return 8192; }
static void *heap_caps_malloc(size_t bytes, int caps) { (void)caps; return malloc(bytes); }
static void lv_image_cache_drop(void *image) { (void)image; }
static void *tracked_memcpy(void *target, const void *source, size_t bytes)
{ copied_bytes += bytes; return memcpy(target, source, bytes); }
#define memcpy tracked_memcpy
#include "capture.inc"
#undef memcpy
typedef struct { int unused; } lv_timer_t;
static bool s_menu_animating, s_grid_shown = true, s_loading_shown;
static char s_config_buffer[32], s_rendered_config[32];
static unsigned config_reads, widget_updates, grid_builds;
static bool goopad_wifi_connected(void) { return true; }
static bool goopad_wifi_connecting(void) { return false; }
static void goopad_get_config(char *out, size_t capacity)
{ (void)capacity; config_reads++; strcpy(out, "{}"); }
static void update_widgets(void) { widget_updates++; }
static void build_grid_screen(void) { grid_builds++; }
static void build_loading_screen(void) { assert(!"unexpected loading screen"); }
static void build_setup_screen(void) { assert(!"unexpected setup screen"); }
#include "refresh.inc"

int main(void)
{
    uint8_t frame[8 * 6 * 2], expected[8 * 4 * 2];
    for (unsigned i = 0; i < sizeof(frame); i++) frame[i] = (uint8_t)i;
    lv_area_t full = {0, 0, 7, 5};
    goopad_ui_capture_frame(frame, &full, true);
    page_cache_t *slot = NULL;
    for (unsigned i = 0; i < PAGE_CACHE_SLOTS; i++)
        if (s_page_cache[i].pixels) slot = &s_page_cache[i];
    assert(slot);
    assert(slot->valid && copied_bytes == sizeof(expected));
    memcpy(expected, frame + 16, sizeof(expected));
    assert(!memcmp(expected, slot->pixels, sizeof(expected)));

    slot->valid = false;
    copied_bytes = 0;
    frame[2 * 16 + 3 * 2] = 201;
    expected[1 * 16 + 3 * 2] = 201;
    lv_area_t first = {3, 2, 3, 2};
    goopad_ui_capture_frame(frame, &first, false);
    assert(copied_bytes == 2 && !slot->valid);
    assert(!memcmp(expected, slot->pixels, sizeof(expected)));

    frame[3 * 16 + 2 * 2] = 202;
    expected[2 * 16 + 2 * 2] = 202;
    lv_area_t last = {2, 3, 2, 3};
    goopad_ui_capture_frame(frame, &last, true);
    assert(copied_bytes == 4 && slot->valid);
    assert(!memcmp(expected, slot->pixels, sizeof(expected)));

    scroll_x = 8;
    copied_bytes = 0;
    goopad_ui_capture_frame(frame, &first, false);
    assert(copied_bytes == 0);
    goopad_ui_capture_frame(frame, &last, true);
    assert(copied_bytes == sizeof(expected));
    page_cache_t *second = NULL;
    for (unsigned i = 0; i < PAGE_CACHE_SLOTS; i++)
        if (s_page_cache[i].pixels && s_page_cache[i].page == 1) second = &s_page_cache[i];
    assert(second && second->valid);
    assert(!memcmp(frame + 16, second->pixels, sizeof(expected)));
    for (unsigned i = 0; i < PAGE_CACHE_SLOTS; i++) free(s_page_cache[i].pixels);
    strcpy(s_rendered_config, "{}");
    s_page_scrolling = true;
    refresh_cb(NULL);
    assert(config_reads == 0 && widget_updates == 0);
    s_page_scrolling = false;
    s_menu_animating = true;
    refresh_cb(NULL);
    assert(config_reads == 0 && widget_updates == 0);
    s_menu_animating = false;
    refresh_cb(NULL);
    assert(config_reads == 1 && widget_updates == 1 && grid_builds == 0);
    puts("Actual page capture: dirty patches, final-flush validity and new-page full copies passed");
    puts("Actual refresh: blocking configuration/widget reads deferred during page/menu motion");
}
