#include "app.h"
#include "board.h"

#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "esp_https_server.h"
#include "goopad_tls.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_ota_ops.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_timer.h"
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
#include <sys/time.h>

static const char *TAG = "goopad";
static const char *NVS_NS = "goopad";
#define CONFIG_MAX_BYTES 12000
#define MAX_MACROS 32
#define ICON_BYTES (32 * 32 * 2)
#define ALPHA_ICON_BYTES (32 * 32 * 3)
#define LARGE_ICON_BYTES (160 * 160 * 3)
#define BACKGROUND_BYTES (480 * 480 * 2)
#define MACRO_BODY_BYTES 3072
#define SOUND_MAX_BYTES (44 + BSP_I2S_SAMPLE_RATE * 2 * 30)
#define GIF_MAX_BYTES (128 * 1024)

static const char s_default_config[] =
    "{\"cols\":3,\"rows\":3,\"theme\":\"midnight\",\"background\":\"111827\","
    "\"shape\":\"rounded\",\"macros\":["
    "{\"name\":\"Welcome\",\"icon\":\"\",\"color\":\"4F46E5\","
    "\"action\":\"webhook\",\"method\":\"POST\",\"url\":\"\",\"secret\":\"\","
    "\"body\":\"\"}]}";

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t fa_icons_json_start[] asm("_binary_fa_icons_json_start");
extern const uint8_t fa_icons_json_end[] asm("_binary_fa_icons_json_end");
extern const uint8_t fa_index_start[] asm("_binary_fa_index_json_gz_start");
extern const uint8_t fa_index_end[] asm("_binary_fa_index_json_gz_end");

static char *s_config;
static char s_web_key[65];
static char s_initial_key[9];
static char s_ssid[33];
static volatile bool s_connected;
static volatile bool s_wifi_connecting;
static volatile unsigned s_wifi_connect_attempts;
static httpd_handle_t s_server;
static volatile bool s_server_starting;
static char s_ip_address[16];
static SemaphoreHandle_t s_config_lock;
static portMUX_TYPE s_action_lock = portMUX_INITIALIZER_UNLOCKED;
static unsigned s_action_count;
static portMUX_TYPE s_monitor_lock = portMUX_INITIALIZER_UNLOCKED;
static char s_monitor_text[64] = "Set PC pairing";
static double s_monitor_values[GOOPAD_PC_STAT_COUNT];
static bool s_monitor_valid[GOOPAD_PC_STAT_COUNT];
static bool s_monitor_sampling;
static const char *s_pc_stat_keys[GOOPAD_PC_STAT_COUNT] = {
    "summary", "cpu", "ram", "ram_used", "gpu", "gpu_memory", "cpu_temp", "gpu_temp"
};

int goopad_pc_stat_index(const char *key)
{
    for (unsigned i = 0; key && i < GOOPAD_PC_STAT_COUNT; i++)
        if (!strcmp(key, s_pc_stat_keys[i])) return (int)i;
    return -1;
}

const char *goopad_pc_stat_key(unsigned index)
{
    return index < GOOPAD_PC_STAT_COUNT ? s_pc_stat_keys[index] : NULL;
}

const char *goopad_widget_time_format(bool date, const char *key)
{
    static const char *keys[2][6] = {
        {"24h", "12h", "24h_seconds", "12h_seconds", "12h_plain", "12h_plain_seconds"}, {"iso", "mdy", "dmy", "month_short"}
    };
    static const char *formats[2][6] = {
        {"%H:%M", "%I:%M %p", "%H:%M:%S", "%I:%M:%S %p", "%I:%M", "%I:%M:%S"},
        {"%Y-%m-%d", "%m/%d/%Y", "%d/%m/%Y", "%b %d"}
    };
    for (unsigned i = 0; key && i < 6; i++)
        if (keys[date][i] && !strcmp(key, keys[date][i])) return formats[date][i];
    return NULL;
}

const goopad_widget_style_t *goopad_widget_style(const char *key)
{
    static const char *keys[] = {"minimal", "neon", "terminal", "paper", "ocean", "sunset", "retro"};
    static const goopad_widget_style_t styles[] = {
        {0x0F172A, 0x0F172A, 0xF8FAFC, 0x94A3B8},
        {0x06121C, 0x102A43, 0xCFFAFE, 0x22D3EE},
        {0x081A14, 0x081A14, 0x86EFAC, 0x22C55E},
        {0xF8FAFC, 0xE2E8F0, 0x172033, 0xEF4444},
        {0x082F49, 0x0E7490, 0xE0F2FE, 0x38BDF8},
        {0x7C2D12, 0x9F1239, 0xFFF7ED, 0xFDBA74},
        {0x292524, 0x44403C, 0xFEF3C7, 0xF59E0B},
    };
    for (unsigned i = 0; key && i < sizeof(keys) / sizeof(keys[0]); i++)
        if (!strcmp(key, keys[i])) return &styles[i];
    return NULL;
}

bool goopad_widget_graphic_valid(const char *graphic, const char *widget, const char *stat)
{
    if (!graphic || !strcmp(graphic, "text") || !strcmp(graphic, "icon")) return true;
    if (!widget) return false;
    if (!strcmp(graphic, "analog")) return !strcmp(widget, "clock");
    if (!strcmp(graphic, "calendar")) return !strcmp(widget, "date");
    if (strcmp(graphic, "bar") && strcmp(graphic, "ring")) return false;
    return !strcmp(widget, "pc_monitor") && (!stat || !strcmp(stat, "summary") ||
        !strcmp(stat, "cpu") || !strcmp(stat, "ram") || !strcmp(stat, "gpu"));
}

#define WIFI_CONNECT_MAX_ATTEMPTS 5

static esp_err_t auth_required(httpd_req_t *req);
static bool valid_icon_key(const char *key);

static bool receive_bytes(httpd_req_t *req, uint8_t *out, size_t bytes)
{
    size_t offset = 0;
    while (offset < bytes) {
        int read = httpd_req_recv(req, (char *)out + offset, bytes - offset);
        if (read <= 0) return false;
        offset += read;
    }
    return true;
}

static bool decode_icon_rle(const uint8_t *input, size_t length, uint8_t *output, unsigned side)
{
    if (length < 6 || memcmp(input, "GPR1", 4) || (side != 32 && side != 160)) return false;
    size_t count = side * side, pixel = 0;
    for (size_t offset = 6; offset < length; offset += 5) {
        if (length - offset < 5) return false;
        unsigned run = input[offset] | (input[offset + 1] << 8);
        if (!run || run > count - pixel) return false;
        for (unsigned i = 0; i < run; i++, pixel++) {
            output[pixel * 2] = input[offset + 2];
            output[pixel * 2 + 1] = input[offset + 3];
            output[count * 2 + pixel] = input[offset + 4];
        }
    }
    return pixel == count;
}

