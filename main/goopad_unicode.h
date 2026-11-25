#pragma once

#include "lvgl.h"

typedef struct {
    bool available;
    uint32_t glyphs;
    uint32_t cache_bytes;
    uint32_t cache_peak;
    uint32_t codepoint_index_bytes;
    uint32_t metrics_cache_bytes;
    uint32_t hits;
    uint32_t reads;
} goopad_unicode_status_t;

const lv_font_t *goopad_unicode_font(unsigned size, const lv_font_t *offline);
void goopad_unicode_status(goopad_unicode_status_t *status);

typedef bool (*goopad_font_bitmap_evictor_t)(const void *exclude, uint32_t target_stamp,
                                             uint32_t *stamp, uint32_t *bytes);
void goopad_font_bitmap_register_evictor(goopad_font_bitmap_evictor_t evictor);
bool goopad_font_bitmap_reserve(uint32_t bytes, const void *exclude);
void goopad_font_bitmap_release(uint32_t bytes);
uint32_t goopad_font_bitmap_cache_used(void);
uint32_t goopad_font_bitmap_cache_stamp(void);
