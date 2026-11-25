#pragma once

#include "lvgl.h"

int goopad_font_index(const char *name);
unsigned goopad_font_variants(unsigned family);
const lv_font_t *goopad_font_variant(unsigned family, unsigned variant, unsigned *size);
