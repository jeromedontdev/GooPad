/*******************************************************************************
 * Size: 8 px
 * Bpp: 1
 * Lato essential offline fallback; see sources/Lato-Regular.ttf and licenses/lato.txt.
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef GOOPAD_FONT_ESSENTIAL_8
#define GOOPAD_FONT_ESSENTIAL_8 1
#endif

#if GOOPAD_FONT_ESSENTIAL_8

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0x55, 0x40,

    /* U+00A2 "¢" */
    0x27, 0xaa, 0x72,

    /* U+00A3 "£" */
    0x32, 0x11, 0xe4, 0x7c,

    /* U+00A4 "¤" */
    0xf7, 0x80,

    /* U+00A5 "¥" */
    0x8a, 0x94, 0xe7, 0x10,

    /* U+00A6 "¦" */
    0xee,

    /* U+00A7 "§" */
    0x64, 0xa6, 0x26,

    /* U+00A8 "¨" */
    0xa0,

    /* U+00A9 "©" */
    0x33, 0xec, 0x71, 0xb9, 0xc0,

    /* U+00AA "ª" */
    0xf8,

    /* U+00AB "«" */
    0x3c,

    /* U+00AC "¬" */
    0xf1,

    /* U+00AD "­" */
    0xc0,

    /* U+00AE "®" */
    0x33, 0xeb, 0x69, 0x89, 0xc0,

    /* U+00AF "¯" */
    0xe0,

    /* U+00B0 "°" */
    0x55, 0x0,

    /* U+00B1 "±" */
    0x27, 0xc9, 0xf0,

    /* U+00B2 "²" */
    0xd8,

    /* U+00B3 "³" */
    0xdc,

    /* U+00B4 "´" */
    0x80,

    /* U+00B5 "µ" */
    0x94, 0xa5, 0xf8, 0x0,

    /* U+00B6 "¶" */
    0x7f, 0xa6, 0x8a, 0x28, 0xa2, 0x80,

    /* U+00B7 "·" */
    0xc0,

    /* U+00B8 "¸" */
    0xc0,

    /* U+00B9 "¹" */
    0x54,

    /* U+00BA "º" */
    0xf5, 0x0,

    /* U+00BB "»" */
    0x3c,

    /* U+00BC "¼" */
    0xd2, 0xb8, 0xb5, 0x80,

    /* U+00BD "½" */
    0x49, 0x4f, 0xa, 0x48, 0x30,

    /* U+00BE "¾" */
    0x8a, 0xa9, 0x45, 0xcc,

    /* U+00BF "¿" */
    0x41, 0x4e,

    /* U+00C0 "À" */
    0x21, 0xc, 0xa5, 0x3e, 0x20,

    /* U+00C1 "Á" */
    0x11, 0xc, 0xa5, 0x3e, 0x20,

    /* U+00C2 "Â" */
    0x31, 0xc, 0xa5, 0x3e, 0x20,

    /* U+00C3 "Ã" */
    0x31, 0xc, 0xa5, 0x3e, 0x20,

    /* U+00C4 "Ä" */
    0x51, 0xc, 0xa5, 0x3e, 0x20,

    /* U+00C5 "Å" */
    0x21, 0x8, 0x65, 0x29, 0xf1,

    /* U+00C6 "Æ" */
    0x1e, 0x60, 0xc2, 0x77, 0x91, 0xc0,

    /* U+00C7 "Ç" */
    0x7c, 0x21, 0x8, 0x3c, 0x40,

    /* U+00C8 "È" */
    0x4f, 0x88, 0xe8, 0xf0,

    /* U+00C9 "É" */
    0x2f, 0x88, 0xe8, 0xf0,

    /* U+00CA "Ê" */
    0x6f, 0x88, 0xe8, 0xf0,

    /* U+00CB "Ë" */
    0xaf, 0x88, 0xe8, 0xf0,

    /* U+00CC "Ì" */
    0x55, 0x54,

    /* U+00CD "Í" */
    0xaa, 0xa8,

    /* U+00CE "Î" */
    0xc9, 0x24, 0x90,

    /* U+00CF "Ï" */
    0x95, 0x54,

    /* U+00D0 "Ð" */
    0x79, 0x14, 0x79, 0x45, 0xe0,

    /* U+00D1 "Ñ" */
    0x74, 0x73, 0x5a, 0xce, 0x20,

    /* U+00D2 "Ò" */
    0x0, 0x87, 0xa1, 0x86, 0x18, 0x5e,

    /* U+00D3 "Ó" */
    0x8, 0x47, 0xa1, 0x86, 0x18, 0x5e,

    /* U+00D4 "Ô" */
    0x31, 0xe8, 0x61, 0x86, 0x17, 0x80,

    /* U+00D5 "Õ" */
    0x31, 0xe8, 0x61, 0x86, 0x17, 0x80,

    /* U+00D6 "Ö" */
    0x21, 0xe8, 0x61, 0x86, 0x17, 0x80,

    /* U+00D7 "×" */
    0xaa, 0x80,

    /* U+00D8 "Ø" */
    0x7a, 0x59, 0x69, 0xc5, 0xe8, 0x0,

    /* U+00D9 "Ù" */
    0x41, 0x23, 0x18, 0xc6, 0x2e,

    /* U+00DA "Ú" */
    0x11, 0x23, 0x18, 0xc6, 0x2e,

    /* U+00DB "Û" */
    0x64, 0x63, 0x18, 0xc5, 0xc0,

    /* U+00DC "Ü" */
    0x64, 0x63, 0x18, 0xc5, 0xc0,

    /* U+00DD "Ý" */
    0x24, 0x54, 0xa2, 0x10, 0x80,

    /* U+00DE "Þ" */
    0x8e, 0x99, 0xe8,

    /* U+00DF "ß" */
    0x69, 0xaa, 0x9f,

    /* U+00E0 "à" */
    0x43, 0x97, 0xc0,

    /* U+00E1 "á" */
    0x43, 0x97, 0xc0,

    /* U+00E2 "â" */
    0x43, 0x97, 0xc0,

    /* U+00E3 "ã" */
    0xbb, 0x97, 0xc0,

    /* U+00E4 "ä" */
    0x63, 0x97, 0xc0,

    /* U+00E5 "å" */
    0x6b, 0x97, 0xc0,

    /* U+00E6 "æ" */
    0x7d, 0xf9, 0x3f,

    /* U+00E7 "ç" */
    0x78, 0x86, 0x20,

    /* U+00E8 "è" */
    0x40, 0x6f, 0x86,

    /* U+00E9 "é" */
    0x20, 0x6f, 0x86,

    /* U+00EA "ê" */
    0x60, 0x6f, 0x86,

    /* U+00EB "ë" */
    0x40, 0x6f, 0x86,

    /* U+00EC "ì" */
    0x45, 0x50,

    /* U+00ED "í" */
    0x8a, 0xa0,

    /* U+00EE "î" */
    0x41, 0x24, 0x80,

    /* U+00EF "ï" */
    0xca, 0xa0,

    /* U+00F0 "ð" */
    0x42, 0x79, 0x96,

    /* U+00F1 "ñ" */
    0x62, 0xf9, 0x99,

    /* U+00F2 "ò" */
    0x40, 0x69, 0x96,

    /* U+00F3 "ó" */
    0x20, 0x69, 0x96,

    /* U+00F4 "ô" */
    0x60, 0x69, 0x96,

    /* U+00F5 "õ" */
    0x42, 0x69, 0x96,

    /* U+00F6 "ö" */
    0x40, 0x69, 0x96,

    /* U+00F7 "÷" */
    0x20, 0xf2,

    /* U+00F8 "ø" */
    0x7, 0xdd, 0x60,

    /* U+00F9 "ù" */
    0x40, 0x99, 0x9f,

    /* U+00FA "ú" */
    0x20, 0x99, 0x9f,

    /* U+00FB "û" */
    0x60, 0x99, 0x9f,

    /* U+00FC "ü" */
    0x40, 0x99, 0x9f,

    /* U+00FD "ý" */
    0x20, 0x96, 0x64, 0x40,

    /* U+00FE "þ" */
    0x88, 0xf9, 0x9e, 0x80,

    /* U+00FF "ÿ" */
    0x60, 0x96, 0x64, 0x40,

    /* U+0100 "Ā" */
    0x31, 0xc, 0xa5, 0x3e, 0x20,

    /* U+0101 "ā" */
    0xe3, 0x97, 0xc0,

    /* U+0102 "Ă" */
    0x31, 0xc, 0xa5, 0x3e, 0x20,

    /* U+0103 "ă" */
    0xab, 0x97, 0xc0,

    /* U+0104 "Ą" */
    0x21, 0x45, 0x1c, 0x8a, 0x20, 0xc0,

    /* U+0105 "ą" */
    0xee, 0xf2,

    /* U+0106 "Ć" */
    0x13, 0xe3, 0x8, 0x41, 0xe0,

    /* U+0107 "ć" */
    0x20, 0x78, 0x86,

    /* U+0108 "Ĉ" */
    0x33, 0xe1, 0x8, 0x41, 0xe0,

    /* U+0109 "ĉ" */
    0x60, 0x78, 0x86,

    /* U+010A "Ċ" */
    0x13, 0xe1, 0x8, 0x41, 0xe0,

    /* U+010B "ċ" */
    0x20, 0x78, 0x86,

    /* U+010C "Č" */
    0x33, 0xe1, 0x8, 0x41, 0xe0,

    /* U+010D "č" */
    0x60, 0x78, 0x86,

    /* U+010E "Ď" */
    0x67, 0xa3, 0x18, 0xc7, 0xc0,

    /* U+010F "ď" */
    0x18, 0x9d, 0x29, 0x78,

    /* U+0110 "Đ" */
    0x79, 0x14, 0x79, 0x45, 0xe0,

    /* U+0111 "đ" */
    0x11, 0xdd, 0x29, 0x78,

    /* U+0112 "Ē" */
    0x6f, 0x88, 0xe8, 0xf0,

    /* U+0113 "ē" */
    0x60, 0x6f, 0x86,

    /* U+0114 "Ĕ" */
    0x6f, 0x88, 0xe8, 0xf0,

    /* U+0115 "ĕ" */
    0xa6, 0x6f, 0x86,

    /* U+0116 "Ė" */
    0x4f, 0x88, 0xe8, 0xf0,

    /* U+0117 "ė" */
    0x20, 0x6f, 0x86,

    /* U+0118 "Ę" */
    0xf8, 0xe8, 0x8f, 0x20,

    /* U+0119 "ę" */
    0x6f, 0x86, 0x40,

    /* U+011A "Ě" */
    0x6f, 0x88, 0xe8, 0xf0,

    /* U+011B "ě" */
    0x60, 0x6f, 0x86,

    /* U+011C "Ĝ" */
    0x33, 0xe1, 0x9, 0xc5, 0xe0,

    /* U+011D "ĝ" */
    0x40, 0xe6, 0x8e, 0xe0,

    /* U+011E "Ğ" */
    0x33, 0xe1, 0x9, 0xc5, 0xe0,

    /* U+011F "ğ" */
    0x40, 0xe6, 0x8e, 0xe0,

    /* U+0120 "Ġ" */
    0x13, 0xe1, 0x9, 0xc5, 0xe0,

    /* U+0121 "ġ" */
    0x20, 0xe6, 0x8e, 0xe0,

    /* U+0122 "Ģ" */
    0x7c, 0x21, 0x38, 0xbc, 0x80,

    /* U+0123 "ģ" */
    0x40, 0xe6, 0x8e, 0xe0,

    /* U+0124 "Ĥ" */
    0x64, 0x63, 0x1f, 0xc6, 0x20,

    /* U+0125 "ĥ" */
    0xc2, 0x10, 0xf4, 0xa5, 0x20,

    /* U+0126 "Ħ" */
    0x45, 0xfd, 0x13, 0xe4, 0x48, 0x80,

    /* U+0127 "ħ" */
    0x47, 0x1e, 0x94, 0xa4,

    /* U+0128 "Ĩ" */
    0xe9, 0x24, 0x90,

    /* U+0129 "ĩ" */
    0xf1, 0x24, 0x80,

    /* U+012A "Ī" */
    0xe9, 0x24, 0x90,

    /* U+012B "ī" */
    0xe1, 0x24, 0x80,

    /* U+012C "Ĭ" */
    0x69, 0x24, 0x90,

    /* U+012D "ĭ" */
    0xa9, 0x24, 0x80,

    /* U+012E "Į" */
    0xfe,

    /* U+012F "į" */
    0xbe,

    /* U+0130 "İ" */
    0xfe,

    /* U+0131 "ı" */
    0xf0,

    /* U+0132 "Ĳ" */
    0x8c, 0x63, 0x18, 0xd8,

    /* U+0133 "ĳ" */
    0xb0, 0xaa, 0xaa, 0x60,

    /* U+0134 "Ĵ" */
    0x62, 0x22, 0x22, 0xc0,

    /* U+0135 "ĵ" */
    0xc1, 0x24, 0xb0,

    /* U+0136 "Ķ" */
    0x95, 0x31, 0x49, 0x49, 0x0,

    /* U+0137 "ķ" */
    0x88, 0xac, 0xaa, 0x40,

    /* U+0138 "ĸ" */
    0xac, 0xaa,

    /* U+0139 "Ĺ" */
    0x92, 0x49, 0x38,

    /* U+013A "ĺ" */
    0xaa, 0xa8,

    /* U+013B "Ļ" */
    0x92, 0x49, 0xd0,

    /* U+013C "ļ" */
    0xfe,

    /* U+013D "Ľ" */
    0xb2, 0x49, 0xc0,

    /* U+013E "ľ" */
    0xea, 0xa0,

    /* U+013F "Ŀ" */
    0x92, 0xc9, 0xc0,

    /* U+0140 "ŀ" */
    0x92, 0x59, 0x0,

    /* U+0141 "Ł" */
    0x44, 0x6c, 0x47,

    /* U+0142 "ł" */
    0x49, 0xa4, 0x80,

    /* U+0143 "Ń" */
    0x24, 0x73, 0x5a, 0xce, 0x20,

    /* U+0144 "ń" */
    0x20, 0xf9, 0x99,

    /* U+0145 "Ņ" */
    0x8e, 0x6b, 0x59, 0xc4, 0x80,

    /* U+0146 "ņ" */
    0xf9, 0x99, 0x40,

    /* U+0147 "Ň" */
    0x64, 0x73, 0x5a, 0xce, 0x20,

    /* U+0148 "ň" */
    0x60, 0xf9, 0x99,

    /* U+0149 "ŉ" */
    0x84, 0x1e, 0x94, 0xa4,

    /* U+014A "Ŋ" */
    0x8e, 0x6b, 0x59, 0xc4, 0x60,

    /* U+014B "ŋ" */
    0xf9, 0x99, 0x30,

    /* U+014C "Ō" */
    0x31, 0xe8, 0x61, 0x86, 0x17, 0x80,

    /* U+014D "ō" */
    0x60, 0x69, 0x96,

    /* U+014E "Ŏ" */
    0x31, 0xe8, 0x61, 0x86, 0x17, 0x80,

    /* U+014F "ŏ" */
    0x64, 0x69, 0x96,

    /* U+0150 "Ő" */
    0x21, 0xe8, 0x61, 0x86, 0x17, 0x80,

    /* U+0151 "ő" */
    0x20, 0x69, 0x96,

    /* U+0152 "Œ" */
    0x77, 0x88, 0x88, 0x8e, 0x88, 0x77,

    /* U+0153 "œ" */
    0x6e, 0x79, 0x1b,

    /* U+0154 "Ŕ" */
    0x27, 0x25, 0x2e, 0x52, 0x40,

    /* U+0155 "ŕ" */
    0x43, 0xc9, 0x0,

    /* U+0156 "Ŗ" */
    0xf4, 0xb9, 0x49, 0x49, 0x0,

    /* U+0157 "ŗ" */
    0xf2, 0x48,

    /* U+0158 "Ř" */
    0x67, 0x25, 0x2e, 0x52, 0x40,

    /* U+0159 "ř" */
    0xc3, 0x49, 0x0,

    /* U+015A "Ś" */
    0xe, 0x8c, 0x22, 0xe0,

    /* U+015B "ś" */
    0x21, 0xc7, 0xc0,

    /* U+015C "Ŝ" */
    0x6e, 0x8c, 0x22, 0xe0,

    /* U+015D "ŝ" */
    0x45, 0xc7, 0xc0,

    /* U+015E "Ş" */
    0xe8, 0xc2, 0x2e, 0x60,

    /* U+015F "ş" */
    0x78, 0xb4,

    /* U+0160 "Š" */
    0x6e, 0x8c, 0x22, 0xe0,

    /* U+0161 "š" */
    0x61, 0xc7, 0xc0,

    /* U+0162 "Ţ" */
    0xf9, 0x8, 0x42, 0x10, 0x80,

    /* U+0163 "ţ" */
    0xb, 0xa4, 0x48,

    /* U+0164 "Ť" */
    0x67, 0xc8, 0x42, 0x10, 0x80,

    /* U+0165 "ť" */
    0x14, 0xe4, 0x46,

    /* U+0166 "Ŧ" */
    0xf9, 0x1c, 0x42, 0x10,

    /* U+0167 "ŧ" */
    0xb, 0xf4, 0xc0,

    /* U+0168 "Ũ" */
    0x74, 0x63, 0x18, 0xc5, 0xc0,

    /* U+0169 "ũ" */
    0x62, 0x99, 0x9f,

    /* U+016A "Ū" */
    0x74, 0x63, 0x18, 0xc5, 0xc0,

    /* U+016B "ū" */
    0x60, 0x99, 0x9f,

    /* U+016C "Ŭ" */
    0x24, 0x63, 0x18, 0xc5, 0xc0,

    /* U+016D "ŭ" */
    0xa6, 0x99, 0x9f,

    /* U+016E "Ů" */
    0x42, 0x23, 0x18, 0xc6, 0x2e,

    /* U+016F "ů" */
    0x62, 0x99, 0x9f,

    /* U+0170 "Ű" */
    0x34, 0x63, 0x18, 0xc5, 0xc0,

    /* U+0171 "ű" */
    0x70, 0x99, 0x9f,

    /* U+0172 "Ų" */
    0x8c, 0x63, 0x18, 0xb8, 0x80,

    /* U+0173 "ų" */
    0x99, 0x9f, 0x20,

    /* U+0174 "Ŵ" */
    0x18, 0x89, 0x99, 0x5a, 0x66, 0x66, 0x22,

    /* U+0175 "ŵ" */
    0x30, 0x9, 0x6d, 0x79, 0x20,

    /* U+0176 "Ŷ" */
    0x64, 0x54, 0xa2, 0x10, 0x80,

    /* U+0177 "ŷ" */
    0x60, 0x96, 0x64, 0x40,

    /* U+0178 "Ÿ" */
    0x54, 0x54, 0xa2, 0x10, 0x80,

    /* U+0179 "Ź" */
    0x27, 0xc4, 0x42, 0x23, 0xe0,

    /* U+017A "ź" */
    0x23, 0x95, 0xc0,

    /* U+017B "Ż" */
    0x27, 0xc4, 0x42, 0x23, 0xe0,

    /* U+017C "ż" */
    0x43, 0x95, 0xc0,

    /* U+017D "Ž" */
    0x67, 0xc4, 0x42, 0x23, 0xe0,

    /* U+017E "ž" */
    0x63, 0x95, 0xc0,

    /* U+017F "ſ" */
    0x69, 0x24, 0x80,

    /* U+2010 "‐" */
    0xc0,

    /* U+2012 "‒" */
    0xe0,

    /* U+2013 "–" */
    0xe0,

    /* U+2014 "—" */
    0xf8,

    /* U+2015 "―" */
    0xf8,

    /* U+2016 "‖" */
    0xff, 0xff,

    /* U+2017 "‗" */
    0xfc,

    /* U+2018 "‘" */
    0x40,

    /* U+2019 "’" */
    0x40,

    /* U+201A "‚" */
    0xc0,

    /* U+201B "‛" */
    0xc0,

    /* U+201C "“" */
    0x18,

    /* U+201D "”" */
    0x18,

    /* U+201E "„" */
    0xd8,

    /* U+201F "‟" */
    0xd8,

    /* U+2020 "†" */
    0x21, 0x3e, 0x42, 0x10, 0x80,

    /* U+2021 "‡" */
    0x21, 0x3e, 0x4f, 0x90, 0x80,

    /* U+2022 "•" */
    0xff, 0x80,

    /* U+2026 "…" */
    0xa4,

    /* U+20AC "€" */
    0x32, 0x3d, 0xc4, 0x18
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 33, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 32, .box_w = 2, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 3, .adv_w = 74, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 6, .adv_w = 74, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 10, .adv_w = 74, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 12, .adv_w = 74, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 16, .adv_w = 32, .box_w = 1, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 17, .adv_w = 63, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 20, .adv_w = 51, .box_w = 3, .box_h = 1, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 21, .adv_w = 106, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 26, .adv_w = 47, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 27, .adv_w = 55, .box_w = 2, .box_h = 4, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 28, .adv_w = 74, .box_w = 4, .box_h = 2, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 29, .adv_w = 48, .box_w = 2, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 30, .adv_w = 106, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 35, .adv_w = 51, .box_w = 3, .box_h = 1, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 36, .adv_w = 53, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 38, .adv_w = 74, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 41, .adv_w = 43, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 42, .adv_w = 43, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 43, .adv_w = 51, .box_w = 2, .box_h = 1, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 44, .adv_w = 83, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 48, .adv_w = 90, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 54, .adv_w = 33, .box_w = 2, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 55, .adv_w = 51, .box_w = 1, .box_h = 2, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 56, .adv_w = 43, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 57, .adv_w = 51, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 59, .adv_w = 55, .box_w = 2, .box_h = 4, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 60, .adv_w = 94, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 64, .adv_w = 93, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 69, .adv_w = 95, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 73, .adv_w = 56, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 75, .adv_w = 87, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 80, .adv_w = 87, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 85, .adv_w = 87, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 90, .adv_w = 87, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 95, .adv_w = 87, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 100, .adv_w = 87, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 105, .adv_w = 119, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 111, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 116, .adv_w = 74, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 120, .adv_w = 74, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 124, .adv_w = 74, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 128, .adv_w = 74, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 132, .adv_w = 36, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 134, .adv_w = 36, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 136, .adv_w = 36, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 139, .adv_w = 36, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 141, .adv_w = 98, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 146, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 151, .adv_w = 102, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 157, .adv_w = 102, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 163, .adv_w = 102, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 169, .adv_w = 102, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 175, .adv_w = 102, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 181, .adv_w = 74, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 183, .adv_w = 102, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 189, .adv_w = 94, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 194, .adv_w = 94, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 199, .adv_w = 94, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 204, .adv_w = 94, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 209, .adv_w = 80, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 214, .adv_w = 77, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 217, .adv_w = 74, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 220, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 223, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 226, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 229, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 232, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 235, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 238, .adv_w = 103, .box_w = 6, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 241, .adv_w = 61, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 244, .adv_w = 68, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 247, .adv_w = 68, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 250, .adv_w = 68, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 253, .adv_w = 68, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 256, .adv_w = 31, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 258, .adv_w = 31, .box_w = 2, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 260, .adv_w = 31, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 263, .adv_w = 31, .box_w = 2, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 265, .adv_w = 72, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 268, .adv_w = 71, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 271, .adv_w = 73, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 274, .adv_w = 73, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 277, .adv_w = 73, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 280, .adv_w = 73, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 283, .adv_w = 73, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 286, .adv_w = 74, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 288, .adv_w = 73, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 291, .adv_w = 71, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 294, .adv_w = 71, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 297, .adv_w = 71, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 300, .adv_w = 71, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 303, .adv_w = 66, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 307, .adv_w = 72, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 311, .adv_w = 66, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 315, .adv_w = 87, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 320, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 323, .adv_w = 87, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 328, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 331, .adv_w = 87, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 337, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 339, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 344, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 347, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 352, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 355, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 360, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 363, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 368, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 371, .adv_w = 97, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 376, .adv_w = 72, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 380, .adv_w = 98, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 385, .adv_w = 72, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 389, .adv_w = 74, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 393, .adv_w = 68, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 396, .adv_w = 74, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 400, .adv_w = 68, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 403, .adv_w = 74, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 407, .adv_w = 68, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 410, .adv_w = 74, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 414, .adv_w = 68, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 417, .adv_w = 74, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 421, .adv_w = 68, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 424, .adv_w = 94, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 429, .adv_w = 67, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 433, .adv_w = 94, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 438, .adv_w = 67, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 442, .adv_w = 94, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 447, .adv_w = 67, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 451, .adv_w = 94, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 456, .adv_w = 67, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 460, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 465, .adv_w = 71, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 470, .adv_w = 99, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 476, .adv_w = 71, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 480, .adv_w = 36, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 483, .adv_w = 31, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 486, .adv_w = 36, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 489, .adv_w = 31, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 492, .adv_w = 36, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 495, .adv_w = 31, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 498, .adv_w = 36, .box_w = 1, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 499, .adv_w = 31, .box_w = 1, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 500, .adv_w = 36, .box_w = 1, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 501, .adv_w = 31, .box_w = 1, .box_h = 4, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 502, .adv_w = 94, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 506, .adv_w = 63, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 510, .adv_w = 54, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 514, .adv_w = 31, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 517, .adv_w = 85, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 522, .adv_w = 65, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 526, .adv_w = 65, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 528, .adv_w = 66, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 531, .adv_w = 30, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 533, .adv_w = 66, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 536, .adv_w = 30, .box_w = 1, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 537, .adv_w = 66, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 540, .adv_w = 30, .box_w = 2, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 542, .adv_w = 68, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 545, .adv_w = 48, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 548, .adv_w = 68, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 551, .adv_w = 39, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 554, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 559, .adv_w = 71, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 562, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 567, .adv_w = 71, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 570, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 575, .adv_w = 71, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 578, .adv_w = 77, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 582, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 587, .adv_w = 71, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 590, .adv_w = 102, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 596, .adv_w = 73, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 599, .adv_w = 102, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 605, .adv_w = 73, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 608, .adv_w = 102, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 614, .adv_w = 73, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 617, .adv_w = 140, .box_w = 8, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 623, .adv_w = 112, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 626, .adv_w = 80, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 631, .adv_w = 47, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 634, .adv_w = 80, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 639, .adv_w = 47, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 641, .adv_w = 80, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 646, .adv_w = 47, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 649, .adv_w = 69, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 653, .adv_w = 55, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 656, .adv_w = 69, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 660, .adv_w = 55, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 663, .adv_w = 69, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 667, .adv_w = 55, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 669, .adv_w = 69, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 673, .adv_w = 55, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 676, .adv_w = 76, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 681, .adv_w = 46, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 684, .adv_w = 76, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 689, .adv_w = 46, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 692, .adv_w = 77, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 696, .adv_w = 46, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 699, .adv_w = 94, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 704, .adv_w = 71, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 707, .adv_w = 94, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 712, .adv_w = 71, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 715, .adv_w = 94, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 720, .adv_w = 71, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 723, .adv_w = 94, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 728, .adv_w = 71, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 731, .adv_w = 94, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 736, .adv_w = 71, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 739, .adv_w = 94, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 744, .adv_w = 71, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 747, .adv_w = 133, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 754, .adv_w = 101, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 759, .adv_w = 80, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 764, .adv_w = 66, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 768, .adv_w = 80, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 773, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 778, .adv_w = 58, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 781, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 786, .adv_w = 58, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 789, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 794, .adv_w = 58, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 797, .adv_w = 36, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 800, .adv_w = 48, .box_w = 2, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 801, .adv_w = 69, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 802, .adv_w = 74, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 803, .adv_w = 102, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 804, .adv_w = 102, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 805, .adv_w = 57, .box_w = 2, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 807, .adv_w = 59, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 808, .adv_w = 27, .box_w = 1, .box_h = 2, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 809, .adv_w = 27, .box_w = 1, .box_h = 2, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 810, .adv_w = 28, .box_w = 1, .box_h = 2, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 811, .adv_w = 27, .box_w = 1, .box_h = 2, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 812, .adv_w = 47, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 813, .adv_w = 47, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 814, .adv_w = 47, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 815, .adv_w = 47, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 816, .adv_w = 74, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 821, .adv_w = 74, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 826, .adv_w = 74, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 828, .adv_w = 96, .box_w = 6, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 829, .adv_w = 74, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0}
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
const lv_font_t goopad_font_essential_8 = {
#else
lv_font_t goopad_font_essential_8 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 10,          /*The maximum line height required by the font*/
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



#endif /*#if GOOPAD_FONT_ESSENTIAL_8*/