bool goopad_prepare_gif(uint8_t *data, size_t *size, size_t capacity)
{
    size_t length = *size;
    if (length > capacity || length < 14 || (memcmp(data, "GIF89a", 6) && memcmp(data, "GIF87a", 6))) return false;
    unsigned width = data[6] | data[7] << 8, height = data[8] | data[9] << 8;
    if (!width || !height || width > 160 || height > 160) return false;
    size_t offset = 13;
    if (data[10] & 128) offset += 3u << ((data[10] & 7) + 1);
    unsigned frames = 0;
    bool control = false;
    while (offset < length) {
        uint8_t tag = data[offset++];
        if (tag == 0x3B) { *size = length; return frames > 0 && offset == length; }
        if (tag == 0x21) {
            if (offset >= length) return false;
            uint8_t extension = data[offset++];
            if (extension == 0xF9) {
                if (length - offset < 6 || data[offset] != 4 || data[offset + 5] != 0) return false;
                control = true;
                unsigned delay = data[offset + 2] | data[offset + 3] << 8;
                if (delay < 10) { data[offset + 2] = 10; data[offset + 3] = 0; }
            }
        } else if (tag == 0x2C) {
            if (++frames > 240 || length - offset < 9) return false;
            if (!control) {
                if (length + 8 > GIF_MAX_BYTES || length + 8 > capacity) return false;
                const uint8_t timing[] = {0x21, 0xF9, 4, 0, 10, 0, 0, 0};
                memmove(data + offset + 7, data + offset - 1, length - offset + 1);
                memcpy(data + offset - 1, timing, sizeof(timing));
                memcpy(data, "GIF89a", 6);
                length += 8; offset += 8;
            }
            control = false;
            unsigned x = data[offset] | data[offset + 1] << 8;
            unsigned y = data[offset + 2] | data[offset + 3] << 8;
            unsigned w = data[offset + 4] | data[offset + 5] << 8;
            unsigned h = data[offset + 6] | data[offset + 7] << 8;
            uint8_t packed = data[offset + 8];
            if (!w || !h || x + w > width || y + h > height) return false;
            offset += 9;
            if (packed & 128) offset += 3u << ((packed & 7) + 1);
            if (offset >= length || data[offset] < 2 || data[offset] > 8) return false;
            offset++;
        } else return false;
        while (true) {
            if (offset >= length) return false;
            unsigned block = data[offset++];
            if (!block) break;
            if (block > length - offset) return false;
            offset += block;
        }
    }
    return false;
}

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
    char content_type[64] = "";
    httpd_req_get_hdr_value_str(req, "Content-Type", content_type, sizeof(content_type));
    bool rle = !strcmp(content_type, "application/x-goopad-rle");
    bool gif = !strcmp(content_type, "image/gif");
    if ((rle && (req->content_len < 11 || req->content_len > LARGE_ICON_BYTES)) ||
        (gif && (req->content_len < 14 || req->content_len > GIF_MAX_BYTES)) ||
        (!rle && !gif && req->content_len != ICON_BYTES && req->content_len != ALPHA_ICON_BYTES &&
        req->content_len != LARGE_ICON_BYTES && req->content_len != BACKGROUND_BYTES)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Expected a supported icon or 480x480 background");
    }
    char query[32];
    char value[20];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, "key", value, sizeof(value)) != ESP_OK ||
        !valid_icon_key(value)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Valid icon key is required");
    }

    size_t image_bytes = (size_t)req->content_len;
    uint8_t *prepared = NULL;
    if (rle || gif) {
        uint8_t *encoded = heap_caps_malloc(image_bytes + (gif ? 240 * 8 : 0), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!encoded) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Image memory unavailable");
        if (!receive_bytes(req, encoded, image_bytes)) {
            free(encoded);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Image upload interrupted");
        }
        if (gif) {
            if (!goopad_prepare_gif(encoded, &image_bytes, req->content_len + 240 * 8)) {
                free(encoded);
                return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "GIF invalid or exceeds 160x160, 128 KB or 240 frames");
            }
            prepared = encoded;
        } else {
            unsigned side = encoded[4] | encoded[5] << 8;
            size_t decoded_bytes = side == 160 ? LARGE_ICON_BYTES : ALPHA_ICON_BYTES;
            prepared = heap_caps_malloc(decoded_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            bool decoded = prepared && decode_icon_rle(encoded, image_bytes, prepared, side);
            free(encoded);
            if (!decoded) {
                free(prepared);
                return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid compressed icon");
            }
            image_bytes = decoded_bytes;
        }
    }
    char *image = heap_caps_malloc(4096, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!image) { free(prepared); return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Image buffer unavailable"); }

    const char *directory = BSP_SD_MOUNT_POINT "/goopad";
    if (mkdir(directory, 0775) != 0 && errno != EEXIST) {
        free(image);
        free(prepared);
        ESP_LOGE(TAG, "could not create SD icon directory: errno=%d", errno);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not create icon folder");
    }
    char temporary[48];
    char path[64];
    snprintf(temporary, sizeof(temporary), "%s/icon-%s.tmp", directory, value);
    snprintf(path, sizeof(path), "%s/icon-%s.%s", directory, value,
             gif ? "gif" : image_bytes == ICON_BYTES ? "rgb565" :
             image_bytes == BACKGROUND_BYTES ? "480.rgb565" :
             image_bytes == LARGE_ICON_BYTES ? "160.rgb565a8" : "rgb565a8");
    FILE *file = fopen(temporary, "wb");
    if (!file) {
        free(image);
        free(prepared);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not write icon to SD");
    }
    size_t received = 0;
    bool ok = true;
    if (prepared) {
        ok = fwrite(prepared, 1, image_bytes, file) == image_bytes;
        received = image_bytes;
        free(prepared);
    }
    while (received < image_bytes) {
        size_t remaining = image_bytes - received;
        int n = httpd_req_recv(req, image, remaining < 4096 ? remaining : 4096);
        if (n <= 0 || fwrite(image, 1, n, file) != (size_t)n) {
            ok = false;
            break;
        }
        received += n;
    }
    free(image);
    if (fclose(file) != 0) ok = false;
    if (!ok || rename(temporary, path) != 0) {
        remove(temporary);
        ESP_LOGE(TAG, "image upload failed after %u bytes", (unsigned)received);
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

static esp_err_t icon_get_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    char query[96], key[20], format[16], size[8];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, "key", key, sizeof(key)) != ESP_OK ||
        !valid_icon_key(key)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid icon key");
    }
    bool gif = httpd_query_key_value(query, "format", format, sizeof(format)) == ESP_OK &&
               !strcmp(format, "gif");
    bool alpha = httpd_query_key_value(query, "format", format, sizeof(format)) == ESP_OK &&
                 !strcmp(format, "rgb565a8");
    bool large = alpha && httpd_query_key_value(query, "size", size, sizeof(size)) == ESP_OK &&
                 !strcmp(size, "160");
    bool background = !alpha && httpd_query_key_value(query, "size", size, sizeof(size)) == ESP_OK &&
                      !strcmp(size, "480");
    char path[64];
    snprintf(path, sizeof(path), BSP_SD_MOUNT_POINT "/goopad/icon-%s.%s",
             key, gif ? "gif" : background ? "480.rgb565" : large ? "160.rgb565a8" : alpha ? "rgb565a8" : "rgb565");
    FILE *file = g_app.sd.mounted ? fopen(path, "rb") : NULL;
    if (!file) return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Icon not found on SD");
    size_t bytes = background ? BACKGROUND_BYTES : large ? LARGE_ICON_BYTES : alpha ? ALPHA_ICON_BYTES : ICON_BYTES;
    if (gif) {
        struct stat info;
        if (fstat(fileno(file), &info) != 0 || info.st_size < 14 || info.st_size > GIF_MAX_BYTES) {
            fclose(file);
            return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid GIF asset");
        }
        bytes = info.st_size;
    }
    char *pixels = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    bool ok = pixels && fread(pixels, 1, bytes, file) == bytes;
    fclose(file);
    if (!ok) {
        free(pixels);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not read icon");
    }
    httpd_resp_set_type(req, gif ? "image/gif" : "application/octet-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "private, max-age=31536000, immutable");
    esp_err_t err = httpd_resp_send(req, pixels, (ssize_t)bytes);
    free(pixels);
    return err;
}

