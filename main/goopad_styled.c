#include "goopad_styled.h"
#include "board.h"
#include "goopad_resource_manifest.h"
#include "goopad_unicode.h"

#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_log.h"

#ifndef GOOPAD_STYLED_FILE
#define GOOPAD_STYLED_FILE "styled-missing.gsp"
#endif
#ifndef GOOPAD_STYLED_PATH
#define GOOPAD_STYLED_PATH BSP_SD_MOUNT_POINT "/goopad/resources/" GOOPAD_STYLED_FILE
#endif

#define FAMILY_COUNT 15
#define VARIANT_COUNT 4
#define FONT_COUNT (FAMILY_COUNT * VARIANT_COUNT)
#define PACK_GLYPH_COUNT 96
#define HEADER_SIZE 32
#define DIRECTORY_SIZE 40
#define GLYPH_RECORD_SIZE 28
#define METRIC_BATCH_RECORDS 16
#define KERN_RECORD_SIZE 6
#define CACHE_ENTRIES 256
#define CACHE_BYTES (32 * 1024)
#define MAX_FILE_BYTES (16 * 1024 * 1024)
#define MAX_BITMAP CACHE_BYTES

typedef struct {
    uint32_t cp, offset;
    uint16_t length, advance, width, height;
    int16_t x, y;
    uint16_t glyph_id;
    uint8_t encoding;
    uint16_t checksum, record_checksum;
} glyph_t;

typedef struct {
    uint16_t left, right;
    int16_t value;
} kern_pair_t;

typedef struct {
    uint8_t family, variant;
    uint16_t size;
    uint32_t count, index, begin, end, kern_offset, kern_count, kern_first;
    uint16_t line_height;
    int16_t baseline;
    int8_t underline_position;
    uint8_t underline_thickness;
    lv_font_t font;
} styled_font_t;

typedef struct {
    styled_font_t *font;
    uint32_t index, stamp, bitmap_stamp;
    glyph_t glyph;
    uint8_t *bitmap;
    uint16_t bytes, pins;
} cache_t;

static const char *TAG = "styled-font";
static styled_font_t *fonts;
static cache_t *cache;
static kern_pair_t *kern_pairs;
static atomic_bool available;
static atomic_uint_least32_t total_glyphs, indexed_metadata_bytes, descriptor_bytes;
static atomic_uint_least32_t metadata_cache_bytes, kerning_cache_bytes, bitmap_cache_bytes, bitmap_cache_peak;
static atomic_uint_least32_t total_kern_pairs;
static atomic_uint_least32_t hits, reads;
static bool initialized;
static uint32_t clock_tick;

static void discard(cache_t *entry);
static cache_t *oldest(cache_t *exclude, bool bitmap_only);
static cache_t *find(styled_font_t *font, uint32_t value, bool by_index);
static cache_t *retain(styled_font_t *font, uint32_t index, const glyph_t *glyph);

static bool evict_bitmaps(const void *exclude, uint32_t target_stamp, uint32_t *stamp, uint32_t *bytes)
{
    cache_t *victim = NULL;
    for (unsigned i = 0; i < CACHE_ENTRIES; i++) {
        cache_t *entry = &cache[i];
        if (entry == exclude || entry->pins || !entry->bitmap) continue;
        if (!victim || entry->bitmap_stamp < victim->bitmap_stamp) victim = entry;
    }
    if (!victim) return false;
    *stamp = victim->bitmap_stamp;
    *bytes = victim->bytes;
    if (target_stamp == UINT32_MAX) return true;
    if (victim->bitmap_stamp != target_stamp) return false;
    discard(victim);
    return true;
}

static uint16_t u16(const uint8_t *p) { return (uint16_t)p[0] | (uint16_t)p[1] << 8; }
static uint32_t u32(const uint8_t *p) { return (uint32_t)u16(p) | (uint32_t)u16(p + 2) << 16; }

static uint16_t crc16(uint16_t crc, const uint8_t *data, size_t length)
{
    while (length--) {
        crc ^= *data++;
        for (unsigned i = 0; i < 8; i++) crc = (crc >> 1) ^ ((crc & 1) ? 0xa001 : 0);
    }
    return crc;
}

