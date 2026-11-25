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
