/*******************************************************************************
 * Size: 12 px
 * Bpp: 1
 * Lato essential offline fallback; see sources/Lato-Regular.ttf and licenses/lato.txt.
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef GOOPAD_FONT_ESSENTIAL_12
#define GOOPAD_FONT_ESSENTIAL_12 1
#endif

#if GOOPAD_FONT_ESSENTIAL_12

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0x9f,

    /* U+00A2 "¢" */
    0x10, 0x47, 0xa4, 0x92, 0x4d, 0xf, 0x20,

    /* U+00A3 "£" */
    0x39, 0x14, 0x10, 0xf9, 0x4, 0x3f,

    /* U+00A4 "¤" */
    0x1, 0xe4, 0x92, 0x49, 0xe0, 0x0,

    /* U+00A5 "¥" */
    0x44, 0x88, 0xa1, 0x47, 0xcf, 0x84, 0x8,

    /* U+00A6 "¦" */
    0xf1, 0xe0,

    /* U+00A7 "§" */
    0x72, 0x10, 0xe9, 0x68, 0xc2, 0x93, 0x80,

    /* U+00A8 "¨" */
    0xa0,

    /* U+00A9 "©" */
    0x3e, 0x31, 0xb7, 0x76, 0x1a, 0xd, 0x7, 0xc6,
    0xfe, 0x3e, 0x0,

    /* U+00AA "ª" */
    0xee, 0xf0,

    /* U+00AB "«" */
    0x5a, 0xa5,

    /* U+00AC "¬" */
    0xf8, 0x42,

    /* U+00AD "­" */
    0xe0,

    /* U+00AE "®" */
    0x3e, 0x31, 0xb7, 0x72, 0x99, 0x8c, 0xc7, 0x56,
    0xc6, 0x3e, 0x0,

    /* U+00AF "¯" */
    0xe0,

    /* U+00B0 "°" */
    0x6b, 0xb6,

    /* U+00B1 "±" */
    0x21, 0x3e, 0x42, 0x7c,

    /* U+00B2 "²" */
    0xe5, 0x70,

    /* U+00B3 "³" */
    0x68, 0xf0,

    /* U+00B4 "´" */
    0x60,

    /* U+00B5 "µ" */
    0x8a, 0x28, 0xa2, 0x8b, 0xf8, 0x20,

    /* U+00B6 "¶" */
    0x7f, 0x72, 0x72, 0x72, 0x12, 0x12, 0x12, 0x12,
    0x12, 0x12,

    /* U+00B7 "·" */
    0xc0,

    /* U+00B8 "¸" */
    0x5c,

    /* U+00B9 "¹" */
    0xc9, 0x70,

    /* U+00BA "º" */
    0x69, 0x96,

    /* U+00BB "»" */
    0xa5, 0x5a,

    /* U+00BC "¼" */
    0x42, 0xc4, 0x48, 0x48, 0xf0, 0x16, 0x2a, 0x2f,
    0x42,

    /* U+00BD "½" */
    0x41, 0x89, 0x22, 0x4f, 0x4, 0xc8, 0xa2, 0x4e,

    /* U+00BE "¾" */
    0xc4, 0xc4, 0x48, 0x28, 0xd0, 0x16, 0x2a, 0x4f,
    0x42,

    /* U+00BF "¿" */
    0x60, 0x22, 0x48, 0x87,

    /* U+00C0 "À" */
    0x30, 0x18, 0x0, 0x18, 0x18, 0x24, 0x24, 0x26,
    0x7e, 0x42, 0x81,

    /* U+00C1 "Á" */
    0xc, 0x8, 0x0, 0x18, 0x18, 0x24, 0x24, 0x26,
    0x7e, 0x42, 0x81,

    /* U+00C2 "Â" */
    0x8, 0x14, 0x0, 0x18, 0x18, 0x24, 0x24, 0x26,
    0x7e, 0x42, 0x81,

    /* U+00C3 "Ã" */
    0x3c, 0x0, 0x18, 0x18, 0x24, 0x24, 0x26, 0x7e,
    0x42, 0x81,

    /* U+00C4 "Ä" */
    0x24, 0x0, 0x18, 0x18, 0x24, 0x24, 0x26, 0x7e,
    0x42, 0x81,

    /* U+00C5 "Å" */
    0x18, 0x18, 0x0, 0x18, 0x18, 0x24, 0x24, 0x26,
    0x7e, 0x42, 0x81,

    /* U+00C6 "Æ" */
    0xf, 0xe1, 0x80, 0x50, 0xb, 0xe2, 0x20, 0xfc,
    0x10, 0x84, 0x1f,

    /* U+00C7 "Ç" */
    0x3c, 0x86, 0x4, 0x8, 0x10, 0x10, 0x9e, 0x18,
    0x30,

    /* U+00C8 "È" */
    0x41, 0x1, 0xf8, 0x43, 0xd0, 0x84, 0x3e,

    /* U+00C9 "É" */
    0x11, 0x1, 0xf8, 0x43, 0xd0, 0x84, 0x3e,

    /* U+00CA "Ê" */
    0x22, 0x81, 0xf8, 0x43, 0xd0, 0x84, 0x3e,

    /* U+00CB "Ë" */
    0x58, 0x3f, 0x8, 0x7a, 0x10, 0x87, 0xc0,

    /* U+00CC "Ì" */
    0x44, 0x12, 0x49, 0x24, 0x80,

    /* U+00CD "Í" */
    0x50, 0x49, 0x24, 0x92, 0x0,

    /* U+00CE "Î" */
    0x54, 0x24, 0x92, 0x49, 0x0,

    /* U+00CF "Ï" */
    0xb0, 0x44, 0x44, 0x44, 0x44,

    /* U+00D0 "Ð" */
    0x7c, 0x42, 0x41, 0xf1, 0x41, 0x41, 0x42, 0x7c,

    /* U+00D1 "Ñ" */
    0x38, 0x2, 0xe, 0x1a, 0x32, 0x64, 0xc5, 0x87,
    0x4,

    /* U+00D2 "Ò" */
    0x30, 0x10, 0x0, 0x3c, 0x42, 0x81, 0x81, 0x81,
    0x81, 0x42, 0x3c,

    /* U+00D3 "Ó" */
    0xc, 0x8, 0x0, 0x3c, 0x42, 0x81, 0x81, 0x81,
    0x81, 0x42, 0x3c,

    /* U+00D4 "Ô" */
    0x8, 0x18, 0x0, 0x3c, 0x42, 0x81, 0x81, 0x81,
    0x81, 0x42, 0x3c,

    /* U+00D5 "Õ" */
    0x34, 0x2c, 0x0, 0x3c, 0x42, 0x81, 0x81, 0x81,
    0x81, 0x42, 0x3c,

    /* U+00D6 "Ö" */
    0x2c, 0x0, 0x3c, 0x42, 0x81, 0x81, 0x81, 0x81,
    0x42, 0x3c,

    /* U+00D7 "×" */
    0x8a, 0x99, 0xb8, 0x80,

    /* U+00D8 "Ø" */
    0x2, 0x3e, 0x43, 0x85, 0x89, 0x91, 0xa1, 0x42,
    0x7c, 0x80,

    /* U+00D9 "Ù" */
    0x20, 0x20, 0x4, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x8d, 0xf0,

    /* U+00DA "Ú" */
    0x8, 0x20, 0x4, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x8d, 0xf0,

    /* U+00DB "Û" */
    0x10, 0x50, 0x4, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x8d, 0xf0,

    /* U+00DC "Ü" */
    0x68, 0x2, 0xc, 0x18, 0x30, 0x60, 0xc1, 0xc6,
    0xf8,

    /* U+00DD "Ý" */
    0x8, 0x20, 0x4, 0x14, 0x4d, 0x8a, 0x8, 0x10,
    0x20, 0x40,

    /* U+00DE "Þ" */
    0x83, 0xe8, 0xe1, 0x8f, 0xe8, 0x20,

    /* U+00DF "ß" */
    0x7b, 0x28, 0xa4, 0x92, 0x28, 0xae,

    /* U+00E0 "à" */
    0x20, 0x1c, 0x90, 0xbe, 0x2f,

    /* U+00E1 "á" */
    0x10, 0x1c, 0x90, 0xbe, 0x2f,

    /* U+00E2 "â" */
    0x30, 0x1c, 0x90, 0xbe, 0x2f,

    /* U+00E3 "ã" */
    0x70, 0x1c, 0x90, 0xbe, 0x2f,

    /* U+00E4 "ä" */
    0x58, 0x1c, 0x90, 0xbe, 0x2f,

    /* U+00E5 "å" */
    0x32, 0x8c, 0xe4, 0x85, 0xf1, 0x78,

    /* U+00E6 "æ" */
    0x77, 0x4, 0x5f, 0xf1, 0x9, 0x4b, 0xb8,

    /* U+00E7 "ç" */
    0x76, 0x21, 0xc, 0x18, 0x46,

    /* U+00E8 "è" */
    0x20, 0x7, 0xa2, 0xfa, 0xc, 0x1e,

    /* U+00E9 "é" */
    0x10, 0x7, 0xa2, 0xfa, 0xc, 0x1e,

    /* U+00EA "ê" */
    0x30, 0x7, 0xa2, 0xfa, 0xc, 0x1e,

    /* U+00EB "ë" */
    0x50, 0x7, 0xa2, 0xfa, 0xc, 0x1e,

    /* U+00EC "ì" */
    0x45, 0x55,

    /* U+00ED "í" */
    0x6a, 0xaa,

    /* U+00EE "î" */
    0x71, 0x24, 0x92,

    /* U+00EF "ï" */
    0xa1, 0x24, 0x92,

    /* U+00F0 "ð" */
    0x68, 0x60, 0x9d, 0xce, 0x18, 0x73, 0x78,

    /* U+00F1 "ñ" */
    0x70, 0x3d, 0x18, 0xc6, 0x31,

    /* U+00F2 "ò" */
    0x20, 0x7, 0xb3, 0x86, 0x1c, 0xde,

    /* U+00F3 "ó" */
    0x10, 0x7, 0xb3, 0x86, 0x1c, 0xde,

    /* U+00F4 "ô" */
    0x30, 0x7, 0xb3, 0x86, 0x1c, 0xde,

    /* U+00F5 "õ" */
    0x78, 0x7, 0xb3, 0x86, 0x1c, 0xde,

    /* U+00F6 "ö" */
    0x50, 0x7, 0xb3, 0x86, 0x1c, 0xde,

    /* U+00F7 "÷" */
    0x20, 0x3e, 0x2, 0x0,

    /* U+00F8 "ø" */
    0x1, 0xfc, 0xe5, 0xa7, 0x3f, 0x80,

    /* U+00F9 "ù" */
    0x60, 0x23, 0x18, 0xc6, 0x2f,

    /* U+00FA "ú" */
    0x11, 0x23, 0x18, 0xc6, 0x2f,

    /* U+00FB "û" */
    0x50, 0x23, 0x18, 0xc6, 0x2f,

    /* U+00FC "ü" */
    0x50, 0x23, 0x18, 0xc6, 0x2f,

    /* U+00FD "ý" */
    0x10, 0xc, 0x52, 0x48, 0xa3, 0x4, 0x20, 0x80,

    /* U+00FE "þ" */
    0x84, 0x21, 0x6c, 0xc6, 0x31, 0xf4, 0x20,

    /* U+00FF "ÿ" */
    0x68, 0xc, 0x52, 0x48, 0xa3, 0x4, 0x20, 0x80,

    /* U+0100 "Ā" */
    0x3c, 0x0, 0x18, 0x18, 0x24, 0x24, 0x26, 0x7e,
    0x42, 0x81,

    /* U+0101 "ā" */
    0x78, 0x1c, 0x90, 0xbe, 0x2f,

    /* U+0102 "Ă" */
    0x24, 0x38, 0x0, 0x18, 0x18, 0x28, 0x24, 0x44,
    0x7e, 0xc2, 0x83,

    /* U+0103 "ă" */
    0x70, 0xc, 0x90, 0xbe, 0x2f,

    /* U+0104 "Ą" */
    0x18, 0x38, 0x28, 0x24, 0x44, 0x7e, 0xc2, 0x83,
    0x2, 0x3,

    /* U+0105 "ą" */
    0x71, 0x20, 0x9e, 0x89, 0xe1, 0x3,

    /* U+0106 "Ć" */
    0xc, 0x10, 0x1, 0xe4, 0x10, 0x20, 0x40, 0x80,
    0x84, 0xf0,

    /* U+0107 "ć" */
    0x10, 0x1d, 0x88, 0x43, 0x2e,

    /* U+0108 "Ĉ" */
    0x8, 0x28, 0x1, 0xe4, 0x30, 0x20, 0x40, 0x80,
    0x84, 0xf0,

    /* U+0109 "ĉ" */
    0x32, 0x1d, 0x88, 0x43, 0xe,

    /* U+010A "Ċ" */
    0x18, 0x0, 0xf2, 0x18, 0x10, 0x20, 0x40, 0x42,
    0x78,

    /* U+010B "ċ" */
    0x20, 0x0, 0xec, 0x42, 0x18, 0x70,

    /* U+010C "Č" */
    0x24, 0x30, 0x1, 0xe4, 0x30, 0x20, 0x40, 0x80,
    0x84, 0xf0,

    /* U+010D "č" */
    0x51, 0x1d, 0x88, 0x43, 0x2e,

    /* U+010E "Ď" */
    0x48, 0x60, 0x7, 0xc8, 0x50, 0x60, 0xc1, 0x83,
    0xb, 0xe0,

    /* U+010F "ď" */
    0xa, 0x18, 0x23, 0xc8, 0x91, 0x22, 0x44, 0x78,

    /* U+0110 "Đ" */
    0x7c, 0x42, 0x41, 0xf1, 0x41, 0x41, 0x42, 0x7c,

    /* U+0111 "đ" */
    0x8, 0x70, 0x9e, 0x8a, 0x28, 0xa2, 0x78,

    /* U+0112 "Ē" */
    0x70, 0x3f, 0x8, 0x7a, 0x10, 0x87, 0xc0,

    /* U+0113 "ē" */
    0x78, 0x7, 0xa2, 0xfa, 0xc, 0x1e,

    /* U+0114 "Ĕ" */
    0x4b, 0x81, 0xf8, 0x43, 0xf0, 0x84, 0x3e,

    /* U+0115 "ĕ" */
    0x70, 0x7, 0xa2, 0xfa, 0xc, 0x1e,

    /* U+0116 "Ė" */
    0x30, 0x3f, 0x8, 0x7a, 0x10, 0x87, 0xc0,

    /* U+0117 "ė" */
    0x20, 0x0, 0x1e, 0x8b, 0xe8, 0x30, 0x78,

    /* U+0118 "Ę" */
    0xfc, 0x21, 0xe8, 0x42, 0x1f, 0x10, 0xc0,

    /* U+0119 "ę" */
    0x7a, 0x2f, 0xa0, 0xc1, 0xe2, 0xc,

    /* U+011A "Ě" */
    0x41, 0x81, 0xf8, 0x43, 0xd0, 0x84, 0x3e,

    /* U+011B "ě" */
    0x50, 0x87, 0xa2, 0xfa, 0xc, 0x1e,

    /* U+011C "Ĝ" */
    0x8, 0x30, 0x1, 0xe4, 0x30, 0x20, 0x47, 0x82,
    0x84, 0xf0,

    /* U+011D "ĝ" */
    0x18, 0x83, 0xd1, 0x44, 0xe4, 0xf, 0xc5, 0xe0,

    /* U+011E "Ğ" */
    0x24, 0x30, 0x1, 0xe4, 0x30, 0x20, 0x47, 0x82,
    0x84, 0xf0,

    /* U+011F "ğ" */
    0x38, 0x3, 0xd1, 0x44, 0xe4, 0xf, 0xc5, 0xe0,

    /* U+0120 "Ġ" */
    0x18, 0x0, 0xf2, 0x18, 0x10, 0x23, 0xc1, 0x42,
    0x78,

    /* U+0121 "ġ" */
    0x30, 0x0, 0xf, 0x45, 0x13, 0x90, 0x3f, 0x17,
    0x80,

    /* U+0122 "Ģ" */
    0x3c, 0x86, 0x4, 0x8, 0xf0, 0x50, 0x9e, 0x0,
    0x20,

    /* U+0123 "ģ" */
    0x10, 0x3, 0xd1, 0x44, 0xe4, 0xf, 0xc5, 0xe0,

    /* U+0124 "Ĥ" */
    0x10, 0x50, 0x4, 0x18, 0x30, 0x7f, 0xc1, 0x83,
    0x6, 0x8,

    /* U+0125 "ĥ" */
    0x42, 0x84, 0x10, 0x41, 0x66, 0x51, 0x45, 0x14,
    0x40,

    /* U+0126 "Ħ" */
    0x41, 0x7f, 0xd0, 0x4f, 0xe4, 0x12, 0x9, 0x4,
    0x82,

    /* U+0127 "ħ" */
    0x43, 0xc4, 0x16, 0x65, 0x14, 0x51, 0x44,

    /* U+0128 "Ĩ" */
    0xe1, 0x24, 0x92, 0x48,

    /* U+0129 "ĩ" */
    0xe1, 0x24, 0x92,

    /* U+012A "Ī" */
    0xe1, 0x24, 0x92, 0x48,

    /* U+012B "ī" */
    0xe1, 0x24, 0x92,

    /* U+012C "Ĭ" */
    0xbe, 0x4, 0x44, 0x44, 0x44, 0x40,

    /* U+012D "ĭ" */
    0xe1, 0x24, 0x92,

    /* U+012E "Į" */
    0x49, 0x24, 0x92, 0x8c,

    /* U+012F "į" */
    0x40, 0x24, 0x92, 0x51, 0x80,

    /* U+0130 "İ" */
    0xbf, 0xc0,

    /* U+0131 "ı" */
    0xfc,

    /* U+0132 "Ĳ" */
    0x83, 0x6, 0xc, 0x18, 0x30, 0x60, 0xce,

    /* U+0133 "ĳ" */
    0x90, 0x9, 0x99, 0x99, 0x91, 0x30,

    /* U+0134 "Ĵ" */
    0x11, 0x40, 0x21, 0x8, 0x42, 0x10, 0xb8,

    /* U+0135 "ĵ" */
    0xe1, 0x24, 0x92, 0x58,

    /* U+0136 "Ķ" */
    0x85, 0x32, 0xc7, 0xb, 0x13, 0x23, 0x42, 0x0,
    0x40,

    /* U+0137 "ķ" */
    0x84, 0x21, 0x2a, 0x72, 0x92, 0x88, 0x8,

    /* U+0138 "ĸ" */
    0x95, 0x39, 0x49, 0x44,

    /* U+0139 "Ĺ" */
    0x44, 0x1, 0x8, 0x42, 0x10, 0x84, 0x3e,

    /* U+013A "ĺ" */
    0x52, 0x49, 0x24, 0x92, 0x0,

    /* U+013B "Ļ" */
    0x84, 0x21, 0x8, 0x42, 0x1f, 0x1, 0x0,

    /* U+013C "ļ" */
    0xff, 0xa0,

    /* U+013D "Ľ" */
    0x25, 0x21, 0x8, 0x42, 0x10, 0xf8,

    /* U+013E "ľ" */
    0xb6, 0x49, 0x24, 0x80,

    /* U+013F "Ŀ" */
    0x84, 0x21, 0x38, 0x42, 0x1f,

    /* U+0140 "ŀ" */
    0x92, 0x4b, 0x24, 0x80,

    /* U+0141 "Ł" */
    0x41, 0x4, 0x1c, 0xc1, 0x4, 0x1f,

    /* U+0142 "ł" */
    0x49, 0x3c, 0x92, 0x40,

    /* U+0143 "Ń" */
    0x8, 0x20, 0x4, 0x1c, 0x34, 0x64, 0xc9, 0x8b,
    0xe, 0x8,

    /* U+0144 "ń" */
    0x11, 0x3d, 0x18, 0xc6, 0x31,

    /* U+0145 "Ņ" */
    0x83, 0x86, 0x8c, 0x99, 0x31, 0x61, 0xc1, 0x0,
    0x20,

    /* U+0146 "ņ" */
    0xf4, 0x63, 0x18, 0xc4, 0x4,

    /* U+0147 "Ň" */
    0x24, 0x30, 0x4, 0x1c, 0x34, 0x64, 0xc9, 0x8b,
    0xe, 0x8,

    /* U+0148 "ň" */
    0x51, 0x3d, 0x18, 0xc6, 0x31,

    /* U+0149 "ŉ" */
    0x82, 0x8, 0x1e, 0x45, 0x14, 0x51, 0x44,

    /* U+014A "Ŋ" */
    0x83, 0x86, 0x8c, 0x99, 0x31, 0x61, 0xc1, 0x2,
    0x18,

    /* U+014B "ŋ" */
    0xf4, 0x63, 0x18, 0xc4, 0x26,

    /* U+014C "Ō" */
    0x3c, 0x0, 0x3c, 0x42, 0x81, 0x81, 0x81, 0x81,
    0x42, 0x3c,

    /* U+014D "ō" */
    0x78, 0x7, 0xb3, 0x86, 0x1c, 0xde,

    /* U+014E "Ŏ" */
    0x24, 0x18, 0x0, 0x3c, 0x42, 0x81, 0x81, 0x81,
    0x81, 0x42, 0x3c,

    /* U+014F "ŏ" */
    0x78, 0x7, 0xb3, 0x86, 0x1c, 0xde,

    /* U+0150 "Ő" */
    0x14, 0x0, 0x3c, 0x42, 0x81, 0x81, 0x81, 0x81,
    0x42, 0x3c,

    /* U+0151 "ő" */
    0x28, 0x7, 0xb3, 0x86, 0x1c, 0xde,

    /* U+0152 "Œ" */
    0x3d, 0xf4, 0x30, 0x81, 0x8, 0x1e, 0x81, 0x8,
    0x10, 0x43, 0x3, 0xdf,

    /* U+0153 "œ" */
    0x73, 0xa3, 0x28, 0xfa, 0x30, 0x8c, 0x1c, 0xe0,

    /* U+0154 "Ŕ" */
    0x18, 0x40, 0x7, 0xc8, 0xd0, 0xa3, 0x7c, 0x99,
    0x12, 0x10,

    /* U+0155 "ŕ" */
    0x2b, 0xc9, 0x24,

    /* U+0156 "Ŗ" */
    0xf9, 0xa, 0x17, 0xc9, 0x13, 0x23, 0x42, 0x0,
    0x40,

    /* U+0157 "ŗ" */
    0xf2, 0x49, 0x4,

    /* U+0158 "Ř" */
    0x88, 0xe0, 0x7, 0xc8, 0xd0, 0xa3, 0x7c, 0x99,
    0x12, 0x10,

    /* U+0159 "ř" */
    0xab, 0xc9, 0x24,

    /* U+015A "Ś" */
    0x11, 0x80, 0xe8, 0xc1, 0x83, 0xc, 0x7c,

    /* U+015B "ś" */
    0x20, 0x78, 0xc3, 0x1e,

    /* U+015C "Ŝ" */
    0x22, 0x80, 0xe8, 0xc1, 0x83, 0xc, 0x7c,

    /* U+015D "ŝ" */
    0x60, 0x78, 0xc3, 0x1e,

    /* U+015E "Ş" */
    0x74, 0x60, 0xc1, 0x86, 0x3e, 0x21, 0x80,

    /* U+015F "ş" */
    0x78, 0xc3, 0x1e, 0x26,

    /* U+0160 "Š" */
    0x51, 0x1d, 0x18, 0x30, 0x61, 0x8f, 0x80,

    /* U+0161 "š" */
    0x60, 0x78, 0xc3, 0x1e,

    /* U+0162 "Ţ" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x30,

    /* U+0163 "ţ" */
    0x44, 0xf4, 0x44, 0x42, 0x22,

    /* U+0164 "Ť" */
    0x20, 0x70, 0x7, 0xf1, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x40,

    /* U+0165 "ť" */
    0x12, 0x91, 0xe4, 0x21, 0x8, 0x70,

    /* U+0166 "Ŧ" */
    0xfe, 0x20, 0x43, 0xe1, 0x2, 0x4, 0x8,

    /* U+0167 "ŧ" */
    0x44, 0xf4, 0xf4, 0x47,

    /* U+0168 "Ũ" */
    0x34, 0xb0, 0x4, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x8d, 0xf0,

    /* U+0169 "ũ" */
    0x70, 0x23, 0x18, 0xc6, 0x2f,

    /* U+016A "Ū" */
    0x38, 0x2, 0xc, 0x18, 0x30, 0x60, 0xc1, 0xc6,
    0xf8,

    /* U+016B "ū" */
    0x70, 0x23, 0x18, 0xc6, 0x2f,

    /* U+016C "Ŭ" */
    0x0, 0x70, 0x4, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x8d, 0xf0,

    /* U+016D "ŭ" */
    0x70, 0x23, 0x18, 0xc6, 0x2f,

    /* U+016E "Ů" */
    0x30, 0x60, 0x4, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x8d, 0xf0,

    /* U+016F "ů" */
    0x62, 0x99, 0x18, 0xc6, 0x31, 0x78,

    /* U+0170 "Ű" */
    0x14, 0x2, 0xc, 0x18, 0x30, 0x60, 0xc1, 0xc6,
    0xf8,

    /* U+0171 "ű" */
    0x30, 0x23, 0x18, 0xc6, 0x2f,

    /* U+0172 "Ų" */
    0x83, 0x6, 0xc, 0x18, 0x30, 0x71, 0x3c, 0x10,
    0x30,

    /* U+0173 "ų" */
    0x8a, 0x28, 0xa2, 0x89, 0xe1, 0x3,

    /* U+0174 "Ŵ" */
    0x2, 0x0, 0x50, 0x0, 0xc, 0x21, 0x46, 0x14,
    0x52, 0x65, 0x22, 0x92, 0x28, 0xc3, 0xc, 0x10,
    0xc0,

    /* U+0175 "ŵ" */
    0xc, 0x8, 0x22, 0x29, 0x95, 0x52, 0xa8, 0xcc,
    0x44,

    /* U+0176 "Ŷ" */
    0x10, 0x50, 0x4, 0x14, 0x4d, 0x8a, 0x8, 0x10,
    0x20, 0x40,

    /* U+0177 "ŷ" */
    0x30, 0x2c, 0x52, 0x48, 0xa3, 0x4, 0x20, 0x80,

    /* U+0178 "Ÿ" */
    0x2c, 0x2, 0xa, 0x26, 0xc5, 0x4, 0x8, 0x10,
    0x20,

    /* U+0179 "Ź" */
    0x8, 0x20, 0x3, 0xf0, 0x41, 0x6, 0x8, 0x20,
    0x83, 0xf8,

    /* U+017A "ź" */
    0x10, 0x1e, 0x21, 0x11, 0x1f,

    /* U+017B "Ż" */
    0x10, 0x1, 0xf8, 0x20, 0x83, 0x4, 0x10, 0x41,
    0xfc,

    /* U+017C "ż" */
    0x30, 0x0, 0xf1, 0x8, 0x88, 0xf8,

    /* U+017D "Ž" */
    0x24, 0x30, 0x3, 0xf0, 0x41, 0x6, 0x8, 0x20,
    0x83, 0xf8,

    /* U+017E "ž" */
    0x51, 0x1e, 0x21, 0x11, 0x1f,

    /* U+017F "ſ" */
    0x74, 0xc4, 0x44, 0x44,

    /* U+2010 "‐" */
    0xe0,

    /* U+2012 "‒" */
    0xf8,

    /* U+2013 "–" */
    0xf8,

    /* U+2014 "—" */
    0xff,

    /* U+2015 "―" */
    0xff,

    /* U+2016 "‖" */
    0xb6, 0xdb, 0x6d, 0xb6, 0x80,

    /* U+2017 "‗" */
    0xff, 0xc0,

    /* U+2018 "‘" */
    0xe0,

    /* U+2019 "’" */
    0xe0,

    /* U+201A "‚" */
    0xe0,

    /* U+201B "‛" */
    0xe0,

    /* U+201C "“" */
    0xbb, 0x0,

    /* U+201D "”" */
    0xb6, 0x80,

    /* U+201E "„" */
    0x96, 0xa0,

    /* U+201F "‟" */
    0xbb, 0x0,

    /* U+2020 "†" */
    0x21, 0x9, 0xf2, 0x10, 0x84, 0x21, 0x8,

    /* U+2021 "‡" */
    0x21, 0x9, 0xf2, 0x10, 0x9f, 0x21, 0x8,

    /* U+2022 "•" */
    0x6f, 0xf6,

    /* U+2026 "…" */
    0x92,

    /* U+20AC "€" */
    0x3c, 0xc3, 0xe2, 0xf, 0x88, 0x18, 0x1e
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 49, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 48, .box_w = 1, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2, .adv_w = 111, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 9, .adv_w = 111, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 15, .adv_w = 111, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 21, .adv_w = 111, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 28, .adv_w = 49, .box_w = 1, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 30, .adv_w = 95, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 37, .adv_w = 77, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 38, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 49, .adv_w = 71, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 51, .adv_w = 82, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 53, .adv_w = 111, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 55, .adv_w = 71, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 56, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 67, .adv_w = 77, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 68, .adv_w = 80, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 70, .adv_w = 111, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 74, .adv_w = 64, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 76, .adv_w = 64, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 78, .adv_w = 77, .box_w = 2, .box_h = 2, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 79, .adv_w = 125, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 85, .adv_w = 134, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 95, .adv_w = 50, .box_w = 1, .box_h = 2, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 96, .adv_w = 77, .box_w = 3, .box_h = 2, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 97, .adv_w = 64, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 99, .adv_w = 77, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 101, .adv_w = 82, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 103, .adv_w = 141, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 112, .adv_w = 139, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 120, .adv_w = 142, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 129, .adv_w = 84, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 133, .adv_w = 130, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 144, .adv_w = 130, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 155, .adv_w = 130, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 166, .adv_w = 130, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 176, .adv_w = 130, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 186, .adv_w = 130, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 197, .adv_w = 179, .box_w = 11, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 208, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 217, .adv_w = 111, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 224, .adv_w = 111, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 231, .adv_w = 111, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 238, .adv_w = 111, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 245, .adv_w = 54, .box_w = 3, .box_h = 11, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 250, .adv_w = 54, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 255, .adv_w = 54, .box_w = 3, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 260, .adv_w = 54, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 265, .adv_w = 148, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 273, .adv_w = 147, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 282, .adv_w = 154, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 293, .adv_w = 154, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 304, .adv_w = 154, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 315, .adv_w = 154, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 326, .adv_w = 154, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 336, .adv_w = 111, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 340, .adv_w = 154, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 350, .adv_w = 141, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 360, .adv_w = 141, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 370, .adv_w = 141, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 380, .adv_w = 141, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 389, .adv_w = 120, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 399, .adv_w = 115, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 405, .adv_w = 111, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 411, .adv_w = 95, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 416, .adv_w = 95, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 421, .adv_w = 95, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 426, .adv_w = 95, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 431, .adv_w = 95, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 436, .adv_w = 95, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 442, .adv_w = 155, .box_w = 9, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 449, .adv_w = 92, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 454, .adv_w = 101, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 460, .adv_w = 101, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 466, .adv_w = 101, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 472, .adv_w = 101, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 478, .adv_w = 46, .box_w = 2, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 480, .adv_w = 46, .box_w = 2, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 482, .adv_w = 46, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 485, .adv_w = 46, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 488, .adv_w = 109, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 495, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 500, .adv_w = 109, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 506, .adv_w = 109, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 512, .adv_w = 109, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 518, .adv_w = 109, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 524, .adv_w = 109, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 530, .adv_w = 111, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 534, .adv_w = 109, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 540, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 545, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 550, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 555, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 560, .adv_w = 99, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 568, .adv_w = 108, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 575, .adv_w = 99, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 583, .adv_w = 130, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 593, .adv_w = 95, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 598, .adv_w = 130, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 609, .adv_w = 95, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 614, .adv_w = 130, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 624, .adv_w = 95, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 630, .adv_w = 128, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 640, .adv_w = 92, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 645, .adv_w = 128, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 655, .adv_w = 92, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 660, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 669, .adv_w = 92, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 675, .adv_w = 128, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 685, .adv_w = 92, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 690, .adv_w = 146, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 700, .adv_w = 108, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 708, .adv_w = 148, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 716, .adv_w = 108, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 723, .adv_w = 111, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 730, .adv_w = 101, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 736, .adv_w = 111, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 743, .adv_w = 101, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 749, .adv_w = 111, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 756, .adv_w = 101, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 763, .adv_w = 111, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 770, .adv_w = 101, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 776, .adv_w = 111, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 783, .adv_w = 101, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 789, .adv_w = 140, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 799, .adv_w = 100, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 807, .adv_w = 140, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 817, .adv_w = 100, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 825, .adv_w = 140, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 834, .adv_w = 100, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 843, .adv_w = 140, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 852, .adv_w = 100, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 860, .adv_w = 147, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 870, .adv_w = 107, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 879, .adv_w = 148, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 888, .adv_w = 107, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 895, .adv_w = 54, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 899, .adv_w = 46, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 902, .adv_w = 54, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 906, .adv_w = 46, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 909, .adv_w = 54, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 915, .adv_w = 46, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 918, .adv_w = 54, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 922, .adv_w = 46, .box_w = 3, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 927, .adv_w = 54, .box_w = 1, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 929, .adv_w = 46, .box_w = 1, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 930, .adv_w = 141, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 937, .adv_w = 95, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 943, .adv_w = 81, .box_w = 5, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 950, .adv_w = 46, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 954, .adv_w = 127, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 963, .adv_w = 98, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 970, .adv_w = 98, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 974, .adv_w = 99, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 981, .adv_w = 45, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 986, .adv_w = 99, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 993, .adv_w = 45, .box_w = 1, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 995, .adv_w = 99, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1001, .adv_w = 45, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1005, .adv_w = 103, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1010, .adv_w = 71, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1014, .adv_w = 101, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1020, .adv_w = 59, .box_w = 3, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1024, .adv_w = 147, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1034, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1039, .adv_w = 147, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1048, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1053, .adv_w = 147, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1063, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1068, .adv_w = 116, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1075, .adv_w = 147, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1084, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1089, .adv_w = 154, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1099, .adv_w = 109, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1105, .adv_w = 154, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1116, .adv_w = 109, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1122, .adv_w = 154, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1132, .adv_w = 109, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1138, .adv_w = 209, .box_w = 12, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1150, .adv_w = 168, .box_w = 10, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1158, .adv_w = 120, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1168, .adv_w = 70, .box_w = 3, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1171, .adv_w = 120, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1180, .adv_w = 70, .box_w = 3, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1183, .adv_w = 120, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1193, .adv_w = 70, .box_w = 3, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1196, .adv_w = 104, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1203, .adv_w = 83, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1207, .adv_w = 104, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1214, .adv_w = 83, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1218, .adv_w = 104, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1225, .adv_w = 83, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1229, .adv_w = 104, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1236, .adv_w = 83, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1240, .adv_w = 113, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1249, .adv_w = 69, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1254, .adv_w = 113, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1264, .adv_w = 69, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1270, .adv_w = 116, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1277, .adv_w = 70, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1281, .adv_w = 141, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1291, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1296, .adv_w = 141, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1305, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1310, .adv_w = 141, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1320, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1325, .adv_w = 141, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1335, .adv_w = 107, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1341, .adv_w = 141, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1350, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1355, .adv_w = 141, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1364, .adv_w = 107, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1370, .adv_w = 199, .box_w = 12, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1387, .adv_w = 151, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1396, .adv_w = 120, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1406, .adv_w = 99, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1414, .adv_w = 120, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1423, .adv_w = 116, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1433, .adv_w = 87, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1438, .adv_w = 116, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1447, .adv_w = 87, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1453, .adv_w = 116, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1463, .adv_w = 87, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1468, .adv_w = 55, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1472, .adv_w = 71, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 1473, .adv_w = 104, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 1474, .adv_w = 111, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 1475, .adv_w = 152, .box_w = 8, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 1476, .adv_w = 152, .box_w = 8, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 1477, .adv_w = 85, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1482, .adv_w = 88, .box_w = 5, .box_h = 2, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1484, .adv_w = 41, .box_w = 1, .box_h = 3, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 1485, .adv_w = 41, .box_w = 1, .box_h = 3, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 1486, .adv_w = 41, .box_w = 1, .box_h = 4, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1487, .adv_w = 41, .box_w = 1, .box_h = 3, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 1488, .adv_w = 70, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 1490, .adv_w = 70, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 1492, .adv_w = 71, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1494, .adv_w = 70, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 1496, .adv_w = 111, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1503, .adv_w = 111, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1510, .adv_w = 111, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1512, .adv_w = 144, .box_w = 7, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1513, .adv_w = 111, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0}
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
const lv_font_t goopad_font_essential_12 = {
#else
lv_font_t goopad_font_essential_12 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 14,          /*The maximum line height required by the font*/
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



#endif /*#if GOOPAD_FONT_ESSENTIAL_12*/
