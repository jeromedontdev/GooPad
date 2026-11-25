#include "goopad_unicode.h"
#include "board.h"
#include "goopad_resource_manifest.h"

#include <stdio.h>
#include <stdatomic.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_log.h"

#define FONT_PATH BSP_SD_MOUNT_POINT "/goopad/resources/" GOOPAD_TEXT_FILE
#define CACHE_ENTRIES 512
#define MAX_CODEPOINTS 131072
#define CACHE_BYTES (32 * 1024)
#define MAX_BITMAP CACHE_BYTES
#define SHARED_CACHE_BYTES (32 * 1024)
#define MAX_EVICTORS 2

typedef struct {
    uint32_t cp, offset;
    uint16_t length, advance, width, height;
    int16_t x, y;
    uint8_t encoding;
    uint16_t checksum, record_checksum;
} glyph_t;

typedef struct {
    uint32_t count, index, begin, end;
    uint16_t size;
    lv_font_t font;
} font_t;

typedef struct {
    font_t *font;
    uint32_t index, stamp, bitmap_stamp;
    glyph_t glyph;
    uint8_t *bitmap;
    uint16_t bytes, pins;
} cache_t;

static const char *TAG = "unicode";
static font_t fonts[10];
static cache_t *cache;
static uint32_t *codepoints;
static struct {
    atomic_bool available;
    atomic_uint_least32_t glyphs, cache_bytes, cache_peak, index_bytes, metrics_bytes, hits, reads;
} health;
static bool initialized;
static uint32_t clock_tick;
static goopad_font_bitmap_evictor_t bitmap_evictors[MAX_EVICTORS];
static unsigned bitmap_evictor_count;
static uint32_t shared_bitmap_cache_bytes;
static void discard(cache_t *entry);
static cache_t *oldest(cache_t *exclude, bool bitmap_only);

void goopad_font_bitmap_register_evictor(goopad_font_bitmap_evictor_t evictor)
{
    for (unsigned i = 0; i < bitmap_evictor_count; i++)
        if (bitmap_evictors[i] == evictor) return;
    if (evictor && bitmap_evictor_count < MAX_EVICTORS)
        bitmap_evictors[bitmap_evictor_count++] = evictor;
}

bool goopad_font_bitmap_reserve(uint32_t bytes, const void *exclude)
{
    if (bytes > SHARED_CACHE_BYTES) return false;
    while (shared_bitmap_cache_bytes + bytes > SHARED_CACHE_BYTES) {
        uint32_t oldest_stamp = UINT32_MAX, candidate_stamp, candidate_bytes;
        unsigned selected = MAX_EVICTORS;
        for (unsigned i = 0; i < bitmap_evictor_count; i++) {
            if (bitmap_evictors[i](exclude, UINT32_MAX, &candidate_stamp, &candidate_bytes) &&
                candidate_stamp < oldest_stamp) {
                oldest_stamp = candidate_stamp;
                selected = i;
            }
        }
        if (selected == MAX_EVICTORS ||
            !bitmap_evictors[selected](exclude, oldest_stamp, &candidate_stamp, &candidate_bytes))
            return false;
    }
    shared_bitmap_cache_bytes += bytes;
    return true;
}

void goopad_font_bitmap_release(uint32_t bytes)
{
    shared_bitmap_cache_bytes = bytes > shared_bitmap_cache_bytes ? 0 : shared_bitmap_cache_bytes - bytes;
}

uint32_t goopad_font_bitmap_cache_used(void)
{
    return shared_bitmap_cache_bytes;
}

