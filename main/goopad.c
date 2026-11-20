#include "app.h"
#include "board.h"

#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_ota_ops.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "mbedtls/base64.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

static const char *TAG = "goopad";
static const char *NVS_NS = "goopad";
#define CONFIG_MAX_BYTES 12000
#define MAX_MACROS 32
#define ICON_BYTES (32 * 32 * 2)

static const char s_default_config[] =
    "{\"cols\":3,\"rows\":3,\"theme\":\"midnight\",\"background\":\"111827\","
    "\"shape\":\"rounded\",\"macros\":["
    "{\"name\":\"Welcome\",\"icon\":\"+\",\"color\":\"4F46E5\","
    "\"action\":\"webhook\",\"method\":\"POST\",\"url\":\"\",\"secret\":\"\","
    "\"body\":\"\"}]}";

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t fa_icons_json_start[] asm("_binary_fa_icons_json_start");
extern const uint8_t fa_icons_json_end[] asm("_binary_fa_icons_json_end");

static char *s_config;
static char s_web_key[25];
static char s_ssid[33];
static volatile bool s_connected;
static volatile bool s_wifi_connecting;
static volatile unsigned s_wifi_connect_attempts;
static httpd_handle_t s_server;
static char s_ip_address[16];
static SemaphoreHandle_t s_config_lock;

#define WIFI_CONNECT_MAX_ATTEMPTS 5

static esp_err_t auth_required(httpd_req_t *req);
static bool valid_icon_key(const char *key);

static bool read_string(const char *key, char *out, size_t capacity)
{
    nvs_handle_t nvs;
    if (nvs_open(NVS_NS, NVS_READONLY, &nvs) != ESP_OK) return false;
    size_t length = capacity;
    esp_err_t err = nvs_get_str(nvs, key, out, &length);
    nvs_close(nvs);
    return err == ESP_OK;
}

static esp_err_t icon_upload_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    if (!g_app.sd.mounted) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Insert and mount an SD card");
    }
    if (req->content_len != ICON_BYTES) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Expected a 32x32 RGB565 icon");
    }
    char query[32];
    char value[20];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, "key", value, sizeof(value)) != ESP_OK ||
        !valid_icon_key(value)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Valid icon key is required");
    }

    char *image = heap_caps_malloc(ICON_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!image) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Image buffer unavailable");
    size_t received = 0;
    while (received < ICON_BYTES) {
        int n = httpd_req_recv(req, image + received, ICON_BYTES - received);
        if (n <= 0) {
            free(image);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Icon upload interrupted");
        }
        received += (size_t)n;
    }

    const char *directory = BSP_SD_MOUNT_POINT "/goopad";
    if (mkdir(directory, 0775) != 0 && errno != EEXIST) {
        free(image);
        ESP_LOGE(TAG, "could not create SD icon directory: errno=%d", errno);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not create icon folder");
    }
    char temporary[48];
    char path[48];
    snprintf(temporary, sizeof(temporary), "%s/icon-%s.tmp", directory, value);
    snprintf(path, sizeof(path), "%s/icon-%s.rgb565", directory, value);
    FILE *file = fopen(temporary, "wb");
    if (!file) {
        free(image);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not write icon to SD");
    }
    bool ok = fwrite(image, 1, ICON_BYTES, file) == ICON_BYTES;
    free(image);
    if (fclose(file) != 0) ok = false;
    if (!ok || rename(temporary, path) != 0) {
        remove(temporary);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not save icon to SD");
    }
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static esp_err_t save_string(const char *key, const char *value)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;
    err = nvs_set_str(nvs, key, value);
    if (err == ESP_OK) err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
}

static void ensure_web_key(void)
{
    if (read_string("webkey", s_web_key, sizeof(s_web_key)) &&
        strlen(s_web_key) == 24) return;

    static const char hex[] = "0123456789abcdef";
    uint8_t random[12];
    esp_fill_random(random, sizeof(random));
    for (size_t i = 0; i < sizeof(random); i++) {
        s_web_key[i * 2] = hex[random[i] >> 4];
        s_web_key[i * 2 + 1] = hex[random[i] & 15];
    }
    s_web_key[24] = '\0';
    ESP_ERROR_CHECK(save_string("webkey", s_web_key));
}