static bool failed(const char *message)
{
    if (available) ESP_LOGE(TAG, "%s; styled SD fonts unavailable", message);
    available = false;
    return false;
}

static bool read_at(FILE *fp, uint32_t offset, void *out, size_t length)
{
    reads++;
    return fseek(fp, offset, SEEK_SET) == 0 && fread(out, 1, length, fp) == length;
}

static bool decode_record(const uint8_t *b, styled_font_t *font, uint32_t index, glyph_t *glyph)
{
    *glyph = (glyph_t){
        .cp = u32(b), .offset = u32(b + 4), .length = u16(b + 8),
        .advance = u16(b + 10), .width = u16(b + 12), .height = u16(b + 14),
        .x = (int16_t)u16(b + 16), .y = (int16_t)u16(b + 18),
        .glyph_id = u16(b + 20), .encoding = b[22],
        .checksum = u16(b + 26), .record_checksum = crc16(0xffff, b, 26)
    };
    size_t raw = ((size_t)glyph->width * glyph->height + 3) / 4;
    return glyph->cp >= 0x20 && glyph->cp <= 0x10ffff &&
        !(glyph->cp >= 0xd800 && glyph->cp <= 0xdfff) &&
        glyph->width <= 1024 && glyph->height <= 128 && raw <= MAX_BITMAP &&
        glyph->length == raw && glyph->encoding == 0 && b[23] == 2 &&
        glyph->offset >= font->begin && glyph->offset <= font->end &&
        glyph->length <= font->end - glyph->offset &&
        (index == 0 || glyph->cp != 0);
}

static bool read_record(FILE *fp, styled_font_t *font, uint32_t index, glyph_t *glyph)
{
    uint8_t b[GLYPH_RECORD_SIZE];
    if (index >= font->count ||
        !read_at(fp, font->index + index * GLYPH_RECORD_SIZE, b, sizeof(b)) ||
        !decode_record(b, font, index, glyph))
        return failed("Invalid styled glyph record");
    return true;
}

static bool pack_codepoint_index(const styled_font_t *font, uint32_t cp, uint32_t *index)
{
    if (font->count != PACK_GLYPH_COUNT) return false;
    /* All packaged variants contain ASCII 0x20-0x7e followed by U+00B0. */
    if (cp >= 32 && cp <= 126) {
        *index = cp - 32;
        return true;
    }
    if (cp == 0x00b0) {
        *index = 95;
        return true;
    }
    return false;
}

static bool lookup(FILE *fp, styled_font_t *font, uint32_t cp, glyph_t *glyph, uint32_t *index)
{
    if (font->count == PACK_GLYPH_COUNT) {
        if (!pack_codepoint_index(font, cp, index)) return false;
        uint32_t begin = *index / METRIC_BATCH_RECORDS * METRIC_BATCH_RECORDS;
        unsigned count = font->count - begin;
        if (count > METRIC_BATCH_RECORDS) count = METRIC_BATCH_RECORDS;
        uint8_t records[METRIC_BATCH_RECORDS * GLYPH_RECORD_SIZE];
        if (!read_at(fp, font->index + begin * GLYPH_RECORD_SIZE, records,
                     count * GLYPH_RECORD_SIZE)) return failed("Could not read styled metrics batch");
        for (unsigned i = 0; i < count; i++) {
            glyph_t decoded;
            uint32_t ordinal;
            if (!decode_record(records + i * GLYPH_RECORD_SIZE, font, begin + i, &decoded) ||
                !pack_codepoint_index(font, decoded.cp, &ordinal) || ordinal != begin + i)
                return failed("Invalid styled metrics batch");
        }
        for (unsigned i = 0; i < count; i++) {
            glyph_t decoded;
            decode_record(records + i * GLYPH_RECORD_SIZE, font, begin + i, &decoded);
            if (begin + i == *index) {
                *glyph = decoded;
            }
            else if (!find(font, begin + i, true) && oldest(NULL, false)) {
                retain(font, begin + i, &decoded);
            }
        }
        return true;
    }

    uint32_t low = 0, high = font->count;
    while (low < high) {
        uint32_t middle = low + (high - low) / 2;
        glyph_t entry;
        if (!read_record(fp, font, middle, &entry)) return false;
        if (entry.cp < cp) low = middle + 1;
        else high = middle;
    }
    if (low == font->count) return false;
    if (!read_record(fp, font, low, glyph)) return false;
    if (glyph->cp != cp) return false;
    *index = low;
    return true;
}

