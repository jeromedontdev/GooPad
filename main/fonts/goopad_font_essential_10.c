/*******************************************************************************
 * Size: 10 px
 * Bpp: 1
 * Lato essential offline fallback; see sources/Lato-Regular.ttf and licenses/lato.txt.
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef GOOPAD_FONT_ESSENTIAL_10
#define GOOPAD_FONT_ESSENTIAL_10 1
#endif

#if GOOPAD_FONT_ESSENTIAL_10

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0x9e,

    /* U+00A2 "¢" */
    0x23, 0xa9, 0x4a, 0x38, 0x80,

    /* U+00A3 "£" */
    0x3a, 0x51, 0xe4, 0x23, 0xe0,

    /* U+00A4 "¤" */
    0xfa, 0xaf,

    /* U+00A5 "¥" */
    0x8a, 0x94, 0xcf, 0xfc, 0x80,

    /* U+00A6 "¦" */
    0xf7, 0x80,

    /* U+00A7 "§" */
    0x68, 0x4b, 0xd2, 0x26,

    /* U+00A8 "¨" */
    0xf0,

    /* U+00A9 "©" */
    0x3c, 0x7a, 0xc1, 0xc1, 0xc1, 0x7a, 0x3c,

    /* U+00AA "ª" */
    0x6d, 0x80,

    /* U+00AB "«" */
    0x17, 0x50,

    /* U+00AC "¬" */
    0xf8, 0x42,

    /* U+00AD "­" */
    0xc0,

    /* U+00AE "®" */
    0x3c, 0x7a, 0xa9, 0xb9, 0xa9, 0x42, 0x3c,

    /* U+00AF "¯" */
    0xc0,

    /* U+00B0 "°" */
    0xe9, 0xe0,

    /* U+00B1 "±" */
    0x27, 0xc8, 0x4f, 0x80,

    /* U+00B2 "²" */
    0x65, 0x70,

    /* U+00B3 "³" */
    0xdb,

    /* U+00B4 "´" */
    0x10,

    /* U+00B5 "µ" */
    0x94, 0xa5, 0x2f, 0xc2, 0x0,

    /* U+00B6 "¶" */
    0xff, 0xae, 0x9a, 0x28, 0xa2, 0x8a,

    /* U+00B7 "·" */
    0x80,

    /* U+00B8 "¸" */
    0xb0,

    /* U+00B9 "¹" */
    0x59, 0x20,

    /* U+00BA "º" */
    0xf5, 0x0,

    /* U+00BB "»" */
    0xa, 0x6a,

    /* U+00BC "¼" */
    0xc8, 0x91, 0x47, 0x82, 0xcb, 0xd1, 0x0,

    /* U+00BD "½" */
    0xc4, 0x91, 0x47, 0xa2, 0x28, 0x93, 0x80,

    /* U+00BE "¾" */
    0xc9, 0x11, 0x47, 0x2, 0xcb, 0xc1, 0x0,

    /* U+00BF "¿" */
    0x20, 0x4, 0x88, 0xe0,

    /* U+00C0 "À" */
    0x20, 0x20, 0x41, 0x82, 0x89, 0x1f, 0x22, 0x82,

    /* U+00C1 "Á" */
    0x8, 0x20, 0x41, 0x82, 0x89, 0x1f, 0x22, 0x82,

    /* U+00C2 "Â" */
    0x10, 0x50, 0x41, 0x82, 0x89, 0x1f, 0x22, 0x82,

    /* U+00C3 "Ã" */
    0x28, 0x30, 0x41, 0x82, 0x89, 0x1f, 0x22, 0x82,

    /* U+00C4 "Ä" */
    0x28, 0x0, 0x41, 0x82, 0x89, 0x1f, 0x22, 0x82,

    /* U+00C5 "Å" */
    0x10, 0x20, 0x41, 0x82, 0x89, 0x1f, 0x22, 0x82,

    /* U+00C6 "Æ" */
    0x1f, 0x8c, 0xa, 0x4, 0xe7, 0xc2, 0x22, 0x1e,

    /* U+00C7 "Ç" */
    0x39, 0x18, 0x20, 0x81, 0x17, 0x84, 0x10,

    /* U+00C8 "È" */
    0x42, 0xf8, 0x8f, 0x88, 0xf0,

    /* U+00C9 "É" */
    0x30, 0xf8, 0x8f, 0x88, 0xf0,

    /* U+00CA "Ê" */
    0x61, 0xf8, 0x8f, 0x88, 0xf0,

    /* U+00CB "Ë" */
    0x50, 0xf8, 0x8f, 0x88, 0xf0,

    /* U+00CC "Ì" */
    0x95, 0x55, 0x40,

    /* U+00CD "Í" */
    0x6a, 0xaa, 0x80,

    /* U+00CE "Î" */
    0xd5, 0x24, 0x92, 0x40,

    /* U+00CF "Ï" */
    0xa1, 0x24, 0x92, 0x40,

    /* U+00D0 "Ð" */
    0x78, 0x89, 0xf, 0x14, 0x28, 0x9e, 0x0,

    /* U+00D1 "Ñ" */
    0x28, 0x48, 0x71, 0xa6, 0x59, 0x63, 0x84,

    /* U+00D2 "Ò" */
    0x20, 0x20, 0xe2, 0x28, 0x30, 0x60, 0xa2, 0x38,

    /* U+00D3 "Ó" */
    0x8, 0x20, 0xe2, 0x28, 0x30, 0x60, 0xa2, 0x38,

    /* U+00D4 "Ô" */
    0x30, 0x50, 0xe2, 0x28, 0x30, 0x60, 0xa2, 0x38,

    /* U+00D5 "Õ" */
    0x30, 0x70, 0xe2, 0x28, 0x30, 0x60, 0xa2, 0x38,

    /* U+00D6 "Ö" */
    0x3c, 0x0, 0xe2, 0x28, 0x30, 0x60, 0xa2, 0x38,

    /* U+00D7 "×" */
    0x96, 0x69,

    /* U+00D8 "Ø" */
    0x3c, 0x8a, 0x2c, 0x9a, 0x28, 0x9e, 0x40,

    /* U+00D9 "Ù" */
    0x20, 0x48, 0x61, 0x86, 0x18, 0x73, 0x78,

    /* U+00DA "Ú" */
    0x10, 0x88, 0x61, 0x86, 0x18, 0x73, 0x78,

    /* U+00DB "Û" */
    0x31, 0x28, 0x61, 0x86, 0x18, 0x73, 0x78,

    /* U+00DC "Ü" */
    0x50, 0x8, 0x61, 0x86, 0x18, 0x73, 0x78,

    /* U+00DD "Ý" */
    0x8, 0x21, 0x12, 0x22, 0x86, 0x4, 0x8, 0x10,

    /* U+00DE "Þ" */
    0x87, 0xa3, 0x18, 0xfa, 0x0,

    /* U+00DF "ß" */
    0x74, 0x65, 0x49, 0xc6, 0xe0,

    /* U+00E0 "à" */
    0x40, 0x71, 0x79, 0xf0,

    /* U+00E1 "á" */
    0x20, 0x71, 0x79, 0xf0,

    /* U+00E2 "â" */
    0x60, 0x71, 0x79, 0xf0,

    /* U+00E3 "ã" */
    0x60, 0x71, 0x79, 0xf0,

    /* U+00E4 "ä" */
    0x50, 0x71, 0x79, 0xf0,

    /* U+00E5 "å" */
    0x22, 0x71, 0x79, 0xf0,

    /* U+00E6 "æ" */
    0x6e, 0x12, 0x7e, 0x90, 0xee,

    /* U+00E7 "ç" */
    0x78, 0x88, 0x72, 0x20,

    /* U+00E8 "è" */
    0x41, 0x1d, 0x2f, 0x41, 0xc0,

    /* U+00E9 "é" */
    0x20, 0x1d, 0x2f, 0x41, 0xc0,

    /* U+00EA "ê" */
    0x22, 0x1d, 0x2f, 0x41, 0xc0,

    /* U+00EB "ë" */
    0x50, 0x1d, 0x2f, 0x41, 0xc0,

    /* U+00EC "ì" */
    0x45, 0x54,

    /* U+00ED "í" */
    0xca, 0xa8,

    /* U+00EE "î" */
    0x55, 0x24, 0x90,

    /* U+00EF "ï" */
    0xe1, 0x24, 0x90,

    /* U+00F0 "ð" */
    0x30, 0x9f, 0x18, 0xc5, 0xc0,

    /* U+00F1 "ñ" */
    0x60, 0xf9, 0x99, 0x90,

    /* U+00F2 "ò" */
    0x20, 0x1d, 0x18, 0xc5, 0xc0,

    /* U+00F3 "ó" */
    0x11, 0x1d, 0x18, 0xc5, 0xc0,

    /* U+00F4 "ô" */
    0x22, 0x9d, 0x18, 0xc5, 0xc0,

    /* U+00F5 "õ" */
    0x70, 0x1d, 0x18, 0xc5, 0xc0,

    /* U+00F6 "ö" */
    0x50, 0x1d, 0x18, 0xc5, 0xc0,

    /* U+00F7 "÷" */
    0x30, 0x3e, 0x3, 0x0,

    /* U+00F8 "ø" */
    0x3, 0xe7, 0x5c, 0xf8, 0x0,

    /* U+00F9 "ù" */
    0x40, 0x99, 0x99, 0xf0,

    /* U+00FA "ú" */
    0x20, 0x99, 0x99, 0xf0,

    /* U+00FB "û" */
    0x60, 0x99, 0x99, 0xf0,

    /* U+00FC "ü" */
    0xa0, 0x99, 0x99, 0xf0,

    /* U+00FD "ý" */
    0x10, 0x22, 0xa5, 0x18, 0x84, 0x40,

    /* U+00FE "þ" */
    0x84, 0x2d, 0x98, 0xc7, 0xd0, 0x80,

    /* U+00FF "ÿ" */
    0x50, 0x22, 0xa5, 0x18, 0x84, 0x40,

    /* U+0100 "Ā" */
    0x38, 0x20, 0xc1, 0x44, 0x8f, 0x91, 0x41,

    /* U+0101 "ā" */
    0x70, 0x71, 0x79, 0xf0,

    /* U+0102 "Ă" */
    0x28, 0x60, 0x41, 0x82, 0x89, 0x1f, 0x22, 0x82,

    /* U+0103 "ă" */
    0xa6, 0xf1, 0x79, 0xf0,

    /* U+0104 "Ą" */
    0x30, 0x60, 0xa2, 0x44, 0x9f, 0xa1, 0x2, 0x6,

    /* U+0105 "ą" */
    0x70, 0x9d, 0x2f, 0x10, 0x60,

    /* U+0106 "Ć" */
    0x8, 0x43, 0x90, 0x82, 0x8, 0x11, 0x78,

    /* U+0107 "ć" */
    0x20, 0x78, 0x88, 0x70,

    /* U+0108 "Ĉ" */
    0x10, 0xa3, 0x91, 0x82, 0x8, 0x11, 0x78,

    /* U+0109 "ĉ" */
    0x24, 0x78, 0x88, 0x70,

    /* U+010A "Ċ" */
    0x18, 0x3, 0x91, 0x82, 0x8, 0x11, 0x78,

    /* U+010B "ċ" */
    0x20, 0x78, 0x88, 0x70,

    /* U+010C "Č" */
    0x28, 0x43, 0x91, 0x82, 0x8, 0x11, 0x78,

    /* U+010D "č" */
    0x52, 0x78, 0x88, 0x70,

    /* U+010E "Ď" */
    0x50, 0x8f, 0x22, 0x86, 0x18, 0x62, 0xf0,

    /* U+010F "ď" */
    0xc, 0x11, 0xe4, 0x48, 0x91, 0x1e, 0x0,

    /* U+0110 "Đ" */
    0x78, 0x89, 0xf, 0x14, 0x28, 0x9e, 0x0,

    /* U+0111 "đ" */
    0x8, 0x77, 0xa2, 0x8a, 0x27, 0x80,

    /* U+0112 "Ē" */
    0x7f, 0x88, 0xf8, 0x8f,

    /* U+0113 "ē" */
    0x70, 0x1d, 0x2f, 0x41, 0xc0,

    /* U+0114 "Ĕ" */
    0x56, 0xf8, 0x8f, 0x88, 0xf0,

    /* U+0115 "ĕ" */
    0x70, 0x1d, 0x2f, 0x41, 0xc0,

    /* U+0116 "Ė" */
    0x20, 0xf8, 0x8f, 0x88, 0xf0,

    /* U+0117 "ė" */
    0x20, 0x1d, 0x2f, 0x41, 0xc0,

    /* U+0118 "Ę" */
    0xf8, 0x8f, 0x88, 0xf2, 0x10,

    /* U+0119 "ę" */
    0x74, 0xbd, 0x7, 0x20, 0xc0,

    /* U+011A "Ě" */
    0x52, 0xf8, 0x8f, 0x88, 0xf0,

    /* U+011B "ě" */
    0x51, 0x1d, 0x2f, 0x41, 0xc0,

    /* U+011C "Ĝ" */
    0x10, 0xa3, 0x91, 0x82, 0x38, 0x51, 0x38,

    /* U+011D "ĝ" */
    0x60, 0x1f, 0x29, 0x33, 0xd2, 0xf0,

    /* U+011E "Ğ" */
    0x28, 0xc3, 0x91, 0x82, 0x38, 0x51, 0x38,

    /* U+011F "ğ" */
    0x63, 0x1f, 0x29, 0x33, 0xd2, 0xf0,

    /* U+0120 "Ġ" */
    0x8, 0x3, 0x91, 0x82, 0x38, 0x51, 0x38,

    /* U+0121 "ġ" */
    0x20, 0x1f, 0x29, 0x33, 0xd2, 0xf0,

    /* U+0122 "Ģ" */
    0x39, 0x18, 0x20, 0x8d, 0x13, 0x80, 0x20,

    /* U+0123 "ģ" */
    0x20, 0x1f, 0x29, 0x33, 0xd2, 0xf0,

    /* U+0124 "Ĥ" */
    0x31, 0x28, 0x61, 0x87, 0xf8, 0x61, 0x84,

    /* U+0125 "ĥ" */
    0x45, 0x10, 0x87, 0xa5, 0x29, 0x48,

    /* U+0126 "Ħ" */
    0x42, 0xff, 0x42, 0x7e, 0x42, 0x42, 0x42,

    /* U+0127 "ħ" */
    0x47, 0x1e, 0x94, 0xa5, 0x20,

    /* U+0128 "Ĩ" */
    0xdd, 0x24, 0x92, 0x40,

    /* U+0129 "ĩ" */
    0xe1, 0x24, 0x90,

    /* U+012A "Ī" */
    0xe9, 0x24, 0x92,

    /* U+012B "ī" */
    0xe1, 0x24, 0x90,

    /* U+012C "Ĭ" */
    0xbd, 0x24, 0x92, 0x40,

    /* U+012D "ĭ" */
    0xad, 0x24, 0x90,

    /* U+012E "Į" */
    0x55, 0x56, 0x40,

    /* U+012F "į" */
    0x45, 0x56, 0x40,

    /* U+0130 "İ" */
    0xbf, 0x80,

    /* U+0131 "ı" */
    0xf8,

    /* U+0132 "Ĳ" */
    0x86, 0x18, 0x61, 0x86, 0x1b, 0x80,

    /* U+0133 "ĳ" */
    0xa0, 0x99, 0x99, 0x91, 0x30,

    /* U+0134 "Ĵ" */
    0x65, 0x22, 0x22, 0x22, 0xc0,

    /* U+0135 "ĵ" */
    0xc5, 0x24, 0x92, 0xc0,

    /* U+0136 "Ķ" */
    0x8a, 0x4a, 0x38, 0xb2, 0x48, 0x80, 0x20,

    /* U+0137 "ķ" */
    0x88, 0x9a, 0xca, 0x90, 0x40,

    /* U+0138 "ĸ" */
    0x9a, 0xca, 0x90,

    /* U+0139 "Ĺ" */
    0x48, 0x88, 0x88, 0x88, 0xf0,

    /* U+013A "ĺ" */
    0x6a, 0xaa, 0x80,

    /* U+013B "Ļ" */
    0x88, 0x88, 0x88, 0xf0, 0x40,

    /* U+013C "ļ" */
    0xfe, 0x80,

    /* U+013D "Ľ" */
    0xa8, 0x88, 0x88, 0xf0,

    /* U+013E "ľ" */
    0xd2, 0x49, 0x20,

    /* U+013F "Ŀ" */
    0x88, 0x8b, 0x88, 0xf0,

    /* U+0140 "ŀ" */
    0x92, 0x79, 0x20,

    /* U+0141 "Ł" */
    0x42, 0x14, 0xc4, 0x21, 0xe0,

    /* U+0142 "ł" */
    0x49, 0xe4, 0x90,

    /* U+0143 "Ń" */
    0x10, 0x88, 0x71, 0xa6, 0x59, 0x63, 0x84,

    /* U+0144 "ń" */
    0x20, 0xf9, 0x99, 0x90,

    /* U+0145 "Ņ" */
    0x87, 0x1a, 0x65, 0x96, 0x38, 0x40, 0x20,

    /* U+0146 "ņ" */
    0xf9, 0x99, 0x90, 0x40,

    /* U+0147 "Ň" */
    0x30, 0x8, 0x71, 0xa6, 0x59, 0x63, 0x84,

    /* U+0148 "ň" */
    0x60, 0xf9, 0x99, 0x90,

    /* U+0149 "ŉ" */
    0x84, 0x1e, 0x94, 0xa5, 0x20,

    /* U+014A "Ŋ" */
    0x87, 0x1a, 0x69, 0x96, 0x38, 0x41, 0x1c,

    /* U+014B "ŋ" */
    0xf9, 0x99, 0x91, 0x30,

    /* U+014C "Ō" */
    0x38, 0x0, 0xe2, 0x28, 0x30, 0x60, 0xa2, 0x38,

    /* U+014D "ō" */
    0x70, 0x1d, 0x18, 0xc5, 0xc0,

    /* U+014E "Ŏ" */
    0x28, 0x70, 0xe2, 0x28, 0x30, 0x60, 0xa2, 0x38,

    /* U+014F "ŏ" */
    0x70, 0x1d, 0x18, 0xc5, 0xc0,

    /* U+0150 "Ő" */
    0x18, 0x0, 0xe2, 0x28, 0x30, 0x60, 0xa2, 0x38,

    /* U+0151 "ő" */
    0x30, 0x1d, 0x18, 0xc5, 0xc0,

    /* U+0152 "Œ" */
    0x3b, 0xd1, 0x88, 0x22, 0xf, 0x82, 0x11, 0x87,
    0xbc,

    /* U+0153 "œ" */
    0x66, 0x99, 0x9f, 0x98, 0x66,

    /* U+0154 "Ŕ" */
    0x31, 0xf, 0x22, 0x8b, 0xc9, 0x26, 0x88,

    /* U+0155 "ŕ" */
    0x43, 0xc9, 0x20,

    /* U+0156 "Ŗ" */
    0xf2, 0x28, 0xbc, 0x92, 0x68, 0x80, 0x20,

    /* U+0157 "ŗ" */
    0xf2, 0x48, 0x20,

    /* U+0158 "Ř" */
    0x50, 0x8f, 0x22, 0x8b, 0xc9, 0x26, 0x88,

    /* U+0159 "ř" */
    0xab, 0xc9, 0x20,

    /* U+015A "Ś" */
    0x11, 0x1c, 0x94, 0x18, 0x31, 0x70,

    /* U+015B "ś" */
    0x20, 0xe8, 0x42, 0xe0,

    /* U+015C "Ŝ" */
    0x32, 0x5c, 0x94, 0x18, 0x31, 0x70,

    /* U+015D "ŝ" */
    0x60, 0xe8, 0x42, 0xe0,

    /* U+015E "Ş" */
    0x72, 0x50, 0x60, 0xc5, 0xc4, 0x30,

    /* U+015F "ş" */
    0xe8, 0x62, 0xe4, 0x60,

    /* U+0160 "Š" */
    0x51, 0x1c, 0x94, 0x18, 0x31, 0x70,

    /* U+0161 "š" */
    0xa4, 0xe8, 0x42, 0xe0,

    /* U+0162 "Ţ" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x20,

    /* U+0163 "ţ" */
    0x4, 0xe4, 0x44, 0x72, 0x20,

    /* U+0164 "Ť" */
    0x51, 0x3e, 0x42, 0x10, 0x84, 0x20,

    /* U+0165 "ť" */
    0x54, 0xe4, 0x44, 0x60,

    /* U+0166 "Ŧ" */
    0xf9, 0x9, 0xf2, 0x10, 0x80,

    /* U+0167 "ŧ" */
    0x4, 0xee, 0x44, 0x60,

    /* U+0168 "Ũ" */
    0x29, 0x48, 0x61, 0x86, 0x18, 0x73, 0x78,

    /* U+0169 "ũ" */
    0x60, 0x99, 0x99, 0xf0,

    /* U+016A "Ū" */
    0x30, 0x8, 0x61, 0x86, 0x18, 0x73, 0x78,

    /* U+016B "ū" */
    0x60, 0x99, 0x99, 0xf0,

    /* U+016C "Ŭ" */
    0x48, 0xc8, 0x61, 0x86, 0x18, 0x73, 0x78,

    /* U+016D "ŭ" */
    0xa6, 0x99, 0x99, 0xf0,

    /* U+016E "Ů" */
    0x21, 0x42, 0x21, 0x86, 0x18, 0x61, 0xcd, 0xe0,

    /* U+016F "ů" */
    0x66, 0x99, 0x99, 0xf0,

    /* U+0170 "Ű" */
    0x28, 0x8, 0x61, 0x86, 0x18, 0x73, 0x78,

    /* U+0171 "ű" */
    0x60, 0x99, 0x99, 0xf0,

    /* U+0172 "Ų" */
    0x86, 0x18, 0x61, 0x87, 0x37, 0x88, 0x30,

    /* U+0173 "ų" */
    0x94, 0xa5, 0x2f, 0x10, 0x60,

    /* U+0174 "Ŵ" */
    0xc, 0x0, 0x88, 0x45, 0x31, 0x4c, 0x94, 0xa3,
    0x28, 0xcc, 0x21, 0x0,

    /* U+0175 "ŵ" */
    0x18, 0x0, 0x99, 0x5a, 0x6a, 0x6a, 0x24,

    /* U+0176 "Ŷ" */
    0x30, 0x51, 0x12, 0x22, 0x86, 0x4, 0x8, 0x10,

    /* U+0177 "ŷ" */
    0x30, 0x22, 0xa5, 0x18, 0x84, 0x40,

    /* U+0178 "Ÿ" */
    0x28, 0x1, 0x12, 0x22, 0x86, 0x4, 0x8, 0x10,

    /* U+0179 "Ź" */
    0x10, 0x8f, 0xc2, 0x10, 0x82, 0x10, 0xfc,

    /* U+017A "ź" */
    0x20, 0xf2, 0x24, 0xf0,

    /* U+017B "Ż" */
    0x30, 0xf, 0xc2, 0x10, 0x82, 0x10, 0xfc,

    /* U+017C "ż" */
    0x20, 0xf2, 0x24, 0xf0,

    /* U+017D "Ž" */
    0x30, 0x4f, 0xc2, 0x10, 0x82, 0x10, 0xfc,

    /* U+017E "ž" */
    0x60, 0xf2, 0x24, 0xf0,

    /* U+017F "ſ" */
    0x6b, 0x24, 0x90,

    /* U+2010 "‐" */
    0xc0,

    /* U+2012 "‒" */
    0xf0,

    /* U+2013 "–" */
    0xf0,

    /* U+2014 "—" */
    0xfc,

    /* U+2015 "―" */
    0xfc,

    /* U+2016 "‖" */
    0xb6, 0xdb, 0x6d, 0xa0,

    /* U+2017 "‗" */
    0xff,

    /* U+2018 "‘" */
    0xa0,

    /* U+2019 "’" */
    0xc0,

    /* U+201A "‚" */
    0x50,

    /* U+201B "‛" */
    0xa0,

    /* U+201C "“" */
    0xb4,

    /* U+201D "”" */
    0xf0,

    /* U+201E "„" */
    0x6c, 0x0,

    /* U+201F "‟" */
    0xb4,

    /* U+2020 "†" */
    0x21, 0x3e, 0x42, 0x10, 0x84, 0x20,

    /* U+2021 "‡" */
    0x21, 0x3e, 0x42, 0x13, 0xe4, 0x20,

    /* U+2022 "•" */
    0x6f, 0x60,

    /* U+2026 "…" */
    0xa4,

    /* U+20AC "€" */
    0x39, 0xf, 0xbc, 0x41, 0x3, 0x80
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 41, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 40, .box_w = 1, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2, .adv_w = 93, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 7, .adv_w = 93, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 12, .adv_w = 93, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 14, .adv_w = 93, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 19, .adv_w = 41, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 21, .adv_w = 79, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 25, .adv_w = 64, .box_w = 4, .box_h = 1, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 26, .adv_w = 133, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 33, .adv_w = 59, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 35, .adv_w = 69, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 37, .adv_w = 93, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 39, .adv_w = 59, .box_w = 2, .box_h = 1, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 40, .adv_w = 133, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 47, .adv_w = 64, .box_w = 2, .box_h = 1, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 48, .adv_w = 66, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 50, .adv_w = 93, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 54, .adv_w = 53, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 56, .adv_w = 53, .box_w = 2, .box_h = 4, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 57, .adv_w = 64, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 58, .adv_w = 104, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 63, .adv_w = 112, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 69, .adv_w = 42, .box_w = 1, .box_h = 1, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 70, .adv_w = 64, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 71, .adv_w = 53, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 73, .adv_w = 64, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 75, .adv_w = 69, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 77, .adv_w = 117, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 84, .adv_w = 116, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 91, .adv_w = 119, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 98, .adv_w = 70, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 102, .adv_w = 108, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 110, .adv_w = 108, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 118, .adv_w = 108, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 126, .adv_w = 108, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 134, .adv_w = 108, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 142, .adv_w = 108, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 150, .adv_w = 149, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 158, .adv_w = 107, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 165, .adv_w = 92, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 170, .adv_w = 92, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 175, .adv_w = 92, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 180, .adv_w = 92, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 185, .adv_w = 45, .box_w = 2, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 188, .adv_w = 45, .box_w = 2, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 191, .adv_w = 45, .box_w = 3, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 195, .adv_w = 45, .box_w = 3, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 199, .adv_w = 123, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 206, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 213, .adv_w = 128, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 221, .adv_w = 128, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 229, .adv_w = 128, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 237, .adv_w = 128, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 245, .adv_w = 128, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 253, .adv_w = 93, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 255, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 262, .adv_w = 118, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 269, .adv_w = 118, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 276, .adv_w = 118, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 283, .adv_w = 118, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 290, .adv_w = 100, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 298, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 303, .adv_w = 93, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 308, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 312, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 316, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 320, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 324, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 328, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 332, .adv_w = 129, .box_w = 8, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 337, .adv_w = 76, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 341, .adv_w = 84, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 346, .adv_w = 84, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 351, .adv_w = 84, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 356, .adv_w = 84, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 361, .adv_w = 38, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 363, .adv_w = 38, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 365, .adv_w = 38, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 368, .adv_w = 38, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 371, .adv_w = 91, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 376, .adv_w = 89, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 380, .adv_w = 91, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 385, .adv_w = 91, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 390, .adv_w = 91, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 395, .adv_w = 91, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 400, .adv_w = 91, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 405, .adv_w = 93, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 409, .adv_w = 91, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 414, .adv_w = 89, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 418, .adv_w = 89, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 422, .adv_w = 89, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 426, .adv_w = 89, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 430, .adv_w = 82, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 436, .adv_w = 90, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 442, .adv_w = 82, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 448, .adv_w = 108, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 455, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 459, .adv_w = 108, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 467, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 471, .adv_w = 108, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 479, .adv_w = 80, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 484, .adv_w = 107, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 491, .adv_w = 76, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 495, .adv_w = 107, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 502, .adv_w = 76, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 506, .adv_w = 107, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 513, .adv_w = 76, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 517, .adv_w = 107, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 524, .adv_w = 76, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 528, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 535, .adv_w = 90, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 542, .adv_w = 123, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 549, .adv_w = 90, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 555, .adv_w = 92, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 559, .adv_w = 84, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 564, .adv_w = 92, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 569, .adv_w = 84, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 574, .adv_w = 92, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 579, .adv_w = 84, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 584, .adv_w = 92, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 589, .adv_w = 84, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 594, .adv_w = 92, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 599, .adv_w = 84, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 604, .adv_w = 117, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 611, .adv_w = 83, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 617, .adv_w = 117, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 624, .adv_w = 83, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 630, .adv_w = 117, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 637, .adv_w = 83, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 643, .adv_w = 117, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 650, .adv_w = 83, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 656, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 663, .adv_w = 89, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 669, .adv_w = 123, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 676, .adv_w = 89, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 681, .adv_w = 45, .box_w = 3, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 685, .adv_w = 38, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 688, .adv_w = 45, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 691, .adv_w = 38, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 694, .adv_w = 45, .box_w = 3, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 698, .adv_w = 38, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 701, .adv_w = 45, .box_w = 2, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 704, .adv_w = 38, .box_w = 2, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 707, .adv_w = 45, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 709, .adv_w = 38, .box_w = 1, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 710, .adv_w = 118, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 716, .adv_w = 79, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 721, .adv_w = 68, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 726, .adv_w = 38, .box_w = 3, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 730, .adv_w = 106, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 737, .adv_w = 81, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 742, .adv_w = 81, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 745, .adv_w = 82, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 750, .adv_w = 38, .box_w = 2, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 753, .adv_w = 82, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 758, .adv_w = 38, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 760, .adv_w = 82, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 764, .adv_w = 38, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 767, .adv_w = 85, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 771, .adv_w = 59, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 774, .adv_w = 84, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 779, .adv_w = 49, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 782, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 789, .adv_w = 89, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 793, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 800, .adv_w = 89, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 804, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 811, .adv_w = 89, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 815, .adv_w = 97, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 820, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 827, .adv_w = 89, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 831, .adv_w = 128, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 839, .adv_w = 91, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 844, .adv_w = 128, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 852, .adv_w = 91, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 857, .adv_w = 128, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 865, .adv_w = 91, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 870, .adv_w = 174, .box_w = 10, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 879, .adv_w = 140, .box_w = 8, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 884, .adv_w = 100, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 891, .adv_w = 58, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 894, .adv_w = 100, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 901, .adv_w = 58, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 904, .adv_w = 100, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 911, .adv_w = 58, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 914, .adv_w = 87, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 920, .adv_w = 69, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 924, .adv_w = 87, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 930, .adv_w = 69, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 934, .adv_w = 87, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 940, .adv_w = 69, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 944, .adv_w = 87, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 950, .adv_w = 69, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 954, .adv_w = 94, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 960, .adv_w = 57, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 965, .adv_w = 94, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 971, .adv_w = 57, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 975, .adv_w = 97, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 980, .adv_w = 58, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 984, .adv_w = 118, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 991, .adv_w = 89, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 995, .adv_w = 118, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1002, .adv_w = 89, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1006, .adv_w = 118, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1013, .adv_w = 89, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1017, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1025, .adv_w = 89, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1029, .adv_w = 118, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1036, .adv_w = 89, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1040, .adv_w = 118, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1047, .adv_w = 89, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1052, .adv_w = 166, .box_w = 10, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1064, .adv_w = 126, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1071, .adv_w = 100, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1079, .adv_w = 82, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1085, .adv_w = 100, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1093, .adv_w = 96, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1100, .adv_w = 72, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1104, .adv_w = 96, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1111, .adv_w = 72, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1115, .adv_w = 96, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1122, .adv_w = 72, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1126, .adv_w = 45, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1129, .adv_w = 59, .box_w = 2, .box_h = 1, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1130, .adv_w = 86, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1131, .adv_w = 93, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1132, .adv_w = 127, .box_w = 6, .box_h = 1, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1133, .adv_w = 127, .box_w = 6, .box_h = 1, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1134, .adv_w = 71, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1138, .adv_w = 74, .box_w = 4, .box_h = 2, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1139, .adv_w = 34, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 1140, .adv_w = 34, .box_w = 1, .box_h = 2, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 1141, .adv_w = 34, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1142, .adv_w = 34, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 1143, .adv_w = 58, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 1144, .adv_w = 58, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 1145, .adv_w = 59, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1147, .adv_w = 58, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 1148, .adv_w = 93, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1154, .adv_w = 93, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1160, .adv_w = 93, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1162, .adv_w = 120, .box_w = 6, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1163, .adv_w = 93, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0}
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
const lv_font_t goopad_font_essential_10 = {
#else
lv_font_t goopad_font_essential_10 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 12,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if GOOPAD_FONT_ESSENTIAL_10*/
