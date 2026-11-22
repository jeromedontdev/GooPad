#include "goopad_homekit_credentials.h"
#include "goopad_homekit_storage.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <esp_random.h>
#include <hap.h>
#include <nvs.h>

typedef struct {
    uint8_t version;
    char setup_id[5];
} credential_record_t;

static esp_err_t identity_absent(void)
{
    /* Never generate a replacement setup ID over an existing HAP identity:
     * doing so could silently invalidate the controller's pairing. */
    nvs_handle_t identity;
    esp_err_t err = nvs_open_from_partition(GOOPAD_HOMEKIT_NVS_PARTITION, "hap_main", NVS_READONLY, &identity);
    if (err == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    if (err != ESP_OK) return err;
    size_t size = 0;
    err = nvs_get_blob(identity, "identity", NULL, &size);
    nvs_close(identity);
    if (err == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    return err == ESP_OK ? ESP_ERR_INVALID_STATE : err;
}

static void generate_setup_id(char setup_id[5])
{
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    uint32_t id = esp_random();
    for (int i = 0; i < 4; i++) {
        setup_id[i] = alphabet[id & 31];
        id >>= 5;
    }
    setup_id[4] = '\0';
}

esp_err_t goopad_homekit_generate_code(char code[11])
{
    if (!code) return ESP_ERR_INVALID_ARG;
    for (int attempt = 0; attempt < 256; attempt++) {
        uint32_t random = esp_random();
        /* Reject the incomplete modulo range to avoid biasing setup codes. */
        if (random >= UINT32_C(4200000000)) continue;
        uint32_t value = random % UINT32_C(100000000);
        snprintf(code, 11, "%03u-%02u-%03u",
                 (unsigned)(value / 100000), (unsigned)((value / 1000) % 100),
                 (unsigned)(value % 1000));
        if (hap_setup_code_valid(code)) return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t goopad_homekit_credentials_load(goopad_homekit_credentials_t *credentials)
{
    if (!credentials) return ESP_ERR_INVALID_ARG;
    memset(credentials, 0, sizeof(*credentials));
    nvs_handle_t nvs;
    esp_err_t err = nvs_open_from_partition(GOOPAD_HOMEKIT_NVS_PARTITION, "goopad_hk", NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;
    credential_record_t record = {0};
    size_t size = sizeof(record);
    err = nvs_get_blob(nvs, "credentials", &record, &size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        err = identity_absent();
        if (err == ESP_OK) {
            record.version = 1;
            generate_setup_id(record.setup_id);
        }
        if (err == ESP_OK) err = nvs_set_blob(nvs, "credentials", &record, sizeof(record));
        if (err == ESP_OK) err = nvs_commit(nvs);
    } else if (err == ESP_OK &&
               (size != sizeof(record) || record.version != 1 || !hap_setup_id_valid(record.setup_id))) {
        err = ESP_ERR_INVALID_STATE;
    }
    nvs_close(nvs);
    if (err == ESP_OK) {
        goopad_homekit_credentials_t generated = {0};
        memcpy(generated.setup_id, record.setup_id, sizeof(generated.setup_id));
        err = goopad_homekit_generate_code(generated.code);
        if (err == ESP_OK) *credentials = generated;
        memset(&generated, 0, sizeof(generated));
    }
    memset(&record, 0, sizeof(record));
    return err;
}
