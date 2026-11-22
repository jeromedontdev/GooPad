#include <limits.h>
#include <string.h>
#include <nvs.h>
#include <hap.h>
#include <esp_hap_database.h>
#include <esp_hap_keystore.h>
#include <esp_mfi_debug.h>

#define HAP_AID_NAMESPACE "hap_aids"
#define HAP_AID_COUNTER "cur_aid"

int hap_aid_init(bool new_identity)
{
    uint32_t counter = 0;
    size_t size = sizeof(counter);
    int err = hap_keystore_get(HAP_KEYSTORE_NAMESPACE_HAPMAIN, HAP_AID_COUNTER,
                               (uint8_t *)&counter, &size);
    if (err == ESP_ERR_NVS_NOT_FOUND && new_identity) {
        counter = 1;
        err = hap_keystore_set(HAP_KEYSTORE_NAMESPACE_HAPMAIN, HAP_AID_COUNTER,
                               (uint8_t *)&counter, sizeof(counter));
    }
    if (err != HAP_SUCCESS || size != sizeof(counter) || counter < 1 || counter > INT_MAX) {
        ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "HomeKit AID counter unavailable or invalid");
        return HAP_FAIL;
    }
    hap_priv.cur_aid = counter;
    return HAP_SUCCESS;
}

int hap_get_next_aid(void)
{
    if (hap_priv.cur_aid < 1 || hap_priv.cur_aid >= INT_MAX) {
        ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "HomeKit AID counter exhausted or uninitialized");
        return HAP_FAIL;
    }
    uint32_t next = hap_priv.cur_aid + 1;
    if (hap_keystore_set(HAP_KEYSTORE_NAMESPACE_HAPMAIN, HAP_AID_COUNTER,
                         (uint8_t *)&next, sizeof(next)) != HAP_SUCCESS) {
        ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "Could not persist HomeKit AID counter");
        return HAP_FAIL;
    }
    hap_priv.cur_aid = next;
    return (int)next;
}

int hap_get_unique_aid(const char *id)
{
    if (!id || !id[0] || strlen(id) > 15) {
        ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "Invalid HomeKit accessory identifier");
        return HAP_FAIL;
    }
    uint32_t aid = 0;
    size_t size = sizeof(aid);
    int err = hap_keystore_get(HAP_AID_NAMESPACE, id, (uint8_t *)&aid, &size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        int next = hap_get_next_aid();
        if (next < 2) return HAP_FAIL;
        aid = (uint32_t)next;
        if (hap_keystore_set(HAP_AID_NAMESPACE, id, (uint8_t *)&aid, sizeof(aid)) != HAP_SUCCESS) {
            ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "Could not persist HomeKit accessory AID");
            return HAP_FAIL;
        }
    } else if (err != HAP_SUCCESS || size != sizeof(aid) || aid < 2 ||
               aid > hap_priv.cur_aid || aid > INT_MAX) {
        ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "HomeKit accessory AID unavailable or invalid");
        return HAP_FAIL;
    }
    return (int)aid;
}
