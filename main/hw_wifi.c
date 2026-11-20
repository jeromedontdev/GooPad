/*
 * WiFi: scan for access points.
 *
 * A scan is the least invasive way to prove the radio works: it needs no
 * credentials, no DHCP and no association, and it exercises the RF front
 * end, the PHY and the supplicant in one go. If a scan returns networks,
 * the radio is functional.
 *
 * The application owns the Wi-Fi driver's running state. A scan must not
 * stop it afterward, because that can disconnect an active GooPad session
 * and race the application's reconnect handler.
 */
#include "app.h"
#include "board.h"

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "wifi";

static bool s_netif_ready;
static bool s_wifi_ready;
static bool s_wifi_started;

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

esp_err_t hw_wifi_start(void)
{
    if (!s_wifi_ready) {
        esp_err_t err = hw_wifi_init();
        if (err != ESP_OK) return err;
    }
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

/* Run a blocking scan and record the results. Call from a worker task. */
esp_err_t hw_wifi_scan(void)
{
    if (!s_wifi_ready) {
        esp_err_t err = hw_wifi_init();
        if (err != ESP_OK) {
            return err;
        }
    }

    ESP_LOGI(TAG, "starting scan");
    g_app.wifi.scanning = true;

    bool started_for_scan = !s_wifi_started;
    esp_err_t err = started_for_scan ? hw_wifi_start() : ESP_OK;
    if (err != ESP_OK) {
        g_app.wifi.scanning = false;
        g_app.results.wifi = TEST_FAIL;
        return err;
    }

    wifi_scan_config_t scan_cfg = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
    };

    err = esp_wifi_scan_start(&scan_cfg, true /* block */);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "scan failed: %s", esp_err_to_name(err));
        g_app.results.wifi = TEST_FAIL;
        if (started_for_scan) hw_wifi_stop();
        g_app.wifi.scanning = false;
        return err;
    }

    uint16_t count = 0;
    err = esp_wifi_scan_get_ap_num(&count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "could not get scan result count: %s", esp_err_to_name(err));
        g_app.wifi.ap_count = 0;
        g_app.results.wifi = TEST_FAIL;
        if (started_for_scan) hw_wifi_stop();
        g_app.wifi.scanning = false;
        return err;
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
            g_app.results.wifi = TEST_FAIL;
            if (started_for_scan) hw_wifi_stop();
            g_app.wifi.scanning = false;
            return err;
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

    if (started_for_scan) hw_wifi_stop();
    g_app.wifi.scanning = false;
    return ESP_OK;
}

void hw_wifi_stop(void)
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