static void load_config(void)
{
    nvs_handle_t nvs;
    size_t length = 0;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &nvs);
    if (err == ESP_OK) {
        err = nvs_get_blob(nvs, "config", NULL, &length);
        if (err == ESP_OK && length > 0 && length <= CONFIG_MAX_BYTES + 1) {
            err = nvs_get_blob(nvs, "config", s_config, &length);
        } else if (err == ESP_OK) {
            err = ESP_ERR_INVALID_SIZE;
        }
        nvs_close(nvs);
    }
    cJSON *saved = err == ESP_OK ? cJSON_Parse(s_config) : NULL;
    bool valid = cJSON_IsObject(saved);
    cJSON_Delete(saved);
    if (!valid) {
        strlcpy(s_config, s_default_config, CONFIG_MAX_BYTES + 1);
    }
}

bool goopad_configured(void)
{
    return s_ssid[0] != '\0';
}

bool goopad_wifi_connected(void)
{
    return s_connected;
}

bool goopad_wifi_connecting(void)
{
    return s_wifi_connecting;
}

void goopad_get_config(char *out, size_t capacity)
{
    if (capacity && s_config && s_config_lock &&
        xSemaphoreTake(s_config_lock, portMAX_DELAY) == pdTRUE) {
        strlcpy(out, s_config, capacity);
        xSemaphoreGive(s_config_lock);
    }
}

const char *goopad_web_password(void)
{
    return s_web_key;
}

void goopad_get_address(char *out, size_t capacity)
{
    if (capacity) strlcpy(out, s_ip_address, capacity);
}

static bool constant_time_equal(const char *a, const char *b, size_t n)
{
    unsigned char diff = 0;
    for (size_t i = 0; i < n; i++) diff |= (unsigned char)(a[i] ^ b[i]);
    return diff == 0;
}

static bool authorized(httpd_req_t *req)
{
    char origin[96];
    char host[64];
    if (httpd_req_get_hdr_value_str(req, "Origin", origin, sizeof(origin)) == ESP_OK) {
        if (httpd_req_get_hdr_value_str(req, "Host", host, sizeof(host)) != ESP_OK) return false;
        char expected[104];
        snprintf(expected, sizeof(expected), "http://%s", host);
        if (strcmp(origin, expected) != 0) return false;
    }
    char header[128];
    if (httpd_req_get_hdr_value_str(req, "Authorization", header, sizeof(header)) != ESP_OK ||
        strncmp(header, "Basic ", 6) != 0) return false;
    unsigned char decoded[64];
    size_t decoded_len = 0;
    if (mbedtls_base64_decode(decoded, sizeof(decoded), &decoded_len,
                              (const unsigned char *)header + 6,
                              strlen(header + 6)) != 0) return false;
    static const char username[] = "admin:";
    if (decoded_len != sizeof(username) - 1 + 24 ||
        memcmp(decoded, username, sizeof(username) - 1) != 0) return false;
    return constant_time_equal((const char *)decoded + sizeof(username) - 1,
                               s_web_key, 24);
}

static esp_err_t auth_required(httpd_req_t *req)
{
    if (authorized(req)) return ESP_OK;
    httpd_resp_set_status(req, "401 Unauthorized");
    httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"GooPad\", charset=\"UTF-8\"");
    return httpd_resp_send(req, "Authentication required", HTTPD_RESP_USE_STRLEN);
}

static esp_err_t receive_body(httpd_req_t *req, char **out)
{
    if (req->content_len <= 0 || req->content_len > CONFIG_MAX_BYTES) return ESP_ERR_INVALID_SIZE;
    char *body = calloc(1, (size_t)req->content_len + 1);
    if (!body) return ESP_ERR_NO_MEM;
    int received = 0;
    while (received < req->content_len) {
        int n = httpd_req_recv(req, body + received, req->content_len - received);
        if (n <= 0) {
            free(body);
            return ESP_FAIL;
        }
        received += n;
    }
    *out = body;
    return ESP_OK;
}

