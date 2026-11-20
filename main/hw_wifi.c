/*
 * WiFi: scan for access points.
 *
 * A scan is the least invasive way to prove the radio works: it needs no
 * credentials, no DHCP and no association, and it exercises the RF front
 * end, the PHY and the supplicant in one go. If a scan returns networks,
 * the radio is functional.
 *
 * The application owns the Wi-Fi driver's running state. A scan preserves
 * an already-running driver, and stops it afterward only if the scan started it.
 */
#include "app.h"
#include "board.h"

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "wifi";

static bool s_netif_ready;
static bool s_wifi_ready;
static bool s_wifi_started;
static SemaphoreHandle_t s_wifi_mutex;

static void wifi_stop_locked(void);
static void wifi_finish_scan_locked(bool started_for_scan, bool reconnect);

static esp_err_t ensure_init(void)
{
    if (!s_netif_ready) {
        ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "netif init");
        esp_err_t err = esp_event_loop_create_default();
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "event loop: %s", esp_err_to_name(err));
            return err;
        }
        esp_netif_create_default_wifi_sta();
        s_netif_ready = true;
    }

    if (!s_wifi_ready) {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "wifi init");
        ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_RAM), TAG, "storage");
        ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "mode");
        s_wifi_ready = true;
    }
    if (!s_wifi_mutex) {
        s_wifi_mutex = xSemaphoreCreateMutex();
        if (!s_wifi_mutex) {
            ESP_LOGE(TAG, "could not create Wi-Fi operation mutex");
            return ESP_ERR_NO_MEM;
        }
    }
    return ESP_OK;
}

esp_err_t hw_wifi_init(void)
{
    esp_err_t err = ensure_init();
    if (err != ESP_OK) {
        g_app.wifi.ready = false;
        g_app.results.wifi = TEST_FAIL;
        return err;
    }
    g_app.wifi.ready = true;
    g_app.results.wifi = TEST_PASS;
    ESP_LOGI(TAG, "WiFi initialised (station mode, not connected)");
    return ESP_OK;
}

static esp_err_t wifi_start_locked(void)
{
    if (s_wifi_started) return ESP_OK;

    esp_err_t err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wifi start: %s", esp_err_to_name(err));
        g_app.results.wifi = TEST_FAIL;
        return err;
    }
    s_wifi_started = true;
    return ESP_OK;
}

esp_err_t hw_wifi_start(void)
{
    esp_err_t err = hw_wifi_init();
    if (err != ESP_OK) return err;
    if (xSemaphoreTake(s_wifi_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "could not lock Wi-Fi driver");
        return ESP_FAIL;
    }
    err = wifi_start_locked();
    xSemaphoreGive(s_wifi_mutex);
    return err;
}