static void discard(cache_t *entry)
{
    if (entry->bitmap) {
        goopad_font_bitmap_release(entry->bytes);
        bitmap_cache_bytes -= entry->bytes;
        free(entry->bitmap);
        entry->bitmap = NULL;
        entry->bytes = 0;
    }
}

static cache_t *oldest(cache_t *exclude, bool bitmap_only)
{
    cache_t *result = NULL;
    for (unsigned i = 0; i < CACHE_ENTRIES; i++) {
        cache_t *entry = &cache[i];
        if (entry == exclude || entry->pins || (bitmap_only && !entry->bitmap)) continue;
        if (!result || clock_tick - entry->stamp > clock_tick - result->stamp) result = entry;
    }
    return result;
}

static cache_t *find(styled_font_t *font, uint32_t value, bool by_index)
{
    for (unsigned i = 0; i < CACHE_ENTRIES; i++) {
        cache_t *entry = &cache[i];
        if (entry->font == font && (by_index ? entry->index == value : entry->glyph.cp == value)) {
            entry->stamp = ++clock_tick;
            if (entry->bitmap) entry->bitmap_stamp = goopad_font_bitmap_cache_stamp();
            hits++;
            return entry;
        }
    }
    return NULL;
}

static cache_t *retain(styled_font_t *font, uint32_t index, const glyph_t *glyph)
{
    cache_t *entry = oldest(NULL, false);
    if (!entry) { failed("Styled glyph cache is pinned"); return NULL; }
    discard(entry);
    *entry = (cache_t){.font = font, .index = index, .glyph = *glyph, .stamp = ++clock_tick};
    return entry;
}

static int16_t kerning(styled_font_t *font, uint16_t left, uint16_t right)
{
    uint32_t low = 0, high = font->kern_count;
    while (low < high) {
        uint32_t middle = low + (high - low) / 2;
        const kern_pair_t *pair = &kern_pairs[font->kern_first + middle];
        if (pair->left < left || (pair->left == left && pair->right < right)) low = middle + 1;
        else high = middle;
    }
    if (low == font->kern_count) return 0;
    const kern_pair_t *pair = &kern_pairs[font->kern_first + low];
    return pair->left == left && pair->right == right ? pair->value : 0;
}

static bool glyph_dsc(const lv_font_t *font, lv_font_glyph_dsc_t *out, uint32_t cp, uint32_t next)
{
    if (!available) return false;
    styled_font_t *source = font->user_data;
    bool is_tab = cp == '\t';
    if (is_tab) cp = ' ';
    cache_t *entry = find(source, cp, false);
    FILE *fp = NULL;
    if (!entry) {
        uint32_t known_index;
        if (source->count == PACK_GLYPH_COUNT &&
            !pack_codepoint_index(source, cp, &known_index)) return false;
        fp = fopen(GOOPAD_STYLED_PATH, "rb");
        if (!fp) return failed("Could not open styled font pack");
        glyph_t glyph;
        uint32_t index;
        bool ok = lookup(fp, source, cp, &glyph, &index);
        if (!ok) {
            fclose(fp);
            return false;
        }
        entry = retain(source, index, &glyph);
        if (!entry) { fclose(fp); return false; }
    }
    int16_t kern = 0;
    if (source->kern_count && next) {
        entry->pins++;
        cache_t *right = find(source, next, false);
        if (!right) {
            uint32_t known_index;
            if (source->count != PACK_GLYPH_COUNT ||
                pack_codepoint_index(source, next, &known_index)) {
                if (!fp) fp = fopen(GOOPAD_STYLED_PATH, "rb");
                if (!fp) { entry->pins--; return failed("Could not open styled font pack"); }
                glyph_t glyph;
                uint32_t index;
                if (lookup(fp, source, next, &glyph, &index))
                    right = retain(source, index, &glyph);
                if (!available) { entry->pins--; fclose(fp); return false; }
            }
        }
        if (right) kern = kerning(source, entry->glyph.glyph_id, right->glyph.glyph_id);
        entry->pins--;
    }
    if (fp) fclose(fp);
    int32_t advance = (int32_t)entry->glyph.advance + kern;
    if (is_tab) advance *= 2;
    if (advance < 0) advance = 0;
    out->adv_w = ((uint32_t)advance + 8) >> 4;
    out->box_w = entry->glyph.width;
    out->box_h = entry->glyph.height;
    out->ofs_x = entry->glyph.x;
    out->ofs_y = entry->glyph.y;
    out->format = LV_FONT_GLYPH_FORMAT_A2;
    out->gid.index = entry->index;
    out->stride = 0;
    out->is_placeholder = false;
    out->entry = NULL;
    return true;
}