static bool valid_url(const char *url)
{
    return url && (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0) &&
           strlen(url) < 180 && !strchr(url, '\r') && !strchr(url, '\n') &&
           !strchr(url, '@');
}

static bool valid_icon_key(const char *key)
{
    if (!key || strlen(key) != 17 || key[0] != 'i') return false;
    for (size_t i = 1; i < 17; i++) {
        if (!((key[i] >= '0' && key[i] <= '9') ||
              (key[i] >= 'a' && key[i] <= 'f'))) return false;
    }
    return true;
}

static bool validate_config(const char *json)
{
    cJSON *root = cJSON_Parse(json);
    if (!root) return false;
    cJSON *cols = cJSON_GetObjectItemCaseSensitive(root, "cols");
    cJSON *rows = cJSON_GetObjectItemCaseSensitive(root, "rows");
    cJSON *macros = cJSON_GetObjectItemCaseSensitive(root, "macros");
    bool valid = cJSON_IsNumber(cols) && cols->valueint >= 2 && cols->valueint <= 4 &&
                 cJSON_IsNumber(rows) && rows->valueint >= 2 && rows->valueint <= 4 &&
                 cJSON_IsArray(macros) && cJSON_GetArraySize(macros) <= MAX_MACROS;
    for (cJSON *item = valid ? macros->child : NULL; item; item = item->next) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "name");
        cJSON *url = cJSON_GetObjectItemCaseSensitive(item, "url");
        cJSON *method = cJSON_GetObjectItemCaseSensitive(item, "method");
        cJSON *action = cJSON_GetObjectItemCaseSensitive(item, "action");
        cJSON *icon = cJSON_GetObjectItemCaseSensitive(item, "icon");
        cJSON *body = cJSON_GetObjectItemCaseSensitive(item, "body");
        cJSON *secret = cJSON_GetObjectItemCaseSensitive(item, "secret");
        cJSON *icon_image = cJSON_GetObjectItemCaseSensitive(item, "iconImage");
        cJSON *fa_icon = cJSON_GetObjectItemCaseSensitive(item, "faIcon");
        cJSON *action_mode = cJSON_GetObjectItemCaseSensitive(item, "actionMode");
        cJSON *pc_address = cJSON_GetObjectItemCaseSensitive(item, "pcAddress");
        cJSON *pc_target = cJSON_GetObjectItemCaseSensitive(item, "pcTarget");
        valid = cJSON_IsObject(item) && cJSON_IsString(name) && strlen(name->valuestring) <= 24 &&
                cJSON_IsString(icon) && strlen(icon->valuestring) <= 8 &&
                cJSON_IsString(url) && valid_url(url->valuestring) &&
                cJSON_IsString(action) &&
                (strcmp(action->valuestring, "webhook") == 0 ||
                 strcmp(action->valuestring, "http") == 0) &&
                cJSON_IsString(method) &&
                (strcmp(method->valuestring, "GET") == 0 ||
                 strcmp(method->valuestring, "POST") == 0) &&
                cJSON_IsString(body) && strlen(body->valuestring) <= 256 &&
                cJSON_IsString(secret) && strlen(secret->valuestring) <= 64 &&
                (!icon_image || (cJSON_IsString(icon_image) &&
                                 valid_icon_key(icon_image->valuestring))) &&
                (!fa_icon || (cJSON_IsString(fa_icon) && strlen(fa_icon->valuestring) <= 80)) &&
                (!action_mode || (cJSON_IsString(action_mode) &&
                                  (!strcmp(action_mode->valuestring, "simple_link") ||
                                   !strcmp(action_mode->valuestring, "pc_open_url") ||
                                   !strcmp(action_mode->valuestring, "advanced")))) &&
                (!pc_address || (cJSON_IsString(pc_address) &&
                                 strlen(pc_address->valuestring) <= 160)) &&
                (!pc_target || (cJSON_IsString(pc_target) &&
                                valid_url(pc_target->valuestring)));
        if (!valid) break;
    }
    cJSON_Delete(root);
    return valid;
}

