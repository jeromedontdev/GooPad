#pragma once

#include "lvgl.h"

typedef struct {
    bool available;
    uint32_t fonts;
    uint32_t glyphs;
    uint32_t indexed_metadata_bytes;
    uint32_t descriptor_bytes;
    uint32_t metadata_cache_bytes;
    uint32_t kerning_cache_bytes;
    uint32_t kerning_pairs;
    uint32_t bitmap_cache_bytes;
    uint32_t bitmap_cache_peak;
    uint32_t shared_bitmap_cache_bytes;
    uint32_t bitmap_cache_budget;
    uint32_t hits;
    uint32_t reads;
} goopad_styled_status_t;

const lv_font_t *goopad_styled_font(unsigned family, unsigned variant, const lv_font_t *fallback);
void goopad_styled_status(goopad_styled_status_t *status);
