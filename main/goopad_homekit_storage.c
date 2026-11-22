#include "goopad_homekit_storage.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "esp_idf_version.h"
#include "esp_log.h"
#include "sdkconfig.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 1, 0) && CONFIG_NVS_BDL_STACK
#include "esp_blockdev.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"

#define SECTOR_SIZE 4096U
#define STORE_SIZE 0x10000U
#define RESERVED_SIZE (SECTOR_SIZE + STORE_SIZE)
#define CLAIMED 0xfe
#define READY 0xfc
#define META_MAGIC UINT32_C(0x47504831)

static const char *TAG = "homekit_storage";
static const uint8_t signature[] = {
    'G', 'o', 'o', 'P', 'a', 'd', '-', 'H', 'A', 'P', '-', 'N', 'V', 'S', 1, 0
};
static const esp_partition_t *parent;
static size_t base;
static bool initialized;

static bool fits(uint64_t offset, size_t length)
{
    return offset <= STORE_SIZE && length <= STORE_SIZE - offset;
}

static esp_err_t read_block(esp_blockdev_handle_t dev, uint8_t *dst, size_t capacity,
                            uint64_t offset, size_t length)
{
    (void)dev;
    if (!dst || length > capacity || !fits(offset, length)) return ESP_ERR_INVALID_SIZE;
    return esp_partition_read(parent, base + SECTOR_SIZE + offset, dst, length);
}

static esp_err_t write_block(esp_blockdev_handle_t dev, const uint8_t *src,
                             uint64_t offset, size_t length)
{
    (void)dev;
    if (!src || !fits(offset, length)) return ESP_ERR_INVALID_SIZE;
    return esp_partition_write(parent, base + SECTOR_SIZE + offset, src, length);
}

static esp_err_t erase_block(esp_blockdev_handle_t dev, uint64_t offset, size_t length)
{
    (void)dev;
    if (!fits(offset, length) || offset % SECTOR_SIZE || length % SECTOR_SIZE)
        return ESP_ERR_INVALID_SIZE;
    return esp_partition_erase_range(parent, base + SECTOR_SIZE + offset, length);
}

static const esp_blockdev_ops_t ops = {
    .read = read_block, .write = write_block, .erase = erase_block,
};

static esp_blockdev_t device = {
    .device_flags = ESP_BLOCKDEV_FLAGS_CONFIG_DEFAULT(),
    .geometry = {.disk_size = STORE_SIZE, .read_size = 1, .write_size = 1, .erase_size = SECTOR_SIZE},
    .ops = &ops,
};

static esp_err_t check_erased(size_t begin, size_t end)
{
    uint8_t buffer[512];
    while (begin < end) {
        size_t length = end - begin < sizeof(buffer) ? end - begin : sizeof(buffer);
        esp_err_t err = esp_partition_read(parent, begin, buffer, length);
        if (err != ESP_OK) return err;
        for (size_t i = 0; i < length; i++)
            if (buffer[i] != 0xff) return ESP_ERR_INVALID_STATE;
        begin += length;
    }
    return ESP_OK;
}

esp_err_t goopad_homekit_storage_init(void)
{
    if (initialized) return ESP_OK;
    parent = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, "storage");
    if (!parent || parent->encrypted || parent->size < RESERVED_SIZE ||
        parent->size % SECTOR_SIZE) return ESP_ERR_NOT_SUPPORTED;
    base = parent->size - RESERVED_SIZE;

    uint8_t header[sizeof(signature) + 1];
    esp_err_t err = esp_partition_read(parent, base, header, sizeof(header));
    if (err != ESP_OK) return err;
    bool blank_header = true;
    for (size_t i = 0; i < sizeof(header); i++) blank_header &= header[i] == 0xff;
    bool valid_header = memcmp(header, signature, sizeof(signature)) == 0;
    if (!blank_header && !valid_header) return ESP_ERR_INVALID_STATE;
    uint8_t state = header[sizeof(signature)];
    if (valid_header && state != 0xff && state != CLAIMED && state != READY)
        return ESP_ERR_INVALID_STATE;

    err = check_erased(0, base);
    if (err == ESP_OK) err = check_erased(base + sizeof(header), base + SECTOR_SIZE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Storage partition contains data outside the reserved area");
        return err;
    }
    if (blank_header || state == 0xff) {
        err = check_erased(base + SECTOR_SIZE, parent->size);
        if (err != ESP_OK) return err;
        if (blank_header) {
            err = esp_partition_write(parent, base, signature, sizeof(signature));
            if (err != ESP_OK) return err;
        }
        state = CLAIMED;
        err = esp_partition_write(parent, base + sizeof(signature), &state, 1);
        if (err != ESP_OK) return err;
    }

    err = nvs_flash_init_partition_bdl(GOOPAD_HOMEKIT_NVS_PARTITION, &device);
    if (err != ESP_OK) return err;
    nvs_handle_t nvs;
    err = nvs_open_from_partition(GOOPAD_HOMEKIT_NVS_PARTITION, "gp_meta", NVS_READWRITE, &nvs);
    if (err == ESP_OK) {
        uint32_t magic = 0;
        err = nvs_get_u32(nvs, "magic", &magic);
        if (err == ESP_ERR_NVS_NOT_FOUND && state == CLAIMED) {
            err = nvs_set_u32(nvs, "magic", META_MAGIC);
            if (err == ESP_OK) err = nvs_commit(nvs);
        } else if (err == ESP_OK && magic != META_MAGIC) {
            err = ESP_ERR_INVALID_STATE;
        }
        nvs_close(nvs);
    }
    if (err == ESP_OK && state == CLAIMED) {
        state = READY;
        err = esp_partition_write(parent, base + sizeof(signature), &state, 1);
    }
    if (err != ESP_OK) {
        esp_err_t deinit_err = nvs_flash_deinit_partition(GOOPAD_HOMEKIT_NVS_PARTITION);
        if (deinit_err != ESP_OK) ESP_LOGE(TAG, "NVS cleanup failed: %s", esp_err_to_name(deinit_err));
        return err;
    }
    initialized = true;
    return ESP_OK;
}
#else
esp_err_t goopad_homekit_storage_init(void)
{
    ESP_LOGE("homekit_storage", "HomeKit requires ESP-IDF 6.1 and NVS block-device support");
    return ESP_ERR_NOT_SUPPORTED;
}
#endif
