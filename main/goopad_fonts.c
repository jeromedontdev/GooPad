#include "goopad_fonts.h"
#include "goopad_styled.h"
#include "goopad_unicode.h"
#include "goopad_text.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#include <string.h>
#include <stdlib.h>

typedef struct emphasized_font {
    lv_font_t font;
    const lv_font_t *base;
    unsigned emphasis;
    struct emphasized_font *next;
} emphasized_font_t;
static emphasized_font_t *emphasized_fonts;

static bool emphasized_descriptor(const lv_font_t *font, lv_font_glyph_dsc_t *out,
                                   uint32_t cp, uint32_t next)
{
    emphasized_font_t *style = font->user_data;
    if (!lv_font_get_glyph_dsc(style->base, out, cp, next)) return false;
    if (out->box_w && out->box_h) {
        unsigned extra = goopad_text_extra(out->box_h, style->emphasis);
        out->box_w += extra;
        out->adv_w += extra;
    }
    out->gid.index = cp;
    out->format = LV_FONT_GLYPH_FORMAT_A8;
    out->entry = NULL;
    return true;
}

static const void *emphasized_bitmap(lv_font_glyph_dsc_t *dsc, lv_draw_buf_t *draw)
{
    emphasized_font_t *style = dsc->resolved_font->user_data;
    lv_font_glyph_dsc_t source;
    if (!lv_font_get_glyph_dsc(style->base, &source, dsc->gid.index, 0)) return NULL;
    bool own = !draw;
    if (own) draw = lv_draw_buf_create(dsc->box_w, dsc->box_h, LV_COLOR_FORMAT_A8, LV_STRIDE_AUTO);
    if (!draw) { ESP_LOGE("goopad_fonts", "Emphasized glyph allocation failed"); return NULL; }
    source.req_raw_bitmap = false;
    const void *bitmap = source.resolved_font->get_glyph_bitmap(&source, draw);
    lv_font_glyph_release_draw_data(&source);
    if (bitmap != draw) {
        ESP_LOGE("goopad_fonts", "Could not decode emphasized glyph U+%04lx", (unsigned long)dsc->gid.index);
        if (own) lv_draw_buf_destroy(draw);
        return NULL;
    }
    goopad_text_emphasize(draw->data, source.box_w, source.box_h,
        lv_draw_buf_width_to_stride(source.box_w, LV_COLOR_FORMAT_A8), draw->header.stride, style->emphasis);
    if (own) dsc->entry = draw;
    return dsc->req_raw_bitmap ? draw->data : (const void *)draw;
}

static void emphasized_release(const lv_font_t *font, lv_font_glyph_dsc_t *dsc)
{
    (void)font;
    if (dsc->entry) lv_draw_buf_destroy(dsc->entry);
    dsc->entry = NULL;
}

const lv_font_t *goopad_font_emphasis(const lv_font_t *base, unsigned emphasis)
{
    if (!emphasis) return base;
    for (emphasized_font_t *style = emphasized_fonts; style; style = style->next)
        if (style->base == base && style->emphasis == emphasis) return &style->font;
    emphasized_font_t *style = heap_caps_calloc(1, sizeof(*style), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!style) { ESP_LOGE("goopad_fonts", "Font emphasis allocation failed"); return base; }
    style->font = *base;
    style->base = base;
    style->emphasis = emphasis;
    style->font.user_data = style;
    style->font.fallback = NULL;
    style->font.static_bitmap = false;
    style->font.get_glyph_dsc = emphasized_descriptor;
    style->font.get_glyph_bitmap = emphasized_bitmap;
    style->font.release_glyph = emphasized_release;
    style->next = emphasized_fonts;
    emphasized_fonts = style;
    return &style->font;
}

void goopad_font_emphasis_clear(void)
{
    while (emphasized_fonts) {
        emphasized_font_t *next = emphasized_fonts->next;
        free(emphasized_fonts);
        emphasized_fonts = next;
    }
}

LV_FONT_DECLARE(goopad_font_essential_8);
LV_FONT_DECLARE(goopad_font_essential_10);
LV_FONT_DECLARE(goopad_font_essential_12);
LV_FONT_DECLARE(goopad_font_essential_14);
LV_FONT_DECLARE(goopad_font_essential_16);
LV_FONT_DECLARE(goopad_font_essential_20);
LV_FONT_DECLARE(goopad_font_essential_24);
LV_FONT_DECLARE(goopad_font_essential_28);
LV_FONT_DECLARE(goopad_font_essential_36);
LV_FONT_DECLARE(goopad_font_essential_40);

static const char *const names[] = {
    "montserrat", "lato", "pacifico", "anton", "archivoblack",
    "abrilfatface", "lobster", "breeserif", "cousine", "sharetechmono", "righteous",
    "acme", "bangers", "patrickhand", "pressstart2p", "spacemono"
};
static const lv_font_t *const montserrat[] = {
    &lv_font_montserrat_40, &lv_font_montserrat_28, &lv_font_montserrat_20,
    &lv_font_montserrat_16, &lv_font_montserrat_14, &lv_font_montserrat_12,
    &lv_font_montserrat_10, &lv_font_montserrat_8
};

int goopad_font_index(const char *name)
{
    for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); i++)
        if (name && !strcmp(name, names[i])) return (int)i;
    return -1;
}

unsigned goopad_font_variants(unsigned family)
{
    return family == 0 ? sizeof(montserrat) / sizeof(montserrat[0]) : 4;
}

const lv_font_t *goopad_font_variant(unsigned family, unsigned variant, unsigned *size)
{
    static const unsigned default_sizes[] = {40, 28, 20, 16, 14, 12, 10, 8};
    if (family >= sizeof(names) / sizeof(names[0]) || variant >= goopad_font_variants(family)) return NULL;
    static lv_font_t extended[8];
    static const unsigned unicode_sizes[] = {8, 10, 12, 14, 16, 20, 24, 28, 36, 40};
    static const lv_font_t *const unicode[] = {
        &goopad_font_essential_8, &goopad_font_essential_10, &goopad_font_essential_12,
        &goopad_font_essential_14, &goopad_font_essential_16, &goopad_font_essential_20,
        &goopad_font_essential_24, &goopad_font_essential_28, &goopad_font_essential_36,
        &goopad_font_essential_40
    };
    *size = family == 0 ? default_sizes[variant] : (variant == 0 ? 36 : variant == 1 ? 24 : variant == 2 ? 16 : 12);
    if (family) {
        static const lv_font_t *const styled_fallbacks[] = {
            &goopad_font_essential_36, &goopad_font_essential_24,
            &goopad_font_essential_16, &goopad_font_essential_12
        };
        const lv_font_t *fallback = goopad_unicode_font(*size, styled_fallbacks[variant]);
        return goopad_styled_font(family - 1, variant, fallback);
    }
    lv_font_t *font = &extended[variant];
    if (!font->get_glyph_dsc) {
        *font = *montserrat[variant];
        for (unsigned i = 0; i < sizeof(unicode_sizes) / sizeof(unicode_sizes[0]); i++)
            if (unicode_sizes[i] == *size) font->fallback = goopad_unicode_font(*size, unicode[i]);
    }
    return font;
}
