#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned pack_opens;
static FILE *counted_fopen(const char *path, const char *mode)
{
    pack_opens++;
    return fopen(path, mode);
}
#define fopen counted_fopen
#include "../main/goopad_styled.c"
#undef fopen

static void check_cold_metrics_batch(const lv_font_t *fallback)
{
    for (unsigned i = 0; i < CACHE_ENTRIES; i++) {
        discard(&cache[i]);
        cache[i] = (cache_t){0};
    }
    uint32_t before = reads;
    unsigned opens_before = pack_opens;
    for (unsigned family = 0; family < FAMILY_COUNT; family++) {
        for (unsigned variant = 0; variant < VARIANT_COUNT; variant++) {
            const lv_font_t *font = goopad_styled_font(family, variant, fallback);
            assert(font != fallback);
            for (uint32_t cp = 32; cp <= 126; cp++) {
                lv_font_glyph_dsc_t dsc = {.resolved_font = font};
                assert(font->get_glyph_dsc(font, &dsc, cp, 0));
            }
        }
    }
    assert(reads - before <= FONT_COUNT * 6);
    assert(pack_opens - opens_before <= FONT_COUNT * 6);
    printf("Cold 5700-descriptor canary: %u SD reads, %u opens (limit 360 each)\n",
           (unsigned)(reads - before), pack_opens - opens_before);
    for (unsigned i = 0; i < CACHE_ENTRIES; i++) {
        discard(&cache[i]);
        cache[i] = (cache_t){0};
    }
}

static void check_unavailable(const lv_font_t *fallback)
{
    assert(goopad_styled_font(0, 0, fallback) == fallback);
    goopad_styled_status_t status;
    goopad_styled_status(&status);
    assert(!status.available && status.fonts == 0);
}

