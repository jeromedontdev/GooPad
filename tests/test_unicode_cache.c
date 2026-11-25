#include <assert.h>
#include <stdlib.h>
#define GOOPAD_BOARD_HEADER "unicode_board.h"
#include "../main/goopad_unicode.c"

int main(int argc, char **argv)
{
    assert(argc == 2);
    lv_font_t offline = {0};
    const lv_font_t *font = goopad_unicode_font(40, &offline);
    if (!strcmp(argv[1], "missing")) {
        assert(font == &offline && !health.available);
        return 0;
    }
    assert(font != &offline && health.available && health.glyphs >= 50000);
    const unsigned samples[] = {0xc6, 0x2014, 0x4e2d, 0x3042, 0x30ab, 0xd55c, 0x1f600};
    uint8_t draw_pixels[1024 * 128];
    lv_draw_buf_t draw = {.data = draw_pixels};
    for (unsigned i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
        lv_font_glyph_dsc_t dsc = {.resolved_font = font};
        assert(font->get_glyph_dsc(font, &dsc, samples[i], 0));
        assert(font->get_glyph_bitmap(&dsc, &draw) == &draw);
        bool ink = false;
        for (unsigned y = 0; y < dsc.box_h; y++)
            for (unsigned x = 0; x < dsc.box_w; x++)
                ink |= draw_pixels[y * lv_draw_buf_width_to_stride(dsc.box_w, 1) + x] != 0;
        assert(ink);
        uint32_t before = health.reads;
        assert(font->get_glyph_dsc(font, &dsc, samples[i], 0));
        assert(font->get_glyph_bitmap(&dsc, &draw) == &draw);
        assert(health.reads == before);
    }
    lv_font_glyph_dsc_t pinned = {.resolved_font = font, .req_raw_bitmap = true};
    assert(font->get_glyph_dsc(font, &pinned, 0x4e2d, 0));
    const uint8_t *bitmap = font->get_glyph_bitmap(&pinned, NULL);
    assert(bitmap && pinned.entry);
    size_t bytes = ((size_t)pinned.box_w * pinned.box_h + 3) / 4;
    uint8_t *saved = malloc(bytes);
    assert(saved); memcpy(saved, bitmap, bytes);
    for (unsigned cp = 0x4e00; cp < 0x5100; cp++) {
        lv_font_glyph_dsc_t dsc = {.resolved_font = font};
        assert(font->get_glyph_dsc(font, &dsc, cp, 0));
        assert(font->get_glyph_bitmap(&dsc, &draw));
        assert(health.cache_bytes <= CACHE_BYTES);
    }
    assert(memcmp(saved, bitmap, bytes) == 0);
    uint32_t before_lookup = health.reads;
    lv_font_glyph_dsc_t uncached = {.resolved_font = font};
    assert(font->get_glyph_dsc(font, &uncached, 0x5500, 0));
    assert(health.reads == before_lookup + 1);
    before_lookup = health.reads;
    assert(!font->get_glyph_dsc(font, &uncached, 0x10ffff, 0));
    assert(health.reads == before_lookup && health.index_bytes <= 512 * 1024);
    font->release_glyph(font, &pinned);
    assert(!pinned.entry);
    free(saved);
    if (!strcmp(argv[1], "corrupt")) {
        lv_font_glyph_dsc_t dsc = {.resolved_font = font};
        assert(font->get_glyph_dsc(font, &dsc, 0x1f600, 0));
        cache_t *entry = find(font->user_data, dsc.gid.index, true);
        assert(entry); discard(entry);
        FILE *fp = fopen(FONT_PATH, "r+b");
        assert(fp && fseek(fp, entry->glyph.offset, SEEK_SET) == 0);
        int byte = fgetc(fp);
        assert(byte != EOF && fseek(fp, entry->glyph.offset, SEEK_SET) == 0);
        assert(fputc(byte ^ 1, fp) != EOF && fclose(fp) == 0);
        assert(!font->get_glyph_bitmap(&dsc, &draw) && !health.available);
    }
    printf("Native glyph decoding, cache hits, pin lifetime and 32 KiB LRU bound passed\n");
    return 0;
}