static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
    httpd_resp_set_hdr(req, "X-Frame-Options", "DENY");
    httpd_resp_set_hdr(req, "Referrer-Policy", "no-referrer");
    httpd_resp_set_hdr(req, "Content-Security-Policy",
        "default-src 'self'; style-src 'self' 'unsafe-inline'; "
        "script-src 'self' 'unsafe-inline'; connect-src 'self' "
        "https://api.github.com https://github.com https://release-assets.githubusercontent.com; "
        "img-src 'self' data: blob:; object-src 'none'; base-uri 'none'; frame-ancestors 'none'");
    return httpd_resp_send(req, (const char *)index_html_start,
                           (ssize_t)(index_html_end - index_html_start));
}

static esp_err_t icons_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, (const char *)fa_icons_json_start,
                           (ssize_t)(fa_icons_json_end - fa_icons_json_start));
}

static esp_err_t config_get_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    xSemaphoreTake(s_config_lock, portMAX_DELAY);
    esp_err_t err = httpd_resp_send(req, s_config, HTTPD_RESP_USE_STRLEN);
    xSemaphoreGive(s_config_lock);
    return err;
}

static esp_err_t config_post_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    char *body = NULL;
    esp_err_t err = receive_body(req, &body);
    if (err != ESP_OK) {
        return httpd_resp_send_err(req,
            err == ESP_ERR_NO_MEM ? HTTPD_500_INTERNAL_SERVER_ERROR : HTTPD_400_BAD_REQUEST,
            "Could not read configuration");
    }
    if (!validate_config(body)) {
        free(body);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid GooPad config");
    }
    nvs_handle_t nvs;
    err = nvs_open(NVS_NS, NVS_READWRITE, &nvs);
    if (err == ESP_OK) {
        err = nvs_set_blob(nvs, "config", body, strlen(body) + 1);
        if (err == ESP_OK) err = nvs_commit(nvs);
        nvs_close(nvs);
    }
    if (err != ESP_OK) {
        free(body);
        ESP_LOGE(TAG, "save config failed: %s", esp_err_to_name(err));
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not save config");
    }
    xSemaphoreTake(s_config_lock, portMAX_DELAY);
    strlcpy(s_config, body, CONFIG_MAX_BYTES + 1);
    xSemaphoreGive(s_config_lock);
    free(body);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

typedef struct {
    char url[180];
    char method[5];
    char body[257];
    char secret[65];
} macro_action_t;

