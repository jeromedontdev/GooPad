#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "goopad_fonts.h"
#include "goopad_styled.h"
#include "goopad_unicode.h"

static bool ascii_glyph(const lv_font_t *font, lv_font_glyph_dsc_t *dsc, uint32_t cp, uint32_t next)
{
    (void)font; (void)next;
    if (cp < 32 || cp > 126) return false;
    dsc->adv_w = 8; dsc->box_w = 1; dsc->box_h = 1;
    dsc->ofs_x = 0; dsc->ofs_y = 0;
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

static void assert_ascii(const lv_font_t *font);

static void render(const lv_font_t *font, uint32_t cp, uint32_t next, lv_draw_buf_t *draw)
{
    lv_font_glyph_dsc_t dsc = {.resolved_font = font};
    assert(font->get_glyph_dsc(font, &dsc, cp, next));
    if (dsc.box_w && dsc.box_h) assert(font->get_glyph_bitmap(&dsc, draw));
}

static void check_styled_corruption(const lv_font_t *styled, const lv_font_t *unicode,
                                    const lv_font_t *offline, lv_draw_buf_t *draw)
{
    lv_font_glyph_dsc_t dsc = {.resolved_font = styled};
    assert(styled->get_glyph_dsc(styled, &dsc, 'A', 0));
    FILE *fp = fopen(GOOPAD_STYLED_PATH, "rb");
    assert(fp);
    uint8_t directory[40], record_offset[4], byte;
    assert(fseek(fp, 32 + 39 * 40 + 12, SEEK_SET) == 0);
    assert(fread(directory, 1, 4, fp) == 4);
    uint32_t index_offset = (uint32_t)directory[0] | (uint32_t)directory[1] << 8 |
        (uint32_t)directory[2] << 16 | (uint32_t)directory[3] << 24;
    assert(fseek(fp, index_offset + dsc.gid.index * 28 + 4, SEEK_SET) == 0);
    assert(fread(record_offset, 1, 4, fp) == 4);
    uint32_t bitmap_offset = (uint32_t)record_offset[0] | (uint32_t)record_offset[1] << 8 |
        (uint32_t)record_offset[2] << 16 | (uint32_t)record_offset[3] << 24;
    fclose(fp);
    fp = fopen(GOOPAD_STYLED_PATH, "r+b");
    assert(fp && fseek(fp, bitmap_offset, SEEK_SET) == 0 && fread(&byte, 1, 1, fp) == 1);
    byte ^= 0x80;
    assert(fseek(fp, bitmap_offset, SEEK_SET) == 0 && fwrite(&byte, 1, 1, fp) == 1);
    fclose(fp);

    assert(styled->get_glyph_bitmap(&dsc, draw) == NULL);
    assert(goopad_styled_font(9, 3, offline) == offline);
    assert(goopad_unicode_font(40, offline) == unicode);
    unsigned size;
    const lv_font_t *selected = goopad_font_variant(10, 3, &size);
    assert(selected != styled && size == 12);
    assert_ascii(selected);
    render(unicode, 0x4e2d, 0, draw);
    goopad_unicode_status_t status;
    goopad_unicode_status(&status);
    assert(status.available);
}

static void assert_ascii(const lv_font_t *font)
{
    lv_font_glyph_dsc_t dsc = {.resolved_font = font};
    assert(font && font->get_glyph_dsc(font, &dsc, 'A', 0));
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "stress";
    lv_font_t offline = {0};
    const lv_font_t *styled = goopad_styled_font(9, 3, &offline);
    const lv_font_t *unicode = goopad_unicode_font(40, &offline);
    uint8_t draw_pixels[1024 * 128] = {0};
    lv_draw_buf_t draw = {.data = draw_pixels};
    if (!strcmp(mode, "styled-missing")) {
        assert(styled == &offline && unicode != &offline);
        unsigned size;
        const lv_font_t *selected = goopad_font_variant(10, 3, &size);
        assert(selected != &offline && size == 12);
        assert_ascii(selected);
        render(unicode, 0x4e2d, 0, &draw);
        puts("Missing styled pack did not disable healthy multilingual text");
        return 0;
    }
    if (!strcmp(mode, "unicode-missing")) {
        assert(styled != &offline && unicode == &offline);
        render(styled, 'A', 'V', &draw);
        unsigned size;
        const lv_font_t *selected = goopad_font_variant(10, 3, &size);
        assert(selected == styled && selected->fallback == &goopad_font_essential_12);
        assert_ascii(selected->fallback);
        puts("Missing multilingual pack did not disable healthy styled text");
        return 0;
    }
    if (!strcmp(mode, "both-missing")) {
        assert(styled == &offline && unicode == &offline);
        unsigned size;
        const lv_font_t *selected = goopad_font_variant(10, 3, &size);
        assert(selected == &goopad_font_essential_12 && size == 12);
        assert_ascii(selected);
        const lv_font_t *montserrat = goopad_font_variant(0, 0, &size);
        assert_ascii(montserrat);
        assert(montserrat->fallback == &goopad_font_essential_40);
        puts("Both missing packs preserve caller-provided core ASCII fallback");
        return 0;
    }
    if (!strcmp(mode, "styled-corrupt")) {
        assert(styled != &offline && unicode != &offline);
        check_styled_corruption(styled, unicode, &offline, &draw);
        puts("Corrupt styled pack did not disable healthy multilingual text");
        return 0;
    }
    assert(styled != &offline && unicode != &offline);

    static const uint32_t styled_text[] = {'A', 'V', 'A', 'T', 'A', 'R', ' ', 'W', 'A'};
    static const uint32_t multilingual[] = {0x4e2d, 0x3042, 0x30ab, 0xd55c, 0x1f600};
    for (unsigned i = 0; i < sizeof(styled_text) / sizeof(styled_text[0]); i++)
        render(styled, styled_text[i], i + 1 < sizeof(styled_text) / sizeof(styled_text[0]) ?
               styled_text[i + 1] : 0, &draw);
    for (unsigned i = 0; i < sizeof(multilingual) / sizeof(multilingual[0]); i++)
        render(unicode, multilingual[i], 0, &draw);

    for (unsigned cp = 0x4e00; cp < 0x5100; cp++) {
        lv_font_glyph_dsc_t dsc = {.resolved_font = unicode};
        if (unicode->get_glyph_dsc(unicode, &dsc, cp, 0) && dsc.box_w && dsc.box_h)
            assert(unicode->get_glyph_bitmap(&dsc, &draw));
        if ((cp & 15) == 0) {
            goopad_unicode_status_t unicode_status;
            goopad_styled_status_t styled_status;
            goopad_unicode_status(&unicode_status);
            goopad_styled_status(&styled_status);
            assert(goopad_font_bitmap_cache_used() <= 32 * 1024);
            assert(unicode_status.cache_bytes + styled_status.bitmap_cache_bytes ==
                   goopad_font_bitmap_cache_used());
        }
    }

    uint32_t styled_reads = 0, unicode_reads = 0;
    for (unsigned pass = 0; pass < 2; pass++) {
        goopad_styled_status_t before_style, after_style;
        goopad_unicode_status_t before_unicode, after_unicode;
        goopad_styled_status(&before_style);
        goopad_unicode_status(&before_unicode);
        for (unsigned i = 0; i < sizeof(styled_text) / sizeof(styled_text[0]); i++)
            render(styled, styled_text[i], i + 1 < sizeof(styled_text) / sizeof(styled_text[0]) ?
                   styled_text[i + 1] : 0, &draw);
        for (unsigned i = 0; i < sizeof(multilingual) / sizeof(multilingual[0]); i++)
            render(unicode, multilingual[i], 0, &draw);
        goopad_styled_status(&after_style);
        goopad_unicode_status(&after_unicode);
        if (pass) {
            assert(after_style.reads == styled_reads);
            assert(after_unicode.reads == unicode_reads);
        }
        styled_reads = after_style.reads;
        unicode_reads = after_unicode.reads;
        assert(goopad_font_bitmap_cache_used() <= 32 * 1024);
    }
    printf("Mixed Unicode/styled cache passed: shared bitmap use=%u/32768; warm reads unchanged "
           "(styled=%u, Unicode=%u)\n", goopad_font_bitmap_cache_used(), styled_reads, unicode_reads);
    return 0;
}
