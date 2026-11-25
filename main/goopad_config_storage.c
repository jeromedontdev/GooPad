#include "goopad_config_storage.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "goopad_homekit_storage.h"
#include "nvs.h"
#include "psa/crypto.h"

#define CONFIG_STORAGE_MAX_BYTES 12000
#define CONFIG_STORAGE_HEADER_BYTES 36

static const char *TAG = "config_storage";
static const uint8_t magic[4] = {'G', 'P', 'C', 1};
static uint8_t legacy_hash[32];
static esp_err_t storage_error;
static bool migrated;

static esp_err_t hash_config(const char *json, uint8_t hash[32])
{
    size_t written = 0;
    if (psa_crypto_init() != PSA_SUCCESS ||
        psa_hash_compute(PSA_ALG_SHA_256, (const uint8_t *)json, strlen(json),
                         hash, 32, &written) != PSA_SUCCESS || written != 32)
        return ESP_FAIL;
    return ESP_OK;
}

esp_err_t goopad_config_storage_init(void)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("goopad", NVS_READONLY, &nvs);
    uint8_t marker = 0;
    if (err == ESP_OK) {
        err = nvs_get_u8(nvs, "config_moved", &marker);
        nvs_close(nvs);
    }
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
    if (marker > 1) return ESP_ERR_INVALID_STATE;
    migrated = marker == 1;
    storage_error = goopad_homekit_storage_init();
    if (storage_error != ESP_OK && migrated) return storage_error;
    if (storage_error != ESP_OK)
        ESP_LOGW(TAG, "Expanded configuration storage unavailable: %s", esp_err_to_name(storage_error));
    return ESP_OK;
}

esp_err_t goopad_config_storage_load(char *out, size_t capacity, const char *legacy,
                                    bool legacy_available)
{
    if (!out || !legacy || !capacity || strlen(legacy) >= capacity) return ESP_ERR_INVALID_ARG;
    if (hash_config(legacy, legacy_hash) != ESP_OK) return ESP_FAIL;
    if (!migrated) {
        if (out != legacy) memcpy(out, legacy, strlen(legacy) + 1);
        return ESP_OK;
    }
    nvs_handle_t nvs;
    esp_err_t err = nvs_open_from_partition(GOOPAD_HOMEKIT_NVS_PARTITION, "goopad_cfg", NVS_READONLY, &nvs);
    if (err != ESP_OK) return err;
    size_t size = 0;
    err = nvs_get_blob(nvs, "config", NULL, &size);
    if (err == ESP_OK && (size <= CONFIG_STORAGE_HEADER_BYTES ||
                          size > CONFIG_STORAGE_HEADER_BYTES + CONFIG_STORAGE_MAX_BYTES + 1))
        err = ESP_ERR_INVALID_SIZE;
    uint8_t *record = err == ESP_OK ? heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : NULL;
    if (err == ESP_OK && !record) err = ESP_ERR_NO_MEM;
    if (err == ESP_OK) err = nvs_get_blob(nvs, "config", record, &size);
    nvs_close(nvs);
    if (err == ESP_OK) {
        size_t text_size = size - CONFIG_STORAGE_HEADER_BYTES;
        const char *text = (const char *)record + CONFIG_STORAGE_HEADER_BYTES;
        if (memcmp(record, magic, sizeof(magic)) || text_size > capacity ||
            text[text_size - 1] != '\0' || strlen(text) != text_size - 1) {
            err = ESP_ERR_INVALID_STATE;
        } else if (memcmp(record + sizeof(magic), legacy_hash, sizeof(legacy_hash))) {
            if (!legacy_available) {
                err = ESP_ERR_INVALID_STATE;
            } else {
                ESP_LOGW(TAG, "Legacy configuration changed after migration; loading that configuration");
                if (out != legacy) memcpy(out, legacy, strlen(legacy) + 1);
            }
        } else {
            memcpy(out, text, text_size);
        }
    }
    free(record);
    return err;
}

esp_err_t goopad_config_storage_save(const char *json)
{
    if (!json || strlen(json) > CONFIG_STORAGE_MAX_BYTES) return ESP_ERR_INVALID_ARG;
    nvs_handle_t nvs;
    if (storage_error != ESP_OK && !migrated) {
        uint8_t next_hash[sizeof(legacy_hash)];
        if (hash_config(json, next_hash) != ESP_OK) return ESP_FAIL;
        esp_err_t err = nvs_open("goopad", NVS_READWRITE, &nvs);
        if (err != ESP_OK) return err;
        err = nvs_set_blob(nvs, "config", json, strlen(json) + 1);
        if (err == ESP_OK) err = nvs_commit(nvs);
        nvs_close(nvs);
        if (err == ESP_OK) memcpy(legacy_hash, next_hash, sizeof(legacy_hash));
        return err;
    }
    size_t text_size = strlen(json) + 1;
    size_t size = CONFIG_STORAGE_HEADER_BYTES + text_size;
    uint8_t *record = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!record) return ESP_ERR_NO_MEM;
    memcpy(record, magic, sizeof(magic));
    memcpy(record + sizeof(magic), legacy_hash, sizeof(legacy_hash));
    memcpy(record + CONFIG_STORAGE_HEADER_BYTES, json, text_size);
    esp_err_t err = nvs_open_from_partition(GOOPAD_HOMEKIT_NVS_PARTITION, "goopad_cfg", NVS_READWRITE, &nvs);
    if (err == ESP_OK) {
        err = nvs_set_blob(nvs, "config", record, size);
        if (err == ESP_OK) err = nvs_commit(nvs);
        nvs_close(nvs);
    }
    free(record);
    if (err != ESP_OK || migrated) return err;
    err = nvs_open("goopad", NVS_READWRITE, &nvs);
    if (err == ESP_OK) {
        err = nvs_set_u8(nvs, "config_moved", 1);
        if (err == ESP_OK) err = nvs_commit(nvs);
        nvs_close(nvs);
    }
    if (err == ESP_OK) migrated = true;
    return err;
}