static void macro_task(void *arg)
{
    macro_action_t *macro = arg;
    esp_http_client_config_t cfg = {
        .url = macro->url,
        .timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        ESP_LOGE(TAG, "HTTP client allocation failed");
        free(macro);
        vTaskDelete(NULL);
        return;
    }
    esp_http_client_set_method(client, strcmp(macro->method, "POST") == 0
                                        ? HTTP_METHOD_POST : HTTP_METHOD_GET);
    if (strcmp(macro->method, "POST") == 0) {
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, macro->body, (int)strlen(macro->body));
    }
    if (macro->secret[0]) {
        esp_http_client_set_header(client, "X-GooPad-Token", macro->secret);
    }
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "macro request completed, status=%d",
                 esp_http_client_get_status_code(client));
    } else {
        ESP_LOGW(TAG, "macro request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    free(macro);
    vTaskDelete(NULL);
}

void goopad_trigger(unsigned index)
{
    xSemaphoreTake(s_config_lock, portMAX_DELAY);
    cJSON *root = cJSON_Parse(s_config);
    cJSON *macros = root ? cJSON_GetObjectItemCaseSensitive(root, "macros") : NULL;
    cJSON *item = cJSON_IsArray(macros) ? cJSON_GetArrayItem(macros, (int)index) : NULL;
    cJSON *url = item ? cJSON_GetObjectItemCaseSensitive(item, "url") : NULL;
    cJSON *method = item ? cJSON_GetObjectItemCaseSensitive(item, "method") : NULL;
    cJSON *body = item ? cJSON_GetObjectItemCaseSensitive(item, "body") : NULL;
    cJSON *secret = item ? cJSON_GetObjectItemCaseSensitive(item, "secret") : NULL;
    if (!cJSON_IsString(url) || !valid_url(url->valuestring) ||
        !cJSON_IsString(method) || !cJSON_IsString(body) || !cJSON_IsString(secret)) {
        cJSON_Delete(root);
        xSemaphoreGive(s_config_lock);
        ESP_LOGW(TAG, "macro %u is not configured", index);
        return;
    }
    macro_action_t *action = calloc(1, sizeof(*action));
    if (!action) {
        cJSON_Delete(root);
        xSemaphoreGive(s_config_lock);
        ESP_LOGE(TAG, "macro task allocation failed");
        return;
    }
    strlcpy(action->url, url->valuestring, sizeof(action->url));
    strlcpy(action->method, method->valuestring, sizeof(action->method));
    strlcpy(action->body, body->valuestring, sizeof(action->body));
    strlcpy(action->secret, secret->valuestring, sizeof(action->secret));
    cJSON_Delete(root);
    xSemaphoreGive(s_config_lock);
    if (xTaskCreate(macro_task, "macro_http", 6144, action, 4, NULL) != pdPASS) {
        free(action);
        ESP_LOGE(TAG, "could not start macro task");
    }
}

static esp_err_t trigger_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    char *body = NULL;
    esp_err_t body_err = receive_body(req, &body);
    if (body_err != ESP_OK) {
        return httpd_resp_send_err(req,
            body_err == ESP_ERR_NO_MEM ? HTTPD_500_INTERNAL_SERVER_ERROR : HTTPD_400_BAD_REQUEST,
            "Could not read action request");
    }
    cJSON *root = cJSON_Parse(body);
    cJSON *index = root ? cJSON_GetObjectItemCaseSensitive(root, "index") : NULL;
    if (!cJSON_IsNumber(index) || index->valueint < 0 || index->valueint >= MAX_MACROS) {
        cJSON_Delete(root);
        free(body);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid macro index");
    }
    goopad_trigger((unsigned)index->valueint);
    cJSON_Delete(root);
    free(body);
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static esp_err_t wifi_join_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    char *body = NULL;
    esp_err_t body_err = receive_body(req, &body);
    if (body_err != ESP_OK) {
        return httpd_resp_send_err(req,
            body_err == ESP_ERR_NO_MEM ? HTTPD_500_INTERNAL_SERVER_ERROR : HTTPD_400_BAD_REQUEST,
            "Could not read Wi-Fi request");
    }
    cJSON *root = cJSON_Parse(body);
    cJSON *ssid = root ? cJSON_GetObjectItemCaseSensitive(root, "ssid") : NULL;
    cJSON *password = root ? cJSON_GetObjectItemCaseSensitive(root, "password") : NULL;
    if (!cJSON_IsString(ssid) || !cJSON_IsString(password) ||
        strlen(ssid->valuestring) == 0 || strlen(ssid->valuestring) > 32 ||
        strlen(password->valuestring) > 64) {
        cJSON_Delete(root);
        free(body);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid Wi-Fi credentials");
    }
    goopad_wifi_join(ssid->valuestring, password->valuestring);
    cJSON_Delete(root);
    free(body);
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static esp_err_t backup_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    if (!g_app.sd.mounted) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No SD card mounted");
    }
    FILE *file = fopen(BSP_SD_MOUNT_POINT "/goopad-config.json", "w");
    if (!file) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "SD write failed");
    xSemaphoreTake(s_config_lock, portMAX_DELAY);
    size_t length = strlen(s_config);
    bool ok = fwrite(s_config, 1, length, file) == length;
    xSemaphoreGive(s_config_lock);
    if (fclose(file) != 0) ok = false;
    if (!ok) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "SD write failed");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static void restart_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(1200));
    esp_restart();
}