static esp_err_t background_delete_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    char query[32], key[20], path[64];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, "key", key, sizeof(key)) != ESP_OK || !valid_icon_key(key)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid background key");
    }
    xSemaphoreTake(s_config_lock, portMAX_DELAY);
    bool used = strstr(s_config, key) != NULL;
    snprintf(path, sizeof(path), BSP_SD_MOUNT_POINT "/goopad/icon-%s.480.rgb565", key);
    int result = used ? -1 : remove(path);
    xSemaphoreGive(s_config_lock);
    if (used) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Background is still in use");
    if (result != 0) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not delete background");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static void ensure_web_key(void)
{
    read_string("initialkey", s_initial_key, sizeof(s_initial_key));
    if (read_string("webkey", s_web_key, sizeof(s_web_key)) &&
        strlen(s_web_key) >= 8 && strlen(s_web_key) <= 64) return;

    static const char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    uint8_t random[8];
    esp_fill_random(random, sizeof(random));
    for (size_t i = 0; i < sizeof(random); i++) {
        s_initial_key[i] = alphabet[random[i] & 31];
    }
    s_initial_key[8] = '\0';
    strlcpy(s_web_key, s_initial_key, sizeof(s_web_key));
    nvs_handle_t nvs;
    ESP_ERROR_CHECK(nvs_open(NVS_NS, NVS_READWRITE, &nvs));
    ESP_ERROR_CHECK(nvs_set_str(nvs, "initialkey", s_initial_key));
    ESP_ERROR_CHECK(nvs_set_str(nvs, "webkey", s_web_key));
    ESP_ERROR_CHECK(nvs_commit(nvs));
    nvs_close(nvs);
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

esp_err_t goopad_set_theme(const char *theme)
{
    if (!theme || (strcmp(theme, "midnight") && strcmp(theme, "ocean") &&
                   strcmp(theme, "forest") && strcmp(theme, "light") &&
                   strcmp(theme, "custom"))) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_config_lock, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    cJSON *root = cJSON_Parse(s_config);
    cJSON *theme_item = root ? cJSON_GetObjectItemCaseSensitive(root, "theme") : NULL;
    if (cJSON_IsString(theme_item)) {
        if (!cJSON_SetValuestring(theme_item, theme)) {
            cJSON_Delete(root);
            xSemaphoreGive(s_config_lock);
            return ESP_ERR_NO_MEM;
        }
    } else if (root) {
        if (!cJSON_AddStringToObject(root, "theme", theme)) {
            cJSON_Delete(root);
            xSemaphoreGive(s_config_lock);
            return ESP_ERR_NO_MEM;
        }
    }
    if (root && strcmp(theme, "custom")) {
        const char *background = !strcmp(theme, "ocean") ? "0B1B2A" :
            !strcmp(theme, "forest") ? "10251D" : !strcmp(theme, "light") ? "F1F5F9" : "111827";
        cJSON *bg = cJSON_GetObjectItemCaseSensitive(root, "background");
        bool ok = cJSON_IsString(bg) ? cJSON_SetValuestring(bg, background) != NULL :
                                       cJSON_AddStringToObject(root, "background", background) != NULL;
        if (!ok) {
            cJSON_Delete(root);
            xSemaphoreGive(s_config_lock);
            return ESP_ERR_NO_MEM;
        }
    }
    char *updated = root ? cJSON_PrintUnformatted(root) : NULL;
    cJSON_Delete(root);
    bool too_large = updated && strlen(updated) > CONFIG_MAX_BYTES;
    if (!updated || too_large) {
        free(updated);
        xSemaphoreGive(s_config_lock);
        return too_large ? ESP_ERR_INVALID_SIZE : ESP_ERR_NO_MEM;
    }

    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &nvs);
    if (err == ESP_OK) {
        err = nvs_set_blob(nvs, "config", updated, strlen(updated) + 1);
        if (err == ESP_OK) err = nvs_commit(nvs);
        nvs_close(nvs);
    }
    if (err == ESP_OK) strlcpy(s_config, updated, CONFIG_MAX_BYTES + 1);
    free(updated);
    xSemaphoreGive(s_config_lock);
    return err;
}

const char *goopad_initial_password(void)
{
    return s_initial_key[0] ? s_initial_key : NULL;
}

esp_err_t goopad_set_controls(int brightness, int volume)
{
    if (brightness < 0 || brightness > 100 || volume < 0 || volume > 100) {
        return ESP_ERR_INVALID_ARG;
    }
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;
    err = nvs_set_u8(nvs, "brightness", (uint8_t)brightness);
    if (err == ESP_OK) err = nvs_set_u8(nvs, "volume", (uint8_t)volume);
    if (err == ESP_OK) err = nvs_commit(nvs);
    nvs_close(nvs);
    if (err == ESP_OK) {
        board_backlight_set(brightness);
        hw_audio_set_volume(volume);
    }
    return err;
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
        snprintf(expected, sizeof(expected), "https://%s", host);
        if (strcmp(origin, expected) != 0) return false;
    }
    char header[128];
    if (httpd_req_get_hdr_value_str(req, "Authorization", header, sizeof(header)) != ESP_OK ||
        strncmp(header, "Basic ", 6) != 0) return false;
    unsigned char decoded[80];
    size_t decoded_len = 0;
    if (mbedtls_base64_decode(decoded, sizeof(decoded), &decoded_len,
                              (const unsigned char *)header + 6,
                              strlen(header + 6)) != 0) return false;
    static const char username[] = "admin:";
    if (decoded_len < sizeof(username) - 1 + 8 ||
        decoded_len > sizeof(username) - 1 + 64 ||
        memcmp(decoded, username, sizeof(username) - 1) != 0) return false;
    size_t password_len = decoded_len - (sizeof(username) - 1);
    if (password_len != strlen(s_web_key)) return false;
    return constant_time_equal((const char *)decoded + sizeof(username) - 1,
                               s_web_key, password_len);
}

static esp_err_t auth_required(httpd_req_t *req)
{
    if (authorized(req)) return ESP_OK;
    httpd_resp_set_status(req, "401 Unauthorized");
    httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"GooPad\", charset=\"UTF-8\"");
    httpd_resp_send(req, "Authentication required", HTTPD_RESP_USE_STRLEN);
    return ESP_FAIL;
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