static unsigned check_all_kerning_advances(unsigned family, unsigned variant)
{
    styled_font_t *source = &fonts[family * VARIANT_COUNT + variant];
    uint32_t codepoints[256] = {0};
    uint16_t advances[256] = {0};
    FILE *fp = fopen(GOOPAD_STYLED_PATH, "rb");
    assert(fp);
    for (uint32_t i = 0; i < source->count; i++) {
        glyph_t glyph;
        assert(read_record(fp, source, i, &glyph));
        assert(glyph.glyph_id < 256);
        codepoints[glyph.glyph_id] = glyph.cp;
        advances[glyph.glyph_id] = glyph.advance;
    }
    fclose(fp);
    for (unsigned gid = 0; gid < 256; gid++) {
        if (!codepoints[gid]) continue;
        lv_font_glyph_dsc_t dsc = {.resolved_font = &source->font};
        assert(source->font.get_glyph_dsc(&source->font, &dsc, codepoints[gid], 0));
    }
    uint32_t read_count = reads;
    unsigned checked = 0;
    for (uint32_t i = 0; i < source->kern_count; i++) {
        const kern_pair_t *pair = &kern_pairs[source->kern_first + i];
        assert(pair->left < 256 && pair->right < 256);
        assert(codepoints[pair->left] && codepoints[pair->right]);
        lv_font_glyph_dsc_t dsc = {.resolved_font = &source->font};
        assert(source->font.get_glyph_dsc(&source->font, &dsc,
                                          codepoints[pair->left], codepoints[pair->right]));
        int32_t expected = (int32_t)advances[pair->left] + pair->value;
        if (expected < 0) expected = 0;
        expected = (expected + 8) >> 4;
        assert(dsc.adv_w == (unsigned)expected);
        checked++;
    }
    assert(reads == read_count);
    return checked;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    lv_font_t fallback = {0};
    if (!strcmp(argv[1], "missing")) {
        check_unavailable(&fallback);
        puts("Missing-pack fallback passed");
        return 0;
    }

    const lv_font_t *font = goopad_styled_font(9, 3, &fallback);
    if (!strcmp(argv[1], "metrics-corrupt")) {
        assert(font != &fallback && available);
        styled_font_t *source = font->user_data;
        FILE *fp = fopen(GOOPAD_STYLED_PATH, "r+b");
        assert(fp && fseek(fp, source->index + 32 * GLYPH_RECORD_SIZE, SEEK_SET) == 0);
        assert(fputc(0, fp) != EOF && fclose(fp) == 0);
        lv_font_glyph_dsc_t dsc = {.resolved_font = font};
        assert(!font->get_glyph_dsc(font, &dsc, 'A', 0) && !available);
        assert(goopad_styled_font(9, 3, &fallback) == &fallback);
        puts("Corrupt adjacent metrics record rejected before batching into cache");
        return 0;
    }
    if (!strcmp(argv[1], "corrupt")) {
        assert(font != &fallback && available);
        lv_font_glyph_dsc_t dsc = {.resolved_font = font};
        assert(font->get_glyph_dsc(font, &dsc, 'A', 0));
        cache_t *entry = find(font->user_data, dsc.gid.index, true);
        assert(entry);
        discard(entry);
        FILE *fp = fopen(GOOPAD_STYLED_PATH, "r+b");
        assert(fp && fseek(fp, entry->glyph.offset, SEEK_SET) == 0);
        int byte = fgetc(fp);
        assert(byte != EOF && fseek(fp, entry->glyph.offset, SEEK_SET) == 0);
        assert(fputc(byte ^ 1, fp) != EOF && fclose(fp) == 0);
        uint8_t draw_pixels[1024 * 128] = {0};
        lv_draw_buf_t draw = {.data = draw_pixels};
        assert(!font->get_glyph_bitmap(&dsc, &draw) && !available);
        assert(goopad_styled_font(9, 0, &fallback) == &fallback);
        puts("Corrupt-glyph detection and fallback passed");
        return 0;
    }

    assert(font != &fallback && available && font->fallback == &fallback);
    goopad_styled_status_t status;
    goopad_styled_status(&status);
    assert(status.available && status.fonts == 60 && status.glyphs > 5000);
    assert(status.indexed_metadata_bytes >= 60 * 96 * GLYPH_RECORD_SIZE);
    assert(status.descriptor_bytes == FONT_COUNT * sizeof(*fonts));
    assert(status.metadata_cache_bytes == CACHE_ENTRIES * sizeof(*cache));
    assert(status.kerning_cache_bytes == 4132 * sizeof(*kern_pairs));
    assert(status.kerning_pairs == 4132);
    assert(CACHE_ENTRIES == 256 && CACHE_BYTES == 32 * 1024);
    check_cold_metrics_batch(&fallback);

    const uint8_t righteous_a_12[] = {
        0x15,0x54,0x15,0x55,0x54,0x00,0x03,0x1d,0x0e,
        0xaa,0x80,0x0b,0xff,0xff,0xfb,0xff,0xff,0xfb
    };
    lv_font_t *righteous = &fonts[9 * VARIANT_COUNT + 3].font;
    assert(righteous->line_height == 14 && righteous->base_line == 3);
    assert(righteous->underline_position == -1 && righteous->underline_thickness == 0);
    lv_font_glyph_dsc_t a = {.resolved_font = righteous};
    lv_font_glyph_dsc_t av = {.resolved_font = righteous};
    uint32_t reads_before = reads;
    assert(righteous->get_glyph_dsc(righteous, &a, 'A', 0));
    assert(reads == reads_before + 1);
    assert(a.adv_w == 8 && a.box_w == 8 && a.box_h == 9 && a.ofs_x == 0 && a.ofs_y == 0);
    assert(righteous->get_glyph_dsc(righteous, &av, 'A', 'V') && av.adv_w == 5);
    reads_before = reads;
    assert(!righteous->get_glyph_dsc(righteous, &av, 0x1f600, 0));
    assert(reads == reads_before);
    reads_before = reads;
    assert(!righteous->get_glyph_dsc(righteous, &av, 0x2014, 0));
    assert(reads == reads_before);
    lv_font_glyph_dsc_t degree = {.resolved_font = righteous};
    assert(righteous->get_glyph_dsc(righteous, &degree, 0x00b0, 0));
    assert(reads == reads_before + 1);
    unsigned kern_advances = 0;
    for (unsigned variant = 0; variant < VARIANT_COUNT; variant++)
        kern_advances += check_all_kerning_advances(9, variant);
    assert(kern_advances == 4132);
    a.req_raw_bitmap = true;
    const uint8_t *pinned = righteous->get_glyph_bitmap(&a, NULL);
    assert(pinned && !memcmp(pinned, righteous_a_12, sizeof(righteous_a_12)));
    uint8_t saved[sizeof(righteous_a_12)];
    memcpy(saved, pinned, sizeof(saved));

    uint8_t draw_pixels[1024 * 128] = {0};
    lv_draw_buf_t draw = {.data = draw_pixels};
    unsigned rendered = 0;
    for (unsigned family = 0; family < FAMILY_COUNT; family++) {
        for (unsigned variant = 0; variant < VARIANT_COUNT; variant++) {
            const lv_font_t *current = goopad_styled_font(family, variant, &fallback);
            assert(current != &fallback);
            for (unsigned cp = 32; cp < 127; cp++) {
                lv_font_glyph_dsc_t dsc = {.resolved_font = current};
                if (!current->get_glyph_dsc(current, &dsc, cp, 0)) continue;
                assert(dsc.box_w <= 1024 && dsc.box_h <= 128);
                if (dsc.box_w && dsc.box_h) assert(current->get_glyph_bitmap(&dsc, &draw));
                rendered++;
                assert(bitmap_cache_bytes <= CACHE_BYTES);
            }
        }
    }
    assert(rendered >= 5000 && bitmap_cache_peak <= CACHE_BYTES);
    assert(!memcmp(saved, pinned, sizeof(saved)));
    righteous->release_glyph(righteous, &a);
    assert(!a.entry);

    static const char warm_text[] = "AVATAR WA";
    for (unsigned pass = 0; pass < 2; pass++) {
        uint32_t before = reads;
        for (unsigned i = 0; i < sizeof(warm_text) - 1; i++) {
            lv_font_glyph_dsc_t dsc = {.resolved_font = righteous};
            uint32_t next = warm_text[i + 1] ? (uint8_t)warm_text[i + 1] : 0;
            assert(righteous->get_glyph_dsc(righteous, &dsc, (uint8_t)warm_text[i], next));
            if (dsc.box_w && dsc.box_h) assert(righteous->get_glyph_bitmap(&dsc, &draw));
        }
        if (pass) assert(reads == before);
    }

    reads_before = reads;
    lv_font_glyph_dsc_t missing = {.resolved_font = font};
    assert(!font->get_glyph_dsc(font, &missing, 0x1f600, 0));
    assert(reads == reads_before);
    goopad_styled_status(&status);
    assert(status.available && status.metadata_cache_bytes == CACHE_ENTRIES * sizeof(*cache));
    assert(status.shared_bitmap_cache_bytes <= status.bitmap_cache_budget &&
           status.bitmap_cache_budget == 32 * 1024);
    printf("GSP1 actual-loader checks passed: %u glyph renders, %u kerning advances; index=%u SD bytes; "
           "descriptors=%u, metrics=%u, kerning=%u, bitmap peak=%u bytes; reads=%u, hits=%u\n",
           rendered, kern_advances, status.indexed_metadata_bytes, status.descriptor_bytes,
           status.metadata_cache_bytes, status.kerning_cache_bytes, status.bitmap_cache_peak,
           status.reads, status.hits);
    return 0;
}
