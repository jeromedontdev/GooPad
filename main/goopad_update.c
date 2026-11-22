#include "goopad_update.h"

#include "cJSON.h"
#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "psa/crypto.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define RELEASE_API "https://api.github.com/repos/jeromedontdev/GooPad/releases/latest"
#define DOWNLOAD_PREFIX "https://github.com/jeromedontdev/GooPad/releases/download/"
#define ASSET_PREFIX "https://release-assets.githubusercontent.com/"
#define RELEASE_MAX_BYTES 65536
#define REDIRECT_MAX_BYTES 4096

static const char *TAG = "goopad_update";

typedef struct {
    char version[GOOPAD_UPDATE_VERSION_SIZE];
    char tag[GOOPAD_UPDATE_VERSION_SIZE + 1];
    uint8_t digest[32];
    size_t size;
} release_t;

static bool parse_version(const char *text, unsigned parts[3])
{
    if (!text || !*text || strlen(text) >= GOOPAD_UPDATE_VERSION_SIZE) return false;
    for (unsigned i = 0; i < 3; i++) {
        unsigned value = 0, digits = 0;
        while (*text >= '0' && *text <= '9') {
            if (++digits > 5) return false;
            value = value * 10 + (unsigned)(*text++ - '0');
        }
        if (!digits || value > 65535 || (i < 2 && *text++ != '.')) return false;
        parts[i] = value;
    }
    return *text == '\0';
}

static esp_http_client_handle_t new_client(const char *url, http_event_handle_cb event, void *data)
{
    esp_http_client_config_t config = {
        .url = url, .timeout_ms = 15000, .crt_bundle_attach = esp_crt_bundle_attach,
        .disable_auto_redirect = true, .event_handler = event, .user_data = data,
        // GitHub's signed asset redirect can exceed ESP-IDF's 512-byte request buffer.
        .buffer_size_tx = !strncmp(url, ASSET_PREFIX, strlen(ASSET_PREFIX)) ? REDIRECT_MAX_BYTES + 16 : 0,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client && esp_http_client_set_header(client, "User-Agent", "GooPad") != ESP_OK) {
        esp_http_client_cleanup(client);
        return NULL;
    }
    return client;
}

static int hex_digit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static esp_err_t read_release(release_t *release)
{
    esp_err_t result = ESP_FAIL;
    char *data = heap_caps_malloc(RELEASE_MAX_BYTES + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!data) return ESP_ERR_NO_MEM;
    esp_http_client_handle_t client = new_client(RELEASE_API, NULL, NULL);
    cJSON *root = NULL;
    if (!client) { result = ESP_ERR_NO_MEM; goto done; }
    if (esp_http_client_open(client, 0) != ESP_OK || esp_http_client_fetch_headers(client) < 0 ||
        esp_http_client_get_status_code(client) != 200) goto done;
    size_t used = 0;
    while (used < RELEASE_MAX_BYTES) {
        int n = esp_http_client_read(client, data + used, (int)(RELEASE_MAX_BYTES - used));
        if (n < 0) goto done;
        if (!n) break;
        used += (size_t)n;
    }
    if (!esp_http_client_is_complete_data_received(client)) goto done;
    data[used] = '\0';
    root = cJSON_Parse(data);
    cJSON *tag = root ? cJSON_GetObjectItemCaseSensitive(root, "tag_name") : NULL;
    cJSON *assets = root ? cJSON_GetObjectItemCaseSensitive(root, "assets") : NULL;
    const char *version = cJSON_IsString(tag) ? tag->valuestring : NULL;
    if (version && *version == 'v') version++;
    unsigned parts[3];
    if (!parse_version(version, parts) || !cJSON_IsArray(assets)) goto done;
    strlcpy(release->version, version, sizeof(release->version));
    strlcpy(release->tag, tag->valuestring, sizeof(release->tag));
    char expected[64];
    snprintf(expected, sizeof(expected), "GooPad-ota-%s.bin", version);
    cJSON *asset;
    cJSON_ArrayForEach(asset, assets) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(asset, "name");
        if (!cJSON_IsString(name) || strcmp(name->valuestring, expected)) continue;
        cJSON *size = cJSON_GetObjectItemCaseSensitive(asset, "size");
        cJSON *digest = cJSON_GetObjectItemCaseSensitive(asset, "digest");
        if (!cJSON_IsNumber(size) || size->valuedouble != size->valueint || size->valueint <= 0 ||
            !cJSON_IsString(digest) || strlen(digest->valuestring) != 71 ||
            strncmp(digest->valuestring, "sha256:", 7)) goto done;
        for (size_t i = 0; i < sizeof(release->digest); i++) {
            int hi = hex_digit(digest->valuestring[7 + i * 2]);
            int lo = hex_digit(digest->valuestring[8 + i * 2]);
            if (hi < 0 || lo < 0) goto done;
            release->digest[i] = (uint8_t)((hi << 4) | lo);
        }
        release->size = (size_t)size->valueint;
        result = ESP_OK;
        break;
    }
done:
    if (result != ESP_OK) ESP_LOGE(TAG, "Could not read a valid GitHub release (%s)", esp_err_to_name(result));
    cJSON_Delete(root);
    if (client) esp_http_client_cleanup(client);
    free(data);
    return result;
}

static esp_err_t newer_than_running(const char *version, bool *available)
{
    unsigned latest[3], current[3];
    if (!parse_version(version, latest) ||
        !parse_version(esp_app_get_description()->version, current)) return ESP_ERR_INVALID_VERSION;
    *available = false;
    for (unsigned i = 0; i < 3; i++) {
        if (latest[i] != current[i]) {
            *available = latest[i] > current[i];
            break;
        }
    }
    return ESP_OK;
}

