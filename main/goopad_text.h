#pragma once
#include <stdint.h>
#include <string.h>

static inline unsigned goopad_text_extra(unsigned height, unsigned emphasis)
{
    return (emphasis & 1 ? 1 : 0) + (emphasis & 2 && height ? (height - 1) / 4 : 0);
}

/* Expand decoded A8 rows bottom-up so source and destination can share a buffer. */
static inline void goopad_text_emphasize(uint8_t *pixels, unsigned width, unsigned height,
                                        unsigned source_stride, unsigned stride, unsigned emphasis)
{
    unsigned extra = goopad_text_extra(height, emphasis);
    for (unsigned row = height; row > 0; row--) {
        unsigned y = row - 1, shift = emphasis & 2 ? (height - 1 - y) / 4 : 0;
        uint8_t *out = pixels + y * stride;
        memmove(out + shift, pixels + y * source_stride, width);
        memset(out, 0, shift);
        memset(out + shift + width, 0, stride - shift - width);
        if (emphasis & 1)
            for (unsigned x = width + extra; x > 1; x--)
                if (out[x - 2] > out[x - 1]) out[x - 1] = out[x - 2];
    }
}