static esp_err_t activate_ota(esp_ota_handle_t handle, httpd_req_t *req)
{
    esp_err_t err = esp_ota_end(handle);
    if (err == ESP_OK) {
        const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);
        err = next ? esp_ota_set_boot_partition(next) : ESP_ERR_NOT_FOUND;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA validation/activation failed: %s", esp_err_to_name(err));
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Firmware image rejected");
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true,\"restarting\":true}");
    if (xTaskCreate(restart_task, "ota_restart", 2048, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "could not schedule OTA restart");
    }
    return ESP_OK;
}

static esp_err_t ota_upload_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
    if (!partition || req->content_len <= 0 ||
        (size_t)req->content_len > partition->size) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid firmware size");
    }
    esp_ota_handle_t handle = 0;
    esp_err_t err = esp_ota_begin(partition, (size_t)req->content_len, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(err));
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA could not start");
    }
    uint8_t *buffer = heap_caps_malloc(4096, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buffer) {
        esp_ota_abort(handle);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA buffer unavailable");
    }
    size_t remaining = (size_t)req->content_len;
    while (remaining) {
        int want = remaining > sizeof(buffer) ? sizeof(buffer) : (int)remaining;
        int n = httpd_req_recv(req, (char *)buffer, want);
        if (n <= 0) {
            free(buffer);
            esp_ota_abort(handle);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Firmware upload interrupted");
        }
        err = esp_ota_write(handle, buffer, (size_t)n);
        if (err != ESP_OK) {
            free(buffer);
            esp_ota_abort(handle);
            ESP_LOGE(TAG, "OTA write failed: %s", esp_err_to_name(err));
            return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Firmware write failed");
        }
        remaining -= (size_t)n;
    }
    free(buffer);
    return activate_ota(handle, req);
}

static esp_err_t ota_sd_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    if (!g_app.sd.mounted) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No SD card mounted");
    }
    FILE *file = fopen(BSP_SD_MOUNT_POINT "/goopad.bin", "rb");
    if (!file) return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "goopad.bin not found on SD");
    const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
    if (!partition || fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not read SD image");
    }
    long length = ftell(file);
    rewind(file);
    if (length <= 0 || (size_t)length > partition->size) {
        fclose(file);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid SD firmware size");
    }
    esp_ota_handle_t handle = 0;
    esp_err_t err = esp_ota_begin(partition, (size_t)length, &handle);
    bool started = err == ESP_OK;
    uint8_t *buffer = heap_caps_malloc(4096, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buffer) {
        fclose(file);
        if (started) esp_ota_abort(handle);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA buffer unavailable");
    }
    size_t remaining = (size_t)length;
    while (err == ESP_OK && remaining) {
        size_t n = fread(buffer, 1, 4096 < remaining ? 4096 : remaining, file);
        if (!n) {
            err = ESP_FAIL;
            break;
        }
        err = esp_ota_write(handle, buffer, n);
        remaining -= n;
    }
    free(buffer);
    fclose(file);
    if (err != ESP_OK) {
        if (started) esp_ota_abort(handle);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "SD firmware update failed");
    }
    return activate_ota(handle, req);
}

static esp_err_t status_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "connected", s_connected);
    cJSON_AddStringToObject(root, "ssid", s_ssid);
    cJSON_AddBoolToObject(root, "sd", g_app.sd.mounted);
    char *response = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!response) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Status unavailable");
    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_sendstr(req, response);
    cJSON_free(response);
    return err;
}

static esp_err_t start_server(void)
{
    if (s_server) return ESP_OK;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 12;
    config.lru_purge_enable = true;
    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) return err;
    const httpd_uri_t routes[] = {
        {.uri = "/", .method = HTTP_GET, .handler = index_handler},
        {.uri = "/api/config", .method = HTTP_GET, .handler = config_get_handler},
        {.uri = "/api/config", .method = HTTP_POST, .handler = config_post_handler},
        {.uri = "/api/trigger", .method = HTTP_POST, .handler = trigger_handler},
        {.uri = "/api/wifi/join", .method = HTTP_POST, .handler = wifi_join_handler},
        {.uri = "/api/backup", .method = HTTP_POST, .handler = backup_handler},
        {.uri = "/api/status", .method = HTTP_GET, .handler = status_handler},
        {.uri = "/api/ota", .method = HTTP_POST, .handler = ota_upload_handler},
        {.uri = "/api/ota/sd", .method = HTTP_POST, .handler = ota_sd_handler},
        {.uri = "/api/icons", .method = HTTP_GET, .handler = icons_handler},
        {.uri = "/api/icon", .method = HTTP_POST, .handler = icon_upload_handler},
    };
    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        err = httpd_register_uri_handler(s_server, &routes[i]);
        if (err != ESP_OK) {
            httpd_stop(s_server);
            s_server = NULL;
            return err;
        }
    }
    return ESP_OK;
}

