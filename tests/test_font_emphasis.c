#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "goopad_fonts.h"
#include "core/lv_global.h"

lv_global_t lv_global;
#define DUMMY_FONT(name) const lv_font_t name = {.line_height = 12}
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
DUMMY_FONT(lv_font_montserrat_8);
DUMMY_FONT(lv_font_montserrat_10);
DUMMY_FONT(lv_font_montserrat_12);
DUMMY_FONT(lv_font_montserrat_14);
DUMMY_FONT(lv_font_montserrat_16);
DUMMY_FONT(lv_font_montserrat_20);
DUMMY_FONT(lv_font_montserrat_28);
DUMMY_FONT(lv_font_montserrat_40);
#undef DUMMY_FONT
const lv_font_t *goopad_unicode_font(unsigned size, const lv_font_t *fallback)
{ (void)size; return fallback; }
const lv_font_t *goopad_styled_font(unsigned family, unsigned variant, const lv_font_t *fallback)
{ (void)family; (void)variant; return fallback; }
void lv_memset(void *dst, uint8_t value, size_t size) { memset(dst, value, size); }
int lv_strcmp(const char *left, const char *right) { return strcmp(left, right); }
uint32_t lv_draw_buf_width_to_stride(uint32_t width, lv_color_format_t format)
{ (void)format; return (width + 3) & ~3u; }
lv_draw_buf_t *lv_draw_buf_create(uint32_t width, uint32_t height, lv_color_format_t format, uint32_t stride)
{
    (void)format;
    lv_draw_buf_t *draw = calloc(1, sizeof(*draw));
    assert(draw);
    draw->header.stride = stride ? stride : lv_draw_buf_width_to_stride(width, format);
    draw->header.w = width; draw->header.h = height;
    draw->data = calloc(height, draw->header.stride);
    assert(draw->data);
    return draw;
}
void lv_draw_buf_destroy(lv_draw_buf_t *draw) { free(draw->data); free(draw); }

static bool glyph(const lv_font_t *font, lv_font_glyph_dsc_t *out, uint32_t cp, uint32_t next)
{
    if (cp != (font->user_data ? 0x4e2d : 'A') && cp != ' ') return false;
    out->adv_w = next == 'B' ? 7 : 8;
    out->box_w = cp == ' ' ? 0 : 8; out->box_h = cp == ' ' ? 0 : 9;
    out->ofs_x = out->ofs_y = 0; out->stride = 0; out->is_placeholder = false;
    out->format = LV_FONT_GLYPH_FORMAT_A8; out->gid.index = cp; out->entry = NULL;
    return true;
}
static const void *bitmap(lv_font_glyph_dsc_t *dsc, lv_draw_buf_t *draw)
{
    assert(!dsc->req_raw_bitmap && draw);
    memset(draw->data, 0, 9 * 8);
    for (unsigned y = 0; y < 9; y++) draw->data[y * 8 + 1] = 255;
    for (unsigned x = 1; x < 7; x++) draw->data[4 * 8 + x] = 255;
    return draw;
}

int main(void)
{
    lv_font_t fallback = {.get_glyph_dsc = glyph, .get_glyph_bitmap = bitmap, .line_height = 12, .user_data = (void *)1};
    lv_font_t base = {.get_glyph_dsc = glyph, .get_glyph_bitmap = bitmap, .line_height = 12, .fallback = &fallback};
    assert(goopad_font_emphasis(&base, 0) == &base);
    for (unsigned emphasis = 1; emphasis <= 3; emphasis++) {
        const lv_font_t *font = goopad_font_emphasis(&base, emphasis);
        assert(font == goopad_font_emphasis(&base, emphasis));
        for (unsigned which = 0; which < 2; which++) {
            lv_font_glyph_dsc_t dsc;
            assert(lv_font_get_glyph_dsc(font, &dsc, which ? 0x4e2d : 'A', 'B'));
            unsigned extra = (emphasis & 1 ? 1 : 0) + (emphasis & 2 ? 2 : 0);
            assert(dsc.box_w == 8 + extra && dsc.adv_w == 7 + extra);
            lv_draw_buf_t *draw = lv_draw_buf_create(dsc.box_w, dsc.box_h, LV_COLOR_FORMAT_A8, 0);
            assert(font->get_glyph_bitmap(&dsc, draw) == draw);
            unsigned ink = 0;
            for (unsigned y = 0; y < 9; y++) for (unsigned x = 0; x < dsc.box_w; x++)
                ink += draw->data[y * draw->header.stride + x] != 0;
            assert(ink == (emphasis & 1 ? 23 : 14));
            assert(draw->data[emphasis & 2 ? 3 : 1] == 255);
            assert(draw->data[8 * draw->header.stride + 1] == 255);
            dsc.req_raw_bitmap = true;
            const void *raw = font->get_glyph_bitmap(&dsc, NULL);
            assert(raw && dsc.entry);
            lv_draw_buf_t *owned = dsc.entry;
            assert(!memcmp(raw, draw->data, 9 * draw->header.stride));
            assert(owned->header.stride == draw->header.stride);
            lv_font_glyph_release_draw_data(&dsc);
            assert(!dsc.entry);
            lv_draw_buf_destroy(draw);
        }
        lv_font_glyph_dsc_t space;
        assert(lv_font_get_glyph_dsc(font, &space, ' ', 0) && space.adv_w == 8 && !space.box_w);
    }
    goopad_font_emphasis_clear();
    puts("Actual LVGL font emphasis metrics, pixels, fallback and ownership passed");
    return 0;
}
