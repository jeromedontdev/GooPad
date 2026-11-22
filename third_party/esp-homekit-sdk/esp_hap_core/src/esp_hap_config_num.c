#include <nvs.h>
#include <hap.h>
#include <esp_hap_database.h>
#include <esp_hap_keystore.h>
#include <esp_mfi_debug.h>

#define HAP_CONFIG_KEY "config_num"

int hap_config_num_init(bool new_identity)
{
    uint32_t number = 0;
    size_t size = sizeof(number);
    int err = hap_keystore_get(HAP_KEYSTORE_NAMESPACE_HAPMAIN, HAP_CONFIG_KEY,
                               (uint8_t *)&number, &size);
    if (err == ESP_ERR_NVS_NOT_FOUND && new_identity) {
        number = 1;
        err = hap_keystore_set(HAP_KEYSTORE_NAMESPACE_HAPMAIN, HAP_CONFIG_KEY,
                               (const uint8_t *)&number, sizeof(number));
    }
    if (err != HAP_SUCCESS || size != sizeof(number) || number < 1 || number > 65535) {
        ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "HomeKit configuration number unavailable or invalid");
        return HAP_FAIL;
    }
    hap_priv.config_num = number;
    return HAP_SUCCESS;
}

int hap_increment_and_save_config_num(void)
{
    if (hap_priv.config_num < 1 || hap_priv.config_num > 65535) return HAP_FAIL;
    uint32_t next = hap_priv.config_num == 65535 ? 1 : hap_priv.config_num + 1;
    if (hap_keystore_set(HAP_KEYSTORE_NAMESPACE_HAPMAIN, HAP_CONFIG_KEY,
                         (const uint8_t *)&next, sizeof(next)) != HAP_SUCCESS) {
        ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "Could not persist HomeKit configuration number");
        return HAP_FAIL;
    }
    hap_priv.config_num = next;
    return HAP_SUCCESS;
}