static void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_connected = false;
        if (g_app.wifi.scanning) {
            ESP_LOGI(TAG, "deferring reconnect until Wi-Fi scan completes");
            return;
        }
        if (s_wifi_connect_attempts >= WIFI_CONNECT_MAX_ATTEMPTS) {
            s_wifi_connecting = false;
            ESP_LOGE(TAG, "Wi-Fi connection failed after %u attempts",
                     s_wifi_connect_attempts);
            return;
        }
        s_wifi_connecting = true;
        s_wifi_connect_attempts++;
        ESP_LOGW(TAG, "Wi-Fi disconnected; reconnecting (attempt %u/%u)",
                 s_wifi_connect_attempts, WIFI_CONNECT_MAX_ATTEMPTS);
        esp_err_t err = esp_wifi_connect();
        if (err != ESP_OK) {
            s_wifi_connecting = false;
            ESP_LOGE(TAG, "Wi-Fi reconnect failed to start: %s", esp_err_to_name(err));
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = data;
        s_connected = true;
        s_wifi_connecting = false;
        s_wifi_connect_attempts = 0;
        snprintf(s_ip_address, sizeof(s_ip_address), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "GooPad web editor: http://" IPSTR "/", IP2STR(&event->ip_info.ip));
        esp_err_t err = start_server();
        if (err != ESP_OK) ESP_LOGE(TAG, "HTTP server start failed: %s", esp_err_to_name(err));
    }
}

void goopad_wifi_join(const char *ssid, const char *password)
{
    if (!ssid || !password || strlen(ssid) == 0 || strlen(ssid) > 32 || strlen(password) > 64) {
        ESP_LOGE(TAG, "rejected invalid Wi-Fi credentials");
        return;
    }
    strlcpy(s_ssid, ssid, sizeof(s_ssid));
    esp_err_t err = save_string("ssid", ssid);
    if (err == ESP_OK) err = save_string("wifi", password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "could not save Wi-Fi credentials: %s", esp_err_to_name(err));
        return;
    }
    s_wifi_connecting = true;
    s_wifi_connect_attempts = 1;
    err = hw_wifi_connect(ssid, password);
    if (err != ESP_OK) {
        s_wifi_connecting = false;
        ESP_LOGE(TAG, "Wi-Fi join failed to start: %s", esp_err_to_name(err));
    }
}

void goopad_init(void)
{
    s_config = heap_caps_malloc(CONFIG_MAX_BYTES + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    ESP_ERROR_CHECK(s_config ? ESP_OK : ESP_ERR_NO_MEM);
    s_config_lock = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(s_config_lock ? ESP_OK : ESP_ERR_NO_MEM);
    load_config();
    ensure_web_key();
    read_string("ssid", s_ssid, sizeof(s_ssid));
    if (esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event, NULL) != ESP_OK ||
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "could not register Wi-Fi events");
        return;
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    if (s_ssid[0]) {
        wifi_config_t config = {0};
        strlcpy((char *)config.sta.ssid, s_ssid, sizeof(config.sta.ssid));
        read_string("wifi", (char *)config.sta.password, sizeof(config.sta.password));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
    }
    ESP_ERROR_CHECK(hw_wifi_start());
    if (s_ssid[0]) {
        s_wifi_connecting = true;
        s_wifi_connect_attempts = 1;
        esp_err_t err = esp_wifi_connect();
        if (err != ESP_OK) {
            s_wifi_connecting = false;
            ESP_LOGE(TAG, "saved Wi-Fi connection failed to start: %s",
                     esp_err_to_name(err));
        }
    }
}
