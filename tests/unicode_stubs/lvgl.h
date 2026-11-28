#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef struct lv_font_t lv_font_t;
typedef struct { uint8_t *data; struct { unsigned stride; } header; } lv_draw_buf_t;
typedef struct {
    unsigned adv_w, box_w, box_h, format, stride;
    int ofs_x, ofs_y;
    struct { uint32_t index; } gid;
    bool is_placeholder, req_raw_bitmap;
    void *entry;
    const lv_font_t *resolved_font;
} lv_font_glyph_dsc_t;
struct lv_font_t {
    unsigned line_height;
    int base_line;
    bool (*get_glyph_dsc)(const lv_font_t *, lv_font_glyph_dsc_t *, uint32_t, uint32_t);
    const void *(*get_glyph_bitmap)(lv_font_glyph_dsc_t *, lv_draw_buf_t *);
    void (*release_glyph)(const lv_font_t *, lv_font_glyph_dsc_t *);
    void *user_data;
    bool static_bitmap;
    const lv_font_t *fallback;
};
#define LV_FONT_GLYPH_FORMAT_A2 2
#define LV_FONT_GLYPH_FORMAT_A8 8
#define LV_STRIDE_AUTO 0
#define LV_COLOR_FORMAT_A8 1
static inline uint32_t lv_draw_buf_width_to_stride(uint32_t width, unsigned format)
{ (void)format; return (width + 3) & ~3u; }
static inline bool lv_font_get_glyph_dsc(const lv_font_t *font, lv_font_glyph_dsc_t *out, uint32_t cp, uint32_t next)
{
    memset(out, 0, sizeof(*out));
    while (font) {
        if (font->get_glyph_dsc(font, out, cp, next)) { out->resolved_font = font; return true; }
        font = font->fallback;
    }
    return false;
}
static inline void lv_font_glyph_release_draw_data(lv_font_glyph_dsc_t *dsc)
{
    if (dsc->resolved_font->release_glyph) dsc->resolved_font->release_glyph(dsc->resolved_font, dsc);
}
static inline lv_draw_buf_t *lv_draw_buf_create(unsigned width, unsigned height, unsigned format, unsigned stride)
{
    lv_draw_buf_t *draw = calloc(1, sizeof(*draw));
    if (!draw) return NULL;
    draw->header.stride = stride ? stride : lv_draw_buf_width_to_stride(width, format);
    draw->data = calloc(height, draw->header.stride);
    if (!draw->data) { free(draw); return NULL; }
    return draw;
}
static inline void lv_draw_buf_destroy(lv_draw_buf_t *draw) { free(draw->data); free(draw); }
