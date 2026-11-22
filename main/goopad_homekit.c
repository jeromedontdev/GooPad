#include "goopad_homekit.h"
#include "goopad_homekit_credentials.h"
#include "goopad_homekit_storage.h"

#include <stdlib.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <hap.h>
#include <hap_apple_chars.h>
#include <hap_apple_servs.h>
#include <nvs.h>
#include <psa/crypto.h>
#include <sodium/utils.h>

typedef struct {
    goopad_homekit_switch_t definition;
    hap_char_t *on;
    bool state;
} switch_entry_t;

static switch_entry_t entries[GOOPAD_HOMEKIT_MAX_SWITCHES];
static size_t entry_count;
static SemaphoreHandle_t switch_lock;
static SemaphoreHandle_t work_done;
static SemaphoreHandle_t pairing_lock;
static QueueHandle_t toggle_queue;
static esp_err_t work_result;
static bool started;
static bool startup_attempted;
static char last_setup_code[11];

typedef struct {
    uint32_t seconds;
    char code[11];
    SemaphoreHandle_t done;
    esp_err_t result;
    int64_t deadline;
    bool pending;
    bool completed;
    bool abandoned;
} pairing_request_t;

static pairing_request_t pairing_request;
static portMUX_TYPE pairing_request_lock = portMUX_INITIALIZER_UNLOCKED;

/* Server work can finish after the caller times out, so its request and
 * completion semaphore must outlive the caller's stack. */
static void open_pairing_work(void *arg)
{
    pairing_request_t *request = arg;
    esp_err_t result = ESP_FAIL;
    bool opened = false;
    if (esp_timer_get_time() >= request->deadline) {
        result = ESP_ERR_TIMEOUT;
    } else if (hap_get_paired_controller_count() != 0) {
        result = ESP_ERR_INVALID_STATE;
    } else if (hap_set_pair_setup_window(0) == HAP_SUCCESS &&
               hap_abort_pair_setup() == HAP_SUCCESS &&
               hap_set_setup_code(request->code) == HAP_SUCCESS &&
               hap_set_pair_setup_window(request->seconds) == HAP_SUCCESS) {
        opened = true;
    }
    portENTER_CRITICAL(&pairing_request_lock);
    if (opened) {
        if (request->abandoned || esp_timer_get_time() >= request->deadline) {
            result = ESP_ERR_TIMEOUT;
        } else {
            memcpy(last_setup_code, request->code, sizeof(last_setup_code));
            result = ESP_OK;
        }
    }
    if (!opened || result == ESP_OK) {
        request->result = result;
        request->completed = true;
    }
    portEXIT_CRITICAL(&pairing_request_lock);
    if (opened && result != ESP_OK) {
        /* A late worker must not leave a pairing window open after reporting
         * timeout. Mark completion only after the window has been closed. */
        if (hap_set_pair_setup_window(0) != HAP_SUCCESS || hap_abort_pair_setup() != HAP_SUCCESS)
            ESP_LOGE("homekit", "Could not close expired pairing request");
        portENTER_CRITICAL(&pairing_request_lock);
        request->result = result;
        request->completed = true;
        portEXIT_CRITICAL(&pairing_request_lock);
    }
    xSemaphoreGive(request->done);
}

