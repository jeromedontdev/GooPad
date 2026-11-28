#pragma once
#include <stdbool.h>
#include <stddef.h>

static inline bool goopad_utf8_valid(const char *text, size_t size)
{
    for (size_t i = 0; i < size;) {
        unsigned char lead = (unsigned char)text[i++];
        if (lead < 0x80) continue;
        unsigned bytes = lead >= 0xc2 && lead <= 0xdf ? 1 :
                         lead >= 0xe0 && lead <= 0xef ? 2 :
                         lead >= 0xf0 && lead <= 0xf4 ? 3 : 0;
        if (!bytes || bytes > size - i) return false;
        unsigned char second = (unsigned char)text[i];
        if ((lead == 0xe0 && second < 0xa0) || (lead == 0xed && second >= 0xa0) ||
            (lead == 0xf0 && second < 0x90) || (lead == 0xf4 && second >= 0x90)) return false;
        while (bytes--) if (((unsigned char)text[i++] & 0xc0) != 0x80) return false;
    }
    return true;
}