static bool valid_hex_color(const cJSON *value)
{
    if (!cJSON_IsString(value) || strlen(value->valuestring) != 6) return false;
    for (size_t i = 0; i < 6; i++) {
        char c = value->valuestring[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F'))) return false;
    }
    return true;
}

static esp_err_t sound_upload_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    if (!g_app.sd.mounted) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SD card required");
    char query[32], key[20];
    if (req->content_len <= 44 || req->content_len > SOUND_MAX_BYTES ||
        httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, "key", key, sizeof(key)) != ESP_OK || !valid_icon_key(key)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid sound size or key");
    }
    uint8_t header[44];
    size_t received = 0;
    while (received < sizeof(header)) {
        int n = httpd_req_recv(req, (char *)header + received, sizeof(header) - received);
        if (n <= 0) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Sound upload interrupted");
        received += (size_t)n;
    }
    if (!hw_audio_valid_wav(header, (size_t)req->content_len)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Expected 22050 Hz mono PCM WAV, at most 30 seconds");
    }
    if (mkdir(BSP_SD_MOUNT_POINT "/goopad", 0775) != 0 && errno != EEXIST) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not create sound folder");
    }
    char path[64], temporary[64];
    snprintf(path, sizeof(path), BSP_SD_MOUNT_POINT "/goopad/sound-%s.wav", key);
    snprintf(temporary, sizeof(temporary), BSP_SD_MOUNT_POINT "/goopad/sound-%s.tmp", key);
    FILE *file = fopen(temporary, "wb");
    char *buffer = heap_caps_malloc(4096, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    bool ok = file && buffer && fwrite(header, 1, sizeof(header), file) == sizeof(header);
    while (ok && received < (size_t)req->content_len) {
        size_t remaining = (size_t)req->content_len - received;
        int n = httpd_req_recv(req, buffer, remaining < 4096 ? remaining : 4096);
        if (n <= 0) { ok = false; break; }
        ok = fwrite(buffer, 1, (size_t)n, file) == (size_t)n;
        received += (size_t)n;
    }
    free(buffer);
    if (file && fclose(file) != 0) ok = false;
    if (!ok || rename(temporary, path) != 0) {
        remove(temporary);
        ESP_LOGE(TAG, "sound upload failed");
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not save sound");
    }
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static esp_err_t sound_delete_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    char query[32], key[20];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, "key", key, sizeof(key)) != ESP_OK || !valid_icon_key(key)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid sound key");
    }
    xSemaphoreTake(s_config_lock, portMAX_DELAY);
    bool used = strstr(s_config, key) != NULL || hw_audio_busy();
    char path[64];
    snprintf(path, sizeof(path), BSP_SD_MOUNT_POINT "/goopad/sound-%s.wav", key);
    int result = used ? -1 : remove(path);
    xSemaphoreGive(s_config_lock);
    if (used) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Sound is in use");
    if (result != 0) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not delete sound");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static esp_err_t controls_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    char *body = NULL;
    if (receive_body(req, &body) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Could not read controls");
    }
    cJSON *root = cJSON_Parse(body);
    cJSON *brightness = cJSON_GetObjectItemCaseSensitive(root, "brightness");
    cJSON *volume = cJSON_GetObjectItemCaseSensitive(root, "volume");
    bool valid = cJSON_IsNumber(brightness) && cJSON_IsNumber(volume) &&
        brightness->valuedouble == brightness->valueint && volume->valuedouble == volume->valueint;
    esp_err_t err = valid ? goopad_set_controls(brightness->valueint, volume->valueint) : ESP_ERR_INVALID_ARG;
    cJSON_Delete(root);
    free(body);
    if (err != ESP_OK) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, esp_err_to_name(err));
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static bool validate_config(const char *json)
{
    cJSON *root = cJSON_Parse(json);
    if (!root) return false;
    cJSON *cols = cJSON_GetObjectItemCaseSensitive(root, "cols");
    cJSON *rows = cJSON_GetObjectItemCaseSensitive(root, "rows");
    cJSON *macros = cJSON_GetObjectItemCaseSensitive(root, "macros");
    cJSON *theme = cJSON_GetObjectItemCaseSensitive(root, "theme");
    cJSON *background = cJSON_GetObjectItemCaseSensitive(root, "background");
    cJSON *background_image = cJSON_GetObjectItemCaseSensitive(root, "backgroundImage");
    cJSON *hide = cJSON_GetObjectItemCaseSensitive(root, "hideUnconfigured");
    cJSON *offset = cJSON_GetObjectItemCaseSensitive(root, "timeOffsetMinutes");
    cJSON *pages = cJSON_GetObjectItemCaseSensitive(root, "pageCount");
    unsigned gif_count = 0;
    bool valid = cJSON_IsNumber(cols) && cols->valueint >= 2 && cols->valueint <= 4 &&
                 cJSON_IsNumber(rows) && rows->valueint >= 2 && rows->valueint <= 4 &&
                 cJSON_IsArray(macros) && cJSON_GetArraySize(macros) <= MAX_MACROS &&
                 (!pages || (cJSON_IsNumber(pages) && pages->valuedouble == pages->valueint &&
                             pages->valueint >= 1 &&
                             pages->valueint <= (MAX_MACROS + cols->valueint * rows->valueint - 1) / (cols->valueint * rows->valueint) &&
                             cJSON_GetArraySize(macros) <= pages->valueint * cols->valueint * rows->valueint)) &&
                 (!theme || (cJSON_IsString(theme) &&
                             (!strcmp(theme->valuestring, "midnight") ||
                              !strcmp(theme->valuestring, "ocean") ||
                              !strcmp(theme->valuestring, "forest") ||
                              !strcmp(theme->valuestring, "light") ||
                              !strcmp(theme->valuestring, "custom")))) &&
                 (!background || valid_hex_color(background)) &&
                 (!background_image || (cJSON_IsString(background_image) &&
                                        valid_icon_key(background_image->valuestring))) &&
                 (!hide || cJSON_IsBool(hide)) &&
                 (!offset || (cJSON_IsNumber(offset) && offset->valuedouble == offset->valueint &&
                              offset->valueint >= -840 && offset->valueint <= 840));
    for (cJSON *item = valid ? macros->child : NULL; item; item = item->next) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "name");
        cJSON *tile_name = cJSON_GetObjectItemCaseSensitive(item, "tileName");
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
        cJSON *shape = cJSON_GetObjectItemCaseSensitive(item, "shape");
        cJSON *text_color = cJSON_GetObjectItemCaseSensitive(item, "textColor");
        cJSON *tile_color = cJSON_GetObjectItemCaseSensitive(item, "color");
        cJSON *image_format = cJSON_GetObjectItemCaseSensitive(item, "iconFormat");
        cJSON *gif_fit = cJSON_GetObjectItemCaseSensitive(item, "gifFit");
        cJSON *sound = cJSON_GetObjectItemCaseSensitive(item, "sound");
        cJSON *sound_file = cJSON_GetObjectItemCaseSensitive(item, "soundFile");
        cJSON *sound_name = cJSON_GetObjectItemCaseSensitive(item, "soundName");
        cJSON *icon_size = cJSON_GetObjectItemCaseSensitive(item, "iconSize");
        cJSON *outline = cJSON_GetObjectItemCaseSensitive(item, "outline");
        cJSON *outline_color = cJSON_GetObjectItemCaseSensitive(item, "outlineColor");
        cJSON *widget = cJSON_GetObjectItemCaseSensitive(item, "widget");
        cJSON *clock_format = cJSON_GetObjectItemCaseSensitive(item, "clockFormat");
        cJSON *date_format = cJSON_GetObjectItemCaseSensitive(item, "dateFormat");
        cJSON *pc_stat = cJSON_GetObjectItemCaseSensitive(item, "pcStat");
        cJSON *temperature_unit = cJSON_GetObjectItemCaseSensitive(item, "temperatureUnit");
        cJSON *widget_theme = cJSON_GetObjectItemCaseSensitive(item, "widgetTheme");
        cJSON *widget_graphic = cJSON_GetObjectItemCaseSensitive(item, "widgetGraphic");
        cJSON *widget_size = cJSON_GetObjectItemCaseSensitive(item, "widgetTextSize");
        cJSON *widget_accent = cJSON_GetObjectItemCaseSensitive(item, "widgetAccent");
        bool gif = cJSON_IsString(image_format) && !strcmp(image_format->valuestring, "gif");
        if (gif) gif_count++;
        valid = cJSON_IsObject(item) && cJSON_IsString(name) && strlen(name->valuestring) <= 24 &&
                (!tile_name || (cJSON_IsString(tile_name) && strlen(tile_name->valuestring) <= 24)) &&
                (!gif_fit || (cJSON_IsString(gif_fit) &&
                             (!strcmp(gif_fit->valuestring, "contain") || !strcmp(gif_fit->valuestring, "cover")))) &&
                (!clock_format || (cJSON_IsString(clock_format) &&
                                   goopad_widget_time_format(false, clock_format->valuestring))) &&
                (!date_format || (cJSON_IsString(date_format) &&
                                  goopad_widget_time_format(true, date_format->valuestring))) &&
                (!pc_stat || (cJSON_IsString(pc_stat) && goopad_pc_stat_index(pc_stat->valuestring) >= 0)) &&
                (!temperature_unit || (cJSON_IsString(temperature_unit) &&
                                       (!strcmp(temperature_unit->valuestring, "C") ||
                                        !strcmp(temperature_unit->valuestring, "F")))) &&
                (!widget_theme || (cJSON_IsString(widget_theme) &&
                                  (!strcmp(widget_theme->valuestring, "custom") || goopad_widget_style(widget_theme->valuestring)))) &&
                (!widget_accent || valid_hex_color(widget_accent)) &&
                (!widget_size || (cJSON_IsNumber(widget_size) &&
                                  (widget_size->valuedouble == 0 || widget_size->valuedouble == 12 ||
                                   widget_size->valuedouble == 14 || widget_size->valuedouble == 16 ||
                                   widget_size->valuedouble == 20 || widget_size->valuedouble == 28 ||
                                   widget_size->valuedouble == 40))) &&
                (!widget_graphic || (cJSON_IsString(widget_graphic) &&
                                    goopad_widget_graphic_valid(widget_graphic->valuestring,
                                        cJSON_IsString(widget) ? widget->valuestring : NULL,
                                        cJSON_IsString(pc_stat) ? pc_stat->valuestring : NULL))) &&
                cJSON_IsString(icon) && strlen(icon->valuestring) <= 8 &&
                cJSON_IsString(url) &&
                (valid_url(url->valuestring) ||
                 (url->valuestring[0] == '\0' &&
                  (!action_mode || (cJSON_IsString(action_mode) &&
                                    (!strcmp(action_mode->valuestring, "simple_link") ||
                                     !strcmp(action_mode->valuestring, "sound") ||
                                     !strcmp(action_mode->valuestring, "widget")))))) &&
                cJSON_IsString(action) &&
                (strcmp(action->valuestring, "webhook") == 0 ||
                 strcmp(action->valuestring, "http") == 0) &&
                cJSON_IsString(method) &&
                (strcmp(method->valuestring, "GET") == 0 ||
                 strcmp(method->valuestring, "POST") == 0) &&
                cJSON_IsString(body) && strlen(body->valuestring) <= MACRO_BODY_BYTES &&
                cJSON_IsString(secret) && strlen(secret->valuestring) <= 64 &&
                (!icon_image || (cJSON_IsString(icon_image) &&
                                 valid_icon_key(icon_image->valuestring))) &&
                (!fa_icon || (cJSON_IsString(fa_icon) && strlen(fa_icon->valuestring) <= 80)) &&
                (!action_mode || (cJSON_IsString(action_mode) &&
                                  (!strcmp(action_mode->valuestring, "simple_link") ||
                                   !strcmp(action_mode->valuestring, "pc_open_url") ||
                                   !strcmp(action_mode->valuestring, "pc_macro") ||
                                   !strcmp(action_mode->valuestring, "sound") ||
                                   !strcmp(action_mode->valuestring, "widget") ||
                                   !strcmp(action_mode->valuestring, "advanced")))) &&
                (!pc_address || (cJSON_IsString(pc_address) &&
                                 strlen(pc_address->valuestring) <= 160)) &&
                (!pc_target || (cJSON_IsString(pc_target) &&
                                valid_url(pc_target->valuestring))) &&
                (!shape || (cJSON_IsString(shape) &&
                            (!strcmp(shape->valuestring, "rounded") ||
                             !strcmp(shape->valuestring, "square") ||
                             !strcmp(shape->valuestring, "pill") ||
                             !strcmp(shape->valuestring, "soft") ||
                             !strcmp(shape->valuestring, "circle")))) &&
                (!image_format || (cJSON_IsString(image_format) &&
                                   (!strcmp(image_format->valuestring, "rgb565a8") || gif))) &&
                (!widget || (cJSON_IsString(widget) &&
                             (!strcmp(widget->valuestring, "clock") || !strcmp(widget->valuestring, "date") ||
                              !strcmp(widget->valuestring, "pc_monitor")))) &&
                (!(cJSON_IsString(action_mode) && !strcmp(action_mode->valuestring, "widget")) ||
                 cJSON_IsString(widget)) &&
                (!(cJSON_IsString(action_mode) && !strcmp(action_mode->valuestring, "widget") &&
                   cJSON_IsString(widget) && !strcmp(widget->valuestring, "pc_monitor")) ||
                 (valid_url(url->valuestring) && strlen(secret->valuestring) >= 32)) &&
                (!gif || (gif_count <= 4 && cJSON_IsString(icon_image))) &&
                (!sound || (cJSON_IsString(sound) &&
                            (!strcmp(sound->valuestring, "none") ||
                             !strcmp(sound->valuestring, "click") ||
                             !strcmp(sound->valuestring, "beep") ||
                             !strcmp(sound->valuestring, "chime") ||
                             !strcmp(sound->valuestring, "file") ||
                             !strcmp(sound->valuestring, "alert")))) &&
                (!sound_file || (cJSON_IsString(sound_file) &&
                                 valid_icon_key(sound_file->valuestring))) &&
                (!sound_name || (cJSON_IsString(sound_name) && strlen(sound_name->valuestring) <= 384)) &&
                (!(cJSON_IsString(sound) && !strcmp(sound->valuestring, "file")) ||
                 cJSON_IsString(sound_file)) &&
                (!icon_size || (cJSON_IsNumber(icon_size) &&
                                (icon_size->valuedouble == 32 || icon_size->valuedouble == 160))) &&
                (!(cJSON_IsNumber(icon_size) && icon_size->valueint == 160) ||
                 (cJSON_IsString(image_format) && (!strcmp(image_format->valuestring, "rgb565a8") || gif))) &&
                (!outline || cJSON_IsBool(outline)) &&
                (!outline_color || valid_hex_color(outline_color)) &&
                (!(cJSON_IsString(action_mode) && !strcmp(action_mode->valuestring, "sound")) ||
                 (cJSON_IsString(sound) && strcmp(sound->valuestring, "none"))) &&
                (!text_color || valid_hex_color(text_color)) &&
                (!tile_color || valid_hex_color(tile_color));
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
    httpd_resp_set_hdr(req, "Cache-Control", "private, max-age=86400");
    char query[192], search[96] = "", name[96] = "", style[16] = "";
    size_t query_length = httpd_req_get_url_query_len(req);
    if (query_length >= sizeof(query))
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Icon search query is too long");
    if (query_length && httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid icon search query");
    if (query_length) {
        httpd_query_key_value(query, "q", search, sizeof(search));
        httpd_query_key_value(query, "name", name, sizeof(name));
        httpd_query_key_value(query, "style", style, sizeof(style));
    }
    if (!search[0] && !name[0]) {
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
        return httpd_resp_send(req, (const char *)fa_index_start, fa_index_end - fa_index_start);
    }
    for (const char *p = search; *p; p++) {
        if (!((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '-'))
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Search by icon name");
    }
    char *reply = heap_caps_malloc(128 * 1024, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!reply) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Icon search memory unavailable");
    size_t used = 10;
    memcpy(reply, "{\"icons\":[", used);
    unsigned matches = 0;
    const char *record = strstr((const char *)fa_icons_json_start, "{\"name\":\"");
    while (record && matches < 24) {
        const char *next = strstr(record + 1, "},{\"name\":\"");
        const char *end = next ? next + 1 : strstr(record, "}]}");
        if (!end) break;
        if (!next) end++;
        char icon_name[96], icon_style[16], combined[116];
        const char *name_end = strchr(record + 9, '"');
        const char *style_start = strstr(record, "\"style\":\"");
        if (!name_end || !style_start || style_start >= end) break;
        size_t name_length = name_end - (record + 9);
        style_start += 9;
        const char *style_end = strchr(style_start, '"');
        if (name_length >= sizeof(icon_name) || !style_end ||
            (size_t)(style_end - style_start) >= sizeof(icon_style)) break;
        memcpy(icon_name, record + 9, name_length); icon_name[name_length] = '\0';
        memcpy(icon_style, style_start, style_end - style_start); icon_style[style_end - style_start] = '\0';
        snprintf(combined, sizeof(combined), "%s %s", icon_name, icon_style);
        bool match = name[0] ? !strcmp(name, icon_name) && !strcmp(style, icon_style)
                             : strstr(combined, search) != NULL;
        if (match) {
            size_t bytes = end - record;
            if (used + bytes + 3 >= 128 * 1024) {
                free(reply);
                return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Icon search response too large");
            }
            if (matches++) reply[used++] = ',';
            memcpy(reply + used, record, bytes);
            used += bytes;
        }
        record = next ? next + 2 : NULL;
    }
    memcpy(reply + used, "]}", 2); used += 2;
    esp_err_t err = httpd_resp_send(req, reply, used);
    free(reply);
    return err;
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

static esp_err_t password_post_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;

    char *body = NULL;
    esp_err_t err = receive_body(req, &body);
    if (err != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                                   "Could not read password change");
    }

    cJSON *root = cJSON_Parse(body);
    cJSON *password = root ? cJSON_GetObjectItemCaseSensitive(root, "password") : NULL;
    bool valid = cJSON_IsString(password) && password->valuestring;
    size_t length = valid ? strlen(password->valuestring) : 0;
    valid = valid && length >= 8 && length <= 64;
    for (size_t i = 0; valid && i < length; i++) {
        unsigned char c = (unsigned char)password->valuestring[i];
        if (c < 0x20 || c > 0x7e) valid = false;
    }
    if (!valid) {
        cJSON_Delete(root);
        memset(body, 0, strlen(body));
        free(body);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                                   "Password must be 8-64 printable ASCII characters");
    }

    err = save_string("webkey", password->valuestring);
    if (err == ESP_OK) {
        strlcpy(s_web_key, password->valuestring, sizeof(s_web_key));
    }
    cJSON_Delete(root);
    memset(body, 0, strlen(body));
    free(body);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "could not save admin password: %s", esp_err_to_name(err));
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Could not save admin password");
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

