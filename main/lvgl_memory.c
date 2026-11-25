#include "lvgl.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#include <stdlib.h>

#if LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM
static const char *TAG = "lvgl_memory";

void lv_mem_init(void) {}
void lv_mem_deinit(void) {}
lv_mem_pool_t lv_mem_add_pool(void *mem, size_t bytes)
{
    (void)mem;
    (void)bytes;
    return NULL;
}
void lv_mem_remove_pool(lv_mem_pool_t pool) { (void)pool; }

void *lv_malloc_core(size_t size)
{
    void *ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!ptr) {
        ESP_LOGW(TAG, "PSRAM allocation failed for %u bytes; trying the normal heap", (unsigned)size);
        ptr = malloc(size);
    }
    return ptr;
}

void *lv_realloc_core(void *ptr, size_t size)
{
    void *result = heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!result) {
        ESP_LOGW(TAG, "PSRAM reallocation failed for %u bytes; trying the normal heap", (unsigned)size);
        result = realloc(ptr, size);
    }
    return result;
}

void lv_free_core(void *ptr) { free(ptr); }

void lv_mem_monitor_core(lv_mem_monitor_t *monitor)
{
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    *monitor = (lv_mem_monitor_t) {
        .total_size = info.total_free_bytes + info.total_allocated_bytes,
        .free_cnt = info.free_blocks,
        .free_size = info.total_free_bytes,
        .free_biggest_size = info.largest_free_block,
        .used_cnt = info.allocated_blocks,
    };
    if (monitor->total_size)
        monitor->used_pct = (uint8_t)(info.total_allocated_bytes * 100 / monitor->total_size);
    if (monitor->free_size)
        monitor->frag_pct = (uint8_t)(100 - info.largest_free_block * 100 / monitor->free_size);
}

lv_result_t lv_mem_test_core(void)
{
    return heap_caps_check_integrity(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT, true)
        ? LV_RESULT_OK : LV_RESULT_INVALID;
}
#endif
