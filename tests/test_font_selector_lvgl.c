#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "goopad_fonts.h"
#include "goopad_styled.h"
#include "goopad_unicode.h"

static bool ascii_glyph(const lv_font_t *font, lv_font_glyph_dsc_t *dsc, uint32_t cp, uint32_t next)
{
    (void)font; (void)next;
    if (cp < 32 || cp > 126) return false;
    dsc->adv_w = 8;
    dsc->box_w = 1;
    dsc->box_h = 1;
    return true;
}

#define DUMMY_FONT(name) const lv_font_t name = {.line_height = 10, .get_glyph_dsc = ascii_glyph}
DUMMY_FONT(goopad_font_essential_8);
DUMMY_FONT(goopad_font_essential_10);
DUMMY_FONT(goopad_font_essential_12);
DUMMY_FONT(goopad_font_essential_14);
DUMMY_FONT(goopad_font_essential_16);
DUMMY_FONT(goopad_font_essential_20);
DUMMY_FONT(goopad_font_essential_24);
DUMMY_FONT(goopad_font_essential_28);
DUMMY_FONT(goopad_font_essential_36);
DUMMY_FONT(goopad_font_essential_40);
DUMMY_FONT(lv_font_montserrat_40);
DUMMY_FONT(lv_font_montserrat_28);
DUMMY_FONT(lv_font_montserrat_20);
DUMMY_FONT(lv_font_montserrat_16);
DUMMY_FONT(lv_font_montserrat_14);
DUMMY_FONT(lv_font_montserrat_12);
DUMMY_FONT(lv_font_montserrat_10);
DUMMY_FONT(lv_font_montserrat_8);
#undef DUMMY_FONT

void lv_memset(void *dst, uint8_t value, size_t size)
{
    memset(dst, value, size);
}

int lv_strcmp(const char *left, const char *right)
{
    return strcmp(left, right);
}

uint32_t lv_draw_buf_width_to_stride(uint32_t width, lv_color_format_t format)
{
    (void)format;
    return width;
}

static void assert_acyclic_fallbacks(const lv_font_t *font)
{
    const lv_font_t *seen[8];
    unsigned count = 0;
    while (font) {
        assert(count < sizeof(seen) / sizeof(seen[0]));
        for (unsigned i = 0; i < count; i++) assert(seen[i] != font);
        seen[count++] = font;
        font = font->fallback;
    }
}

static const lv_font_t *resolve(const lv_font_t *font, uint32_t cp, bool expected)
{
    lv_font_glyph_dsc_t dsc;
    bool found = lv_font_get_glyph_dsc(font, &dsc, cp, 0);
    assert(found == expected);
    return found ? dsc.resolved_font : NULL;
}

static void check_selector_variants(bool unicode_available)
{
    for (unsigned family = 0; family < 16; family++) {
        for (unsigned variant = 0; variant < goopad_font_variants(family); variant++) {
            unsigned size = 0;
            const lv_font_t *font = goopad_font_variant(family, variant, &size);
            assert(font && size);
            assert_acyclic_fallbacks(font);
            assert(resolve(font, 'A', true));
            assert((resolve(font, 0x4e2d, unicode_available) != NULL) == unicode_available);
        }
    }
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    const char *mode = argv[1];
    unsigned size = 0;
    const lv_font_t *selected = goopad_font_variant(10, 3, &size);
    assert(selected && size == 12);
    assert_acyclic_fallbacks(selected);

    goopad_styled_status_t styled_status;
    goopad_unicode_status_t unicode_status;
    goopad_styled_status(&styled_status);
    goopad_unicode_status(&unicode_status);

    if (!strcmp(mode, "healthy")) {
        assert(styled_status.available && unicode_status.available);
        check_selector_variants(true);
        const lv_font_t *unicode = goopad_unicode_font(12, &goopad_font_essential_12);
        assert(selected->fallback == unicode);
        assert(resolve(selected, 'A', true) == selected);
        assert(resolve(selected, 0x4e2d, true) == unicode);
        assert(resolve(selected, 0x10ffff, false) == NULL);
    }
    else if (!strcmp(mode, "styled-missing")) {
        assert(!styled_status.available && unicode_status.available);
        check_selector_variants(true);
        assert(selected == goopad_unicode_font(12, &goopad_font_essential_12));
        assert(resolve(selected, 'A', true) == selected);
        assert(resolve(selected, 0x4e2d, true) == selected);
    }
    else if (!strcmp(mode, "unicode-missing")) {
        assert(styled_status.available && !unicode_status.available);
        check_selector_variants(false);
        assert(selected->fallback == &goopad_font_essential_12);
        assert(resolve(selected, 'A', true) == selected);
        assert(resolve(selected, 0x4e2d, false) == NULL);
    }
    else if (!strcmp(mode, "both-missing")) {
        assert(!styled_status.available && !unicode_status.available);
        check_selector_variants(false);
        assert(selected == &goopad_font_essential_12);
        assert(resolve(selected, 'A', true) == selected);
        const lv_font_t *montserrat = goopad_font_variant(0, 0, &size);
        assert(montserrat && size == 40);
        assert_acyclic_fallbacks(montserrat);
        assert(montserrat->fallback == &goopad_font_essential_40);
        assert(resolve(montserrat, 'A', true) == montserrat);
    }
    else {
        assert(!"Unknown test mode");
    }

    puts("Selector fallback graph resolved through actual LVGL font lookup");
    return 0;
}
