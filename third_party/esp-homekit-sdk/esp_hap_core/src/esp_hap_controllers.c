/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2020 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS products only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <string.h>
#include <nvs.h>
#include <hap.h>
#include <esp_mfi_debug.h>

#include <esp_hap_main.h>
#include <esp_hap_database.h>
#include <esp_hap_controllers.h>
#include <esp_hap_keystore.h>
#include <esp_hap_pair_setup.h>
#include <esp_hap_pair_verify.h>
#include <hap_platform_memory.h>

#define HAP_KEYSTORE_NAMESPACE_CTRL "hap_ctrl"
#define HAP_KEY_CONTROLLERS "controllers"

typedef struct {
    uint8_t valid;
    hap_ctrl_info_t info;
} __attribute__((packed)) stored_controller_t;

static int save_controllers(hap_ctrl_data_t *changed, bool valid, bool clear_all)
{
    stored_controller_t *stored = hap_platform_memory_calloc(HAP_MAX_CONTROLLERS, sizeof(*stored));
    if (!stored) return HAP_FAIL;
    for (int i = 0; i < HAP_MAX_CONTROLLERS && !clear_all; i++) {
        hap_ctrl_data_t *ctrl = &hap_priv.controllers[i];
        bool include = ctrl == changed ? valid : ctrl->valid;
        if (include) {
            stored[i].valid = 1;
            stored[i].info = ctrl->info;
        }
    }
    int ret = hap_keystore_set(HAP_KEYSTORE_NAMESPACE_CTRL, HAP_KEY_CONTROLLERS,
                               (const uint8_t *)stored, HAP_MAX_CONTROLLERS * sizeof(*stored));
    hap_platform_memory_free(stored);
    return ret;
}

int hap_controllers_init()
{
    memset(hap_priv.controllers, 0, sizeof(hap_priv.controllers));
    stored_controller_t *stored = hap_platform_memory_calloc(HAP_MAX_CONTROLLERS, sizeof(*stored));
    if (!stored) return HAP_FAIL;
    size_t size = HAP_MAX_CONTROLLERS * sizeof(*stored);
    int err = hap_keystore_get(HAP_KEYSTORE_NAMESPACE_CTRL, HAP_KEY_CONTROLLERS, (uint8_t *)stored, &size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        char index_str[4];
        hap_ctrl_info_t old;
        for (int i = 0; i < HAP_MAX_CONTROLLERS; i++) {
            size_t old_size = sizeof(old);
            snprintf(index_str, sizeof(index_str), "%d", i);
            err = hap_keystore_get(HAP_KEYSTORE_NAMESPACE_CTRL, index_str, (uint8_t *)&old, &old_size);
            if (err != ESP_ERR_NVS_NOT_FOUND) {
                ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "Legacy controller storage requires migration (%d)", err);
                hap_platform_memory_free(stored);
                return HAP_FAIL;
            }
        }
    } else if (err != HAP_SUCCESS || size != HAP_MAX_CONTROLLERS * sizeof(*stored)) {
        ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "Cannot load controller storage (%d)", err);
        hap_platform_memory_free(stored);
        return HAP_FAIL;
    }
    bool acc_paired = false, admin_paired = false;
    for (int i = 0; i < HAP_MAX_CONTROLLERS; i++) {
        if (stored[i].valid > 1 || (stored[i].valid &&
            (!stored[i].info.id[0] || !memchr(stored[i].info.id, 0, sizeof(stored[i].info.id)) ||
             stored[i].info.perms > 1))) {
            ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "Invalid stored controller %d", i);
            hap_platform_memory_free(stored);
            return HAP_FAIL;
        }
        if (!stored[i].valid) continue;
        hap_priv.controllers[i].info = stored[i].info;
        hap_priv.controllers[i].index = i;
        hap_priv.controllers[i].valid = true;
        acc_paired = true;
        admin_paired |= stored[i].info.perms != 0;
    }
    hap_platform_memory_free(stored);
    if (acc_paired && !admin_paired) {
        ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "Controller storage has no administrator");
        return HAP_FAIL;
    }
    if (acc_paired) {
        ESP_MFI_DEBUG(ESP_MFI_DEBUG_INFO, "Accessory is Paired with at least one controller");
    } else {
        ESP_MFI_DEBUG(ESP_MFI_DEBUG_INFO, "Accessory is not Paired with any controller");
        hap_start_pairing_mode_timer();
    }
    return HAP_SUCCESS;
}