esp_err_t goopad_update_check(char version[GOOPAD_UPDATE_VERSION_SIZE], bool *available)
{
    if (!version || !available) return ESP_ERR_INVALID_ARG;
    release_t release = {0};
    esp_err_t result = read_release(&release);
    if (result == ESP_OK) result = newer_than_running(release.version, available);
    if (result == ESP_OK) strlcpy(version, release.version, GOOPAD_UPDATE_VERSION_SIZE);
    return result;
}

typedef struct {
    char url[REDIRECT_MAX_BYTES];
    bool invalid;
} redirect_t;

static esp_err_t capture_redirect(esp_http_client_event_t *event)
{
    redirect_t *redirect = event->user_data;
    if (event->event_id == HTTP_EVENT_ON_HEADER && event->header_key && event->header_value &&
        !strcasecmp(event->header_key, "Location")) {
        if (strlcpy(redirect->url, event->header_value, sizeof(redirect->url)) >= sizeof(redirect->url))
            redirect->invalid = true;
    }
    return ESP_OK;
}

esp_err_t goopad_update_install(const char *version)
{
    release_t release = {0};
    esp_err_t result = read_release(&release);
    if (result != ESP_OK) return result;
    if (!version || strcmp(version, release.version)) return ESP_ERR_INVALID_VERSION;
    bool available;
    result = newer_than_running(release.version, &available);
    if (result != ESP_OK || !available) return ESP_ERR_INVALID_VERSION;
    const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
    if (!partition || !release.size || release.size > partition->size) return ESP_ERR_INVALID_SIZE;
    redirect_t *redirect = heap_caps_calloc(1, sizeof(*redirect), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    uint8_t *buffer = heap_caps_malloc(4096, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!redirect || !buffer) { free(redirect); free(buffer); return ESP_ERR_NO_MEM; }
    char url[160];
    snprintf(url, sizeof(url), DOWNLOAD_PREFIX "%s/GooPad-ota-%s.bin", release.tag, release.version);
    esp_http_client_handle_t client = new_client(url, capture_redirect, redirect);
    esp_ota_handle_t ota = 0;
    bool started = false;
    const char *stage = "redirect";
    psa_hash_operation_t hash = PSA_HASH_OPERATION_INIT;
    if (!client) { result = ESP_ERR_NO_MEM; goto done; }
    result = ESP_FAIL;
    if (esp_http_client_open(client, 0) != ESP_OK || esp_http_client_fetch_headers(client) < 0 ||
        esp_http_client_get_status_code(client) != 302 || redirect->invalid ||
        strncmp(redirect->url, ASSET_PREFIX, strlen(ASSET_PREFIX)) ||
        strpbrk(redirect->url, "\r\n")) goto done;
    esp_http_client_cleanup(client);
    client = new_client(redirect->url, NULL, NULL);
    if (!client) { result = ESP_ERR_NO_MEM; goto done; }
    stage = "asset headers";
    int64_t length;
    if (esp_http_client_open(client, 0) != ESP_OK ||
        (length = esp_http_client_fetch_headers(client)) < 0 ||
        esp_http_client_get_status_code(client) != 200 || length != (int64_t)release.size) goto done;
    stage = "OTA begin";
    result = esp_ota_begin(partition, release.size, &ota);
    if (result != ESP_OK) goto done;
    started = true;
    result = ESP_FAIL;
    stage = "hash setup";
    psa_status_t hash_status = psa_crypto_init();
    if (hash_status == PSA_SUCCESS) hash_status = psa_hash_setup(&hash, PSA_ALG_SHA_256);
    if (hash_status != PSA_SUCCESS) {
        ESP_LOGE(TAG, "SHA-256 setup failed: %ld", (long)hash_status);
        goto done;
    }
    size_t received = 0;
    stage = "asset download";
    while (received < release.size) {
        size_t remaining = release.size - received;
        int n = esp_http_client_read(client, (char *)buffer, (int)(remaining < 4096 ? remaining : 4096));
        if (n <= 0) {
            ESP_LOGE(TAG, "Asset read failed at %u bytes: %d", (unsigned)received, n);
            goto done;
        }
        if (psa_hash_update(&hash, buffer, (size_t)n) != PSA_SUCCESS) {
            stage = "hash update";
            goto done;
        }
        esp_err_t write_result = esp_ota_write(ota, buffer, (size_t)n);
        if (write_result != ESP_OK) {
            stage = "OTA write";
            result = write_result;
            goto done;
        }
        received += (size_t)n;
    }
    uint8_t digest[32];
    size_t digest_length = 0;
    stage = "asset verification";
    if (!esp_http_client_is_complete_data_received(client) ||
        psa_hash_finish(&hash, digest, sizeof(digest), &digest_length) != PSA_SUCCESS ||
        digest_length != sizeof(digest) || memcmp(digest, release.digest, sizeof(digest))) goto done;
    stage = "OTA validation";
    result = esp_ota_end(ota);
    started = false;
    if (result != ESP_OK) goto done;
    esp_app_desc_t description;
    stage = "version verification";
    result = esp_ota_get_partition_description(partition, &description);
    if (result != ESP_OK || strcmp(description.version, release.version)) {
        result = ESP_ERR_INVALID_VERSION;
        goto done;
    }
    stage = "boot selection";
    result = esp_ota_set_boot_partition(partition);
done:
    if (started) esp_ota_abort(ota);
    psa_hash_abort(&hash);
    if (client) esp_http_client_cleanup(client);
    free(redirect);
    free(buffer);
    if (result != ESP_OK) ESP_LOGE(TAG, "Release installation failed at %s: %s", stage, esp_err_to_name(result));
    return result;
}
