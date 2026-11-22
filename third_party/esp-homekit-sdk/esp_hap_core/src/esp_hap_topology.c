#include <string.h>
#include <nvs.h>
#include <hap.h>
#include <esp_hap_database.h>
#include <esp_hap_keystore.h>
#include <esp_hap_main.h>
#include <esp_mfi_debug.h>

#define HAP_TOPOLOGY_KEY "topology_hash"

int hap_sync_topology(const uint8_t hash[HAP_TOPOLOGY_HASH_SIZE])
{
    if (!hash || is_hap_loop_started() || hap_priv.config_num < 1 ||
        hap_priv.config_num > 65535) {
        ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "Invalid HomeKit topology synchronization");
        return HAP_FAIL;
    }

    uint8_t previous[HAP_TOPOLOGY_HASH_SIZE];
    size_t size = sizeof(previous);
    int err = hap_keystore_get(HAP_KEYSTORE_NAMESPACE_HAPMAIN, HAP_TOPOLOGY_KEY,
                               previous, &size);
    if (err == HAP_SUCCESS) {
        if (size != sizeof(previous)) {
            ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "Invalid stored HomeKit topology hash");
            return HAP_FAIL;
        }
        if (memcmp(previous, hash, sizeof(previous)) == 0) return HAP_SUCCESS;
    } else if (err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "Could not read HomeKit topology hash");
        return HAP_FAIL;
    }

    if (hap_increment_and_save_config_num() != HAP_SUCCESS) return HAP_FAIL;
    if (hap_keystore_set(HAP_KEYSTORE_NAMESPACE_HAPMAIN, HAP_TOPOLOGY_KEY,
                         hash, HAP_TOPOLOGY_HASH_SIZE) != HAP_SUCCESS) {
        ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "Could not persist HomeKit topology hash");
        return HAP_FAIL;
    }
    return HAP_SUCCESS;
}