typedef struct {
    char url[180];
    char method[5];
    char body[MACRO_BODY_BYTES + 1];
    char secret[65];
} macro_action_t;

static void action_finished(void)
{
    portENTER_CRITICAL(&s_action_lock);
    s_action_count--;
    portEXIT_CRITICAL(&s_action_lock);
}

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
        action_finished();
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
    action_finished();
    vTaskDelete(NULL);
}

esp_err_t goopad_trigger(unsigned index)
{
    xSemaphoreTake(s_config_lock, portMAX_DELAY);
    cJSON *root = cJSON_Parse(s_config);
    cJSON *macros = root ? cJSON_GetObjectItemCaseSensitive(root, "macros") : NULL;
    cJSON *item = cJSON_IsArray(macros) ? cJSON_GetArrayItem(macros, (int)index) : NULL;
    cJSON *url = item ? cJSON_GetObjectItemCaseSensitive(item, "url") : NULL;
    cJSON *method = item ? cJSON_GetObjectItemCaseSensitive(item, "method") : NULL;
    cJSON *body = item ? cJSON_GetObjectItemCaseSensitive(item, "body") : NULL;
    cJSON *secret = item ? cJSON_GetObjectItemCaseSensitive(item, "secret") : NULL;
    cJSON *sound = item ? cJSON_GetObjectItemCaseSensitive(item, "sound") : NULL;
    cJSON *mode = item ? cJSON_GetObjectItemCaseSensitive(item, "actionMode") : NULL;
    if (cJSON_IsString(mode) && !strcmp(mode->valuestring, "widget")) {
        cJSON_Delete(root);
        xSemaphoreGive(s_config_lock);
        return ESP_OK;
    }
    esp_err_t sound_err = ESP_OK;
    if (cJSON_IsString(sound) && strcmp(sound->valuestring, "none")) {
        cJSON *file = cJSON_GetObjectItemCaseSensitive(item, "soundFile");
        sound_err = hw_audio_play_sound(sound->valuestring,
                                        cJSON_IsString(file) ? file->valuestring : NULL);
    }
    if (sound_err != ESP_OK) {
        cJSON_Delete(root);
        xSemaphoreGive(s_config_lock);
        return sound_err;
    }
    if (cJSON_IsString(mode) && !strcmp(mode->valuestring, "sound")) {
        cJSON_Delete(root);
        xSemaphoreGive(s_config_lock);
        return sound_err;
    }
    if (!cJSON_IsString(url) || !valid_url(url->valuestring) ||
        !cJSON_IsString(method) || !cJSON_IsString(body) || !cJSON_IsString(secret)) {
        cJSON_Delete(root);
        xSemaphoreGive(s_config_lock);
        ESP_LOGW(TAG, "macro %u is not configured", index);
        return sound_err != ESP_OK ? sound_err : ESP_ERR_INVALID_ARG;
    }
    portENTER_CRITICAL(&s_action_lock);
    bool busy = s_action_count >= 2;
    if (!busy) s_action_count++;
    portEXIT_CRITICAL(&s_action_lock);
    if (busy) {
        cJSON_Delete(root);
        xSemaphoreGive(s_config_lock);
        return ESP_ERR_INVALID_STATE;
    }
    macro_action_t *action = heap_caps_calloc(1, sizeof(*action), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!action) {
        cJSON_Delete(root);
        xSemaphoreGive(s_config_lock);
        ESP_LOGE(TAG, "macro task allocation failed");
        action_finished();
        return ESP_ERR_NO_MEM;
    }
    strlcpy(action->url, url->valuestring, sizeof(action->url));
    strlcpy(action->method, method->valuestring, sizeof(action->method));
    strlcpy(action->body, body->valuestring, sizeof(action->body));
    strlcpy(action->secret, secret->valuestring, sizeof(action->secret));
    cJSON_Delete(root);
    xSemaphoreGive(s_config_lock);
    if (xTaskCreate(macro_task, "macro_http", 6144, action, 4, NULL) != pdPASS) {
        free(action);
        action_finished();
        ESP_LOGE(TAG, "could not start macro task");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
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
    esp_err_t err = goopad_trigger((unsigned)index->valueint);
    cJSON_Delete(root);
    free(body);
    if (err == ESP_ERR_INVALID_STATE) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        return httpd_resp_sendstr(req, "An action is already running; try again shortly");
    }
    if (err != ESP_OK) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, esp_err_to_name(err));
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
    if (req->method == HTTP_GET) {
        FILE *backup = fopen(BSP_SD_MOUNT_POINT "/goopad-config.json", "rb");
        if (!backup) return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "No configuration backup on SD");
        struct stat info;
        if (fstat(fileno(backup), &info) != 0 || info.st_size < 1 || info.st_size > CONFIG_MAX_BYTES) {
            fclose(backup);
            return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid SD backup size");
        }
        char *data = heap_caps_malloc(info.st_size + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        bool ok = data && fread(data, 1, info.st_size, backup) == (size_t)info.st_size;
        fclose(backup);
        if (!ok) {
            free(data);
            return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not read SD backup");
        }
        data[info.st_size] = '\0';
        if (!validate_config(data)) {
            free(data);
            return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid SD backup configuration");
        }
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Cache-Control", "no-store");
        esp_err_t result = httpd_resp_send(req, data, info.st_size);
        free(data);
        return result;
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
    esp_err_t err = goopad_install_ota_from_sd();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SD firmware update failed: %s", esp_err_to_name(err));
        return httpd_resp_send_err(req,
            err == ESP_ERR_NOT_FOUND ? HTTPD_404_NOT_FOUND :
            err == ESP_ERR_INVALID_SIZE ? HTTPD_400_BAD_REQUEST :
            HTTPD_500_INTERNAL_SERVER_ERROR,
            esp_err_to_name(err));
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true,\"restarting\":true}");
    if (xTaskCreate(restart_task, "ota_restart", 2048, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "could not schedule OTA restart");
    }
    return ESP_OK;
}

