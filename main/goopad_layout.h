#pragma once

#include <stdbool.h>
#include <stdint.h>

static inline int goopad_grid_limit(int extent, int reserved, int minimum)
{
    if (minimum < 1 || extent <= reserved + 20) return 0;
    int count = (extent - reserved - 20 + 8) / (minimum + 8);
    return count > 4 ? 4 : count;
}

static inline bool goopad_layout_area(int cols, int rows, int index, int span_cols, int span_rows,
                                      uint32_t *area)
{
    if (!area || cols < 2 || cols > 4 || rows < 2 || rows > 4 ||
        index < 0 || index >= 32 || span_cols < 1 || span_rows < 1 ||
        span_cols > cols || span_rows > rows) return false;
    int capacity = cols * rows;
    int slot = index % capacity;
    int column = slot % cols, row = slot / cols;
    if (column + span_cols > cols || row + span_rows > rows) return false;
    uint32_t result = 0;
    for (int y = 0; y < span_rows; y++) {
        for (int x = 0; x < span_cols; x++) {
            int position = index + y * cols + x;
            if (position >= 32) return false;
            result |= UINT32_C(1) << position;
        }
    }
    *area = result;
    return true;
}

static inline bool goopad_layout_valid(int cols, int rows, bool compact, int count,
                                       const uint8_t *span_cols, const uint8_t *span_rows,
                                       uint32_t occupied)
{
    if (count < 0 || count > 32 || !span_cols || !span_rows) return false;
    uint32_t used = 0;
    for (int i = 0; i < count; i++) {
        uint32_t area;
        if (!goopad_layout_area(cols, rows, i, span_cols[i], span_rows[i], &area)) return false;
        bool enlarged = span_cols[i] > 1 || span_rows[i] > 1;
        if (enlarged && compact) return false;
        if (enlarged || (occupied & (UINT32_C(1) << i))) {
            if (used & area) return false;
            used |= area;
        }
    }
    return true;
}

static inline int goopad_widget_image_top(int slot_top, int slot_size, int source_height)
{
    return slot_top + (slot_size - source_height) / 2;
}

typedef struct {
    int scale, x, y;
} goopad_cover_t;

static inline goopad_cover_t goopad_image_cover(const uint8_t *alpha, int side, int width, int height)
{
    int left = side, top = side, right = -1, bottom = -1;
    if (alpha) {
        for (int y = 0; y < side; y++) {
            for (int x = 0; x < side; x++) {
                if (!alpha[y * side + x]) continue;
                if (x < left) left = x;
                if (x > right) right = x;
                if (y < top) top = y;
                if (y > bottom) bottom = y;
            }
        }
    }
    if (right < 0) {
        left = top = 0;
        right = bottom = side - 1;
    }
    int content_width = right - left + 1, content_height = bottom - top + 1;
    int scale_x = (width * 256 + content_width - 1) / content_width;
    int scale_y = (height * 256 + content_height - 1) / content_height;
    int scale = scale_x > scale_y ? scale_x : scale_y;
    return (goopad_cover_t) {
        .scale = scale,
        .x = (side - left * 2 - content_width) * scale / 512,
        .y = (side - top * 2 - content_height) * scale / 512,
    };
}