uint32_t goopad_font_bitmap_cache_stamp(void)
{
    static uint32_t stamp;
    return ++stamp;
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

static bool failed(const char *message)
{
    if (health.available) ESP_LOGE(TAG, "%s; multilingual SD text unavailable", message);
    health.available = false;
    return false;
}

static bool read_at(FILE *fp, uint32_t offset, void *out, size_t length)
{
    health.reads++;
    return fseek(fp, offset, SEEK_SET) == 0 && fread(out, 1, length, fp) == length;
}

static bool record(FILE *fp, font_t *font, uint32_t index, glyph_t *glyph)
{
    uint8_t b[24];
    if (index >= font->count || !read_at(fp, font->index + index * 24, b, sizeof(b)))
        return failed("Could not read glyph index");
    *glyph = (glyph_t){.cp = u32(b), .offset = u32(b + 4), .length = u16(b + 8),
        .advance = u16(b + 10), .width = u16(b + 12), .height = u16(b + 14),
        .x = (int16_t)u16(b + 16), .y = (int16_t)u16(b + 18), .encoding = b[20],
        .checksum = u16(b + 22), .record_checksum = crc16(0xffff, b, 22)};
    size_t raw = ((size_t)glyph->width * glyph->height + 3) / 4;
    if (glyph->cp > 0x10ffff || (glyph->cp >= 0xd800 && glyph->cp <= 0xdfff) ||
        glyph->width > 1024 || glyph->height > 128 || raw > MAX_BITMAP || glyph->length > MAX_BITMAP ||
        glyph->offset < font->begin || glyph->offset > font->end ||
        glyph->length > font->end - glyph->offset || b[21] != 2 || glyph->encoding > 1 ||
        (!glyph->encoding && glyph->length != raw) ||
        (!raw && glyph->checksum != glyph->record_checksum))
        return failed("Invalid glyph record");
    return true;
}

static void discard(cache_t *entry)
{
    if (entry->bitmap) goopad_font_bitmap_release(entry->bytes);
    health.cache_bytes -= entry->bytes;
    free(entry->bitmap);
    entry->bitmap = NULL;
    entry->bytes = 0;
}

static cache_t *oldest(cache_t *exclude, bool bitmap_only)
{
    cache_t *result = NULL;
    for (unsigned i = 0; i < CACHE_ENTRIES; i++) {
        cache_t *entry = &cache[i];
        if (entry == exclude || entry->pins || (bitmap_only && !entry->bitmap)) continue;
        if (!result ||
            (!bitmap_only && !entry->bitmap && result->bitmap) ||
            ((bitmap_only || !!entry->bitmap == !!result->bitmap) &&
             clock_tick - entry->stamp > clock_tick - result->stamp)) result = entry;
    }
    return result;
}

static cache_t *find(font_t *font, uint32_t value, bool by_index)
{
    for (unsigned i = 0; i < CACHE_ENTRIES; i++) {
        cache_t *entry = &cache[i];
        if (entry->font == font && (by_index ? entry->index == value : entry->glyph.cp == value)) {
            entry->stamp = ++clock_tick;
            if (entry->bitmap) entry->bitmap_stamp = goopad_font_bitmap_cache_stamp();
            health.hits++;
            return entry;
        }
    }
    return NULL;
}

static cache_t *retain(font_t *font, uint32_t index, const glyph_t *glyph)
{
    cache_t *entry = oldest(NULL, false);
    if (!entry) { failed("Glyph cache is exhausted"); return NULL; }
    discard(entry);
    *entry = (cache_t){.font = font, .index = index, .glyph = *glyph, .stamp = ++clock_tick};
    return entry;
}

static bool glyph_dsc(const lv_font_t *font, lv_font_glyph_dsc_t *out, uint32_t cp, uint32_t next)
{
    (void)next;
    if (!health.available) return false;
    font_t *source = font->user_data;
    cache_t *entry = find(source, cp, false);
    if (!entry) {
        uint32_t low = 0, high = source->count;
        while (low < high) {
            uint32_t mid = low + (high - low) / 2;
            if (codepoints[mid] < cp) low = mid + 1;
            else high = mid;
        }
        if (low == source->count || codepoints[low] != cp) return false;
        FILE *fp = fopen(FONT_PATH, "rb");
        if (!fp) return failed("Could not open text pack");
        glyph_t glyph = {0};
        bool valid = record(fp, source, low, &glyph);
        fclose(fp);
        if (!valid) return false;
        if (glyph.cp != cp) return failed("Glyph codepoint index is inconsistent");
        entry = retain(source, low, &glyph);
        if (!entry) return false;
    }
    const glyph_t *g = &entry->glyph;
    out->adv_w = (g->advance + 8) / 16;
    out->box_w = g->width; out->box_h = g->height;
    out->ofs_x = g->x; out->ofs_y = g->y;
    out->format = LV_FONT_GLYPH_FORMAT_A2;
    out->gid.index = entry->index;
    out->stride = 0;
    out->is_placeholder = false;
    out->entry = NULL;
    return true;
}

static const void *glyph_bitmap(lv_font_glyph_dsc_t *dsc, lv_draw_buf_t *draw)
{
    if (!health.available || !dsc->box_w || !dsc->box_h) return NULL;
    font_t *font = dsc->resolved_font->user_data;
    cache_t *entry = find(font, dsc->gid.index, true);
    FILE *fp = NULL;
    if (!entry) {
        glyph_t g;
        fp = fopen(FONT_PATH, "rb");
        if (!fp) { failed("Could not open text pack"); return NULL; }
        bool ok = record(fp, font, dsc->gid.index, &g);
        if (ok) entry = retain(font, dsc->gid.index, &g);
        if (!entry) { fclose(fp); return NULL; }
    }
    if (!entry->bitmap) {
        const glyph_t *g = &entry->glyph;
        unsigned pixels = (unsigned)g->width * g->height;
        unsigned bytes = (pixels + 3) / 4;
        while (health.cache_bytes + bytes > CACHE_BYTES) {
            cache_t *victim = oldest(entry, true);
            if (!victim) break;
            discard(victim);
        }
        bool reserved = bytes && health.cache_bytes + bytes <= CACHE_BYTES &&
            goopad_font_bitmap_reserve(bytes, entry);
        uint8_t *raw = reserved ?
            heap_caps_calloc(1, bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : NULL;
        if (!fp) fp = fopen(FONT_PATH, "rb");
        bool ok = raw && fp;
        uint16_t checksum = g->record_checksum;
        if (ok && !g->encoding) {
            ok = read_at(fp, g->offset, raw, bytes);
            if (ok) checksum = crc16(checksum, raw, bytes);
        }
        else if (ok) {
            uint8_t encoded[256];
            ok = fseek(fp, g->offset, SEEK_SET) == 0;
            unsigned at = 0;
            for (unsigned offset = 0; ok && offset < g->length;) {
                unsigned chunk = g->length - offset;
                if (chunk > sizeof(encoded)) chunk = sizeof(encoded);
                health.reads++;
                ok = fread(encoded, 1, chunk, fp) == chunk;
                if (ok) checksum = crc16(checksum, encoded, chunk);
                offset += chunk;
                for (unsigned i = 0; ok && i < chunk; i++) {
                    unsigned run = (encoded[i] >> 2) + 1;
                    if (run > pixels - at) { ok = false; break; }
                    while (run--) {
                        raw[at >> 2] |= (encoded[i] & 3) << (6 - 2 * (at & 3));
                        at++;
                    }
                }
            }
            ok = ok && at == pixels;
        }
        if (fp) { fclose(fp); fp = NULL; }
        if (!ok || checksum != g->checksum) {
            free(raw);
            if (reserved) goopad_font_bitmap_release(bytes);
            failed("Could not decode/verify glyph bitmap");
            return NULL;
        }
        entry->bitmap = raw; entry->bytes = bytes;
        entry->bitmap_stamp = goopad_font_bitmap_cache_stamp();
        health.cache_bytes += bytes;
        if (health.cache_bytes > health.cache_peak) health.cache_peak = health.cache_bytes;
    }
    if (fp) fclose(fp);
    if (dsc->req_raw_bitmap) {
        entry->pins++;
        dsc->entry = entry;
        return entry->bitmap;
    }
    if (!draw) { failed("Glyph draw buffer unavailable"); return NULL; }
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

static void initialize(void)
{
    initialized = true;
    FILE *fp = fopen(FONT_PATH, "rb");
    if (!fp) { ESP_LOGW(TAG, "SD text pack absent; using essential offline fonts"); return; }
    uint8_t header[32], dir[32];
    bool ok = fseek(fp, 0, SEEK_END) == 0;
    long length = ok ? ftell(fp) : -1;
    ok = length >= 352 && length <= 96 * 1024 * 1024 && read_at(fp, 0, header, sizeof(header)) &&
        !memcmp(header, "GFP2", 4) && u32(header + 4) == (uint32_t)length &&
        u32(header + 8) == 10 && u32(header + 12) == 32;
    static const unsigned sizes[] = {8, 10, 12, 14, 16, 20, 24, 28, 36, 40};
    for (unsigned i = 0; ok && i < 10; i++) {
        ok = read_at(fp, 32 + i * 32, dir, sizeof(dir));
        if (!ok) break;
        font_t *f = &fonts[i];
        f->size = u16(dir); f->count = u32(dir + 8); f->index = u32(dir + 12);
        f->begin = u32(dir + 16); f->end = u32(dir + 20);
        ok = f->size == sizes[i] && f->count && f->count <= MAX_CODEPOINTS &&
            f->count == fonts[0].count && f->index >= 352 &&
            (uint64_t)f->index + (uint64_t)f->count * 24 <= f->begin &&
            f->begin <= f->end && f->end <= (uint32_t)length && u16(dir + 2) > 0 &&
            u16(dir + 2) <= 256 && (int16_t)u16(dir + 4) >= 0 && u16(dir + 4) <= u16(dir + 2);
        f->font = (lv_font_t){.line_height = u16(dir + 2), .base_line = (int16_t)u16(dir + 4),
            .get_glyph_dsc = glyph_dsc, .get_glyph_bitmap = glyph_bitmap, .release_glyph = release_glyph,
            .user_data = f, .static_bitmap = false};
    }
    codepoints = ok ? heap_caps_calloc(fonts[0].count, sizeof(*codepoints), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : NULL;
    ok = ok && codepoints && fseek(fp, fonts[0].index, SEEK_SET) == 0;
    uint8_t records[24 * 32];
    for (uint32_t start = 0; ok && start < fonts[0].count;) {
        unsigned count = fonts[0].count - start;
        if (count > 32) count = 32;
        health.reads++;
        ok = fread(records, 24, count, fp) == count;
        for (unsigned i = 0; ok && i < count; i++) {
            uint32_t cp = u32(records + i * 24);
            ok = cp >= 0x20 && cp <= 0x10ffff && !(cp >= 0xd800 && cp <= 0xdfff) &&
                (start + i == 0 || cp > codepoints[start + i - 1]);
            codepoints[start + i] = cp;
        }
        start += count;
    }
    fclose(fp);
    cache = ok ? heap_caps_calloc(CACHE_ENTRIES, sizeof(*cache), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : NULL;
    health.available = ok && cache;
    if (!health.available) { free(codepoints); codepoints = NULL; }
    else goopad_font_bitmap_register_evictor(evict_bitmaps);
    health.index_bytes = codepoints ? fonts[0].count * sizeof(*codepoints) : 0;
    health.metrics_bytes = cache ? CACHE_ENTRIES * sizeof(*cache) : 0;
    health.glyphs = health.available ? fonts[0].count : 0;
    if (!health.available) ESP_LOGE(TAG, "SD text pack invalid or cache allocation failed; using offline fonts");
    else ESP_LOGI(TAG, "SD Unicode ready: %lu glyphs/size; bitmap budget=%u bytes",
                  (unsigned long)health.glyphs, CACHE_BYTES);
}

const lv_font_t *goopad_unicode_font(unsigned size, const lv_font_t *offline)
{
    if (!initialized) initialize();
    for (unsigned i = 0; i < 10; i++) if (fonts[i].size == size) {
        fonts[i].font.fallback = offline;
        return health.available ? &fonts[i].font : offline;
    }
    return offline;
}

void goopad_unicode_status(goopad_unicode_status_t *status)
{
    *status = (goopad_unicode_status_t){.available = health.available, .glyphs = health.glyphs,
        .cache_bytes = health.cache_bytes, .cache_peak = health.cache_peak,
        .codepoint_index_bytes = health.index_bytes, .metrics_cache_bytes = health.metrics_bytes,
        .hits = health.hits, .reads = health.reads};
}