esp_err_t goopad_install_ota_from_sd(void)
{
    if (!g_app.sd.mounted) {
        return ESP_ERR_INVALID_STATE;
    }
    FILE *file = fopen(BSP_SD_MOUNT_POINT "/goopad.bin", "rb");
    if (!file) return ESP_ERR_NOT_FOUND;
    const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
    if (!partition || fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return ESP_FAIL;
    }
    long length = ftell(file);
    rewind(file);
    if (length <= 0 || (size_t)length > partition->size) {
        fclose(file);
        return ESP_ERR_INVALID_SIZE;
    }
    esp_ota_handle_t handle = 0;
    esp_err_t err = esp_ota_begin(partition, (size_t)length, &handle);
    bool started = err == ESP_OK;
    uint8_t *buffer = heap_caps_malloc(4096, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buffer) {
        fclose(file);
        if (started) esp_ota_abort(handle);
        return ESP_ERR_NO_MEM;
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
        return err;
    }
    err = esp_ota_end(handle);
    if (err == ESP_OK) {
        const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);
        err = next ? esp_ota_set_boot_partition(next) : ESP_ERR_NOT_FOUND;
    }
    if (err != ESP_OK) ESP_LOGE(TAG, "SD OTA validation/activation failed: %s",
                                esp_err_to_name(err));
    return err;
}

static esp_err_t status_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "connected", s_connected);
    cJSON_AddStringToObject(root, "ssid", s_ssid);
    cJSON_AddBoolToObject(root, "sd", g_app.sd.mounted);
    cJSON_AddBoolToObject(root, "soundAvailable", g_app.results.audio == TEST_PASS);
    cJSON_AddNumberToObject(root, "audioTones", g_app.audio_tones_played);
    cJSON_AddNumberToObject(root, "audioBytes", g_app.audio_bytes_written);
    cJSON_AddNumberToObject(root, "audioPeak", g_app.audio_peak);
    cJSON_AddNumberToObject(root, "uptimeMs", esp_timer_get_time() / 1000);
    cJSON_AddNumberToObject(root, "timeEpoch", time(NULL));
    cJSON_AddNumberToObject(root, "renderedTiles", g_app.rendered_tiles);
    cJSON_AddNumberToObject(root, "renderedPages", g_app.rendered_pages);
    cJSON_AddNumberToObject(root, "emptyTiles", g_app.empty_tiles);
    cJSON_AddNumberToObject(root, "widgets", g_app.widget_count);
    cJSON_AddNumberToObject(root, "gifs", g_app.gif_count);
    cJSON_AddNumberToObject(root, "gifLoops", g_app.gif_loops);
    char monitor[64];
    goopad_monitor_text(monitor, sizeof(monitor));
    cJSON_AddStringToObject(root, "pcMonitor", monitor);
    cJSON *stats = cJSON_AddObjectToObject(root, "pcStats");
    cJSON *fahrenheit = cJSON_AddObjectToObject(root, "pcStatsF");
    for (unsigned i = 0; i < GOOPAD_PC_STAT_COUNT; i++) {
        goopad_pc_stat_text(i, false, monitor, sizeof(monitor));
        cJSON_AddStringToObject(stats, goopad_pc_stat_key(i), monitor);
        if (i >= 6) {
            goopad_pc_stat_text(i, true, monitor, sizeof(monitor));
            cJSON_AddStringToObject(fahrenheit, goopad_pc_stat_key(i), monitor);
        }
    }
    cJSON_AddBoolToObject(root, "audioBusy", hw_audio_busy());
    cJSON_AddNumberToObject(root, "brightness", board_backlight_get());
    cJSON_AddNumberToObject(root, "backlightDuty", board_backlight_duty());
    cJSON_AddNumberToObject(root, "volume", hw_audio_get_volume());
    char *response = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!response) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Status unavailable");
    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_sendstr(req, response);
    cJSON_free(response);
    return err;
}

typedef struct {
    char *data;
    size_t length;
    bool overflow;
} companion_reply_t;

static esp_err_t companion_response(esp_http_client_event_t *event)
{
    companion_reply_t *reply = event->user_data;
    if (event->event_id == HTTP_EVENT_ON_DATA && event->data_len > 0) {
        if (reply->length + event->data_len > 2048) reply->overflow = true;
        else {
            memcpy(reply->data + reply->length, event->data, event->data_len);
            reply->length += event->data_len;
        }
    }
    return ESP_OK;
}

