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
#include <esp_hap_database.h>
#include <esp_hap_mdns.h>
#include <esp_mfi_debug.h>

static bool mdns_init_done;

int hap_mdns_serv_start(hap_mdns_handle_t *handle, const char *name, const char *type,
        const char *protocol, int port, mdns_txt_item_t *txt_records, size_t num_txt)
{
    strcpy(handle->type, type);
    strcpy(handle->proto, protocol);
    if (mdns_service_add(name, type, protocol, port, txt_records, num_txt) != 0) {
        return HAP_FAIL;
    }
    return HAP_SUCCESS;
}

int hap_mdns_serv_update_txt(hap_mdns_handle_t *handle, mdns_txt_item_t *txt_records, size_t num_txt)
{
    if (mdns_service_txt_set(handle->type, handle->proto, txt_records, num_txt) != 0) {
        return HAP_FAIL;
    }
    return HAP_SUCCESS;
}

int hap_mdns_serv_name_change(hap_mdns_handle_t *handle, const char * instance_name)
{
    if (mdns_service_instance_name_set(handle->type, handle->proto, instance_name) == ESP_OK) {
        return HAP_SUCCESS;
    }
    return HAP_FAIL;
}

int hap_mdns_serv_stop(hap_mdns_handle_t *handle)
{
    if (mdns_service_remove(handle->type, handle->proto) == ESP_OK) {
        return HAP_SUCCESS;
    }
    return HAP_FAIL;
}

int hap_mdns_init()
{
    if (!mdns_init_done) {
        const char *id = hap_get_acc_id();
        if (!id || strlen(id) != 17) return HAP_FAIL;
        char hostname[17] = "hap-";
        size_t pos = 4;
        for (size_t i = 0; i < 17; i++) {
            if (i % 3 == 2) {
                if (id[i] != ':') return HAP_FAIL;
                continue;
            }
            char ch = id[i];
            if (ch >= 'A' && ch <= 'F') ch += 'a' - 'A';
            if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'))) return HAP_FAIL;
            hostname[pos++] = ch;
        }
        hostname[pos] = '\0';
        if (mdns_init() != ESP_OK) return HAP_FAIL;
        if (mdns_hostname_set(hostname) != ESP_OK) {
            mdns_free();
            return HAP_FAIL;
        }
        mdns_init_done = true;
        ESP_MFI_DEBUG(ESP_MFI_DEBUG_INFO, "mDNS initialised");
        return HAP_SUCCESS;
    }
    return HAP_FAIL;
}

int hap_mdns_deinit()
{
    mdns_free();
    mdns_init_done = false;
    return HAP_SUCCESS;
}