esp_err_t goopad_homekit_open_pairing(uint32_t seconds, char code[11])
{
    if (!code || !seconds || seconds > 600) return ESP_ERR_INVALID_ARG;
    code[0] = '\0';
    if (!started) return ESP_ERR_INVALID_STATE;
    if (xSemaphoreTake(pairing_lock, pdMS_TO_TICKS(1000)) != pdTRUE) return ESP_ERR_TIMEOUT;
    pairing_request_t *request = &pairing_request;
    if (request->pending) {
        if (xSemaphoreTake(request->done, 0) != pdTRUE) {
            xSemaphoreGive(pairing_lock);
            return ESP_ERR_INVALID_STATE;
        }
        request->pending = false;
        sodium_memzero(request->code, sizeof(request->code));
    }
    request->seconds = seconds;
    request->result = ESP_FAIL;
    request->completed = false;
    request->abandoned = false;
    esp_err_t result = ESP_OK;
    for (int attempt = 0; result == ESP_OK && attempt < 8; attempt++) {
        result = goopad_homekit_generate_code(request->code);
        if (result != ESP_OK || strcmp(request->code, last_setup_code)) break;
        if (attempt == 7) result = ESP_FAIL;
    }
    if (result == ESP_OK) {
        request->deadline = esp_timer_get_time() + INT64_C(10000000);
        if (hap_queue_server_work(open_pairing_work, request) != HAP_SUCCESS) {
            result = ESP_FAIL;
        } else {
            request->pending = true;
            if (xSemaphoreTake(request->done, pdMS_TO_TICKS(10000)) != pdTRUE) {
                portENTER_CRITICAL(&pairing_request_lock);
                bool completed = request->completed;
                if (completed) {
                    result = request->result;
                    if (result == ESP_OK) memcpy(code, request->code, sizeof(request->code));
                } else {
                    request->abandoned = true;
                    result = ESP_ERR_TIMEOUT;
                }
                portEXIT_CRITICAL(&pairing_request_lock);
                if (completed && xSemaphoreTake(request->done, 0) == pdTRUE)
                    request->pending = false;
            } else {
                request->pending = false;
                result = request->result;
                if (result == ESP_OK) memcpy(code, request->code, sizeof(request->code));
            }
        }
    }
    if (!request->pending) sodium_memzero(request->code, sizeof(request->code));
    xSemaphoreGive(pairing_lock);
    return result;
}

static int compare_switches(const void *a, const void *b)
{
    const switch_entry_t *left = a, *right = b;
    return strcmp(left->definition.id, right->definition.id);
}

static switch_entry_t *find_switch(const char *id)
{
    for (size_t i = 0; i < entry_count; i++)
        if (!strcmp(entries[i].definition.id, id)) return &entries[i];
    return NULL;
}

static esp_err_t prune_old_aids(void)
{
    /* Retain IDs for surviving switches; prune only obsolete mappings so
     * repeated replacement does not exhaust the dedicated HomeKit store. */
    nvs_handle_t nvs;
    esp_err_t err = nvs_open_from_partition(GOOPAD_HOMEKIT_NVS_PARTITION, "hap_aids", NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;
    for (;;) {
        char obsolete[GOOPAD_HOMEKIT_MAX_SWITCHES][16];
        size_t count = 0;
        nvs_iterator_t iterator = NULL;
        err = nvs_entry_find_in_handle(nvs, NVS_TYPE_ANY, &iterator);
        while (err == ESP_OK && count < GOOPAD_HOMEKIT_MAX_SWITCHES) {
            nvs_entry_info_t info;
            err = nvs_entry_info(iterator, &info);
            if (err != ESP_OK) break;
            if (info.type != NVS_TYPE_BLOB) {
                err = ESP_ERR_INVALID_STATE;
                break;
            }
            if (!find_switch(info.key)) strlcpy(obsolete[count++], info.key, sizeof(obsolete[0]));
            err = nvs_entry_next(&iterator);
        }
        nvs_release_iterator(iterator);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) break;
        err = ESP_OK;
        for (size_t i = 0; err == ESP_OK && i < count; i++) err = nvs_erase_key(nvs, obsolete[i]);
        if (err == ESP_OK && count) err = nvs_commit(nvs);
        if (err != ESP_OK || count < GOOPAD_HOMEKIT_MAX_SWITCHES) break;
    }
    nvs_close(nvs);
    return err;
}

static esp_err_t toggle_now(switch_entry_t *entry)
{
    if (xSemaphoreTake(switch_lock, portMAX_DELAY) != pdTRUE) return ESP_FAIL;
    hap_val_t value = {.b = !entry->state};
    esp_err_t result = ESP_FAIL;
    if (hap_char_update_val(entry->on, &value) == HAP_SUCCESS) {
        entry->state = value.b;
        result = ESP_OK;
    }
    xSemaphoreGive(switch_lock);
    return result;
}

static void toggle_server_work(void *arg)
{
    work_result = toggle_now(arg);
    xSemaphoreGive(work_done);
}