static esp_err_t companion_status_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    char *body = NULL;
    if (receive_body(req, &body) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Could not read Companion settings");
    }
    cJSON *root = cJSON_Parse(body);
    cJSON *url = root ? cJSON_GetObjectItemCaseSensitive(root, "url") : NULL;
    cJSON *token = root ? cJSON_GetObjectItemCaseSensitive(root, "token") : NULL;
    bool valid = cJSON_IsString(url) && valid_url(url->valuestring) &&
                 cJSON_IsString(token) && strlen(token->valuestring) >= 32 &&
                 strlen(token->valuestring) <= 64 && !strchr(token->valuestring, '\r') &&
                 !strchr(token->valuestring, '\n');
    const char *endpoint = valid ? strrchr(url->valuestring, '/') : NULL;
    if (endpoint && !strcmp(endpoint, "/monitor")) {
        cJSON_Delete(root);
        free(body);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
            "Reload the GooPad website to use cached PC monitoring. Save pairing and widget changes first.");
    }
    companion_reply_t reply = {.data = heap_caps_calloc(1, 2049, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)};
    esp_http_client_config_t config = {
        .url = valid ? url->valuestring : NULL, .timeout_ms = 4000,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .disable_auto_redirect = true,
        .event_handler = companion_response, .user_data = &reply,
    };
    esp_http_client_handle_t client = valid && reply.data ? esp_http_client_init(&config) : NULL;
    esp_err_t err = ESP_FAIL;
    int status = 0;
    if (client) {
        esp_http_client_set_header(client, "X-GooPad-Token", token->valuestring);
        err = esp_http_client_perform(client);
        status = esp_http_client_get_status_code(client);
        esp_http_client_cleanup(client);
    }
    cJSON *response = err == ESP_OK && !reply.overflow ? cJSON_Parse(reply.data) : NULL;
    bool ok = status == 200 && cJSON_IsObject(response);
    cJSON_Delete(response);
    cJSON_Delete(root);
    free(body);
    esp_err_t result;
    if (!valid) result = httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Check the PC address and pairing token");
    else if (!ok) {
        ESP_LOGW(TAG, "Companion check failed: %s, HTTP %d", esp_err_to_name(err), status);
        result = httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, status == 401 ?
            "Companion rejected the pairing token" :
            "Could not reach the new Companion. Check its app, address, version and Private-network firewall rule.");
    } else {
        httpd_resp_set_type(req, "application/json");
        result = httpd_resp_send(req, reply.data, reply.length);
    }
    free(reply.data);
    return result;
}

void goopad_monitor_text(char *out, size_t capacity)
{
    goopad_pc_stat_text(0, false, out, capacity);
}

bool goopad_pc_stat_value(unsigned index, double *out)
{
    if (!index) index = 1;
    if (index >= GOOPAD_PC_STAT_COUNT) return false;
    portENTER_CRITICAL(&s_monitor_lock);
    bool valid = !s_monitor_text[0] && s_monitor_valid[index];
    if (valid) *out = s_monitor_values[index];
    portEXIT_CRITICAL(&s_monitor_lock);
    return valid;
}

void goopad_pc_stat_text(unsigned index, bool fahrenheit, char *out, size_t capacity)
{
    static const char *labels[GOOPAD_PC_STAT_COUNT] = {
        "", "CPU", "RAM", "RAM used", "GPU", "GPU RAM", "CPU temp", "GPU temp"
    };
    if (index >= GOOPAD_PC_STAT_COUNT) {
        strlcpy(out, "Invalid PC stat", capacity);
        return;
    }
    double values[GOOPAD_PC_STAT_COUNT];
    bool valid[GOOPAD_PC_STAT_COUNT], sampling;
    char status[64];
    portENTER_CRITICAL(&s_monitor_lock);
    strlcpy(status, s_monitor_text, sizeof(status));
    memcpy(values, s_monitor_values, sizeof(values));
    memcpy(valid, s_monitor_valid, sizeof(valid));
    sampling = s_monitor_sampling;
    portEXIT_CRITICAL(&s_monitor_lock);
    if (status[0]) strlcpy(out, status, capacity);
    else if (!index) {
        if (valid[1]) snprintf(out, capacity, "CPU %.0f%%\nRAM %.0f%%", values[1], values[2]);
        else snprintf(out, capacity, "CPU %s\nRAM %.0f%%", sampling ? "sampling" : "unavailable", values[2]);
    } else if (!valid[index]) {
        snprintf(out, capacity, "%s\n%s", labels[index], index == 1 && sampling ? "Sampling" : "Unavailable");
    } else if (index >= 6) {
        snprintf(out, capacity, "%s\n%.0f %s", labels[index],
                 fahrenheit ? values[index] * 9 / 5 + 32 : values[index], fahrenheit ? "F" : "C");
    } else if (index == 3 || index == 5) {
        snprintf(out, capacity, "%s\n%.1f GiB", labels[index], values[index] / (1024.0 * 1024 * 1024));
    } else snprintf(out, capacity, "%s\n%.0f%%", labels[index], values[index]);
}

