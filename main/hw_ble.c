/*
 * Bluetooth: BLE advertising + a short scan.
 *
 * BLE is used rather than Classic because it is the cheaper test: the
 * controller can advertise and scan without pairing, bonding or a
 * connection, and it exercises the same RF front end. A successful scan
 * that returns at least one advertisement proves the radio and the
 * controller firmware are working.
 *
 * The controller is released again afterwards. NimBLE is used rather than
 * Bluedroid because it is markedly smaller and this is a test rig, not a
 * product.
 */
#include "app.h"
#include "board.h"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_hs_adv.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include <string.h>

static const char *TAG = "ble";

static bool s_nimble_ready;
static volatile bool s_synced;
static volatile int s_seen;

static void on_sync(void)
{
    s_synced = true;
    ESP_LOGI(TAG, "host synced");
}

static void on_reset(int reason)
{
    ESP_LOGW(TAG, "host reset, reason %d", reason);
    s_synced = false;
}

static int on_gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    if (event->type == BLE_GAP_EVENT_DISC) {
        const struct ble_gap_disc_desc *d = &event->disc;
        if (s_seen < HW_BLE_MAX_DEVICES) {
            ble_dev_t *dev = &g_app.ble.devices[s_seen];
            memset(dev, 0, sizeof(*dev));
            dev->rssi = d->rssi;
            dev->addr_type = d->addr.type;

            /* Pull the name out of the advertisement if present. */
            struct ble_hs_adv_fields fields;
            if (ble_hs_adv_parse_fields(&fields, d->data, d->length_data) == 0 &&
                fields.name != NULL && fields.name_len > 0) {
                int n = fields.name_len;
                if (n > (int)sizeof(dev->name) - 1) {
                    n = sizeof(dev->name) - 1;
                }
                memcpy(dev->name, fields.name, n);
                dev->name[n] = '\0';
            } else {
                snprintf(dev->name, sizeof(dev->name), "%02X:%02X:%02X:%02X:%02X:%02X",
                         d->addr.val[5], d->addr.val[4], d->addr.val[3],
                         d->addr.val[2], d->addr.val[1], d->addr.val[0]);
            }
            s_seen++;
        }
        g_app.ble.device_count = s_seen;
    }
    return 0;
}

static void host_task(void *param)
{
    (void)param;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

esp_err_t hw_ble_init(void)
{
    if (s_nimble_ready) {
        return ESP_OK;
    }

    esp_err_t err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init: %s", esp_err_to_name(err));
        g_app.results.ble = TEST_FAIL;
        return err;
    }

    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.reset_cb = on_reset;

    ble_svc_gap_init();
    ble_svc_gap_device_name_set("4848S040-test");

    nimble_port_freertos_init(host_task);
    s_nimble_ready = true;

    /* Wait for the host to sync with the controller. */
    for (int i = 0; i < 200 && !s_synced; i++) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (!s_synced) {
        ESP_LOGW(TAG, "host did not sync within 2 s");
        g_app.results.ble = TEST_FAIL;
        return ESP_ERR_TIMEOUT;
    }

    g_app.ble.ready = true;
    g_app.results.ble = TEST_PASS;
    ESP_LOGI(TAG, "BLE ready (NimBLE, advertising as \"4848S040-test\")");
    return ESP_OK;
}

esp_err_t hw_ble_scan(int duration_ms)
{
    esp_err_t err = hw_ble_init();
    if (err != ESP_OK) {
        return err;
    }

    s_seen = 0;
    g_app.ble.device_count = 0;
    g_app.ble.scanning = true;

    uint8_t own_addr_type;
    err = ble_hs_id_infer_auto(0, &own_addr_type);
    if (err != 0) {
        ESP_LOGE(TAG, "infer addr type failed: %d", err);
        g_app.ble.scanning = false;
        g_app.results.ble = TEST_FAIL;
        return ESP_FAIL;
    }

    struct ble_gap_disc_params params = {
        .itvl = 0,
        .window = 0,
        .filter_policy = 0,
        .limited = 0,
        .passive = 0,
        .filter_duplicates = 1,
    };

    ESP_LOGI(TAG, "scanning for %d ms", duration_ms);
    err = ble_gap_disc(own_addr_type, duration_ms, &params, on_gap_event, NULL);
    if (err != 0) {
        ESP_LOGE(TAG, "ble_gap_disc failed: %d", err);
        g_app.ble.scanning = false;
        g_app.results.ble = TEST_FAIL;
        return ESP_FAIL;
    }

    /* ble_gap_disc is asynchronous; wait for the duration plus margin. */
    vTaskDelay(pdMS_TO_TICKS(duration_ms + 300));
    ble_gap_disc_cancel();

    g_app.ble.scanning = false;
    g_app.ble.scan_count++;
    g_app.results.ble = TEST_PASS;
    ESP_LOGI(TAG, "scan found %d device(s)", s_seen);
    return ESP_OK;
}

void hw_ble_stop(void)
{
    if (s_nimble_ready) {
        ble_gap_disc_cancel();
        ESP_LOGI(TAG, "BLE scan stopped");
    }
}