static const void *glyph_bitmap(lv_font_glyph_dsc_t *dsc, lv_draw_buf_t *draw)
{
    if (!available || !dsc->box_w || !dsc->box_h) return NULL;
    styled_font_t *font = dsc->resolved_font->user_data;
    cache_t *entry = find(font, dsc->gid.index, true);
    FILE *fp = NULL;
    if (!entry) {
        fp = fopen(GOOPAD_STYLED_PATH, "rb");
        if (!fp) { failed("Could not open styled font pack"); return NULL; }
        glyph_t glyph;
        if (!read_record(fp, font, dsc->gid.index, &glyph)) { fclose(fp); return NULL; }
        entry = retain(font, dsc->gid.index, &glyph);
        if (!entry) { fclose(fp); return NULL; }
    }
    if (!entry->bitmap) {
        const glyph_t *glyph = &entry->glyph;
        unsigned pixels = (unsigned)glyph->width * glyph->height;
        unsigned bytes = (pixels + 3) / 4;
        while (bitmap_cache_bytes + bytes > CACHE_BYTES) {
            cache_t *victim = oldest(entry, true);
            if (!victim) break;
            discard(victim);
        }
        bool reserved = bytes && bitmap_cache_bytes + bytes <= CACHE_BYTES &&
            goopad_font_bitmap_reserve(bytes, entry);
        uint8_t *raw = reserved ?
            heap_caps_calloc(1, bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : NULL;
        if (!fp) fp = fopen(GOOPAD_STYLED_PATH, "rb");
        bool ok = raw && fp;
        if (ok) ok = read_at(fp, glyph->offset, raw, bytes);
        if (fp) { fclose(fp); fp = NULL; }
        if (ok) ok = crc16(glyph->record_checksum, raw, bytes) == glyph->checksum;
        if (!ok) {
            free(raw);
            if (reserved) goopad_font_bitmap_release(bytes);
            failed("Could not read/verify styled glyph bitmap");
            return NULL;
        }
        entry->bitmap = raw;
        entry->bytes = bytes;
        entry->bitmap_stamp = goopad_font_bitmap_cache_stamp();
        bitmap_cache_bytes += bytes;
        if (bitmap_cache_bytes > bitmap_cache_peak) bitmap_cache_peak = bitmap_cache_bytes;
    }
    if (dsc->req_raw_bitmap) {
        entry->pins++;
        dsc->entry = entry;
        return entry->bitmap;
    }
    if (!draw) { failed("Styled glyph draw buffer unavailable"); return NULL; }
    uint32_t stride = lv_draw_buf_width_to_stride(dsc->box_w, LV_COLOR_FORMAT_A8);
    for (unsigned y = 0; y < dsc->box_h; y++)
        for (unsigned x = 0; x < dsc->box_w; x++) {
            unsigned at = y * dsc->box_w + x;
            draw->data[y * stride + x] = ((entry->bitmap[at >> 2] >> (6 - 2 * (at & 3))) & 3) * 85;
        }
    return draw;
}

static void release_glyph(const lv_font_t *font, lv_font_glyph_dsc_t *dsc)
{
    (void)font;
    cache_t *entry = dsc->entry;
    if (entry && entry->pins) entry->pins--;
    dsc->entry = NULL;
}

static bool initialize(void)
{
    initialized = true;
    FILE *fp = fopen(GOOPAD_STYLED_PATH, "rb");
    if (!fp) {
        ESP_LOGW(TAG, "GSP1 pack absent; using embedded Montserrat/essential fallback");
        return false;
    }
    uint8_t header[32], dir[DIRECTORY_SIZE];
    bool ok = fseek(fp, 0, SEEK_END) == 0;
    long length = ok ? ftell(fp) : -1;
    ok = length >= HEADER_SIZE && length <= MAX_FILE_BYTES &&
        read_at(fp, 0, header, sizeof(header)) && !memcmp(header, "GSP1", 4) &&
        u32(header + 4) == (uint32_t)length && u32(header + 8) == FONT_COUNT &&
        u32(header + 12) == DIRECTORY_SIZE && u32(header + 16) == HEADER_SIZE &&
        u32(header + 20) == GLYPH_RECORD_SIZE && u32(header + 24) == KERN_RECORD_SIZE &&
        u32(header + 28) == 1;
    fonts = ok ? heap_caps_calloc(FONT_COUNT, sizeof(*fonts), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : NULL;
    ok = ok && fonts;
    uint32_t kern_total = 0;
    for (unsigned i = 0; ok && i < FONT_COUNT; i++) {
        ok = read_at(fp, HEADER_SIZE + i * DIRECTORY_SIZE, dir, sizeof(dir));
        if (!ok) break;
        styled_font_t *font = &fonts[i];
        font->family = dir[0];
        font->variant = dir[1];
        font->size = u16(dir + 2);
        font->line_height = u16(dir + 4);
        font->baseline = (int16_t)u16(dir + 6);
        font->underline_position = (int8_t)dir[36];
        font->underline_thickness = dir[37];
        font->count = u32(dir + 8);
        font->index = u32(dir + 12);
        font->begin = u32(dir + 16);
        font->end = u32(dir + 20);
        font->kern_offset = u32(dir + 24);
        font->kern_count = u32(dir + 28);
        font->kern_first = kern_total;
        kern_total += font->kern_count;
        static const uint16_t expected_sizes[VARIANT_COUNT] = {36, 24, 16, 12};
        uint32_t expected_size = expected_sizes[i % VARIANT_COUNT];
        ok = font->family == i / VARIANT_COUNT && font->variant == i % VARIANT_COUNT &&
            font->size == expected_size && font->line_height && font->line_height <= 128 &&
            font->baseline >= 0 && font->baseline <= font->line_height &&
            font->count && font->count <= 2048 &&
            u16(dir + 32) == GLYPH_RECORD_SIZE && u16(dir + 34) == KERN_RECORD_SIZE &&
            (uint64_t)font->index + (uint64_t)font->count * GLYPH_RECORD_SIZE <= font->begin &&
            font->begin <= font->end && font->end <= (uint32_t)length &&
            font->kern_offset >= font->end &&
            (uint64_t)font->kern_offset + (uint64_t)font->kern_count * KERN_RECORD_SIZE <= (uint32_t)length;
        ok = ok && kern_total <= 10000;
        if (ok) {
            font->font = (lv_font_t){
                .line_height = font->line_height, .base_line = font->baseline,
                .get_glyph_dsc = glyph_dsc, .get_glyph_bitmap = glyph_bitmap,
                .release_glyph = release_glyph, .user_data = font, .static_bitmap = false,
                .underline_position = font->underline_position,
                .underline_thickness = font->underline_thickness
            };
            total_glyphs += font->count;
            indexed_metadata_bytes += font->count * GLYPH_RECORD_SIZE +
                font->kern_count * KERN_RECORD_SIZE;
            total_kern_pairs += font->kern_count;
        }
    }
    kern_pairs = ok && kern_total ?
        heap_caps_calloc(kern_total, sizeof(*kern_pairs), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : NULL;
    ok = ok && (!kern_total || kern_pairs);
    uint8_t kern_bytes[256];
    for (unsigned i = 0; ok && i < FONT_COUNT; i++) {
        styled_font_t *font = &fonts[i];
        if (!font->kern_count) continue;
        ok = fseek(fp, font->kern_offset, SEEK_SET) == 0;
        uint32_t at = 0;
        while (ok && at < font->kern_count) {
            unsigned count = font->kern_count - at;
            if (count > sizeof(kern_bytes) / KERN_RECORD_SIZE) count = sizeof(kern_bytes) / KERN_RECORD_SIZE;
            reads++;
            ok = fread(kern_bytes, KERN_RECORD_SIZE, count, fp) == count;
            for (unsigned j = 0; ok && j < count; j++) {
                const uint8_t *b = kern_bytes + j * KERN_RECORD_SIZE;
                kern_pair_t *pair = &kern_pairs[font->kern_first + at + j];
                *pair = (kern_pair_t){.left = u16(b), .right = u16(b + 2), .value = (int16_t)u16(b + 4)};
                if ((at + j) && (pair->left < kern_pairs[font->kern_first + at + j - 1].left ||
                    (pair->left == kern_pairs[font->kern_first + at + j - 1].left &&
                     pair->right <= kern_pairs[font->kern_first + at + j - 1].right))) ok = false;
            }
            at += count;
        }
    }
    cache = ok ? heap_caps_calloc(CACHE_ENTRIES, sizeof(*cache), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : NULL;
    fclose(fp);
    if (!ok || !fonts || !cache) {
        free(fonts); fonts = NULL;
        free(cache); cache = NULL;
        free(kern_pairs); kern_pairs = NULL;
        total_glyphs = 0;
        indexed_metadata_bytes = 0;
        total_kern_pairs = 0;
        return false;
    }
    descriptor_bytes = FONT_COUNT * sizeof(*fonts);
    metadata_cache_bytes = CACHE_ENTRIES * sizeof(*cache);
    kerning_cache_bytes = kern_total * sizeof(*kern_pairs);
    available = true;
    goopad_font_bitmap_register_evictor(evict_bitmaps);
    ESP_LOGI(TAG, "GSP1 ready: %lu glyphs, %lu SD metadata bytes, %lu PSRAM descriptor/metric/kerning bytes, shared bitmap budget=%u",
             (unsigned long)total_glyphs, (unsigned long)indexed_metadata_bytes,
             (unsigned long)(descriptor_bytes + metadata_cache_bytes + kerning_cache_bytes), CACHE_BYTES);
    return true;
}

const lv_font_t *goopad_styled_font(unsigned family, unsigned variant, const lv_font_t *fallback)
{
    if (!initialized) initialize();
    if (!available || family >= FAMILY_COUNT || variant >= VARIANT_COUNT) return fallback;
    styled_font_t *font = &fonts[family * VARIANT_COUNT + variant];
    font->font.fallback = fallback;
    return &font->font;
}

void goopad_styled_status(goopad_styled_status_t *status)
{
    *status = (goopad_styled_status_t){
        .available = available, .fonts = available ? FONT_COUNT : 0,
        .glyphs = total_glyphs, .indexed_metadata_bytes = indexed_metadata_bytes,
        .descriptor_bytes = descriptor_bytes, .metadata_cache_bytes = metadata_cache_bytes,
        .kerning_cache_bytes = kerning_cache_bytes, .kerning_pairs = total_kern_pairs,
        .bitmap_cache_bytes = bitmap_cache_bytes, .bitmap_cache_peak = bitmap_cache_peak,
        .shared_bitmap_cache_bytes = goopad_font_bitmap_cache_used(),
        .bitmap_cache_budget = CACHE_BYTES,
        .hits = hits, .reads = reads
    };
}
