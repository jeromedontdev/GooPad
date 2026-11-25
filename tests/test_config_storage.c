#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "goopad_config_storage.h"
#include "goopad_homekit_storage.h"
#include "nvs.h"

static uint8_t blob[13000], legacy_blob[13000], pending[13000], marker;
static size_t blob_size, legacy_size, pending_size;
static nvs_handle_t pending_handle;
static bool marker_present, pending_marker, fail_blob, fail_marker;
static esp_err_t store_init = ESP_OK;

const char *esp_err_to_name(esp_err_t err) { (void)err; return "test"; }
esp_err_t goopad_homekit_storage_init(void) { return store_init; }
int psa_crypto_init(void) { return 0; }
int psa_hash_compute(int alg, const uint8_t *input, size_t length, uint8_t *output,
                     size_t capacity, size_t *written)
{
    (void)alg;
    assert(capacity >= 32);
    memset(output, 0, 32);
    for (size_t i = 0; i < length; i++) output[i % 32] = (uint8_t)(output[i % 32] * 33U + input[i]);
    *written = 32;
    return 0;
}
esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *handle)
{
    (void)name; (void)mode; *handle = 1; return ESP_OK;
}
esp_err_t nvs_open_from_partition(const char *partition, const char *name, int mode, nvs_handle_t *handle)
{
    (void)partition; (void)name; (void)mode; *handle = 2; return ESP_OK;
}
esp_err_t nvs_get_u8(nvs_handle_t handle, const char *key, uint8_t *value)
{
    (void)handle; (void)key;
    if (!marker_present) return ESP_ERR_NVS_NOT_FOUND;
    *value = marker; return ESP_OK;
}
esp_err_t nvs_set_u8(nvs_handle_t handle, const char *key, uint8_t value)
{
    (void)handle; (void)key; (void)value;
    if (fail_marker) return ESP_ERR_NVS_NOT_ENOUGH_SPACE;
    pending_marker = true; return ESP_OK;
}
esp_err_t nvs_get_blob(nvs_handle_t handle, const char *key, void *out, size_t *length)
{
    (void)handle; (void)key;
    if (!blob_size) return ESP_ERR_NVS_NOT_FOUND;
    if (out) {
        if (*length < blob_size) return ESP_ERR_INVALID_SIZE;
        memcpy(out, blob, blob_size);
    }
    *length = blob_size; return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key, const void *value, size_t length)
{
    (void)handle; (void)key;
    if (fail_blob) return ESP_ERR_NVS_NOT_ENOUGH_SPACE;
    assert(length <= sizeof(pending));
    memcpy(pending, value, length); pending_size = length; pending_handle = handle; return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t handle)
{
    if (handle == 2 && pending_size && pending_handle == handle) {
        memcpy(blob, pending, pending_size); blob_size = pending_size; pending_size = 0;
    }
    if (handle == 1 && pending_size && pending_handle == handle) {
        memcpy(legacy_blob, pending, pending_size); legacy_size = pending_size; pending_size = 0;
    }
    if (handle == 1 && pending_marker) {
        marker_present = true; marker = 1; pending_marker = false;
    }
    return ESP_OK;
}
void nvs_close(nvs_handle_t handle) { (void)handle; }

int main(void)
{
    char out[12001], large[12001];
    const char *legacy = "{\"old\":1}", *newer = "{\"now_playing\":true}";
    assert(goopad_config_storage_init() == ESP_OK);
    assert(goopad_config_storage_load(out, sizeof(out), legacy, true) == ESP_OK);
    assert(!strcmp(out, legacy));
    fail_marker = true;
    assert(goopad_config_storage_save(newer) == ESP_ERR_NVS_NOT_ENOUGH_SPACE);
    assert(!marker_present);
    assert(goopad_config_storage_init() == ESP_OK);
    assert(goopad_config_storage_load(out, sizeof(out), legacy, true) == ESP_OK && !strcmp(out, legacy));
    fail_marker = false;
    assert(goopad_config_storage_save(newer) == ESP_OK);
    assert(marker_present);
    assert(goopad_config_storage_init() == ESP_OK);
    assert(goopad_config_storage_load(out, sizeof(out), legacy, true) == ESP_OK && !strcmp(out, newer));
    memset(large, 'x', sizeof(large) - 1); large[sizeof(large) - 1] = '\0';
    assert(goopad_config_storage_save(large) == ESP_OK);
    assert(goopad_config_storage_init() == ESP_OK);
    assert(goopad_config_storage_load(out, sizeof(out), legacy, true) == ESP_OK && !strcmp(out, large));
    fail_blob = true;
    assert(goopad_config_storage_save(newer) == ESP_ERR_NVS_NOT_ENOUGH_SPACE);
    assert(goopad_config_storage_load(out, sizeof(out), legacy, true) == ESP_OK && !strcmp(out, large));
    fail_blob = false;
    assert(goopad_config_storage_load(out, sizeof(out), "{\"old\":2}", true) == ESP_OK);
    assert(!strcmp(out, "{\"old\":2}"));
    assert(goopad_config_storage_save(newer) == ESP_OK);
    assert(goopad_config_storage_init() == ESP_OK);
    assert(goopad_config_storage_load(out, sizeof(out), "{\"old\":2}", true) == ESP_OK && !strcmp(out, newer));
    assert(goopad_config_storage_load(out, sizeof(out), "{}", false) == ESP_ERR_INVALID_STATE);
    blob[0] ^= 1;
    assert(goopad_config_storage_load(out, sizeof(out), "{\"old\":2}", true) == ESP_ERR_INVALID_STATE);
    store_init = ESP_FAIL;
    assert(goopad_config_storage_init() == ESP_FAIL);
    marker_present = false;
    assert(goopad_config_storage_init() == ESP_OK);
    assert(goopad_config_storage_load(out, sizeof(out), legacy, true) == ESP_OK);
    assert(goopad_config_storage_save(newer) == ESP_OK);
    assert(legacy_size == strlen(newer) + 1 && !strcmp((const char *)legacy_blob, newer));
    assert(!marker_present);
    puts("Configuration storage migration tests passed");
    return 0;
}
