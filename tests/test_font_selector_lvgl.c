#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "goopad_fonts.h"
#include "goopad_styled.h"
#include "goopad_unicode.h"
#include "core/lv_global.h"

lv_global_t lv_global;
void *lv_malloc(size_t size) { return malloc(size); }
void *lv_malloc_zeroed(size_t size) { return calloc(1, size); }
void lv_free(void *pointer) { free(pointer); }
void *lv_memcpy(void *destination, const void *source, size_t size)
{ return memcpy(destination, source, size); }
void lv_draw_buf_flush_cache(const lv_draw_buf_t *draw, const lv_area_t *area)
{ (void)draw; (void)area; }
void *lv_utils_bsearch(const void *key, const void *base, size_t count, size_t size,
                      int (*compare)(const void *, const void *))
{ return bsearch(key, base, count, size, compare); }

#define FAMILIES(X) \
    X(lato) X(pacifico) X(anton) X(archivoblack) X(abrilfatface) X(lobster) \
    X(breeserif) X(cousine) X(sharetechmono) X(righteous) X(acme) X(bangers) \
    X(patrickhand) X(pressstart2p) X(spacemono)
#define DECLARE_FAMILY(name) \
    LV_FONT_DECLARE(goopad_font_##name##_36); LV_FONT_DECLARE(goopad_font_##name##_24); \
    LV_FONT_DECLARE(goopad_font_##name##_16); LV_FONT_DECLARE(goopad_font_##name##_12);
FAMILIES(DECLARE_FAMILY)
#define REFERENCE_FAMILY(name) \
    &goopad_font_##name##_36, &goopad_font_##name##_24, \
    &goopad_font_##name##_16, &goopad_font_##name##_12,
static const lv_font_t *const originals[] = {FAMILIES(REFERENCE_FAMILY)};

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

static void check_original_glyph_pixels(void)
{
    unsigned checked = 0, kern_checked = 0;
    uint8_t original_pixels[4096], sd_pixels[4096];
    lv_draw_buf_t original_draw = {.data = original_pixels};
    lv_draw_buf_t sd_draw = {.data = sd_pixels};
    for (unsigned i = 0; i < sizeof(originals) / sizeof(originals[0]); i++) {
        unsigned size;
        const lv_font_t *selected = goopad_font_variant(i / 4 + 1, i % 4, &size);
        for (uint32_t cp = 32; cp <= 176; cp++) {
            if (cp > 126 && cp != 176) continue;
            lv_font_glyph_dsc_t expected, actual;
            assert(lv_font_get_glyph_dsc(originals[i], &expected, cp, 0));
            assert(lv_font_get_glyph_dsc(selected, &actual, cp, 0));
            assert(actual.adv_w == expected.adv_w && actual.box_w == expected.box_w &&
                   actual.box_h == expected.box_h && actual.ofs_x == expected.ofs_x &&
                   actual.ofs_y == expected.ofs_y);
            if (actual.box_w && actual.box_h) {
                memset(original_pixels, 0, sizeof(original_pixels));
                memset(sd_pixels, 0, sizeof(sd_pixels));
                assert(actual.box_w * actual.box_h <= sizeof(sd_pixels));
                assert(lv_font_get_glyph_bitmap(&expected, &original_draw));
                assert(lv_font_get_glyph_bitmap(&actual, &sd_draw));
                if (memcmp(original_pixels, sd_pixels, actual.box_w * actual.box_h)) {
                    fprintf(stderr, "Original LVGL pixel mismatch: variant %u U+%04x\n", i, cp);
                    assert(!"SD pixels differ from actual LVGL decoding");
                }
            }
            checked++;
        }
        const lv_font_fmt_txt_dsc_t *descriptor = originals[i]->dsc;
        if (descriptor->kern_dsc) {
            for (uint32_t cp = 32; cp <= 176; cp++) {
                if (cp > 126 && cp != 176) continue;
                for (uint32_t next = 32; next <= 176; next++) {
                    if (next > 126 && next != 176) continue;
                    lv_font_glyph_dsc_t expected, actual;
                    assert(lv_font_get_glyph_dsc(originals[i], &expected, cp, next));
                    assert(lv_font_get_glyph_dsc(selected, &actual, cp, next));
                    if (actual.adv_w != expected.adv_w) {
                        fprintf(stderr, "Original LVGL kerning mismatch: variant %u %c%c %u != %u\n",
                                i, cp, next, actual.adv_w, expected.adv_w);
                        assert(!"SD kerning differs from actual LVGL");
                    }
                    kern_checked++;
                }
            }
        }
    }
    printf("Actual LVGL original-vs-SD parity: %u glyph pixels/metrics, %u kerning combinations\n",
           checked, kern_checked);
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
        check_original_glyph_pixels();
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
