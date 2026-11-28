#pragma once

#include <stdbool.h>
#include <stdint.h>

#define GOOPAD_GLASS_SIDE 120

void goopad_glass_fill(uint16_t *pixels, const uint16_t *background, uint32_t color, bool pastel);
void goopad_glass_blur(uint16_t *pixels, uint16_t *scratch);
