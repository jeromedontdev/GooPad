#include "goopad_glass.h"

static uint16_t rgb565(unsigned r, unsigned g, unsigned b)
{
    return (uint16_t)((r >> 3) << 11 | (g >> 2) << 5 | b >> 3);
}

void goopad_glass_fill(uint16_t *pixels, const uint16_t *background, uint32_t color, bool pastel)
{
    static const int blobs[][6] = {{26, 30, 55, 119, 159, 236}, {104, 82, 70, 182, 141, 226},
                                   {26, 108, 58, 114, 214, 222}};
    for (int y = 0; y < GOOPAD_GLASS_SIDE; y++) for (int x = 0; x < GOOPAD_GLASS_SIDE; x++) {
        uint16_t pixel;
        if (background) {
            unsigned r = 0, g = 0, b = 0;
            for (int dy = 0; dy < 4; dy++) for (int dx = 0; dx < 4; dx++) {
                uint16_t source = background[(y * 4 + dy) * 480 + x * 4 + dx];
                r += source >> 11; g += (source >> 5) & 63; b += source & 31;
            }
            pixel = (uint16_t)((r / 16) << 11 | (g / 16) << 5 | b / 16);
        } else if (pastel) {
            float r = 221, g = 235, b = 246;
            for (unsigned i = 0; i < sizeof(blobs) / sizeof(blobs[0]); i++) {
                int dx = x - blobs[i][0], dy = y - blobs[i][1], radius = blobs[i][2];
                float weight = 1.0f - (float)(dx * dx + dy * dy) / (radius * radius);
                if (weight <= 0) continue;
                weight *= weight;
                r += (blobs[i][3] - r) * weight;
                g += (blobs[i][4] - g) * weight;
                b += (blobs[i][5] - b) * weight;
            }
            pixel = rgb565((unsigned)r, (unsigned)g, (unsigned)b);
        } else pixel = rgb565(color >> 16, (color >> 8) & 255, color & 255);
        pixels[y * GOOPAD_GLASS_SIDE + x] = pixel;
    }
}

void goopad_glass_blur(uint16_t *pixels, uint16_t *scratch)
{
    for (int vertical = 0; vertical < 2; vertical++) {
        const uint16_t *source = vertical ? scratch : pixels;
        uint16_t *target = vertical ? pixels : scratch;
        for (int y = 0; y < GOOPAD_GLASS_SIDE; y++) for (int x = 0; x < GOOPAD_GLASS_SIDE; x++) {
            unsigned r = 0, g = 0, b = 0;
            for (int offset = -3; offset <= 3; offset++) {
                int position = (vertical ? y : x) + offset;
                if (position < 0) position = 0;
                if (position >= GOOPAD_GLASS_SIDE) position = GOOPAD_GLASS_SIDE - 1;
                uint16_t pixel = source[(vertical ? position : y) * GOOPAD_GLASS_SIDE + (vertical ? x : position)];
                r += pixel >> 11; g += (pixel >> 5) & 63; b += pixel & 31;
            }
            target[y * GOOPAD_GLASS_SIDE + x] = (uint16_t)((r / 7) << 11 | (g / 7) << 5 | b / 7);
        }
    }
}
