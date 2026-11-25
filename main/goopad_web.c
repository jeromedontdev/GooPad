#include "goopad_web.h"
#include "board.h"
#include "goopad_web_manifest.h"

#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "psa/crypto.h"

typedef struct {
    const char *name;
    uint32_t offset, length;
    const char *sha256;
} asset_t;

#define ASSET(name, offset, length, hash) {name, offset, length, hash},
static const asset_t assets[] = {GOOPAD_WEB_ASSETS(ASSET)};
#undef ASSET

static const asset_t *lookup(const char *name)
{
    for (unsigned i = 0; name && i < sizeof(assets) / sizeof(assets[0]); i++)
        if (!strcmp(name, assets[i].name)) return &assets[i];
    return NULL;
}

bool goopad_web_known(const char *name)
{
    return lookup(name) != NULL;
}

esp_err_t goopad_web_open(const char *name, goopad_web_asset_t *asset)
{
    *asset = (goopad_web_asset_t){0};
    const asset_t *source = lookup(name);
    if (!source) return ESP_ERR_NOT_FOUND;
    FILE *file = fopen(BSP_SD_MOUNT_POINT "/goopad/resources/" GOOPAD_WEB_FILE, "rb");
    if (!file) {
        ESP_LOGW("web", "SD web resource unavailable");
        return ESP_ERR_NOT_FOUND;
    }
    uint8_t header[8] = {0};
    bool ok = fseek(file, 0, SEEK_END) == 0 && ftell(file) == GOOPAD_WEB_BYTES &&
        fseek(file, 0, SEEK_SET) == 0 && fread(header, 1, sizeof(header), file) == sizeof(header) &&
        !memcmp(header, "GWP1", 4);
    uint32_t length = (uint32_t)header[4] | (uint32_t)header[5] << 8 |
                      (uint32_t)header[6] << 16 | (uint32_t)header[7] << 24;
    ok = ok && length == GOOPAD_WEB_BYTES && source->offset <= length &&
        source->length <= length - source->offset && fseek(file, source->offset, SEEK_SET) == 0;
    psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;
    ok = ok && psa_hash_setup(&operation, PSA_ALG_SHA_256) == PSA_SUCCESS;
    uint8_t buffer[1024], digest[32];
    size_t remaining = source->length, digest_size = 0;
    while (ok && remaining) {
        size_t chunk = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        ok = fread(buffer, 1, chunk, file) == chunk &&
            psa_hash_update(&operation, buffer, chunk) == PSA_SUCCESS;
        remaining -= chunk;
    }
    ok = ok && psa_hash_finish(&operation, digest, sizeof(digest), &digest_size) == PSA_SUCCESS &&
        digest_size == sizeof(digest);
    psa_hash_abort(&operation);
    char hash[65];
    if (ok) {
        for (unsigned i = 0; i < sizeof(digest); i++) snprintf(hash + i * 2, 3, "%02x", digest[i]);
        ok = !strcmp(hash, source->sha256) && fseek(file, source->offset, SEEK_SET) == 0;
    }
    if (!ok) {
        fclose(file);
        ESP_LOGE("web", "SD web resource size/read/integrity check failed");
        return ESP_ERR_INVALID_CRC;
    }
    *asset = (goopad_web_asset_t){.file = file, .length = source->length};
    return ESP_OK;
}