esp_err_t hw_wifi_connect(const char *ssid, const char *password)
{
    if (!ssid || !password || !ssid[0] || strlen(ssid) > 32 || strlen(password) > 64) {
        ESP_LOGE(TAG, "rejected invalid Wi-Fi credentials");
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = hw_wifi_init();
    if (err != ESP_OK) return err;
    if (xSemaphoreTake(s_wifi_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "could not lock Wi-Fi driver");
        return ESP_FAIL;
    }

    err = wifi_start_locked();
    if (err == ESP_OK) {
        wifi_config_t config = {0};
        strlcpy((char *)config.sta.ssid, ssid, sizeof(config.sta.ssid));
        strlcpy((char *)config.sta.password, password, sizeof(config.sta.password));
        err = esp_wifi_set_config(WIFI_IF_STA, &config);
        if (err == ESP_OK) err = esp_wifi_connect();
    }
    xSemaphoreGive(s_wifi_mutex);
    return err;
}

/* Run a blocking scan and record the results. Call from a worker task. */
esp_err_t hw_wifi_scan(void)
{
    if (!s_wifi_ready) {
        esp_err_t err = hw_wifi_init();
        if (err != ESP_OK) {
            return err;
        }
    }

    if (xSemaphoreTake(s_wifi_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "could not lock Wi-Fi driver for scan");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "starting scan");
    g_app.wifi.scanning = true;

    bool started_for_scan = !s_wifi_started;
    esp_err_t err = started_for_scan ? wifi_start_locked() : ESP_OK;
    if (err != ESP_OK) {
        goto fail;
    }

    wifi_ap_record_t connected_ap = {0};
    bool was_connected = esp_wifi_sta_get_ap_info(&connected_ap) == ESP_OK;
    if (!was_connected) {
        esp_err_t disconnect_err = esp_wifi_disconnect();
        if (disconnect_err != ESP_OK && disconnect_err != ESP_ERR_WIFI_NOT_CONNECT) {
            ESP_LOGW(TAG, "could not cancel pending connection before scan: %s",
                     esp_err_to_name(disconnect_err));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    wifi_scan_config_t scan_cfg = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
    };

    for (int attempt = 0; attempt < 3; attempt++) {
        err = esp_wifi_scan_start(&scan_cfg, true /* block */);
        if (err != ESP_ERR_WIFI_STATE || attempt == 2) break;
        ESP_LOGW(TAG, "station still connecting; retrying Wi-Fi scan");
        vTaskDelay(pdMS_TO_TICKS(350));
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "scan failed: %s", esp_err_to_name(err));
        goto fail;
    }

    uint16_t count = 0;
    err = esp_wifi_scan_get_ap_num(&count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "could not get scan result count: %s", esp_err_to_name(err));
        g_app.wifi.ap_count = 0;
        goto fail;
    }
    if (count > HW_WIFI_MAX_APS) {
        count = HW_WIFI_MAX_APS;
    }

    wifi_ap_record_t records[HW_WIFI_MAX_APS];
    memset(records, 0, sizeof(records));
    if (count > 0) {
        err = esp_wifi_scan_get_ap_records(&count, records);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "could not get scan results: %s", esp_err_to_name(err));
            g_app.wifi.ap_count = 0;
            goto fail;
        }
    }

    g_app.wifi.ap_count = count;
    for (int i = 0; i < count; i++) {
        size_t ssid_len = strnlen((const char *)records[i].ssid,
                                  sizeof(records[i].ssid));
        if (ssid_len >= sizeof(g_app.wifi.aps[i].ssid)) {
            ssid_len = sizeof(g_app.wifi.aps[i].ssid) - 1;
        }
        memcpy(g_app.wifi.aps[i].ssid, records[i].ssid, ssid_len);
        g_app.wifi.aps[i].ssid[ssid_len] = '\0';
        g_app.wifi.aps[i].rssi = records[i].rssi;
        g_app.wifi.aps[i].channel = records[i].primary;
        g_app.wifi.aps[i].auth = (int)records[i].authmode;
    }

    g_app.wifi.scan_count++;
    g_app.results.wifi = TEST_PASS;

    ESP_LOGI(TAG, "scan found %d network(s)", count);
    for (int i = 0; i < count; i++) {
        ESP_LOGI(TAG, "  %-24s %4d dBm  ch%2d  auth %d",
                 g_app.wifi.aps[i].ssid, g_app.wifi.aps[i].rssi,
                 g_app.wifi.aps[i].channel, g_app.wifi.aps[i].auth);
    }

    wifi_finish_scan_locked(started_for_scan, true);
    xSemaphoreGive(s_wifi_mutex);
    return ESP_OK;

fail:
    g_app.wifi.ap_count = 0;
    g_app.results.wifi = TEST_FAIL;
    wifi_finish_scan_locked(started_for_scan, true);
    xSemaphoreGive(s_wifi_mutex);
    return err;
}

static void wifi_finish_scan_locked(bool started_for_scan, bool reconnect)
{
    if (started_for_scan) wifi_stop_locked();
    if (!reconnect || !s_wifi_started) {
        g_app.wifi.scanning = false;
        return;
    }

    wifi_ap_record_t connected_ap = {0};
    if (esp_wifi_sta_get_ap_info(&connected_ap) == ESP_OK) {
        g_app.wifi.scanning = false;
        return;
    }

    wifi_config_t config = {0};
    esp_err_t err = esp_wifi_get_config(WIFI_IF_STA, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "could not read station config after scan: %s",
                 esp_err_to_name(err));
        g_app.wifi.scanning = false;
        return;
    }
    if (config.sta.ssid[0]) {
        err = esp_wifi_connect();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "could not resume station connection after scan: %s",
                     esp_err_to_name(err));
        }
    }
    g_app.wifi.scanning = false;
}

static void wifi_stop_locked(void)
{
    if (s_wifi_ready && s_wifi_started) {
        esp_err_t err = esp_wifi_stop();
        if (err == ESP_OK) {
            s_wifi_started = false;
            ESP_LOGI(TAG, "WiFi stopped");
        } else {
            ESP_LOGE(TAG, "WiFi stop failed: %s", esp_err_to_name(err));
        }
    }

}

void hw_wifi_stop(void)
{
    if (!s_wifi_ready || !s_wifi_mutex) return;
    if (xSemaphoreTake(s_wifi_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "could not lock Wi-Fi driver to stop");
        return;
    }
    wifi_stop_locked();
    xSemaphoreGive(s_wifi_mutex);
}
