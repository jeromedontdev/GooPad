#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct lv_font_t lv_font_t;
typedef struct { uint8_t *data; } lv_draw_buf_t;
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
#define LV_COLOR_FORMAT_A8 1
static inline uint32_t lv_draw_buf_width_to_stride(uint32_t width, unsigned format)
{ (void)format; return (width + 3) & ~3u; }