static void monitor_task(void *arg)
{
    (void)arg;
    char *config_json = heap_caps_malloc(CONFIG_MAX_BYTES + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    char *data = heap_caps_calloc(1, 2049, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!config_json || !data) {
        free(config_json); free(data);
        ESP_LOGE(TAG, "PC monitor memory unavailable");
        portENTER_CRITICAL(&s_monitor_lock);
        strlcpy(s_monitor_text, "Monitor memory error", sizeof(s_monitor_text));
        portEXIT_CRITICAL(&s_monitor_lock);
        vTaskDelete(NULL);
        return;
    }
    while (true) {
        goopad_get_config(config_json, CONFIG_MAX_BYTES + 1);
        cJSON *root = cJSON_Parse(config_json);
        cJSON *macros = root ? cJSON_GetObjectItemCaseSensitive(root, "macros") : NULL;
        cJSON *target = NULL;
        for (cJSON *item = macros ? macros->child : NULL; item; item = item->next) {
            cJSON *mode = cJSON_GetObjectItemCaseSensitive(item, "actionMode");
            cJSON *widget = cJSON_GetObjectItemCaseSensitive(item, "widget");
            if (cJSON_IsString(mode) && !strcmp(mode->valuestring, "widget") &&
                cJSON_IsString(widget) && !strcmp(widget->valuestring, "pc_monitor")) {
                target = item; break;
            }
        }
        char text[64] = "Set PC pairing";
        double values[GOOPAD_PC_STAT_COUNT] = {0};
        bool valid[GOOPAD_PC_STAT_COUNT] = {false}, sampling = false;
        if (!root) strlcpy(text, "Monitor config error", sizeof(text));
        if (target) {
            cJSON *url = cJSON_GetObjectItemCaseSensitive(target, "url");
            cJSON *secret = cJSON_GetObjectItemCaseSensitive(target, "secret");
            strlcpy(text, "PC unavailable", sizeof(text));
            if (s_connected && cJSON_IsString(url) && cJSON_IsString(secret)) {
                memset(data, 0, 2049);
                companion_reply_t reply = {.data = data};
                esp_http_client_config_t config = {
                    .url = url->valuestring, .timeout_ms = 3000,
                    .crt_bundle_attach = esp_crt_bundle_attach, .disable_auto_redirect = true,
                    .event_handler = companion_response, .user_data = &reply,
                };
                esp_http_client_handle_t client = esp_http_client_init(&config);
                if (client) {
                    esp_http_client_set_header(client, "X-GooPad-Token", secret->valuestring);
                    esp_err_t err = esp_http_client_perform(client);
                    int status = esp_http_client_get_status_code(client);
                    esp_http_client_cleanup(client);
                    cJSON *snapshot = err == ESP_OK && status == 200 && !reply.overflow ? cJSON_Parse(data) : NULL;
                    cJSON *memory = snapshot ? cJSON_GetObjectItemCaseSensitive(snapshot, "memoryPercent") : NULL;
                    if (cJSON_IsNumber(memory) && memory->valuedouble >= 0 && memory->valuedouble <= 100) {
                        static const char *fields[GOOPAD_PC_STAT_COUNT] = {
                            "", "cpuPercent", "memoryPercent", "memoryUsedBytes", "gpuPercent",
                            "gpuMemoryUsedBytes", "cpuTemperatureC", "gpuTemperatureC"
                        };
                        text[0] = '\0';
                        for (unsigned i = 1; i < GOOPAD_PC_STAT_COUNT; i++) {
                            cJSON *value = cJSON_GetObjectItemCaseSensitive(snapshot, fields[i]);
                            double lower = i >= 6 ? -50 : 0;
                            double upper = i >= 6 ? 200 : i == 3 || i == 5 ? 1e15 : 100;
                            valid[i] = cJSON_IsNumber(value) && value->valuedouble >= lower && value->valuedouble <= upper;
                            if (valid[i]) values[i] = value->valuedouble;
                            if (value && !cJSON_IsNull(value) && !valid[i])
                                strlcpy(text, "Invalid PC data", sizeof(text));
                            if (i == 1) sampling = cJSON_IsNull(value);
                        }
                    }
                    else if (snapshot) strlcpy(text, "Invalid PC data", sizeof(text));
                    cJSON_Delete(snapshot);
                }
            }
        }
        char previous[64];
        portENTER_CRITICAL(&s_monitor_lock);
        strlcpy(previous, s_monitor_text, sizeof(previous));
        portEXIT_CRITICAL(&s_monitor_lock);
        if (strcmp(previous, text) && (strstr(text, "error") || strstr(text, "unavailable") || strstr(text, "Invalid")))
            ESP_LOGW(TAG, "%s", text);
        portENTER_CRITICAL(&s_monitor_lock);
        strlcpy(s_monitor_text, text, sizeof(s_monitor_text));
        memcpy(s_monitor_values, values, sizeof(values));
        memcpy(s_monitor_valid, valid, sizeof(valid));
        s_monitor_sampling = sampling;
        portEXIT_CRITICAL(&s_monitor_lock);
        cJSON_Delete(root);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

static esp_err_t time_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    uint8_t body[128];
    if (req->content_len <= 0 || req->content_len >= sizeof(body) ||
        !receive_bytes(req, body, req->content_len))
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid time request");
    body[req->content_len] = '\0';
    cJSON *root = cJSON_Parse((char *)body);
    cJSON *epoch = root ? cJSON_GetObjectItemCaseSensitive(root, "epoch") : NULL;
    bool valid = cJSON_IsNumber(epoch) && epoch->valuedouble >= 1577836800 &&
                 epoch->valuedouble <= 4102444800;
    struct timeval now = {.tv_sec = valid ? (time_t)epoch->valuedouble : 0};
    cJSON_Delete(root);
    if (!valid) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid epoch");
    if (settimeofday(&now, NULL) != 0) {
        ESP_LOGE(TAG, "Could not set device time");
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not set time");
    }
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static esp_err_t icon_delete_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    char query[32], key[20];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, "key", key, sizeof(key)) != ESP_OK || !valid_icon_key(key))
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid icon key");
    xSemaphoreTake(s_config_lock, portMAX_DELAY);
    cJSON *root = cJSON_Parse(s_config);
    cJSON *macros = root ? cJSON_GetObjectItemCaseSensitive(root, "macros") : NULL;
    bool used = false;
    for (cJSON *item = macros ? macros->child : NULL; item; item = item->next) {
        cJSON *icon = cJSON_GetObjectItemCaseSensitive(item, "iconImage");
        if (cJSON_IsString(icon) && !strcmp(icon->valuestring, key)) used = true;
    }
    bool parsed = root != NULL;
    cJSON_Delete(root);
    xSemaphoreGive(s_config_lock);
    if (!parsed) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not check asset references");
    if (used) {
        httpd_resp_set_status(req, "409 Conflict");
        return httpd_resp_sendstr(req, "Icon is still used by the layout");
    }
    const char *suffixes[] = {"rgb565", "rgb565a8", "160.rgb565a8", "gif"};
    bool deleted = false;
    for (unsigned i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); i++) {
        char path[64];
        snprintf(path, sizeof(path), BSP_SD_MOUNT_POINT "/goopad/icon-%s.%s", key, suffixes[i]);
        if (remove(path) == 0) deleted = true;
        else if (errno != ENOENT) {
            ESP_LOGE(TAG, "Could not delete unused icon: errno=%d", errno);
            return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not delete icon");
        }
    }
    if (!deleted) return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Icon not found");
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static esp_err_t start_server(void)
{
    if (s_server) return ESP_OK;
    httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
    config.httpd.max_uri_handlers = 21;
    config.httpd.max_open_sockets = 6;
    config.httpd.core_id = 1;
    esp_err_t err = goopad_tls_configure(&config, s_ip_address);
    if (err == ESP_OK) err = httpd_ssl_start(&s_server, &config);
    if (err != ESP_OK) return err;
    const httpd_uri_t routes[] = {
        {.uri = "/", .method = HTTP_GET, .handler = index_handler},
        {.uri = "/api/config", .method = HTTP_GET, .handler = config_get_handler},
        {.uri = "/api/config", .method = HTTP_POST, .handler = config_post_handler},
        {.uri = "/api/password", .method = HTTP_POST, .handler = password_post_handler},
        {.uri = "/api/trigger", .method = HTTP_POST, .handler = trigger_handler},
        {.uri = "/api/wifi/join", .method = HTTP_POST, .handler = wifi_join_handler},
        {.uri = "/api/backup", .method = HTTP_POST, .handler = backup_handler},
        {.uri = "/api/backup", .method = HTTP_GET, .handler = backup_handler},
        {.uri = "/api/status", .method = HTTP_GET, .handler = status_handler},
        {.uri = "/api/ota", .method = HTTP_POST, .handler = ota_upload_handler},
        {.uri = "/api/ota/sd", .method = HTTP_POST, .handler = ota_sd_handler},
        {.uri = "/api/icons", .method = HTTP_GET, .handler = icons_handler},
        {.uri = "/api/icon", .method = HTTP_POST, .handler = icon_upload_handler},
        {.uri = "/api/icon", .method = HTTP_GET, .handler = icon_get_handler},
        {.uri = "/api/icon", .method = HTTP_DELETE, .handler = icon_delete_handler},
        {.uri = "/api/time", .method = HTTP_POST, .handler = time_handler},
        {.uri = "/api/sound", .method = HTTP_POST, .handler = sound_upload_handler},
        {.uri = "/api/sound", .method = HTTP_DELETE, .handler = sound_delete_handler},
        {.uri = "/api/controls", .method = HTTP_POST, .handler = controls_handler},
        {.uri = "/api/background", .method = HTTP_DELETE, .handler = background_delete_handler},
        {.uri = "/api/companion", .method = HTTP_POST, .handler = companion_status_handler},
    };
    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        err = httpd_register_uri_handler(s_server, &routes[i]);
        if (err != ESP_OK) {
            httpd_ssl_stop(s_server);
            s_server = NULL;
            return err;
        }
    }
    return ESP_OK;
}

static void server_task(void *arg)
{
    (void)arg;
    esp_err_t err = start_server();
    if (err != ESP_OK) ESP_LOGE(TAG, "HTTPS server start failed: %s", esp_err_to_name(err));
    s_server_starting = false;
    vTaskDelete(NULL);
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
        esp_err_t time_err = esp_netif_sntp_start();
        if (time_err != ESP_OK) ESP_LOGW(TAG, "Network time start: %s", esp_err_to_name(time_err));
        s_wifi_connecting = false;
        s_wifi_connect_attempts = 0;
        snprintf(s_ip_address, sizeof(s_ip_address), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "GooPad web editor: https://" IPSTR "/", IP2STR(&event->ip_info.ip));
        if (!s_server && !s_server_starting) {
            s_server_starting = true;
            if (xTaskCreatePinnedToCore(server_task, "https_start", 12288,
                    NULL, 4, NULL, 1) != pdPASS) {
                s_server_starting = false;
                ESP_LOGE(TAG, "could not start HTTPS task");
            }
        }
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
    esp_sntp_config_t time_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    time_config.start = false;
    ESP_ERROR_CHECK(esp_netif_sntp_init(&time_config));
    s_config = heap_caps_malloc(CONFIG_MAX_BYTES + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    ESP_ERROR_CHECK(s_config ? ESP_OK : ESP_ERR_NO_MEM);
    s_config_lock = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(s_config_lock ? ESP_OK : ESP_ERR_NO_MEM);
    load_config();
    if (xTaskCreatePinnedToCore(monitor_task, "pc_monitor", 6144, NULL, 3, NULL, 1) != pdPASS)
        ESP_LOGE(TAG, "Could not start PC monitor worker");
    ensure_web_key();
    nvs_handle_t controls;
    if (nvs_open(NVS_NS, NVS_READONLY, &controls) == ESP_OK) {
        uint8_t value;
        if (nvs_get_u8(controls, "brightness", &value) == ESP_OK) board_backlight_set(value);
        if (nvs_get_u8(controls, "volume", &value) == ESP_OK) hw_audio_set_volume(value);
        nvs_close(controls);
    }
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
