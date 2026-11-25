/*******************************************************************************
 * Size: 14 px
 * Bpp: 1
 * Lato essential offline fallback; see sources/Lato-Regular.ttf and licenses/lato.txt.
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef GOOPAD_FONT_ESSENTIAL_14
#define GOOPAD_FONT_ESSENTIAL_14 1
#endif

#if GOOPAD_FONT_ESSENTIAL_14

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0xc2, 0xaa, 0x80,

    /* U+00A2 "¢" */
    0x10, 0x47, 0xb4, 0x92, 0x49, 0x15, 0x3c, 0x80,

    /* U+00A3 "£" */
    0x3c, 0xc9, 0x2, 0x4, 0x1f, 0x10, 0x20, 0x41,
    0xfc,

    /* U+00A4 "¤" */
    0x0, 0xf9, 0x12, 0x24, 0x4f, 0x80, 0x0,

    /* U+00A5 "¥" */
    0x82, 0x89, 0x11, 0x42, 0x82, 0x1f, 0x3e, 0x10,
    0x20,

    /* U+00A6 "¦" */
    0xf8, 0xf8,

    /* U+00A7 "§" */
    0x7c, 0x20, 0xc9, 0xc5, 0xa2, 0xc, 0x5c,

    /* U+00A8 "¨" */
    0xb0,

    /* U+00A9 "©" */
    0x1e, 0x18, 0x65, 0xea, 0xc1, 0xa0, 0x68, 0x1b,
    0x25, 0x7a, 0x61, 0x87, 0x80,

    /* U+00AA "ª" */
    0x71, 0x79, 0x70,

    /* U+00AB "«" */
    0x55, 0xa5, 0x50,

    /* U+00AC "¬" */
    0xfc, 0x10, 0x40,

    /* U+00AD "­" */
    0xe0,

    /* U+00AE "®" */
    0x1e, 0x18, 0x65, 0xea, 0x49, 0x92, 0x67, 0x19,
    0x45, 0x4a, 0x61, 0x87, 0x80,

    /* U+00AF "¯" */
    0xf0,

    /* U+00B0 "°" */
    0x74, 0x62, 0xe0,

    /* U+00B1 "±" */
    0x10, 0x20, 0x47, 0xf1, 0x2, 0x0, 0x7f,

    /* U+00B2 "²" */
    0x71, 0x24, 0xf0,

    /* U+00B3 "³" */
    0x75, 0x21, 0x70,

    /* U+00B4 "´" */
    0x50,

    /* U+00B5 "µ" */
    0x85, 0xa, 0x14, 0x28, 0x51, 0xbc, 0xc0, 0x80,

    /* U+00B6 "¶" */
    0x7f, 0xf9, 0x3c, 0x9e, 0x47, 0x20, 0x90, 0x48,
    0x24, 0x12, 0x9, 0x4, 0x80,

    /* U+00B7 "·" */
    0xf0,

    /* U+00B8 "¸" */
    0x47, 0x80,

    /* U+00B9 "¹" */
    0xc9, 0x2e,

    /* U+00BA "º" */
    0x69, 0x99, 0x60,

    /* U+00BB "»" */
    0xaa, 0x5a, 0xa0,

    /* U+00BC "¼" */
    0x41, 0x61, 0x11, 0x8, 0x84, 0x87, 0x40, 0x4c,
    0x4a, 0x27, 0xa0, 0x80,

    /* U+00BD "½" */
    0xc2, 0x44, 0x44, 0x48, 0xf0, 0x17, 0x25, 0x21,
    0x46, 0x47,

    /* U+00BE "¾" */
    0x71, 0x14, 0x42, 0x20, 0x48, 0x94, 0x1a, 0x0,
    0x98, 0x4a, 0x17, 0xc8, 0x20,

    /* U+00BF "¿" */
    0x20, 0x0, 0x46, 0x62, 0x11, 0x70,

    /* U+00C0 "À" */
    0x30, 0xc, 0x1, 0x1, 0x81, 0xc0, 0xa0, 0x48,
    0x44, 0x23, 0x3f, 0x90, 0x48, 0x1c, 0x8,

    /* U+00C1 "Á" */
    0x2, 0x2, 0x2, 0x1, 0x81, 0xc0, 0xa0, 0x48,
    0x44, 0x23, 0x3f, 0x90, 0x48, 0x1c, 0x8,

    /* U+00C2 "Â" */
    0x8, 0xa, 0x0, 0x81, 0x81, 0xc0, 0xa0, 0x48,
    0x44, 0x23, 0x3f, 0x90, 0x48, 0x1c, 0x8,

    /* U+00C3 "Ã" */
    0x1a, 0x3, 0x3, 0x3, 0x81, 0x40, 0x90, 0x88,
    0x46, 0x7f, 0x20, 0x90, 0x38, 0x10,

    /* U+00C4 "Ä" */
    0x36, 0x0, 0x3, 0x3, 0x81, 0x40, 0x90, 0x88,
    0x46, 0x7f, 0x20, 0x90, 0x38, 0x10,

    /* U+00C5 "Å" */
    0xc, 0xa, 0x3, 0x1, 0x81, 0xc0, 0xa0, 0x48,
    0x44, 0x23, 0x3f, 0x90, 0x48, 0x1c, 0x8,

    /* U+00C6 "Æ" */
    0x7, 0xf8, 0x38, 0x3, 0x40, 0x12, 0x1, 0x90,
    0x8, 0xf8, 0xfc, 0x4, 0x20, 0x61, 0x82, 0xf,
    0xc0,

    /* U+00C7 "Ç" */
    0x1e, 0x61, 0x40, 0x80, 0x80, 0x80, 0x80, 0xc0,
    0x61, 0x3e, 0x8, 0x1c,

    /* U+00C8 "È" */
    0xc0, 0x81, 0x3f, 0x82, 0x8, 0x20, 0xfa, 0x8,
    0x20, 0xfc,

    /* U+00C9 "É" */
    0x8, 0x42, 0x3f, 0x82, 0x8, 0x20, 0xfa, 0x8,
    0x20, 0xfc,

    /* U+00CA "Ê" */
    0x21, 0xc4, 0xbf, 0x82, 0x8, 0x20, 0xfa, 0x8,
    0x20, 0xfc,

    /* U+00CB "Ë" */
    0x6c, 0xf, 0xe0, 0x82, 0x8, 0x3e, 0x82, 0x8,
    0x3f,

    /* U+00CC "Ì" */
    0x88, 0x92, 0x49, 0x24, 0x92,

    /* U+00CD "Í" */
    0x2a, 0x49, 0x24, 0x92, 0x48,

    /* U+00CE "Î" */
    0x22, 0xa2, 0x42, 0x10, 0x84, 0x21, 0x8, 0x42,
    0x0,

    /* U+00CF "Ï" */
    0x90, 0x44, 0x44, 0x44, 0x44, 0x44,

    /* U+00D0 "Ð" */
    0x7e, 0x10, 0x64, 0x9, 0x1, 0x40, 0x7c, 0x14,
    0x5, 0x2, 0x41, 0x9f, 0x80,

    /* U+00D1 "Ñ" */
    0x30, 0x2c, 0x81, 0xc1, 0xe1, 0xb1, 0x99, 0x99,
    0x8d, 0x87, 0x83, 0x81,

    /* U+00D2 "Ò" */
    0x30, 0x6, 0x0, 0x40, 0x78, 0x61, 0x90, 0x28,
    0x6, 0x1, 0x80, 0x60, 0x14, 0x9, 0x86, 0x1e,
    0x0,

    /* U+00D3 "Ó" */
    0x3, 0x1, 0x80, 0x80, 0x78, 0x61, 0x90, 0x28,
    0x6, 0x1, 0x80, 0x60, 0x14, 0x9, 0x86, 0x1e,
    0x0,

    /* U+00D4 "Ô" */
    0xc, 0x7, 0x81, 0x30, 0x78, 0x61, 0x90, 0x28,
    0x6, 0x1, 0x80, 0x60, 0x14, 0x9, 0x86, 0x1e,
    0x0,

    /* U+00D5 "Õ" */
    0x1e, 0x0, 0x1, 0xe1, 0x86, 0x40, 0xa0, 0x18,
    0x6, 0x1, 0x80, 0x50, 0x26, 0x18, 0x78,

    /* U+00D6 "Ö" */
    0x36, 0x0, 0x1, 0xe1, 0x86, 0x40, 0xa0, 0x18,
    0x6, 0x1, 0x80, 0x50, 0x26, 0x18, 0x78,

    /* U+00D7 "×" */
    0x7, 0x37, 0x8c, 0x7b, 0x10, 0x0,

    /* U+00D8 "Ø" */
    0x1f, 0x98, 0x64, 0x3a, 0x19, 0x84, 0x63, 0x19,
    0x87, 0xc2, 0x61, 0x9f, 0x88, 0x0,

    /* U+00D9 "Ù" */
    0x60, 0x30, 0x8, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0x42, 0x3c,

    /* U+00DA "Ú" */
    0x6, 0xc, 0x10, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0x42, 0x3c,

    /* U+00DB "Û" */
    0x18, 0x3c, 0x46, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0x42, 0x3c,

    /* U+00DC "Ü" */
    0x24, 0x0, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x42, 0x3c,

    /* U+00DD "Ý" */
    0x2, 0x2, 0x2, 0x8, 0x26, 0x31, 0x10, 0xd8,
    0x28, 0x8, 0x4, 0x2, 0x1, 0x0, 0x80,

    /* U+00DE "Þ" */
    0x81, 0x3, 0xe4, 0x38, 0x30, 0x61, 0xfc, 0x81,
    0x0,

    /* U+00DF "ß" */
    0x79, 0x8a, 0x14, 0x49, 0x13, 0x23, 0x41, 0xa3,
    0x78,

    /* U+00E0 "à" */
    0x30, 0x40, 0xe, 0x44, 0x13, 0xd1, 0x45, 0xf0,

    /* U+00E1 "á" */
    0x8, 0x40, 0xe, 0x44, 0x13, 0xd1, 0x45, 0xf0,

    /* U+00E2 "â" */
    0x18, 0xa0, 0xe, 0x44, 0x13, 0xd1, 0x45, 0xf0,

    /* U+00E3 "ã" */
    0x35, 0x60, 0xe, 0x44, 0x13, 0xd1, 0x45, 0xf0,

    /* U+00E4 "ä" */
    0x6c, 0x0, 0xe, 0x44, 0x13, 0xd1, 0x45, 0xf0,

    /* U+00E5 "å" */
    0x38, 0xa3, 0x8e, 0x44, 0x13, 0xd1, 0x45, 0xf0,

    /* U+00E6 "æ" */
    0x77, 0xa3, 0x30, 0x85, 0xff, 0x88, 0x25, 0x17,
    0x78,

    /* U+00E7 "ç" */
    0x7b, 0x8, 0x20, 0x81, 0x33, 0x4, 0x30,

    /* U+00E8 "è" */
    0x20, 0x20, 0x1, 0xcc, 0x50, 0xbf, 0xc0, 0xc0,
    0xf0,

    /* U+00E9 "é" */
    0x18, 0x20, 0x1, 0xcc, 0x50, 0xbf, 0xc0, 0xc0,
    0xf0,

    /* U+00EA "ê" */
    0x30, 0xd0, 0x1, 0xcc, 0x50, 0xbf, 0xc0, 0xc0,
    0xf0,

    /* U+00EB "ë" */
    0x58, 0x0, 0x1, 0xcc, 0x50, 0xbf, 0xc0, 0xc0,
    0xf0,

    /* U+00EC "ì" */
    0x91, 0x55, 0x50,

    /* U+00ED "í" */
    0x62, 0xaa, 0xa0,

    /* U+00EE "î" */
    0x4a, 0x4, 0x44, 0x44, 0x44,

    /* U+00EF "ï" */
    0xb0, 0x4, 0x44, 0x44, 0x44,

    /* U+00F0 "ð" */
    0x7c, 0x38, 0x1b, 0xdc, 0x70, 0x60, 0xe2, 0x78,

    /* U+00F1 "ñ" */
    0x69, 0x60, 0x2e, 0xc6, 0x18, 0x61, 0x86, 0x10,

    /* U+00F2 "ò" */
    0x30, 0x20, 0x1, 0xcc, 0x50, 0x60, 0xc1, 0x44,
    0xf0,

    /* U+00F3 "ó" */
    0x8, 0x20, 0x1, 0xcc, 0x50, 0x60, 0xc1, 0x44,
    0xf0,

    /* U+00F4 "ô" */
    0x38, 0xd8, 0x1, 0xcc, 0x50, 0x60, 0xc1, 0x44,
    0xf0,

    /* U+00F5 "õ" */
    0x34, 0xb8, 0x1, 0xcc, 0x50, 0x60, 0xc1, 0x44,
    0xf0,

    /* U+00F6 "ö" */
    0x48, 0x0, 0x1, 0xcc, 0x50, 0x60, 0xc1, 0x44,
    0xf0,

    /* U+00F7 "÷" */
    0x30, 0xf, 0xc0, 0x0, 0xc0,

    /* U+00F8 "ø" */
    0x0, 0x7f, 0x3c, 0x59, 0x36, 0x79, 0x7c, 0x0,

    /* U+00F9 "ù" */
    0x60, 0xc0, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,

    /* U+00FA "ú" */
    0x18, 0xc0, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,

    /* U+00FB "û" */
    0x31, 0x20, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,

    /* U+00FC "ü" */
    0x58, 0x0, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,

    /* U+00FD "ý" */
    0x8, 0x20, 0x6, 0x14, 0x4c, 0x8b, 0x1c, 0x18,
    0x20, 0x41, 0x0,

    /* U+00FE "þ" */
    0x82, 0x8, 0x2e, 0xce, 0x18, 0x61, 0x8f, 0xe8,
    0x20,

    /* U+00FF "ÿ" */
    0x2c, 0x0, 0x6, 0x14, 0x4c, 0x8b, 0x1c, 0x18,
    0x20, 0x41, 0x0,

    /* U+0100 "Ā" */
    0x1e, 0x0, 0x3, 0x3, 0x81, 0x40, 0x90, 0x88,
    0x46, 0x7f, 0x20, 0x90, 0x38, 0x10,

    /* U+0101 "ā" */
    0x7c, 0x3, 0x91, 0x4, 0xf4, 0x51, 0x7c,

    /* U+0102 "Ă" */
    0x12, 0x4, 0x80, 0xc0, 0x30, 0xc, 0x7, 0x81,
    0x20, 0x48, 0x21, 0xf, 0xc6, 0x19, 0x2, 0x40,
    0x80,

    /* U+0103 "ă" */
    0x28, 0xe0, 0x1e, 0x4, 0x13, 0xd1, 0x45, 0xf0,

    /* U+0104 "Ą" */
    0x18, 0x7, 0x1, 0x40, 0x90, 0x22, 0x18, 0x87,
    0xf1, 0x4, 0xc1, 0x20, 0x60, 0x10, 0x7,

    /* U+0105 "ą" */
    0x38, 0x88, 0x11, 0xe4, 0x48, 0x9f, 0x4, 0x6,

    /* U+0106 "Ć" */
    0x4, 0x8, 0x1e, 0x63, 0x40, 0x80, 0x80, 0x80,
    0x80, 0x40, 0x63, 0x3e,

    /* U+0107 "ć" */
    0x18, 0x80, 0xfc, 0x42, 0x10, 0xcb, 0xc0,

    /* U+0108 "Ĉ" */
    0x8, 0x14, 0x22, 0x1e, 0x63, 0x40, 0x80, 0x80,
    0x80, 0x80, 0x40, 0x63, 0x3e,

    /* U+0109 "ĉ" */
    0x32, 0x40, 0xfc, 0x42, 0x10, 0xc3, 0xc0,

    /* U+010A "Ċ" */
    0xc, 0x0, 0x1e, 0x63, 0x40, 0x80, 0x80, 0x80,
    0x80, 0x40, 0x63, 0x3e,

    /* U+010B "ċ" */
    0x30, 0x0, 0xfc, 0x42, 0x10, 0xc3, 0xc0,

    /* U+010C "Č" */
    0x22, 0x14, 0x8, 0x1e, 0x63, 0x40, 0x80, 0x80,
    0x80, 0x80, 0x40, 0x63, 0x3e,

    /* U+010D "č" */
    0x49, 0x80, 0xfc, 0x42, 0x10, 0xcb, 0xc0,

    /* U+010E "Ď" */
    0x40, 0x38, 0x10, 0xfc, 0x86, 0x82, 0x81, 0x81,
    0x81, 0x81, 0x82, 0x86, 0xfc,

    /* U+010F "ď" */
    0x5, 0x5, 0x4, 0x7c, 0xc4, 0x84, 0x84, 0x84,
    0xcc, 0x74,

    /* U+0110 "Đ" */
    0x7e, 0x10, 0x64, 0x9, 0x1, 0x40, 0x7c, 0x14,
    0x5, 0x2, 0x41, 0x9f, 0x80,

    /* U+0111 "đ" */
    0x4, 0x3c, 0x13, 0xec, 0x50, 0xa1, 0x42, 0xcc,
    0xe8,

    /* U+0112 "Ē" */
    0x78, 0xf, 0xe0, 0x82, 0x8, 0x3e, 0x82, 0x8,
    0x3f,

    /* U+0113 "ē" */
    0x78, 0x0, 0xe6, 0x28, 0x5f, 0xe0, 0x60, 0x78,

    /* U+0114 "Ĕ" */
    0x4d, 0x23, 0xbf, 0x82, 0x8, 0x20, 0xfe, 0x8,
    0x20, 0xfc,

    /* U+0115 "ĕ" */
    0x48, 0x70, 0x1, 0xcc, 0x50, 0xbf, 0xc0, 0xc0,
    0xf0,

    /* U+0116 "Ė" */
    0x10, 0xf, 0xe0, 0x82, 0x8, 0x3e, 0x82, 0x8,
    0x3f,

    /* U+0117 "ė" */
    0x30, 0x0, 0x1, 0xcc, 0x50, 0xbf, 0xc0, 0xc0,
    0xf0,

    /* U+0118 "Ę" */
    0xfd, 0x2, 0x4, 0xf, 0xd0, 0x20, 0x40, 0x81,
    0xf8, 0x20, 0x70,

    /* U+0119 "ę" */
    0x39, 0x8b, 0xf4, 0x8, 0x8, 0x9e, 0x8, 0x18,

    /* U+011A "Ě" */
    0x49, 0xc2, 0x3f, 0x82, 0x8, 0x20, 0xfa, 0x8,
    0x20, 0xfc,

    /* U+011B "ě" */
    0x68, 0x60, 0x1, 0xcc, 0x50, 0xbf, 0xc0, 0xc0,
    0xf0,

    /* U+011C "Ĝ" */
    0x4, 0x5, 0x4, 0x43, 0xe6, 0x1a, 0x2, 0x1,
    0x0, 0x83, 0xc0, 0x50, 0x2c, 0x11, 0xf0,

    /* U+011D "ĝ" */
    0x30, 0x90, 0x3, 0xf8, 0x50, 0x9e, 0x40, 0x7d,
    0xa, 0x13, 0xc0,

    /* U+011E "Ğ" */
    0x11, 0x9, 0x83, 0x83, 0xe6, 0x1a, 0x2, 0x1,
    0x0, 0x83, 0xc0, 0x50, 0x2c, 0x11, 0xf0,

    /* U+011F "ğ" */
    0x48, 0x70, 0x3, 0xf8, 0x50, 0x9e, 0x40, 0x7d,
    0xa, 0x13, 0xc0,

    /* U+0120 "Ġ" */
    0xc, 0x0, 0x7, 0xcc, 0x34, 0x4, 0x2, 0x1,
    0x7, 0x80, 0xa0, 0x58, 0x23, 0xe0,

    /* U+0121 "ġ" */
    0x18, 0x0, 0x3, 0xf8, 0x50, 0x9e, 0x40, 0x7d,
    0xa, 0x13, 0xc0,

    /* U+0122 "Ģ" */
    0x1f, 0x30, 0xd0, 0x10, 0x8, 0x4, 0x1e, 0x2,
    0x81, 0x60, 0x8f, 0x82, 0x1, 0x0,

    /* U+0123 "ģ" */
    0x10, 0x20, 0x3, 0xf8, 0x50, 0x9e, 0x40, 0x7d,
    0xa, 0x13, 0xc0,

    /* U+0124 "Ĥ" */
    0x18, 0x3c, 0x26, 0x81, 0x81, 0x81, 0x81, 0x81,
    0xff, 0x81, 0x81, 0x81, 0x81,

    /* U+0125 "ĥ" */
    0x20, 0x50, 0x88, 0x20, 0x20, 0x20, 0x2e, 0x31,
    0x21, 0x21, 0x21, 0x21, 0x21,

    /* U+0126 "Ħ" */
    0x40, 0x90, 0x2f, 0xfd, 0x2, 0x40, 0x9f, 0xe4,
    0x9, 0x2, 0x40, 0x90, 0x20,

    /* U+0127 "ħ" */
    0x41, 0xe1, 0x2, 0xe6, 0x28, 0x50, 0xa1, 0x42,
    0x84,

    /* U+0128 "Ĩ" */
    0x68, 0x88, 0x42, 0x10, 0x84, 0x21, 0x8, 0x40,

    /* U+0129 "ĩ" */
    0xf4, 0x24, 0x92, 0x48,

    /* U+012A "Ī" */
    0xe1, 0x24, 0x92, 0x49, 0x20,

    /* U+012B "ī" */
    0xe1, 0x24, 0x92, 0x40,

    /* U+012C "Ĭ" */
    0xca, 0xdc, 0x42, 0x10, 0x84, 0x21, 0x8, 0x42,
    0x0,

    /* U+012D "ĭ" */
    0xbc, 0x24, 0x92, 0x48,

    /* U+012E "Į" */
    0x49, 0x24, 0x92, 0x4a, 0x30,

    /* U+012F "į" */
    0x60, 0x24, 0x92, 0x4a, 0x30,

    /* U+0130 "İ" */
    0xca, 0xaa, 0xaa,

    /* U+0131 "ı" */
    0xfe,

    /* U+0132 "Ĳ" */
    0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x83, 0x9e,

    /* U+0133 "ĳ" */
    0xd8, 0x1, 0x18, 0xc6, 0x31, 0x8c, 0x42, 0x60,

    /* U+0134 "Ĵ" */
    0x10, 0xa4, 0x44, 0x10, 0x41, 0x4, 0x10, 0x41,
    0xc, 0xe0,

    /* U+0135 "ĵ" */
    0x22, 0x80, 0x42, 0x10, 0x84, 0x21, 0x9, 0xc0,

    /* U+0136 "Ķ" */
    0x86, 0x8c, 0x98, 0xb0, 0xe0, 0x90, 0x98, 0x8c,
    0x84, 0x82, 0x10, 0x20,

    /* U+0137 "ķ" */
    0x82, 0x8, 0x22, 0xb3, 0x8a, 0x24, 0x9a, 0x22,
    0x8,

    /* U+0138 "ĸ" */
    0x8a, 0x4a, 0x38, 0xb2, 0x68, 0x80,

    /* U+0139 "Ĺ" */
    0x22, 0x21, 0x8, 0x42, 0x10, 0x84, 0x21, 0xf,
    0x80,

    /* U+013A "ĺ" */
    0x3a, 0x49, 0x24, 0x92, 0x48,

    /* U+013B "Ļ" */
    0x84, 0x21, 0x8, 0x42, 0x10, 0x87, 0xc8, 0x40,

    /* U+013C "ļ" */
    0xff, 0xf0,

    /* U+013D "Ľ" */
    0x95, 0x21, 0x8, 0x42, 0x10, 0x87, 0xc0,

    /* U+013E "ľ" */
    0xb6, 0x49, 0x24, 0x90,

    /* U+013F "Ŀ" */
    0x84, 0x21, 0x8, 0xc2, 0x10, 0x87, 0xc0,

    /* U+0140 "ŀ" */
    0x88, 0x88, 0xb8, 0x88, 0x88,

    /* U+0141 "Ł" */
    0x41, 0x4, 0x12, 0x73, 0x4, 0x10, 0x41, 0xf0,

    /* U+0142 "ł" */
    0x49, 0x3c, 0x92, 0x48,

    /* U+0143 "Ń" */
    0x6, 0xc, 0x10, 0x81, 0xc1, 0xe1, 0xb1, 0x99,
    0x99, 0x8d, 0x87, 0x83, 0x81,

    /* U+0144 "ń" */
    0x18, 0xc0, 0x2e, 0xc6, 0x18, 0x61, 0x86, 0x10,

    /* U+0145 "Ņ" */
    0x81, 0xc1, 0xe1, 0xb1, 0x99, 0x99, 0x8d, 0x87,
    0x83, 0x81, 0x8, 0x10,

    /* U+0146 "ņ" */
    0xbb, 0x18, 0x61, 0x86, 0x18, 0x4c, 0x20,

    /* U+0147 "Ň" */
    0x24, 0x1c, 0x18, 0x81, 0xc1, 0xe1, 0xb1, 0x99,
    0x99, 0x8d, 0x87, 0x83, 0x81,

    /* U+0148 "ň" */
    0x48, 0xc0, 0x2e, 0xc6, 0x18, 0x61, 0x86, 0x10,

    /* U+0149 "ŉ" */
    0x81, 0x2, 0x2, 0xe6, 0x28, 0x50, 0xa1, 0x42,
    0x84,

    /* U+014A "Ŋ" */
    0x81, 0xc1, 0xe1, 0xb1, 0x99, 0x89, 0x8d, 0x87,
    0x83, 0x81, 0x1, 0x6,

    /* U+014B "ŋ" */
    0xbb, 0x18, 0x61, 0x86, 0x18, 0x41, 0x38,

    /* U+014C "Ō" */
    0x1e, 0x0, 0x1, 0xe1, 0x86, 0x40, 0xa0, 0x18,
    0x6, 0x1, 0x80, 0x50, 0x26, 0x18, 0x78,

    /* U+014D "ō" */
    0x7c, 0x0, 0xe6, 0x28, 0x30, 0x60, 0xa2, 0x78,

    /* U+014E "Ŏ" */
    0x12, 0x4, 0x81, 0xe0, 0x78, 0x61, 0x90, 0x28,
    0x6, 0x1, 0x80, 0x60, 0x14, 0x9, 0x86, 0x1e,
    0x0,

    /* U+014F "ŏ" */
    0x44, 0x70, 0x1, 0xcc, 0x50, 0x60, 0xc1, 0x44,
    0xf0,

    /* U+0150 "Ő" */
    0x9, 0x6, 0x80, 0x0, 0x78, 0x61, 0x90, 0x28,
    0x6, 0x1, 0x80, 0x60, 0x14, 0x9, 0x86, 0x1e,
    0x0,

    /* U+0151 "ő" */
    0x14, 0x50, 0x1, 0xcc, 0x50, 0x60, 0xc1, 0x44,
    0xf0,

    /* U+0152 "Œ" */
    0x3e, 0xfd, 0x8e, 0x4, 0x18, 0x20, 0x20, 0x80,
    0x82, 0x3, 0xe8, 0x8, 0x10, 0x60, 0x63, 0x80,
    0xfb, 0xf0,

    /* U+0153 "œ" */
    0x7b, 0xd9, 0x4e, 0x10, 0xc3, 0xf8, 0x41, 0x9c,
    0x1e, 0xf0,

    /* U+0154 "Ŕ" */
    0x8, 0x60, 0x87, 0xc8, 0xd0, 0xa1, 0x46, 0xf1,
    0x22, 0x24, 0x68, 0x60,

    /* U+0155 "ŕ" */
    0x24, 0xb, 0xc8, 0x88, 0x88,

    /* U+0156 "Ŗ" */
    0xf9, 0xa, 0x14, 0x6f, 0x92, 0x26, 0x44, 0x85,
    0xc, 0x41, 0x0,

    /* U+0157 "ŗ" */
    0xbc, 0x88, 0x88, 0x88, 0x80,

    /* U+0158 "Ř" */
    0x88, 0xa0, 0x87, 0xc8, 0xd0, 0xa1, 0x46, 0xf1,
    0x22, 0x24, 0x68, 0x60,

    /* U+0159 "ř" */
    0xa6, 0xb, 0xc8, 0x88, 0x88,

    /* U+015A "Ś" */
    0x18, 0x87, 0xa3, 0x83, 0x7, 0x7, 0x4, 0x18,
    0xfe,

    /* U+015B "ś" */
    0x11, 0x0, 0xe8, 0xe1, 0xc1, 0x9f, 0x80,

    /* U+015C "Ŝ" */
    0x10, 0xa4, 0x5e, 0x86, 0xc, 0x1c, 0x1c, 0x10,
    0x63, 0xf8,

    /* U+015D "ŝ" */
    0x32, 0x80, 0xe8, 0xe1, 0xc1, 0x9f, 0x80,

    /* U+015E "Ş" */
    0x7a, 0x18, 0x30, 0x70, 0x70, 0x41, 0x8f, 0xc3,
    0xc,

    /* U+015F "ş" */
    0x74, 0x30, 0x60, 0xcc, 0x84, 0x60,

    /* U+0160 "Š" */
    0x68, 0x47, 0xa3, 0x83, 0x7, 0x7, 0x4, 0x18,
    0xfe,

    /* U+0161 "š" */
    0x51, 0x80, 0xe8, 0xe1, 0xc1, 0x9f, 0x80,

    /* U+0162 "Ţ" */
    0xff, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x18,

    /* U+0163 "ţ" */
    0x44, 0x4f, 0x44, 0x44, 0x72, 0x13,

    /* U+0164 "Ť" */
    0x4, 0x28, 0x10, 0xff, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10,

    /* U+0165 "ť" */
    0xa, 0x51, 0xe4, 0x21, 0x8, 0x43, 0x80,

    /* U+0166 "Ŧ" */
    0xff, 0x10, 0x10, 0x10, 0x7e, 0x10, 0x10, 0x10,
    0x10, 0x10,

    /* U+0167 "ŧ" */
    0x44, 0xf4, 0xf4, 0x44, 0x70,

    /* U+0168 "Ũ" */
    0x3c, 0x0, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x42, 0x3c,

    /* U+0169 "ũ" */
    0x69, 0x60, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,

    /* U+016A "Ū" */
    0x3c, 0x0, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x42, 0x3c,

    /* U+016B "ū" */
    0x78, 0x8, 0x61, 0x86, 0x18, 0x63, 0x74,

    /* U+016C "Ŭ" */
    0x24, 0x24, 0x3c, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0x42, 0x3c,

    /* U+016D "ŭ" */
    0x49, 0xe0, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,

    /* U+016E "Ů" */
    0x18, 0x28, 0x18, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0x42, 0x3c,

    /* U+016F "ů" */
    0x31, 0x23, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,

    /* U+0170 "Ű" */
    0x12, 0x34, 0x0, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0x42, 0x3c,

    /* U+0171 "ű" */
    0x3c, 0xa0, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,

    /* U+0172 "Ų" */
    0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x42, 0x3c, 0x10, 0x1c,

    /* U+0173 "ų" */
    0x85, 0xa, 0x14, 0x28, 0x51, 0x9d, 0x4, 0xe,

    /* U+0174 "Ŵ" */
    0x1, 0x0, 0xa, 0x0, 0x48, 0x30, 0x41, 0x43,
    0xd, 0xe, 0x36, 0x28, 0x89, 0x22, 0x24, 0xd8,
    0xd1, 0x41, 0x85, 0x6, 0x1c, 0x18, 0x20,

    /* U+0175 "ŵ" */
    0x6, 0x1, 0x40, 0x0, 0x62, 0x34, 0xa4, 0x94,
    0x9a, 0xb1, 0x54, 0x31, 0x86, 0x30,

    /* U+0176 "Ŷ" */
    0x8, 0xa, 0x8, 0x88, 0x26, 0x31, 0x10, 0xd8,
    0x28, 0x8, 0x4, 0x2, 0x1, 0x0, 0x80,

    /* U+0177 "ŷ" */
    0x18, 0x48, 0x6, 0x14, 0x4c, 0x8b, 0x1c, 0x18,
    0x20, 0x41, 0x0,

    /* U+0178 "Ÿ" */
    0x36, 0x0, 0x10, 0x4c, 0x62, 0x21, 0xb0, 0x50,
    0x10, 0x8, 0x4, 0x2, 0x1, 0x0,

    /* U+0179 "Ź" */
    0x6, 0x8, 0x10, 0xff, 0x6, 0x4, 0xc, 0x18,
    0x30, 0x20, 0x60, 0xc0, 0xff,

    /* U+017A "ź" */
    0x18, 0x40, 0x1f, 0x8, 0x61, 0x8, 0x43, 0xf0,

    /* U+017B "Ż" */
    0x30, 0x0, 0xff, 0x6, 0x4, 0xc, 0x18, 0x30,
    0x20, 0x60, 0xc0, 0xff,

    /* U+017C "ż" */
    0x30, 0x0, 0x1f, 0x8, 0x61, 0x8, 0x43, 0xf0,

    /* U+017D "Ž" */
    0x64, 0x38, 0x10, 0xff, 0x6, 0x4, 0xc, 0x18,
    0x30, 0x20, 0x60, 0xc0, 0xff,

    /* U+017E "ž" */
    0x28, 0xc0, 0x1f, 0x8, 0x61, 0x8, 0x43, 0xf0,

    /* U+017F "ſ" */
    0x34, 0x4c, 0x44, 0x44, 0x44,

    /* U+2010 "‐" */
    0xe0,

    /* U+2012 "‒" */
    0xf8,

    /* U+2013 "–" */
    0xfc,

    /* U+2014 "—" */
    0xff, 0x80,

    /* U+2015 "―" */
    0xff, 0x80,

    /* U+2016 "‖" */
    0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x90,

    /* U+2017 "‗" */
    0xff, 0xf0,

    /* U+2018 "‘" */
    0xe0,

    /* U+2019 "’" */
    0xe0,

    /* U+201A "‚" */
    0xf0,

    /* U+201B "‛" */
    0xe0,

    /* U+201C "“" */
    0xb6, 0x80,

    /* U+201D "”" */
    0xb6, 0x80,

    /* U+201E "„" */
    0xb6, 0xd0,

    /* U+201F "‟" */
    0xb6, 0x80,

    /* U+2020 "†" */
    0x10, 0x20, 0x47, 0xf1, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x40, 0x80,

    /* U+2021 "‡" */
    0x10, 0x20, 0x47, 0xf1, 0x2, 0x4, 0x8, 0xfe,
    0x20, 0x40, 0x80,

    /* U+2022 "•" */
    0x77, 0xff, 0xf7, 0x0,

    /* U+2026 "…" */
    0xd9, 0x80,

    /* U+20AC "€" */
    0x1e, 0x31, 0x20, 0x60, 0xfc, 0xfc, 0x60, 0x60,
    0x33, 0x1e
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 57, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 56, .box_w = 2, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 4, .adv_w = 130, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 12, .adv_w = 130, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 21, .adv_w = 130, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 28, .adv_w = 130, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 37, .adv_w = 57, .box_w = 1, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 39, .adv_w = 111, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 46, .adv_w = 90, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 47, .adv_w = 186, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 60, .adv_w = 82, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 63, .adv_w = 96, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 66, .adv_w = 130, .box_w = 6, .box_h = 3, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 69, .adv_w = 83, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 70, .adv_w = 186, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 83, .adv_w = 90, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 84, .adv_w = 93, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 87, .adv_w = 130, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 94, .adv_w = 75, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 97, .adv_w = 75, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 100, .adv_w = 90, .box_w = 3, .box_h = 2, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 101, .adv_w = 145, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 109, .adv_w = 157, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 122, .adv_w = 58, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 123, .adv_w = 90, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 125, .adv_w = 75, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 127, .adv_w = 90, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 130, .adv_w = 96, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 133, .adv_w = 164, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 145, .adv_w = 163, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 155, .adv_w = 166, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 168, .adv_w = 99, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 174, .adv_w = 152, .box_w = 9, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 189, .adv_w = 152, .box_w = 9, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 204, .adv_w = 152, .box_w = 9, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 219, .adv_w = 152, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 233, .adv_w = 152, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 247, .adv_w = 152, .box_w = 9, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 262, .adv_w = 208, .box_w = 13, .box_h = 10, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 279, .adv_w = 150, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 291, .adv_w = 129, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 301, .adv_w = 129, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 311, .adv_w = 129, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 321, .adv_w = 129, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 330, .adv_w = 63, .box_w = 3, .box_h = 13, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 335, .adv_w = 63, .box_w = 3, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 340, .adv_w = 63, .box_w = 5, .box_h = 13, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 349, .adv_w = 63, .box_w = 4, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 355, .adv_w = 172, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 368, .adv_w = 171, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 380, .adv_w = 179, .box_w = 10, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 397, .adv_w = 179, .box_w = 10, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 414, .adv_w = 179, .box_w = 10, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 431, .adv_w = 179, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 446, .adv_w = 179, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 461, .adv_w = 130, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 467, .adv_w = 179, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 481, .adv_w = 165, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 494, .adv_w = 165, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 507, .adv_w = 165, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 520, .adv_w = 165, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 532, .adv_w = 140, .box_w = 9, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 547, .adv_w = 134, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 556, .adv_w = 130, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 565, .adv_w = 111, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 573, .adv_w = 111, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 581, .adv_w = 111, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 589, .adv_w = 111, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 597, .adv_w = 111, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 605, .adv_w = 111, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 613, .adv_w = 180, .box_w = 10, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 622, .adv_w = 107, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 629, .adv_w = 118, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 638, .adv_w = 118, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 647, .adv_w = 118, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 656, .adv_w = 118, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 665, .adv_w = 54, .box_w = 2, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 668, .adv_w = 54, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 671, .adv_w = 54, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 676, .adv_w = 54, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 681, .adv_w = 127, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 689, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 697, .adv_w = 127, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 706, .adv_w = 127, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 715, .adv_w = 127, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 724, .adv_w = 127, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 733, .adv_w = 127, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 742, .adv_w = 130, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 747, .adv_w = 127, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 755, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 763, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 771, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 779, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 787, .adv_w = 115, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 798, .adv_w = 126, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 807, .adv_w = 115, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 818, .adv_w = 152, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 832, .adv_w = 111, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 839, .adv_w = 152, .box_w = 10, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 856, .adv_w = 111, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 864, .adv_w = 151, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 879, .adv_w = 111, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 887, .adv_w = 150, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 899, .adv_w = 107, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 906, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 919, .adv_w = 107, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 926, .adv_w = 150, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 938, .adv_w = 107, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 945, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 958, .adv_w = 107, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 965, .adv_w = 170, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 978, .adv_w = 125, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 988, .adv_w = 172, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1001, .adv_w = 126, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1010, .adv_w = 129, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1019, .adv_w = 118, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1027, .adv_w = 129, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1037, .adv_w = 118, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1046, .adv_w = 129, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1055, .adv_w = 118, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1064, .adv_w = 129, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1075, .adv_w = 118, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1083, .adv_w = 129, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1093, .adv_w = 118, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1102, .adv_w = 164, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1117, .adv_w = 116, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1128, .adv_w = 164, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1143, .adv_w = 116, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1154, .adv_w = 164, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1168, .adv_w = 116, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1179, .adv_w = 164, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1193, .adv_w = 116, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1204, .adv_w = 171, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1217, .adv_w = 125, .box_w = 8, .box_h = 13, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1230, .adv_w = 173, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1243, .adv_w = 125, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1252, .adv_w = 63, .box_w = 5, .box_h = 12, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1260, .adv_w = 54, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1264, .adv_w = 63, .box_w = 3, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1269, .adv_w = 54, .box_w = 3, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1273, .adv_w = 63, .box_w = 5, .box_h = 13, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1282, .adv_w = 54, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1286, .adv_w = 63, .box_w = 3, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1291, .adv_w = 54, .box_w = 3, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1296, .adv_w = 63, .box_w = 2, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1299, .adv_w = 54, .box_w = 1, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1300, .adv_w = 165, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1310, .adv_w = 111, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1318, .adv_w = 95, .box_w = 6, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1328, .adv_w = 54, .box_w = 5, .box_h = 12, .ofs_x = -1, .ofs_y = -2},
    {.bitmap_index = 1336, .adv_w = 148, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1348, .adv_w = 114, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1357, .adv_w = 114, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1363, .adv_w = 115, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1372, .adv_w = 53, .box_w = 3, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1377, .adv_w = 115, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1385, .adv_w = 53, .box_w = 1, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1387, .adv_w = 115, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1394, .adv_w = 53, .box_w = 3, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1398, .adv_w = 120, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1405, .adv_w = 83, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1410, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1418, .adv_w = 69, .box_w = 3, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1422, .adv_w = 171, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1435, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1443, .adv_w = 171, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1455, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1462, .adv_w = 171, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1475, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1483, .adv_w = 135, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1492, .adv_w = 171, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1504, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1511, .adv_w = 179, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1526, .adv_w = 127, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1534, .adv_w = 179, .box_w = 10, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1551, .adv_w = 127, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1560, .adv_w = 179, .box_w = 10, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1577, .adv_w = 127, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1586, .adv_w = 244, .box_w = 14, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1604, .adv_w = 196, .box_w = 11, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1614, .adv_w = 140, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1626, .adv_w = 82, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1631, .adv_w = 140, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1642, .adv_w = 82, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1647, .adv_w = 140, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1659, .adv_w = 82, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1664, .adv_w = 122, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1673, .adv_w = 97, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1680, .adv_w = 122, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1690, .adv_w = 97, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1697, .adv_w = 122, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1706, .adv_w = 97, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1712, .adv_w = 122, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1721, .adv_w = 97, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1728, .adv_w = 132, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1740, .adv_w = 80, .box_w = 4, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1746, .adv_w = 132, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1759, .adv_w = 80, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1766, .adv_w = 136, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1776, .adv_w = 81, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1781, .adv_w = 165, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1793, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1801, .adv_w = 165, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1813, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1820, .adv_w = 165, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1833, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1841, .adv_w = 165, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1854, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1862, .adv_w = 165, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1875, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1883, .adv_w = 165, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1895, .adv_w = 125, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1903, .adv_w = 232, .box_w = 14, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1926, .adv_w = 176, .box_w = 11, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1940, .adv_w = 140, .box_w = 9, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1955, .adv_w = 115, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1966, .adv_w = 140, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1980, .adv_w = 135, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1993, .adv_w = 101, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2001, .adv_w = 135, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2013, .adv_w = 101, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2021, .adv_w = 135, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2034, .adv_w = 101, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2042, .adv_w = 64, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2047, .adv_w = 83, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 2048, .adv_w = 121, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 2049, .adv_w = 130, .box_w = 6, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 2050, .adv_w = 178, .box_w = 9, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 2052, .adv_w = 178, .box_w = 9, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 2054, .adv_w = 99, .box_w = 4, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2061, .adv_w = 103, .box_w = 6, .box_h = 2, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 2063, .adv_w = 48, .box_w = 1, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 2064, .adv_w = 48, .box_w = 1, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 2065, .adv_w = 48, .box_w = 1, .box_h = 4, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2066, .adv_w = 48, .box_w = 1, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 2067, .adv_w = 82, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 2069, .adv_w = 82, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 2071, .adv_w = 82, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2073, .adv_w = 82, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 2075, .adv_w = 130, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2086, .adv_w = 130, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2097, .adv_w = 130, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 2101, .adv_w = 168, .box_w = 9, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2103, .adv_w = 130, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_3[] = {
    0x0, 0x86
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 160, .range_length = 224, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 8208, .range_length = 1, .glyph_id_start = 225,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 8210, .range_length = 17, .glyph_id_start = 226,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 8230, .range_length = 135, .glyph_id_start = 243,
        .unicode_list = unicode_list_3, .glyph_id_ofs_list = NULL, .list_length = 2, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 4,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t goopad_font_essential_14 = {
#else
lv_font_t goopad_font_essential_14 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 16,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -3,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if GOOPAD_FONT_ESSENTIAL_14*/