static void toggle_worker(void *arg)
{
    (void)arg;
    switch_entry_t *entry;
    for (;;) {
        if (xQueueReceive(toggle_queue, &entry, portMAX_DELAY) != pdTRUE) continue;
        bool reported = false;
        for (;;) {
            /* Serialize local presses through the HAP server's work queue;
             * remote writes also run there, preserving their ordering. */
            if (hap_queue_server_work(toggle_server_work, entry) == HAP_SUCCESS) {
                while (xSemaphoreTake(work_done, pdMS_TO_TICKS(5000)) != pdTRUE) {
                    if (!reported) {
                        ESP_LOGE("homekit", "HomeKit server is not processing switch updates");
                        reported = true;
                    }
                }
                if (work_result == ESP_OK) break;
            }
            if (!reported) {
                ESP_LOGW("homekit", "HomeKit switch update delayed; retrying");
                reported = true;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        if (reported) ESP_LOGI("homekit", "HomeKit switch update completed");
    }
}

static int identify(hap_acc_t *acc)
{
    (void)acc;
    return HAP_SUCCESS;
}

static int write_switch(hap_write_data_t writes[], int count, void *priv, void *write_priv)
{
    (void)write_priv;
    switch_entry_t *entry = priv;
    if (!entry || xSemaphoreTake(switch_lock, portMAX_DELAY) != pdTRUE) return HAP_FAIL;
    int result = HAP_SUCCESS;
    for (int i = 0; i < count; i++) {
        if (writes[i].hc != entry->on) {
            *writes[i].status = HAP_STATUS_RES_ABSENT;
            result = HAP_FAIL;
        } else if (hap_char_update_val(entry->on, &writes[i].val) != HAP_SUCCESS) {
            *writes[i].status = HAP_STATUS_RES_BUSY;
            result = HAP_FAIL;
        } else {
            entry->state = writes[i].val.b;
            *writes[i].status = HAP_STATUS_SUCCESS;
        }
    }
    xSemaphoreGive(switch_lock);
    return result;
}

static esp_err_t topology_hash(uint8_t digest[HAP_TOPOLOGY_HASH_SIZE])
{
    psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;
    psa_status_t status = psa_crypto_init();
    if (status == PSA_SUCCESS) status = psa_hash_setup(&operation, PSA_ALG_SHA_256);
    static const char version[] = "goopad-switch-topology-v1";
    if (status == PSA_SUCCESS) status = psa_hash_update(&operation, (const uint8_t *)version, sizeof(version));
    for (size_t i = 0; status == PSA_SUCCESS && i < entry_count; i++) {
        status = psa_hash_update(&operation, (const uint8_t *)entries[i].definition.id,
                                 strlen(entries[i].definition.id) + 1);
        if (status == PSA_SUCCESS)
            status = psa_hash_update(&operation, (const uint8_t *)entries[i].definition.name,
                                     strlen(entries[i].definition.name) + 1);
    }
    size_t size = 0;
    if (status == PSA_SUCCESS) status = psa_hash_finish(&operation, digest, HAP_TOPOLOGY_HASH_SIZE, &size);
    psa_hash_abort(&operation);
    return status == PSA_SUCCESS && size == HAP_TOPOLOGY_HASH_SIZE ? ESP_OK : ESP_FAIL;
}

esp_err_t goopad_homekit_start(const goopad_homekit_switch_t *switches, size_t count,
                               const char *setup_code, const char *setup_id)
{
    if (startup_attempted) return ESP_ERR_INVALID_STATE;
    if (!switches || !count || count > GOOPAD_HOMEKIT_MAX_SWITCHES ||
        !setup_code || !setup_id) return ESP_ERR_INVALID_ARG;
    for (size_t i = 0; i < count; i++) {
        size_t id_len = strnlen(switches[i].id, sizeof(switches[i].id));
        size_t name_len = strnlen(switches[i].name, sizeof(switches[i].name));
        if (!id_len || id_len >= sizeof(switches[i].id) ||
            !name_len || name_len >= sizeof(switches[i].name)) return ESP_ERR_INVALID_ARG;
        entries[i].definition = switches[i];
    }
    entry_count = count;
    /* A stable ID order makes the topology hash independent of tile order. */
    qsort(entries, count, sizeof(entries[0]), compare_switches);
    for (size_t i = 1; i < count; i++)
        if (!strcmp(entries[i - 1].definition.id, entries[i].definition.id)) return ESP_ERR_INVALID_ARG;

    startup_attempted = true;
    switch_lock = xSemaphoreCreateMutex();
    work_done = xSemaphoreCreateBinary();
    pairing_lock = xSemaphoreCreateMutex();
    pairing_request.done = xSemaphoreCreateBinary();
    toggle_queue = xQueueCreate(16, sizeof(switch_entry_t *));
    if (!switch_lock || !work_done || !pairing_lock || !pairing_request.done || !toggle_queue)
        return ESP_ERR_NO_MEM;
    if (hap_init(HAP_TRANSPORT_WIFI) != HAP_SUCCESS) return ESP_FAIL;
    esp_err_t prune_err = prune_old_aids();
    if (prune_err != ESP_OK) {
        ESP_LOGE("homekit", "Could not reclaim obsolete switch identifiers: %s", esp_err_to_name(prune_err));
        return prune_err;
    }
    hap_cfg_t config;
    if (hap_get_config(&config) != HAP_SUCCESS) return ESP_FAIL;
    config.unique_param = UNIQUE_NONE;
    config.disable_config_num_update = true;
    if (hap_set_config(&config) != HAP_SUCCESS) return ESP_FAIL;
    if (hap_set_setup_code(setup_code) != HAP_SUCCESS) return ESP_FAIL;
    memcpy(last_setup_code, setup_code, sizeof(last_setup_code));
    if (hap_set_setup_id(setup_id) != HAP_SUCCESS) return ESP_FAIL;
    hap_acc_cfg_t cfg = {
        .name = "GooPad", .manufacturer = "GooPad", .model = "GooPad",
        .serial_num = "GooPad", .fw_rev = "0.6.2", .pv = "1.1.0",
        .identify_routine = identify, .cid = HAP_CID_BRIDGE,
    };
    hap_acc_t *bridge = hap_acc_create(&cfg);
    if (!bridge) return ESP_ERR_NO_MEM;
    hap_add_accessory(bridge);
    cfg.cid = HAP_CID_SWITCH;
    for (size_t i = 0; i < count; i++) {
        switch_entry_t *entry = &entries[i];
        cfg.name = entry->definition.name;
        cfg.serial_num = entry->definition.id;
        hap_acc_t *child = hap_acc_create(&cfg);
        hap_serv_t *service = hap_serv_switch_create(false);
        if (!child || !service) return ESP_ERR_NO_MEM;
        entry->on = hap_serv_get_char_by_uuid(service, HAP_CHAR_UUID_ON);
        if (!entry->on) return ESP_FAIL;
        hap_serv_set_priv(service, entry);
        hap_serv_set_write_cb(service, write_switch);
        if (hap_acc_add_serv(child, service) != HAP_SUCCESS) return ESP_FAIL;
        int aid = hap_get_unique_aid(entry->definition.id);
        if (aid < 2) return ESP_FAIL;
        hap_add_bridged_accessory(child, aid);
        if (hap_acc_get_by_aid(aid) != child) return ESP_FAIL;
    }
    uint8_t digest[HAP_TOPOLOGY_HASH_SIZE];
    if (topology_hash(digest) != ESP_OK || hap_sync_topology(digest) != HAP_SUCCESS ||
        hap_start() != HAP_SUCCESS) return ESP_FAIL;
    if (xTaskCreate(toggle_worker, "homekit-toggle", 4096, NULL, 5, NULL) != pdPASS)
        return ESP_ERR_NO_MEM;
    started = true;
    return ESP_OK;
}

esp_err_t goopad_homekit_toggle(const char *id)
{
    if (!started || !id) return ESP_ERR_INVALID_STATE;
    switch_entry_t *entry = find_switch(id);
    if (!entry) return ESP_ERR_NOT_FOUND;
    return xQueueSend(toggle_queue, &entry, 0) == pdTRUE ? ESP_OK : ESP_ERR_INVALID_STATE;
}

esp_err_t goopad_homekit_state(const char *id, bool *state)
{
    if (!started || !id || !state) return ESP_ERR_INVALID_ARG;
    if (xSemaphoreTake(switch_lock, pdMS_TO_TICKS(10)) != pdTRUE) return ESP_ERR_TIMEOUT;
    switch_entry_t *entry = find_switch(id);
    if (entry) *state = entry->state;
    xSemaphoreGive(switch_lock);
    return entry ? ESP_OK : ESP_ERR_NOT_FOUND;
}