hap_ctrl_data_t *hap_controller_get_empty_loc()
{
	int i;
	for (i = 0; i < HAP_MAX_CONTROLLERS; i++) {
		if (!hap_priv.controllers[i].valid) {
            hap_priv.controllers[i].index = i;
			return &hap_priv.controllers[i];
        }
	}
	return NULL;
}

int hap_get_paired_controller_count()
{
	int i, cnt = 0;
	for (i = 0; i < HAP_MAX_CONTROLLERS; i++) {
		if (hap_priv.controllers[i].valid) {
            cnt++;
        }
	}
	return cnt;
}

bool is_accessory_paired()
{
	int i;
	for (i = 0; i < HAP_MAX_CONTROLLERS; i++) {
		if (hap_priv.controllers[i].valid)
			return true;
	}
	return false;
}

bool is_admin_paired()
{
	int i;
	for (i = 0; i < HAP_MAX_CONTROLLERS; i++) {
		if (hap_priv.controllers[i].valid && hap_priv.controllers[i].info.perms)
			return true;
	}
	return false;
}

int hap_controller_save(hap_ctrl_data_t *ctrl_data)
{
    int ret = save_controllers(ctrl_data, true, false);

    if (ret != HAP_SUCCESS) {
        ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "Failed to store controller %d", ctrl_data->index);
        return HAP_FAIL;
    }
    ctrl_data->valid = true;
    hap_report_event(HAP_EVENT_CTRL_PAIRED, ctrl_data->info.id, sizeof(ctrl_data->info.id));
    return HAP_SUCCESS;
}

int hap_controller_remove(hap_ctrl_data_t *ctrl_data)
{
	if (!ctrl_data)
        return HAP_SUCCESS;
    char id[HAP_CTRL_ID_LEN];
    strncpy(id, ctrl_data->info.id, sizeof(id));
    if (save_controllers(ctrl_data, false, false) != HAP_SUCCESS) {
        ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "Failed to remove controller %d", ctrl_data->index);
        return HAP_FAIL;
    }
    memset(ctrl_data, 0, sizeof(hap_ctrl_data_t));
    hap_report_event(HAP_EVENT_CTRL_UNPAIRED, id, sizeof(id));
    return HAP_SUCCESS;
}

int hap_controllers_remove_all(void)
{
    if (save_controllers(NULL, false, true) != HAP_SUCCESS) {
        ESP_MFI_DEBUG(ESP_MFI_DEBUG_ERR, "Failed to remove controllers");
        return HAP_FAIL;
    }
    for (int i = 0; i < HAP_MAX_CONTROLLERS; i++) {
        hap_ctrl_data_t *ctrl = &hap_priv.controllers[i];
        if (!ctrl->valid) continue;
        hap_close_sessions_of_ctrl(ctrl);
        hap_report_event(HAP_EVENT_CTRL_UNPAIRED, ctrl->info.id, sizeof(ctrl->info.id));
        memset(ctrl, 0, sizeof(*ctrl));
    }
    return HAP_SUCCESS;
}

hap_ctrl_data_t *hap_get_controller(char *ctrl_id)
{
	int i;
	for (i = 0; i < HAP_MAX_CONTROLLERS; i++) {
		if (hap_priv.controllers[i].valid
				&& (!strcmp(hap_priv.controllers[i].info.id, ctrl_id)))
			return &hap_priv.controllers[i];
	}
	return NULL;
}

void hap_erase_controller_info()
{
    hap_keystore_delete_namespace(HAP_KEYSTORE_NAMESPACE_CTRL);
}
