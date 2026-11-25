#include "app.h"
#include "board.h"

#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "esp_https_server.h"
#include "goopad_tls.h"
#include "goopad_update.h"
#include "goopad_diagnostics.h"
#include "goopad_fonts.h"
#include "goopad_unicode.h"
#include "goopad_web.h"
#include "goopad_styled.h"
#include "goopad_resource_manifest.h"
#include "goopad_layout.h"
#include "goopad_market.h"
#include "goopad_network.h"
#include "goopad_alerts.h"
#include "goopad_config_storage.h"
#include "goopad_companion.h"
#ifdef CONFIG_GOOPAD_HOMEKIT
#include "goopad_homekit.h"
#include "goopad_homekit_credentials.h"
#include "goopad_homekit_storage.h"
#include "sodium/utils.h"
#endif
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
#include "mbedtls/x509_crt.h"
#include "psa/crypto.h"
#include "miniz.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/sockets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static const char *TAG = "goopad";
static const char *NVS_NS = "goopad";
#define CONFIG_MAX_BYTES 12000
#define MAX_MACROS 32
#define ICON_BYTES (32 * 32 * 2)
#define ALPHA_ICON_BYTES (32 * 32 * 3)
#define LARGE_ICON_BYTES (160 * 160 * 3)
#define BACKGROUND_BYTES (480 * 480 * 2)
#define MACRO_BODY_BYTES 3072
#define COMPANION_CERT_BYTES 2048
#define COMPANION_TLS_NAME "companion.goopad"
#define SOUND_MAX_BYTES (44 + BSP_I2S_SAMPLE_RATE * 2 * 30)
#define GIF_MAX_BYTES (128 * 1024)
#define MEDIA_TEXT_BYTES 768
#define RESOURCE_DIR BSP_SD_MOUNT_POINT "/goopad/resources"

static const char s_default_config[] =
    "{\"cols\":3,\"rows\":3,\"theme\":\"midnight\",\"background\":\"111827\","
    "\"shape\":\"rounded\",\"macros\":["
    "{\"name\":\"Welcome\",\"icon\":\"\",\"color\":\"303A48\",\"outline\":false,"
    "\"action\":\"webhook\",\"method\":\"POST\",\"url\":\"\",\"secret\":\"\","
    "\"body\":\"\"}]}";

extern const uint8_t recovery_html_start[] asm("_binary_recovery_html_start");
extern const uint8_t recovery_html_end[] asm("_binary_recovery_html_end");
extern const uint8_t fa_index_start[] asm("_binary_fa_index_json_gz_start");
extern const uint8_t fa_index_end[] asm("_binary_fa_index_json_gz_end");
extern const uint8_t font_notice_start[] asm("_binary_NOTICE_txt_start");
extern const uint8_t font_notice_end[] asm("_binary_NOTICE_txt_end");

static char *s_config;
#ifdef GOOPAD_CAPACITY_TEST
static char *s_capacity_config;
static int64_t s_capacity_until;
#endif
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
static SemaphoreHandle_t s_network_lock;
static QueueHandle_t s_action_queue;
static portMUX_TYPE s_action_lock = portMUX_INITIALIZER_UNLOCKED;
static unsigned s_action_count;
static unsigned s_network_foreground_waiters;
static bool s_media_waiting;
static unsigned s_action_serial;
static int s_last_action_index = -1;
static int s_last_action_status;
static int s_last_action_tls_error;
static bool s_last_action_running;
static bool s_last_action_success;
static char s_last_action_message[160];
static portMUX_TYPE s_monitor_lock = portMUX_INITIALIZER_UNLOCKED;
static char s_monitor_text[64] = "Set PC pairing";
static double s_monitor_values[GOOPAD_PC_STAT_COUNT];
static bool s_monitor_valid[GOOPAD_PC_STAT_COUNT];
static bool s_monitor_sampling;
static portMUX_TYPE s_live_lock = portMUX_INITIALIZER_UNLOCKED;
static char s_live_text[32][64];
static char *s_media_text;
static uint32_t s_media_mask;
static uint32_t s_media_revision, s_media_poll_ms, s_media_age_ms;
static int s_weather_code[32];

enum { NETWORK_MACRO = 1, NETWORK_DIAGNOSTIC, NETWORK_WIDGET, NETWORK_PC };

static esp_err_t network_begin(TickType_t wait, unsigned source)
{
    if (!s_network_lock) return ESP_ERR_INVALID_STATE;
    if (xSemaphoreTake(s_network_lock, wait) != pdTRUE) {
        goopad_diag_record(DIAG_NETWORK_DEFERRED, ESP_ERR_TIMEOUT, source);
        return ESP_ERR_TIMEOUT;
    }
    if (heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) < 24 * 1024 ||
        heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) < 4096) {
        xSemaphoreGive(s_network_lock);
        ESP_LOGW(TAG, "Network request deferred: low internal memory");
        goopad_diag_record(DIAG_NETWORK_DEFERRED, ESP_ERR_NO_MEM, source);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

static void network_end(void)
{
    xSemaphoreGive(s_network_lock);
}

static bool network_priority_pending(void)
{
    portENTER_CRITICAL(&s_action_lock);
    bool pending = s_action_count || s_network_foreground_waiters;
    portEXIT_CRITICAL(&s_action_lock);
    return pending;
}

static bool widget_priority_pending(void)
{
    portENTER_CRITICAL(&s_action_lock);
    bool pending = s_action_count || s_network_foreground_waiters || s_media_waiting;
    portEXIT_CRITICAL(&s_action_lock);
    return pending;
}

static esp_err_t network_begin_foreground(TickType_t wait)
{
    portENTER_CRITICAL(&s_action_lock);
    s_network_foreground_waiters++;
    portEXIT_CRITICAL(&s_action_lock);
    esp_err_t result = network_begin(wait, NETWORK_DIAGNOSTIC);
    portENTER_CRITICAL(&s_action_lock);
    s_network_foreground_waiters--;
    portEXIT_CRITICAL(&s_action_lock);
    return result;
}

static esp_err_t network_begin_background(unsigned source)
{
    if ((source == NETWORK_WIDGET ? widget_priority_pending() : network_priority_pending()))
        return ESP_ERR_INVALID_STATE;
    /* Queue an eligible widget behind the active request instead of racing
     * a continuously polling monitor on every one-second retry. */
    esp_err_t result = network_begin(source == NETWORK_WIDGET ? pdMS_TO_TICKS(10000) : 0, source);
    if (result == ESP_OK && network_priority_pending()) {
        network_end();
        return ESP_ERR_INVALID_STATE;
    }
    return result;
}

#ifdef CONFIG_GOOPAD_HOMEKIT
static volatile bool s_homekit_active;
static volatile bool s_homekit_starting;
static volatile esp_err_t s_homekit_error = ESP_ERR_INVALID_STATE;
static void restart_task(void *arg);
#endif
static const char *s_pc_stat_keys[GOOPAD_PC_STAT_COUNT] = {
    "summary", "cpu", "ram", "ram_used", "gpu", "gpu_memory", "cpu_temp", "gpu_temp",
    "net_down", "net_up", "fan"
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
    static const char *keys[] = {"minimal", "neon", "terminal", "paper", "ocean", "sunset", "retro",
                                 "midnight_glass", "cloud", "lavender", "mint", "coral", "graphite",
                                 "aurora", "ember", "sand", "rose", "indigo", "arctic",
                                 "lime", "plum", "copper", "mono", "berry", "sky"};
    static const goopad_widget_style_t styles[] = {
        {0x0F172A, 0x0F172A, 0xF8FAFC, 0x94A3B8},
        {0x06121C, 0x102A43, 0xCFFAFE, 0x22D3EE},
        {0x081A14, 0x081A14, 0x86EFAC, 0x22C55E},
        {0xF8FAFC, 0xE2E8F0, 0x172033, 0xEF4444},
        {0x082F49, 0x0E7490, 0xE0F2FE, 0x38BDF8},
        {0x7C2D12, 0x9F1239, 0xFFF7ED, 0xFDBA74},
        {0x292524, 0x44403C, 0xFEF3C7, 0xF59E0B},
        {0x273650, 0x101A2D, 0xF7FAFF, 0x8AB4FF},
        {0xFFFFFF, 0xE7EDF5, 0x182439, 0x4779B9},
        {0x51477B, 0x30284E, 0xF8F3FF, 0xD5BFFF},
        {0xCFF2E6, 0x8ECFBD, 0x153D37, 0x287D70},
        {0xFFA595, 0xDF6B74, 0x40232D, 0x8B3347},
        {0x3A414D, 0x1B202A, 0xF5F7FA, 0xA7C6EB},
        {0x103B49, 0x173059, 0xECFEFF, 0x5EEAD4},
        {0x4A2022, 0x1F1720, 0xFFF1E8, 0xFB923C},
        {0xF6E8D0, 0xDFC59D, 0x352819, 0x986331},
        {0xFBE5EA, 0xEAB9C9, 0x4A2432, 0xA33E68},
        {0x303B83, 0x181D49, 0xF5F3FF, 0xB8B8FF},
        {0xE5F6FA, 0xA9D6E4, 0x14384A, 0x26718C},
        {0x253C20, 0x112417, 0xF2FCE8, 0xA3E635},
        {0x4C2850, 0x21172E, 0xFFF1FF, 0xE9A8F0},
        {0x5B3426, 0x291C19, 0xFFF2E5, 0xF4B183},
        {0x424242, 0x151515, 0xFAFAFA, 0xD4D4D4},
        {0x621E45, 0x30192F, 0xFFF1F5, 0xFB8BB4},
        {0xDDEEFF, 0xB5D2F2, 0x1A3657, 0x3569AB},
    };
    for (unsigned i = 0; key && i < sizeof(keys) / sizeof(keys[0]); i++)
        if (!strcmp(key, keys[i])) return &styles[i];
    return NULL;
}

bool goopad_widget_graphic_valid(const char *graphic, const char *widget, const char *stat)
{
    if (!graphic || !strcmp(graphic, "text")) return true;
    if (!strcmp(graphic, "custom_icon")) return true;
    if (!strcmp(graphic, "icon")) return !widget || !strcmp(widget, "clock") ||
        !strcmp(widget, "date") || !strcmp(widget, "pc_monitor");
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
    /* GPR1 stores runs of little-endian RGB565 plus alpha; the decoded
     * image has a color plane followed by a separate alpha plane. */
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
    /* Validate the entire stream and ensure every frame has a >=100 ms delay
     * before handing the data to LVGL. Missing timing blocks may grow it. */
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

static bool matching_files(const char *left, const char *right)
{
    FILE *a = fopen(left, "rb"), *b = fopen(right, "rb");
    bool same = a && b;
    char first[256], second[256];
    while (same) {
        size_t na = fread(first, 1, sizeof(first), a);
        size_t nb = fread(second, 1, sizeof(second), b);
        if (na != nb || memcmp(first, second, na) || ferror(a) || ferror(b)) same = false;
        if (na < sizeof(first) || nb < sizeof(second)) break;
    }
    if (a) fclose(a);
    if (b) fclose(b);
    return same;
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
    bool saved = ok && rename(temporary, path) == 0;
    if (ok && !saved && matching_files(temporary, path)) saved = remove(temporary) == 0;
    if (!saved) {
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

static esp_err_t saved_icons_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    if (!g_app.sd.mounted) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Insert and mount an SD card");
    char query[32], value[16], *end;
    unsigned long offset = 0;
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "offset", value, sizeof(value)) != ESP_OK ||
            value[0] < '0' || value[0] > '9') {
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid offset");
        }
        offset = strtoul(value, &end, 10);
        if (*end || offset > 100000) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid offset");
    }
    DIR *dir = opendir(BSP_SD_MOUNT_POINT "/goopad");
    if (!dir && errno != ENOENT) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not read SD assets");
    char response[1800];
    size_t pos = snprintf(response, sizeof(response), "{\"assets\":[");
    unsigned long seen = 0;
    unsigned count = 0;
    bool more = false;
    if (dir) {
        struct dirent *entry;
        errno = 0;
        while ((entry = readdir(dir))) {
            const char *name = entry->d_name;
            if (strlen(name) < 23 || strncmp(name, "icon-", 5)) continue;
            char key[18];
            memcpy(key, name + 5, 17); key[17] = '\0';
            if (!valid_icon_key(key)) continue;
            const char *suffix = name + 22;
            const char *format;
            int size;
            if (!strcmp(suffix, ".gif")) { format = "gif"; size = 160; }
            else if (!strcmp(suffix, ".160.rgb565a8")) { format = "rgb565a8"; size = 160; }
            else if (!strcmp(suffix, ".rgb565a8")) { format = "rgb565a8"; size = 32; }
            else if (!strcmp(suffix, ".rgb565")) { format = "rgb565"; size = 32; }
            else continue;
            if (seen++ < offset) continue;
            if (count == 24) { more = true; break; }
            int n = snprintf(response + pos, sizeof(response) - pos,
                "%s{\"key\":\"%s\",\"format\":\"%s\",\"size\":%d}", count ? "," : "", key, format, size);
            if (n < 0 || (size_t)n >= sizeof(response) - pos) {
                closedir(dir);
                return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Asset list too large");
            }
            pos += n;
            count++;
        }
        int read_error = errno;
        closedir(dir);
        if (!more && read_error) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not read SD assets");
    }
    if (more) snprintf(response + pos, sizeof(response) - pos, "],\"next\":%lu}", offset + count);
    else snprintf(response + pos, sizeof(response) - pos, "],\"next\":null}");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, response);
}

static esp_err_t icon_match_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    if (!g_app.sd.mounted) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Insert and mount an SD card");
    char query[192], format[16], size[8], hash[65], offset_text[16], *end;
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, "format", format, sizeof(format)) != ESP_OK ||
        httpd_query_key_value(query, "size", size, sizeof(size)) != ESP_OK ||
        httpd_query_key_value(query, "hash", hash, sizeof(hash)) != ESP_OK ||
        httpd_query_key_value(query, "offset", offset_text, sizeof(offset_text)) != ESP_OK ||
        strlen(hash) != 64 || offset_text[0] < '0' || offset_text[0] > '9')
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid asset search");
    unsigned long offset = strtoul(offset_text, &end, 10);
    if (*end || offset > 100000) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid asset offset");
    uint8_t expected[32];
    for (size_t i = 0; i < sizeof(expected); i++) {
        char a = hash[i * 2], b = hash[i * 2 + 1];
        if (!((a >= '0' && a <= '9') || (a >= 'a' && a <= 'f')) ||
            !((b >= '0' && b <= '9') || (b >= 'a' && b <= 'f')))
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid asset hash");
        expected[i] = (uint8_t)((a <= '9' ? a - '0' : a - 'a' + 10) * 16 +
                                (b <= '9' ? b - '0' : b - 'a' + 10));
    }
    const char *suffix = NULL;
    if (!strcmp(format, "gif") && !strcmp(size, "160")) suffix = ".gif";
    else if (!strcmp(format, "rgb565a8") && !strcmp(size, "160")) suffix = ".160.rgb565a8";
    else if (!strcmp(format, "rgb565") && !strcmp(size, "480")) suffix = ".480.rgb565";
    if (!suffix) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Unsupported asset type");
    const char *directory = BSP_SD_MOUNT_POINT "/goopad";
    DIR *dir = opendir(directory);
    if (!dir && errno != ENOENT) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not read SD assets");
    uint8_t *buffer = heap_caps_malloc(4096, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buffer) {
        if (dir) closedir(dir);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Asset search memory unavailable");
    }
    bool failed = psa_crypto_init() != PSA_SUCCESS, more = false;
    unsigned long seen = 0;
    unsigned checked = 0;
    char match[18] = "";
    while (dir && !failed) {
        errno = 0;
        struct dirent *entry = readdir(dir);
        if (!entry) { if (errno) failed = true; break; }
        const char *name = entry->d_name;
        if (strlen(name) != 22 + strlen(suffix) || strncmp(name, "icon-", 5) ||
            strcmp(name + 22, suffix)) continue;
        char key[18];
        memcpy(key, name + 5, 17); key[17] = '\0';
        if (!valid_icon_key(key)) continue;
        if (seen++ < offset) continue;
        if (checked == 16) { more = true; break; }
        checked++;
        char path[80];
        snprintf(path, sizeof(path), "%s/icon-%s%s", directory, key, suffix);
        FILE *file = fopen(path, "rb");
        if (!file) { failed = true; break; }
        psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;
        psa_status_t status = psa_hash_setup(&operation, PSA_ALG_SHA_256);
        size_t read = 0;
        while (status == PSA_SUCCESS && (read = fread(buffer, 1, 4096, file)) > 0)
            status = psa_hash_update(&operation, buffer, read);
        if (ferror(file)) failed = true;
        if (fclose(file) != 0) failed = true;
        uint8_t digest[32];
        size_t digest_size = 0;
        if (!failed && status == PSA_SUCCESS)
            status = psa_hash_finish(&operation, digest, sizeof(digest), &digest_size);
        psa_hash_abort(&operation);
        if (failed || status != PSA_SUCCESS || digest_size != sizeof(digest)) { failed = true; break; }
        if (!memcmp(digest, expected, sizeof(digest))) {
            memcpy(match, key, sizeof(match));
            break;
        }
    }
    if (dir && closedir(dir) != 0) failed = true;
    free(buffer);
    if (failed) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not search SD assets");
    char response[96];
    if (match[0]) snprintf(response, sizeof(response), "{\"key\":\"%s\",\"next\":null}", match);
    else if (more) snprintf(response, sizeof(response), "{\"key\":null,\"next\":%lu}", offset + checked);
    else snprintf(response, sizeof(response), "{\"key\":null,\"next\":null}");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, response);
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
            if (err == ESP_OK && s_config[length - 1] != '\0') err = ESP_ERR_INVALID_SIZE;
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
        cJSON *defaults = cJSON_Parse(s_config);
        int cols = goopad_grid_limit(BSP_LCD_H_RES, 0, BSP_TILE_MIN_PX);
        int rows = goopad_grid_limit(BSP_LCD_V_RES, 84, BSP_TILE_MIN_PX);
        if (!defaults || cols < 2 || rows < 2) {
            ESP_LOGE(TAG, "Board viewport cannot fit the minimum 2x2 layout");
            ESP_ERROR_CHECK(ESP_ERR_NOT_SUPPORTED);
        }
        cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(defaults, "cols"), cols < 3 ? cols : 3);
        cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(defaults, "rows"), rows < 3 ? rows : 3);
        char *json = cJSON_PrintUnformatted(defaults);
        ESP_ERROR_CHECK(json ? ESP_OK : ESP_ERR_NO_MEM);
        strlcpy(s_config, json, CONFIG_MAX_BYTES + 1);
        cJSON_free(json);
        cJSON_Delete(defaults);
    }
    ESP_ERROR_CHECK(goopad_config_storage_load(s_config, CONFIG_MAX_BYTES + 1, s_config, valid));
    saved = cJSON_Parse(s_config);
    valid = cJSON_IsObject(saved);
    cJSON_Delete(saved);
    if (!valid) {
        ESP_LOGE(TAG, "Stored configuration is invalid; preserving it");
        ESP_ERROR_CHECK(ESP_ERR_INVALID_STATE);
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
#ifdef GOOPAD_CAPACITY_TEST
        if (s_capacity_config && esp_timer_get_time() >= s_capacity_until) {
            free(s_capacity_config);
            s_capacity_config = NULL;
        }
        strlcpy(out, s_capacity_config ? s_capacity_config : s_config, capacity);
#else
        strlcpy(out, s_config, capacity);
#endif
        xSemaphoreGive(s_config_lock);
    }
}

#ifdef GOOPAD_CAPACITY_TEST
bool goopad_capacity_active(void)
{
    xSemaphoreTake(s_config_lock, portMAX_DELAY);
    bool active = s_capacity_config != NULL;
    xSemaphoreGive(s_config_lock);
    return active;
}

esp_err_t goopad_capacity_set(const char *kind, unsigned count)
{
    if (!strcmp(kind, "stop")) {
        xSemaphoreTake(s_config_lock, portMAX_DELAY);
        free(s_capacity_config);
        s_capacity_config = NULL;
        xSemaphoreGive(s_config_lock);
        return ESP_OK;
    }
    if (!count || count > MAX_MACROS ||
        (strcmp(kind, "clock") && strcmp(kind, "image") && strcmp(kind, "gif") &&
         strcmp(kind, "remote") && strcmp(kind, "pc")) ||
        (!strcmp(kind, "gif") && count > 4)) return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(s_config_lock, portMAX_DELAY);
    cJSON *saved = cJSON_Parse(s_config);
    xSemaphoreGive(s_config_lock);
    if (!saved) return ESP_ERR_NO_MEM;
    cJSON *root = cJSON_CreateObject();
    cJSON *macros = root ? cJSON_AddArrayToObject(root, "macros") : NULL;
    bool ok = root && macros && cJSON_AddNumberToObject(root, "cols", 4) &&
              cJSON_AddNumberToObject(root, "rows", 4) &&
              cJSON_AddStringToObject(root, "theme", "midnight");
    cJSON *source = NULL;
    cJSON *saved_macros = cJSON_GetObjectItemCaseSensitive(saved, "macros");
    for (cJSON *item = cJSON_IsArray(saved_macros) ? saved_macros->child : NULL; item; item = item->next) {
        cJSON *format = cJSON_GetObjectItemCaseSensitive(item, "iconFormat");
        cJSON *image = cJSON_GetObjectItemCaseSensitive(item, "iconImage");
        cJSON *size = cJSON_GetObjectItemCaseSensitive(item, "iconSize");
        cJSON *mode = cJSON_GetObjectItemCaseSensitive(item, "actionMode");
        cJSON *widget = cJSON_GetObjectItemCaseSensitive(item, "widget");
        bool gif = cJSON_IsString(format) && !strcmp(format->valuestring, "gif");
        if ((!strcmp(kind, "gif") && gif && cJSON_IsString(image)) ||
            (!strcmp(kind, "image") && cJSON_IsString(image) && cJSON_IsString(format) &&
             !strcmp(format->valuestring, "rgb565a8") && cJSON_IsNumber(size) && size->valueint == 160) ||
            (!strcmp(kind, "pc") && cJSON_IsString(mode) && !strcmp(mode->valuestring, "widget") &&
             cJSON_IsString(widget) && !strcmp(widget->valuestring, "pc_monitor"))) {
            source = item;
            break;
        }
    }
    if ((!strcmp(kind, "image") || !strcmp(kind, "gif") || !strcmp(kind, "pc")) && !source) ok = false;
    if (ok && !strcmp(kind, "pc")) {
        cJSON *certificate = cJSON_GetObjectItemCaseSensitive(saved, "companionCertificate");
        ok = certificate && cJSON_AddItemToObject(root, "companionCertificate", cJSON_Duplicate(certificate, true));
        if (ok && cJSON_GetObjectItemCaseSensitive(source, "sharedPc")) {
            const char *fields[] = {"companionAddress", "companionToken"};
            for (size_t i = 0; ok && i < sizeof(fields) / sizeof(fields[0]); i++) {
                cJSON *value = cJSON_GetObjectItemCaseSensitive(saved, fields[i]);
                ok = value && cJSON_AddItemToObject(root, fields[i], cJSON_Duplicate(value, true));
            }
        }
    }
    for (unsigned i = 0; ok && i < count; i++) {
        cJSON *item = !strcmp(kind, "pc") ? cJSON_Duplicate(source, true) : cJSON_CreateObject();
        if (!item || !cJSON_AddItemToArray(macros, item)) {
            cJSON_Delete(item);
            ok = false;
            break;
        }
        if (!strcmp(kind, "image") || !strcmp(kind, "gif")) {
            cJSON *image = cJSON_GetObjectItemCaseSensitive(source, "iconImage");
            cJSON *format = cJSON_GetObjectItemCaseSensitive(source, "iconFormat");
            cJSON *size = cJSON_GetObjectItemCaseSensitive(source, "iconSize");
            ok = cJSON_AddStringToObject(item, "name", "Image") &&
                 cJSON_AddStringToObject(item, "icon", "") &&
                 cJSON_AddStringToObject(item, "actionMode", "simple_link") &&
                 cJSON_AddStringToObject(item, "iconImage", image->valuestring) &&
                 cJSON_AddStringToObject(item, "iconFormat", format->valuestring) &&
                 (!size || cJSON_AddNumberToObject(item, "iconSize", size->valueint));
        } else if (strcmp(kind, "pc")) {
            ok = cJSON_AddStringToObject(item, "name", "Test") &&
                 cJSON_AddStringToObject(item, "icon", "") &&
                 cJSON_AddStringToObject(item, "actionMode", "widget") &&
                 cJSON_AddStringToObject(item, "widget", !strcmp(kind, "clock") ? "clock" : "weather");
            if (!strcmp(kind, "clock"))
                ok = ok && cJSON_AddStringToObject(item, "clockFormat", "24h_seconds") &&
                     cJSON_AddStringToObject(item, "widgetGraphic", "analog");
            else
                ok = ok && cJSON_AddNumberToObject(item, "weatherLatitude", 40) &&
                     cJSON_AddNumberToObject(item, "weatherLongitude", -74);
        }
    }
    char *text = ok ? cJSON_PrintUnformatted(root) : NULL;
    cJSON_Delete(root);
    cJSON_Delete(saved);
    if (!text) return !source && (!strcmp(kind, "image") || !strcmp(kind, "gif") || !strcmp(kind, "pc"))
        ? ESP_ERR_NOT_FOUND : ESP_ERR_NO_MEM;
    if (strlen(text) > CONFIG_MAX_BYTES) {
        cJSON_free(text);
        return ESP_ERR_INVALID_SIZE;
    }
    xSemaphoreTake(s_config_lock, portMAX_DELAY);
    free(s_capacity_config);
    s_capacity_config = text;
    s_capacity_until = esp_timer_get_time() + 240LL * 1000000;
    xSemaphoreGive(s_config_lock);
    return ESP_OK;
}
#endif

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
    /* Browser requests with an Origin must be same-origin. Non-browser clients
     * may omit it, but still need the device's Basic credentials over HTTPS. */
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

static bool valid_companion_certificate(const cJSON *item)
{
    if (!cJSON_IsString(item)) return false;
    const char *pem = item->valuestring;
    const char *footer = strstr(pem, "-----END CERTIFICATE-----");
    if (strlen(pem) >= COMPANION_CERT_BYTES ||
        strncmp(pem, "-----BEGIN CERTIFICATE-----", 27) || !footer) return false;
    for (const char *p = footer + strlen("-----END CERTIFICATE-----"); *p; p++) {
        if (*p != '\r' && *p != '\n' && *p != ' ' && *p != '\t') return false;
    }
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    bool valid = mbedtls_x509_crt_parse(&cert, (const unsigned char *)pem, strlen(pem) + 1) == 0 &&
                 cert.next == NULL && mbedtls_x509_crt_get_ca_istrue(&cert);
    mbedtls_x509_crt_free(&cert);
    return valid;
}

static bool pc_action_mode(const cJSON *mode)
{
    return cJSON_IsString(mode) &&
           (!strcmp(mode->valuestring, "pc_macro") || !strcmp(mode->valuestring, "pc_open_url"));
}

static bool companion_target(const cJSON *root, const char *url)
{
    const cJSON *address = cJSON_GetObjectItemCaseSensitive(root, "companionAddress");
    if (!cJSON_IsString(address)) return false;
    const char *expected = address->valuestring;
    if (!strncmp(expected, "https://", 8)) expected += 8;
    else if (!strncmp(expected, "http://", 7)) expected += 7;
    const char *actual = url + (!strncmp(url, "https://", 8) ? 8 : 7);
    size_t size = strcspn(actual, "/");
    return size == strcspn(expected, "/") && !strncmp(actual, expected, size);
}

static void client_tls(esp_http_client_config_t *config, const char *certificate)
{
    /* Companion uses its paired CA and fixed DNS identity even at an IP URL;
     * other HTTPS targets use the public CA bundle. Never follow redirects. */
    config->cert_pem = certificate;
    config->common_name = certificate ? COMPANION_TLS_NAME : NULL;
    config->crt_bundle_attach = certificate ? NULL : esp_crt_bundle_attach;
    config->disable_auto_redirect = true;
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

static bool valid_home_token(const cJSON *value)
{
    if (!cJSON_IsString(value)) return false;
    size_t length = strlen(value->valuestring);
    if (!length || length > 512) return false;
    for (const unsigned char *p = (const unsigned char *)value->valuestring; *p; p++)
        if (*p < 0x21 || *p > 0x7e) return false;
    return true;
}

static bool valid_home_state(const cJSON *address, const cJSON *entity, const cJSON *token, const cJSON *url)
{
    if (!cJSON_IsString(address) || !cJSON_IsString(entity) || !valid_home_token(token) ||
        !cJSON_IsString(url) || !valid_url(url->valuestring) ||
        strncmp(address->valuestring, "https://", 8)) return false;
    const char *host = address->valuestring + 8;
    if (!*host || strpbrk(host, "/?#@ \t\r\n")) return false;
    const char *id = entity->valuestring;
    size_t length = strlen(id);
    if (!length || length > 100) return false;
    bool dot = false;
    for (size_t i = 0; i < length; i++) {
        char ch = id[i];
        if (ch == '.' && !dot && i && i + 1 < length) dot = true;
        else if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_')) return false;
    }
    if (!dot) return false;
    char expected[180];
    int written = snprintf(expected, sizeof(expected), "%s/api/states/%s", address->valuestring, id);
    return written > 0 && written < sizeof(expected) && !strcmp(expected, url->valuestring);
}

static bool valid_market_text(const cJSON *value, bool key)
{
    if (!cJSON_IsString(value)) return false;
    size_t length = strlen(value->valuestring);
    if (!length || length > (key ? 64 : 16)) return false;
    for (const char *p = value->valuestring; *p; p++)
        if (!((*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') ||
              (key && *p >= 'a' && *p <= 'z') || (!key && (*p == '.' || *p == '-')))) return false;
    return true;
}

static bool valid_finnhub_key(const cJSON *value)
{
    return valid_home_token(value) && strlen(value->valuestring) <= 64;
}

static bool valid_github_user(const cJSON *value)
{
    if (!cJSON_IsString(value)) return false;
    const char *name = value->valuestring;
    size_t length = strlen(name);
    if (!length || length > 39 || name[0] == '-' || name[length - 1] == '-') return false;
    for (size_t i = 0; i < length; i++) {
        char ch = name[i];
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
              (ch >= '0' && ch <= '9') || (ch == '-' && i > 0 && name[i - 1] != '-'))) return false;
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

#ifdef CONFIG_GOOPAD_HOMEKIT
static bool homekit_id_valid(const char *id)
{
    if (!id || strlen(id) != 15 || id[0] != 's') return false;
    for (size_t i = 1; i < 15; i++)
        if (!((id[i] >= '0' && id[i] <= '9') || (id[i] >= 'a' && id[i] <= 'f'))) return false;
    return true;
}

static bool homekit_switches(cJSON *root, goopad_homekit_switch_t *switches, size_t *count)
{
    cJSON *macros = cJSON_GetObjectItemCaseSensitive(root, "macros");
    if (!cJSON_IsArray(macros)) return false;
    *count = 0;
    for (cJSON *item = macros->child; item; item = item->next) {
        cJSON *mode = cJSON_GetObjectItemCaseSensitive(item, "actionMode");
        if (!cJSON_IsString(mode) || strcmp(mode->valuestring, "homekit")) continue;
        cJSON *id = cJSON_GetObjectItemCaseSensitive(item, "homekitId");
        cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "homekitName");
        if (!cJSON_IsString(id) || !homekit_id_valid(id->valuestring) ||
            !cJSON_IsString(name) || !name->valuestring[0] || strlen(name->valuestring) > 24 ||
            *count >= GOOPAD_HOMEKIT_MAX_SWITCHES) return false;
        for (size_t i = 0; i < *count; i++)
            if (!strcmp(switches[i].id, id->valuestring)) return false;
        strlcpy(switches[*count].id, id->valuestring, sizeof(switches[*count].id));
        strlcpy(switches[*count].name, name->valuestring, sizeof(switches[*count].name));
        (*count)++;
    }
    return true;
}

static bool homekit_topology_changed(cJSON *old_root, cJSON *new_root, bool *changed)
{
    /* Tile position is not identity: reordering must not restart the bridge
     * or change a HomeKit accessory ID. Name and membership changes do. */
    goopad_homekit_switch_t old[GOOPAD_HOMEKIT_MAX_SWITCHES], next[GOOPAD_HOMEKIT_MAX_SWITCHES];
    size_t old_count, next_count;
    if (!homekit_switches(old_root, old, &old_count) ||
        !homekit_switches(new_root, next, &next_count)) return false;
    *changed = old_count != next_count;
    for (size_t i = 0; !*changed && i < old_count; i++) {
        bool found = false;
        for (size_t j = 0; j < next_count; j++) {
            if (!strcmp(old[i].id, next[j].id) && !strcmp(old[i].name, next[j].name)) {
                found = true;
                break;
            }
        }
        if (!found) *changed = true;
    }
    return true;
}
#endif

static bool parse_wake_mac(const char *text, uint8_t mac[6])
{
    if (!text || strlen(text) != 17 || (text[2] != ':' && text[2] != '-')) return false;
    char separator = text[2];
    bool nonzero = false;
    for (size_t i = 0; i < 6; i++) {
        unsigned value = 0;
        for (size_t j = 0; j < 2; j++) {
            char c = text[i * 3 + j];
            unsigned digit;
            if (c >= '0' && c <= '9') digit = (unsigned)(c - '0');
            else if (c >= 'a' && c <= 'f') digit = (unsigned)(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') digit = (unsigned)(c - 'A' + 10);
            else return false;
            value = value * 16 + digit;
        }
        if (i < 5 && text[i * 3 + 2] != separator) return false;
        mac[i] = (uint8_t)value;
        nonzero |= value != 0;
    }
    return nonzero && !(mac[0] & 1);
}

static bool layout_occupied(cJSON *item)
{
    const char *fields[] = {"name", "icon", "url", "iconImage"};
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        cJSON *value = cJSON_GetObjectItemCaseSensitive(item, fields[i]);
        if (cJSON_IsString(value) && value->valuestring[0]) return true;
    }
    cJSON *mode = cJSON_GetObjectItemCaseSensitive(item, "actionMode");
    cJSON *sound = cJSON_GetObjectItemCaseSensitive(item, "sound");
    return (cJSON_IsString(mode) && strcmp(mode->valuestring, "simple_link")) ||
           (cJSON_IsString(sound) && strcmp(sound->valuestring, "none"));
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
    cJSON *hide_empty = cJSON_GetObjectItemCaseSensitive(root, "hideEmptyTiles");
    cJSON *companion_cert = cJSON_GetObjectItemCaseSensitive(root, "companionCertificate");
    cJSON *offset = cJSON_GetObjectItemCaseSensitive(root, "timeOffsetMinutes");
    cJSON *pages = cJSON_GetObjectItemCaseSensitive(root, "pageCount");
    unsigned gif_count = 0;
    bool valid = cJSON_IsNumber(cols) && cols->valueint >= 2 &&
                 cols->valueint <= goopad_grid_limit(BSP_LCD_H_RES, 0, BSP_TILE_MIN_PX) &&
                 cJSON_IsNumber(rows) && rows->valueint >= 2 &&
                 rows->valueint <= goopad_grid_limit(BSP_LCD_V_RES, 84, BSP_TILE_MIN_PX) &&
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
                              !strcmp(theme->valuestring, "slate") ||
                              !strcmp(theme->valuestring, "violet") ||
                              !strcmp(theme->valuestring, "ember") ||
                              !strcmp(theme->valuestring, "berry") ||
                              !strcmp(theme->valuestring, "teal") ||
                              !strcmp(theme->valuestring, "black") ||
                              !strcmp(theme->valuestring, "custom")))) &&
                 (!background || valid_hex_color(background)) &&
                 (!background_image || (cJSON_IsString(background_image) &&
                                        valid_icon_key(background_image->valuestring))) &&
                 (!hide || cJSON_IsBool(hide)) &&
                 (!hide_empty || cJSON_IsBool(hide_empty)) &&
                 (!companion_cert || valid_companion_certificate(companion_cert)) &&
                 (!offset || (cJSON_IsNumber(offset) && offset->valuedouble == offset->valueint &&
                              offset->valueint >= -840 && offset->valueint <= 840));
    for (cJSON *item = valid ? macros->child : NULL; item; item = item->next) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "name");
        cJSON *tile_name = cJSON_GetObjectItemCaseSensitive(item, "tileName");
        cJSON *url = cJSON_GetObjectItemCaseSensitive(item, "url");
        cJSON *method = cJSON_GetObjectItemCaseSensitive(item, "method");
        cJSON *action = cJSON_GetObjectItemCaseSensitive(item, "action");
        cJSON *icon = cJSON_GetObjectItemCaseSensitive(item, "icon");
        cJSON *emoji_render = cJSON_GetObjectItemCaseSensitive(item, "emojiRender");
        cJSON *visual_fit = cJSON_GetObjectItemCaseSensitive(item, "visualFit");
        cJSON *emoji_image_key = cJSON_GetObjectItemCaseSensitive(item, "emojiImageKey");
        cJSON *body = cJSON_GetObjectItemCaseSensitive(item, "body");
        cJSON *secret = cJSON_GetObjectItemCaseSensitive(item, "secret");
        cJSON *shared_pc = cJSON_GetObjectItemCaseSensitive(item, "sharedPc");
        cJSON *icon_image = cJSON_GetObjectItemCaseSensitive(item, "iconImage");
        cJSON *fa_icon = cJSON_GetObjectItemCaseSensitive(item, "faIcon");
        cJSON *action_mode = cJSON_GetObjectItemCaseSensitive(item, "actionMode");
        cJSON *wake_mac = cJSON_GetObjectItemCaseSensitive(item, "wakeMac");
        cJSON *pc_address = cJSON_GetObjectItemCaseSensitive(item, "pcAddress");
        cJSON *pc_target = cJSON_GetObjectItemCaseSensitive(item, "pcTarget");
        cJSON *shape = cJSON_GetObjectItemCaseSensitive(item, "shape");
        cJSON *text_color = cJSON_GetObjectItemCaseSensitive(item, "textColor");
        cJSON *icon_color = cJSON_GetObjectItemCaseSensitive(item, "iconColor");
        cJSON *tile_color = cJSON_GetObjectItemCaseSensitive(item, "color");
        cJSON *tile_gradient = cJSON_GetObjectItemCaseSensitive(item, "tileGradient");
        cJSON *visual_scale = cJSON_GetObjectItemCaseSensitive(item, "visualScale");
        cJSON *page_direction = cJSON_GetObjectItemCaseSensitive(item, "pageDirection");
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
        cJSON *text_size = cJSON_GetObjectItemCaseSensitive(item, "textSize");
        cJSON *font = cJSON_GetObjectItemCaseSensitive(item, "font");
        cJSON *text_align = cJSON_GetObjectItemCaseSensitive(item, "textAlign");
        cJSON *text_decoration = cJSON_GetObjectItemCaseSensitive(item, "textDecoration");
        cJSON *text_spacing = cJSON_GetObjectItemCaseSensitive(item, "textSpacing");
        cJSON *focus_minutes = cJSON_GetObjectItemCaseSensitive(item, "focusMinutes");
        cJSON *break_minutes = cJSON_GetObjectItemCaseSensitive(item, "breakMinutes");
        cJSON *widget_accent = cJSON_GetObjectItemCaseSensitive(item, "widgetAccent");
        cJSON *widget_gradient = cJSON_GetObjectItemCaseSensitive(item, "widgetGradient");
        cJSON *widget_layout = cJSON_GetObjectItemCaseSensitive(item, "widgetLayout");
        cJSON *countdown_at = cJSON_GetObjectItemCaseSensitive(item, "countdownAt");
        cJSON *home_token = cJSON_GetObjectItemCaseSensitive(item, "homeToken");
        cJSON *home_address = cJSON_GetObjectItemCaseSensitive(item, "homeAddress");
        cJSON *home_entity = cJSON_GetObjectItemCaseSensitive(item, "homeEntity");
        cJSON *latitude = cJSON_GetObjectItemCaseSensitive(item, "weatherLatitude");
        cJSON *longitude = cJSON_GetObjectItemCaseSensitive(item, "weatherLongitude");
        cJSON *market_symbol = cJSON_GetObjectItemCaseSensitive(item, "marketSymbol");
        cJSON *stock_key = cJSON_GetObjectItemCaseSensitive(item, "stockKey");
        cJSON *stock_provider = cJSON_GetObjectItemCaseSensitive(item, "stockProvider");
        cJSON *finnhub_key = cJSON_GetObjectItemCaseSensitive(item, "finnhubKey");
        cJSON *github_user = cJSON_GetObjectItemCaseSensitive(item, "githubUser");
        cJSON *currency_base = cJSON_GetObjectItemCaseSensitive(item, "currencyBase");
        cJSON *currency_quote = cJSON_GetObjectItemCaseSensitive(item, "currencyQuote");
        bool gif = cJSON_IsString(image_format) && !strcmp(image_format->valuestring, "gif");
        char shared_url[180];
        const char *effective_url = cJSON_IsString(url) ? url->valuestring : NULL;
        const char *effective_secret = cJSON_IsString(secret) ? secret->valuestring : NULL;
        bool shared_valid = !shared_pc ||
            (goopad_companion_resolve(root, item, shared_url, sizeof(shared_url), &effective_secret) &&
             companion_cert);
        if (shared_pc && shared_valid) effective_url = shared_url;
        if (gif) gif_count++;
        valid = shared_valid && cJSON_IsObject(item) && cJSON_IsString(name) && strlen(name->valuestring) <= 24 &&
                (!tile_name || (cJSON_IsString(tile_name) && strlen(tile_name->valuestring) <= 24)) &&
                (!gif_fit || (cJSON_IsString(gif_fit) &&
                             (!strcmp(gif_fit->valuestring, "contain") || !strcmp(gif_fit->valuestring, "cover")))) &&
                (!visual_fit || (cJSON_IsString(visual_fit) &&
                             (!strcmp(visual_fit->valuestring, "contain") || !strcmp(visual_fit->valuestring, "cover")))) &&
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
                (!widget_gradient || valid_hex_color(widget_gradient)) &&
                (!widget_layout || (cJSON_IsString(widget_layout) &&
                                  (!strcmp(widget_layout->valuestring, "classic") ||
                                   !strcmp(widget_layout->valuestring, "panel") ||
                                   !strcmp(widget_layout->valuestring, "layered")))) &&
                (!countdown_at || (cJSON_IsNumber(countdown_at) && countdown_at->valuedouble >= 1577836800 &&
                                   countdown_at->valuedouble <= 4102444800 &&
                                   countdown_at->valuedouble == (int64_t)countdown_at->valuedouble)) &&
                (!home_token || valid_home_token(home_token)) &&
                (!latitude || (cJSON_IsNumber(latitude) && isfinite(latitude->valuedouble) && latitude->valuedouble >= -90 && latitude->valuedouble <= 90)) &&
                (!longitude || (cJSON_IsNumber(longitude) && isfinite(longitude->valuedouble) && longitude->valuedouble >= -180 && longitude->valuedouble <= 180)) &&
                (!market_symbol || valid_market_text(market_symbol, false)) &&
                (!stock_key || valid_market_text(stock_key, true)) &&
                (!stock_provider || (cJSON_IsString(stock_provider) &&
                    (!strcmp(stock_provider->valuestring, "alphavantage") || !strcmp(stock_provider->valuestring, "finnhub")))) &&
                (!finnhub_key || valid_finnhub_key(finnhub_key)) &&
                (!github_user || valid_github_user(github_user)) &&
                (!currency_base || (cJSON_IsString(currency_base) && goopad_currency_code(currency_base->valuestring))) &&
                (!currency_quote || (cJSON_IsString(currency_quote) && goopad_currency_code(currency_quote->valuestring))) &&
                (!widget_size || (cJSON_IsNumber(widget_size) &&
                                  (widget_size->valuedouble == 0 || widget_size->valuedouble == 12 ||
                                   widget_size->valuedouble == 14 || widget_size->valuedouble == 16 ||
                                   widget_size->valuedouble == 20 || widget_size->valuedouble == 28 ||
                                   widget_size->valuedouble == 40))) &&
                (!text_size || (cJSON_IsNumber(text_size) &&
                                (text_size->valuedouble == 12 || text_size->valuedouble == 14 ||
                                 text_size->valuedouble == 16 || text_size->valuedouble == 20 ||
                                 text_size->valuedouble == 28 || text_size->valuedouble == 40))) &&
                (!font || (cJSON_IsString(font) && goopad_font_index(font->valuestring) >= 0)) &&
                (!text_align || (cJSON_IsString(text_align) &&
                                 (!strcmp(text_align->valuestring, "left") || !strcmp(text_align->valuestring, "center") ||
                                  !strcmp(text_align->valuestring, "right")))) &&
                (!text_decoration || (cJSON_IsString(text_decoration) &&
                                      (!strcmp(text_decoration->valuestring, "none") ||
                                       !strcmp(text_decoration->valuestring, "underline") ||
                                       !strcmp(text_decoration->valuestring, "strike")))) &&
                (!text_spacing || (cJSON_IsNumber(text_spacing) && text_spacing->valuedouble == text_spacing->valueint &&
                                   text_spacing->valueint >= 0 && text_spacing->valueint <= 4)) &&
                (!focus_minutes || (cJSON_IsNumber(focus_minutes) && focus_minutes->valuedouble == focus_minutes->valueint &&
                                    focus_minutes->valueint >= 1 && focus_minutes->valueint <= 180)) &&
                (!break_minutes || (cJSON_IsNumber(break_minutes) && break_minutes->valuedouble == break_minutes->valueint &&
                                    break_minutes->valueint >= 1 && break_minutes->valueint <= 180)) &&
                (!widget_graphic || (cJSON_IsString(widget_graphic) &&
                                    goopad_widget_graphic_valid(widget_graphic->valuestring,
                                        cJSON_IsString(widget) ? widget->valuestring : NULL,
                                        cJSON_IsString(pc_stat) ? pc_stat->valuestring : NULL))) &&
                cJSON_IsString(icon) && strlen(icon->valuestring) <= 32 &&
                (!emoji_render || (cJSON_IsString(emoji_render) &&
                                   (!strcmp(emoji_render->valuestring, "image") ||
                                    !strcmp(emoji_render->valuestring, "symbol")))) &&
                (!emoji_image_key || (cJSON_IsString(emoji_image_key) && strlen(emoji_image_key->valuestring) <= 64)) &&
                cJSON_IsString(url) &&
                (valid_url(effective_url) ||
                 (effective_url[0] == '\0' &&
                  (!action_mode || (cJSON_IsString(action_mode) &&
                                    (!strcmp(action_mode->valuestring, "simple_link") ||
                                     !strcmp(action_mode->valuestring, "sound") ||
                                     !strcmp(action_mode->valuestring, "wake_on_lan") ||
                                     !strcmp(action_mode->valuestring, "page_navigation") ||
                                     !strcmp(action_mode->valuestring, "widget")
#ifdef CONFIG_GOOPAD_HOMEKIT
                                     || !strcmp(action_mode->valuestring, "homekit")
#endif
                                     ))))) &&
                cJSON_IsString(action) &&
                (strcmp(action->valuestring, "webhook") == 0 ||
                 strcmp(action->valuestring, "http") == 0) &&
                cJSON_IsString(method) &&
                (strcmp(method->valuestring, "GET") == 0 ||
                 strcmp(method->valuestring, "POST") == 0) &&
                cJSON_IsString(body) && strlen(body->valuestring) <= MACRO_BODY_BYTES &&
                cJSON_IsString(secret) && strlen(effective_secret) <= 64 &&
                (!icon_image || (cJSON_IsString(icon_image) &&
                                 valid_icon_key(icon_image->valuestring))) &&
                (!fa_icon || (cJSON_IsString(fa_icon) && strlen(fa_icon->valuestring) <= 80)) &&
                (!action_mode || (cJSON_IsString(action_mode) &&
                                  (!strcmp(action_mode->valuestring, "simple_link") ||
                                   !strcmp(action_mode->valuestring, "pc_open_url") ||
                                   !strcmp(action_mode->valuestring, "pc_macro") ||
                                   !strcmp(action_mode->valuestring, "sound") ||
                                   !strcmp(action_mode->valuestring, "wake_on_lan") ||
                                   !strcmp(action_mode->valuestring, "page_navigation") ||
                                   !strcmp(action_mode->valuestring, "widget") ||
                                   !strcmp(action_mode->valuestring, "home_assistant") ||
                                   !strcmp(action_mode->valuestring, "advanced")
#ifdef CONFIG_GOOPAD_HOMEKIT
                                   || !strcmp(action_mode->valuestring, "homekit")
#endif
                                   ))) &&
                (!(cJSON_IsString(action_mode) && !strcmp(action_mode->valuestring, "wake_on_lan")) ||
                 (cJSON_IsString(wake_mac) && parse_wake_mac(wake_mac->valuestring, (uint8_t[6]){0}))) &&
                (!(cJSON_IsString(action_mode) && !strcmp(action_mode->valuestring, "home_assistant")) ||
                 (valid_home_token(home_token) && cJSON_IsString(url) && !strncmp(url->valuestring, "https://", 8) &&
                  cJSON_IsString(method) && !strcmp(method->valuestring, "POST"))) &&
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
                              !strcmp(widget->valuestring, "pc_monitor") || !strcmp(widget->valuestring, "countdown") ||
                              !strcmp(widget->valuestring, "stopwatch") || !strcmp(widget->valuestring, "pomodoro") || !strcmp(widget->valuestring, "weather") ||
                              !strcmp(widget->valuestring, "stock") || !strcmp(widget->valuestring, "crypto") ||
                              !strcmp(widget->valuestring, "github") || !strcmp(widget->valuestring, "air_quality") ||
                              !strcmp(widget->valuestring, "exchange_rate") || !strcmp(widget->valuestring, "weather_alerts") ||
                              !strcmp(widget->valuestring, "home_state") || !strcmp(widget->valuestring, "now_playing")))) &&
                (!(cJSON_IsString(widget) && !strcmp(widget->valuestring, "countdown")) || countdown_at) &&
                (!(cJSON_IsString(widget) && !strcmp(widget->valuestring, "weather")) || (latitude && longitude)) &&
                (!(cJSON_IsString(widget) && !strcmp(widget->valuestring, "air_quality")) || (latitude && longitude)) &&
                (!(cJSON_IsString(widget) && !strcmp(widget->valuestring, "weather_alerts")) || (latitude && longitude)) &&
                (!(cJSON_IsString(widget) && !strcmp(widget->valuestring, "stock")) ||
                 (market_symbol && ((cJSON_IsString(stock_provider) && !strcmp(stock_provider->valuestring, "finnhub"))
                                    ? finnhub_key : stock_key))) &&
                (!(cJSON_IsString(widget) && !strcmp(widget->valuestring, "crypto")) || market_symbol) &&
                (!(cJSON_IsString(widget) && !strcmp(widget->valuestring, "github")) || github_user) &&
                (!(cJSON_IsString(widget) && !strcmp(widget->valuestring, "exchange_rate")) ||
                 (currency_base && currency_quote && strcmp(currency_base->valuestring, currency_quote->valuestring))) &&
                (!(cJSON_IsString(action_mode) && !strcmp(action_mode->valuestring, "widget") &&
                   cJSON_IsString(widget) && !strcmp(widget->valuestring, "home_state")) ||
                 valid_home_state(home_address, home_entity, home_token, url)) &&
                (!(cJSON_IsString(action_mode) && !strcmp(action_mode->valuestring, "widget")) ||
                 cJSON_IsString(widget)) &&
                (!(cJSON_IsString(action_mode) && !strcmp(action_mode->valuestring, "widget") &&
                   cJSON_IsString(widget) && (!strcmp(widget->valuestring, "pc_monitor") ||
                                              !strcmp(widget->valuestring, "now_playing"))) ||
                 (valid_url(effective_url) && strlen(effective_secret) >= 32)) &&
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
                (!page_direction || (cJSON_IsString(page_direction) &&
                                    (!strcmp(page_direction->valuestring, "next") ||
                                     !strcmp(page_direction->valuestring, "previous") ||
                                     !strcmp(page_direction->valuestring, "first")))) &&
                (!(cJSON_IsString(action_mode) && !strcmp(action_mode->valuestring, "page_navigation")) ||
                 cJSON_IsString(page_direction)) &&
                (!tile_gradient || valid_hex_color(tile_gradient)) &&
                (!visual_scale || (cJSON_IsNumber(visual_scale) &&
                                  (visual_scale->valuedouble == 50 || visual_scale->valuedouble == 75 ||
                                   visual_scale->valuedouble == 100))) &&
                (!text_color || valid_hex_color(text_color)) &&
                (!icon_color || valid_hex_color(icon_color)) &&
                (!tile_color || valid_hex_color(tile_color));
        if (!valid) break;
    }
    if (valid) {
        uint8_t widths[MAX_MACROS], heights[MAX_MACROS];
        uint32_t occupied = 0;
        int count = 0;
        for (cJSON *item = macros->child; item; item = item->next, count++) {
            cJSON *width = cJSON_GetObjectItemCaseSensitive(item, "spanCols");
            cJSON *height = cJSON_GetObjectItemCaseSensitive(item, "spanRows");
            if ((width && (!cJSON_IsNumber(width) || width->valuedouble != width->valueint ||
                           width->valueint < 1 || width->valueint > 4)) ||
                (height && (!cJSON_IsNumber(height) || height->valuedouble != height->valueint ||
                            height->valueint < 1 || height->valueint > 4))) {
                valid = false;
                break;
            }
            widths[count] = width ? width->valueint : 1;
            heights[count] = height ? height->valueint : 1;
            if (layout_occupied(item)) occupied |= UINT32_C(1) << count;
        }
        if (valid) valid = goopad_layout_valid(cols->valueint, rows->valueint, cJSON_IsTrue(hide),
                                                count, widths, heights, occupied);
    }
#ifdef CONFIG_GOOPAD_HOMEKIT
    goopad_homekit_switch_t switches[GOOPAD_HOMEKIT_MAX_SWITCHES];
    size_t switch_count;
    if (valid) valid = homekit_switches(root, switches, &switch_count);
#endif
    cJSON_Delete(root);
    return valid;
}

static esp_err_t send_web_asset(httpd_req_t *req, goopad_web_asset_t *asset)
{
    char buffer[1024];
    size_t remaining = asset->length;
    esp_err_t result = ESP_OK;
    while (remaining && result == ESP_OK) {
        size_t chunk = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        if (fread(buffer, 1, chunk, asset->file) != chunk) {
            ESP_LOGE(TAG, "SD web asset read failed during response");
            result = ESP_FAIL;
            break;
        }
        result = httpd_resp_send_chunk(req, buffer, chunk);
        remaining -= chunk;
    }
    if (fclose(asset->file) != 0 && result == ESP_OK) {
        ESP_LOGE(TAG, "SD web asset close failed");
        result = ESP_FAIL;
    }
    asset->file = NULL;
    return result == ESP_OK ? httpd_resp_send_chunk(req, NULL, 0) : result;
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
    httpd_resp_set_hdr(req, "Vary", "Accept-Encoding");
    char encoding[128];
    bool accepts_gzip = false;
    if (httpd_req_get_hdr_value_str(req, "Accept-Encoding", encoding, sizeof(encoding)) == ESP_OK) {
        char *save = NULL;
        for (char *token = strtok_r(encoding, ",", &save); token; token = strtok_r(NULL, ",", &save)) {
            while (*token == ' ' || *token == '\t') token++;
            char *end = token + strlen(token);
            while (end > token && (end[-1] == ' ' || end[-1] == '\t')) *--end = '\0';
            if (!strcmp(token, "gzip")) accepts_gzip = true;
        }
    }
    goopad_web_asset_t asset;
    if (goopad_web_open(accepts_gzip ? "index.html.gz" : "index.html", &asset) != ESP_OK) {
        httpd_resp_set_status(req, "503 Service Unavailable");
        return httpd_resp_send(req, (const char *)recovery_html_start, recovery_html_end - recovery_html_start);
    }
    if (accepts_gzip) httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return send_web_asset(req, &asset);
}

static esp_err_t font_notice_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
    return httpd_resp_send(req, (const char *)font_notice_start, font_notice_end - font_notice_start);
}

static esp_err_t preview_font_handler(httpd_req_t *req)
{
    char query[80], name[24];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, "name", name, sizeof(name)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing font");
    char filename[40];
    snprintf(filename, sizeof(filename), "%s.woff2", name);
    if (!goopad_web_known(filename)) return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown font");
    goopad_web_asset_t asset;
    if (goopad_web_open(filename, &asset) != ESP_OK) {
        httpd_resp_set_status(req, "503 Service Unavailable");
        httpd_resp_set_type(req, "text/plain; charset=utf-8");
        return httpd_resp_sendstr(req, "Restore the matching SD web resource pack");
    }
    httpd_resp_set_type(req, "font/woff2");
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=86400");
    httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
    return send_web_asset(req, &asset);
}

static uint32_t catalog_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
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
    FILE *catalog = fopen(RESOURCE_DIR "/" GOOPAD_ICONS_FILE, "rb");
    if (!catalog) {
        ESP_LOGW(TAG, "SD icon catalog unavailable");
        httpd_resp_set_status(req, "503 Service Unavailable");
        return httpd_resp_sendstr(req, "{\"error\":\"Install the GooPad SD icon resource pack\"}");
    }
    bool sized = fseek(catalog, 0, SEEK_END) == 0;
    long catalog_length = sized ? ftell(catalog) : -1;
    bool valid = catalog_length >= 7 && fseek(catalog, 0, SEEK_SET) == 0;
    char *reply = heap_caps_malloc(16, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!reply) {
        fclose(catalog);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Icon search memory unavailable");
    }
    size_t used = 10;
    memcpy(reply, "{\"icons\":[", used);
    unsigned matches = 0;
    tinfl_decompressor *decoder = NULL;
    uint8_t header[10];
    valid = valid && fread(header, 1, 7, catalog) == 7 && !memcmp(header, "GFAC\1", 5);
    unsigned count = valid ? (unsigned)header[5] | (unsigned)header[6] << 8 : 0;
    for (unsigned i = 0; valid && i < count && matches < 24; i++) {
        if (fread(header, 1, sizeof(header), catalog) != sizeof(header)) { valid = false; break; }
        size_t name_length = header[0], style_length = header[1];
        size_t compressed = catalog_u32(header + 2), plain = catalog_u32(header + 6);
        long position = ftell(catalog);
        if (position < 0 || position > catalog_length || name_length >= 96 || style_length >= 16 ||
            compressed > 128 * 1024 ||
            name_length + style_length + compressed > (size_t)(catalog_length - position)) {
            valid = false; break;
        }
        char icon_name[96], icon_style[16], combined[116];
        if (fread(icon_name, 1, name_length, catalog) != name_length ||
            fread(icon_style, 1, style_length, catalog) != style_length) { valid = false; break; }
        icon_name[name_length] = '\0'; icon_style[style_length] = '\0';
        snprintf(combined, sizeof(combined), "%s %s", icon_name, icon_style);
        bool match = name[0] ? !strcmp(name, icon_name) && !strcmp(style, icon_style)
                             : strstr(combined, search) != NULL;
        if (match) {
            if (plain >= 128 * 1024 - used - 3) { valid = false; break; }
            char *grown = heap_caps_realloc(reply, used + plain + 3, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (!grown) { valid = false; break; }
            reply = grown;
            if (!decoder) decoder = heap_caps_malloc(sizeof(*decoder), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (!decoder) { valid = false; break; }
            uint8_t *encoded = heap_caps_malloc(compressed, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (!encoded) { valid = false; break; }
            if (fread(encoded, 1, compressed, catalog) != compressed) {
                free(encoded); valid = false; break;
            }
            if (matches++) reply[used++] = ',';
            tinfl_init(decoder);
            size_t input = compressed, output = plain;
            tinfl_status status = tinfl_decompress(decoder, encoded, &input,
                (uint8_t *)reply + used, (uint8_t *)reply + used, &output,
                TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
            free(encoded);
            if (status != TINFL_STATUS_DONE || input != compressed || output != plain) {
                valid = false; break;
            }
            used += output;
        } else if (fseek(catalog, compressed, SEEK_CUR) != 0) { valid = false; break; }
    }
    fclose(catalog);
    free(decoder);
    if (!valid) {
        free(reply);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Icon catalog could not be decoded");
    }
    memcpy(reply + used, "]}", 2); used += 2;
    esp_err_t err = httpd_resp_send(req, reply, used);
    free(reply);
    return err;
}

static esp_err_t resource_upload_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    char query[128], kind[16], hash[65];
    static const struct {
        const char *kind, *file, *hash, *magic;
    } resources[] = {
        {"text", GOOPAD_TEXT_FILE, GOOPAD_TEXT_SHA256, "GFP2"},
        {"icons", GOOPAD_ICONS_FILE, GOOPAD_ICONS_SHA256, "GFAC"},
        {"web", GOOPAD_WEB_FILE, GOOPAD_WEB_SHA256, "GWP1"},
        {"styled", GOOPAD_STYLED_FILE, GOOPAD_STYLED_SHA256, "GSP1"}
    };
    unsigned selected = sizeof(resources) / sizeof(resources[0]);
    if (!g_app.sd.mounted || req->content_len < 7 || req->content_len > 96 * 1024 * 1024 ||
        httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, "kind", kind, sizeof(kind)) != ESP_OK ||
        httpd_query_key_value(query, "sha256", hash, sizeof(hash)) != ESP_OK || strlen(hash) != 64)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Mounted SD and valid resource kind/hash required");
    for (unsigned i = 0; i < sizeof(resources) / sizeof(resources[0]); i++)
        if (!strcmp(kind, resources[i].kind)) selected = i;
    if (selected == sizeof(resources) / sizeof(resources[0]))
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Unknown resource kind");
    for (unsigned i = 0; i < 64; i++)
        if (!((hash[i] >= '0' && hash[i] <= '9') || (hash[i] >= 'a' && hash[i] <= 'f')))
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Expected lowercase SHA-256");
    if (strcmp(hash, resources[selected].hash))
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Use the resource bundle matching this firmware");
    if (mkdir(BSP_SD_MOUNT_POINT "/goopad", 0775) != 0 && errno != EEXIST)
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not create SD directory");
    if (mkdir(RESOURCE_DIR, 0775) != 0 && errno != EEXIST)
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not create resource directory");
    char path[96], temporary[104];
    snprintf(path, sizeof(path), RESOURCE_DIR "/%s", resources[selected].file);
    snprintf(temporary, sizeof(temporary), "%s.part", path);
    FILE *file = fopen(temporary, "wb");
    uint8_t *buffer = heap_caps_malloc(8192, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;
    bool ok = file && buffer && psa_hash_setup(&operation, PSA_ALG_SHA_256) == PSA_SUCCESS;
    size_t received = 0;
    while (ok && received < (size_t)req->content_len) {
        size_t chunk = req->content_len - received;
        if (chunk > 8192) chunk = 8192;
        int n = httpd_req_recv(req, (char *)buffer, chunk);
        ok = n > 0 && fwrite(buffer, 1, n, file) == (size_t)n &&
            psa_hash_update(&operation, buffer, n) == PSA_SUCCESS;
        if (n > 0) received += n;
    }
    uint8_t digest[32]; size_t digest_size = 0;
    ok = ok && psa_hash_finish(&operation, digest, sizeof(digest), &digest_size) == PSA_SUCCESS && digest_size == 32;
    psa_hash_abort(&operation);
    free(buffer);
    if (file && fclose(file) != 0) ok = false;
    char actual[65];
    if (ok) {
        for (unsigned i = 0; i < 32; i++) snprintf(actual + i * 2, 3, "%02x", digest[i]);
        ok = !strcmp(actual, hash);
    }
    uint8_t signature[4];
    file = ok ? fopen(temporary, "rb") : NULL;
    if (file) { ok = fread(signature, 1, 4, file) == 4; fclose(file); }
    else ok = false;
    ok = ok && !memcmp(signature, resources[selected].magic, 4);
    bool reused = false;
    struct stat existing;
    if (ok) {
        if (stat(path, &existing) == 0) {
            reused = matching_files(path, temporary);
            if (!reused) {
                if (unlink(temporary) != 0 && errno != ENOENT)
                    ESP_LOGE(TAG, "Could not remove conflicting resource staging file");
                httpd_resp_set_status(req, "409 Conflict");
                return httpd_resp_sendstr(req, "{\"error\":\"A different immutable resource pack is already installed\"}");
            }
        } else if (errno != ENOENT) {
            ok = false;
        }
    }
    if (ok && !reused) ok = rename(temporary, path) == 0;
    if (!ok || reused) {
        if (unlink(temporary) != 0 && errno != ENOENT) ESP_LOGE(TAG, "Could not remove resource staging file");
    }
    if (!ok) {
        ESP_LOGE(TAG, "Resource upload failed at %u bytes", (unsigned)received);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Resource upload/integrity check failed");
    }
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, reused ? "{\"ok\":true,\"reused\":true}" : "{\"ok\":true,\"reused\":false,\"restartRequired\":true}");
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
#ifdef CONFIG_GOOPAD_HOMEKIT
    cJSON *new_root = cJSON_Parse(body);
    xSemaphoreTake(s_config_lock, portMAX_DELAY);
    cJSON *old_root = cJSON_Parse(s_config);
    xSemaphoreGive(s_config_lock);
    bool topology_changed = false;
    bool topology_valid = new_root && old_root &&
                          homekit_topology_changed(old_root, new_root, &topology_changed);
    cJSON_Delete(new_root);
    cJSON_Delete(old_root);
    if (!topology_valid) {
        free(body);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not compare HomeKit switches");
    }
#endif
    /* Persist before publishing the new in-memory snapshot: a failed NVS
     * commit must not make the display and saved configuration diverge. */
    err = goopad_config_storage_save(body);
    if (err != ESP_OK) {
        free(body);
        ESP_LOGE(TAG, "save config failed: %s", esp_err_to_name(err));
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
            err == ESP_ERR_NVS_NOT_ENOUGH_SPACE ? "Configuration storage is full; remove some tiles or settings" :
                                                 "Could not save config");
    }
    xSemaphoreTake(s_config_lock, portMAX_DELAY);
    strlcpy(s_config, body, CONFIG_MAX_BYTES + 1);
    xSemaphoreGive(s_config_lock);
    free(body);
    httpd_resp_set_type(req, "application/json");
#ifdef CONFIG_GOOPAD_HOMEKIT
    if (topology_changed) {
        if (xTaskCreate(restart_task, "homekit_restart", 2048, NULL, 5, NULL) != pdPASS) {
            ESP_LOGE(TAG, "Configuration saved, but HomeKit restart could not be scheduled");
            return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                       "Configuration saved; restart GooPad to apply HomeKit changes");
        }
        return httpd_resp_sendstr(req, "{\"ok\":true,\"restarting\":true}");
    }
#endif
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
    char bearer[520];
    char certificate[COMPANION_CERT_BYTES];
    bool wake_on_lan;
    bool diagnostic;
    uint8_t wake_mac[6];
    unsigned serial;
    unsigned index;
    int64_t queued_at;
} macro_action_t;

typedef struct {
    esp_http_client_handle_t client;
    char url[180];
    char certificate[COMPANION_CERT_BYTES];
} macro_client_cache_t;

static void macro_client_clear(macro_client_cache_t *cache)
{
    if (cache && cache->client) {
        esp_http_client_cleanup(cache->client);
        cache->client = NULL;
        cache->url[0] = '\0';
        cache->certificate[0] = '\0';
    }
}

static esp_err_t macro_optional_header(esp_http_client_handle_t client, const char *name, const char *value)
{
    esp_err_t err = esp_http_client_set_header(client, name, value[0] ? value : NULL);
    return err == ESP_ERR_NOT_FOUND ? ESP_OK : err;
}

static void action_result(const macro_action_t *action, int status, esp_err_t error, int tls_error)
{
    int elapsed = (int)((esp_timer_get_time() - action->queued_at) / 1000);
    if (action->diagnostic) {
        goopad_diag_record(DIAG_PC_TEST_LATENCY, error == ESP_OK ? status : -(int)error, elapsed);
        return;
    }
    goopad_diag_record(DIAG_MACRO_RESULT, error, status);
    goopad_diag_record(DIAG_MACRO_LATENCY, error, elapsed);
    portENTER_CRITICAL(&s_action_lock);
    if (action->serial == s_action_serial) {
        s_last_action_status = status;
        s_last_action_tls_error = tls_error;
        s_last_action_running = false;
        s_last_action_success = error == ESP_OK &&
                                (action->wake_on_lan || (status >= 200 && status < 300));
        if (action->wake_on_lan && error == ESP_OK)
            snprintf(s_last_action_message, sizeof(s_last_action_message),
                     "Tile %u: wake packet sent; the PC may take a moment to start.", action->index + 1);
        else if (action->wake_on_lan)
            snprintf(s_last_action_message, sizeof(s_last_action_message),
                     "Tile %u: could not send wake packet (%s).", action->index + 1, esp_err_to_name(error));
        else if (error != ESP_OK) snprintf(s_last_action_message, sizeof(s_last_action_message),
            "Tile %u: request failed (%s; TLS %d).", action->index + 1, esp_err_to_name(error), tls_error);
        else snprintf(s_last_action_message, sizeof(s_last_action_message),
            "Tile %u: %s (HTTP %d).", action->index + 1,
            s_last_action_success ? (status == 202 ? "accepted" : "completed") : "request rejected", status);
    }
    portEXIT_CRITICAL(&s_action_lock);
}

static void action_finished(void)
{
    portENTER_CRITICAL(&s_action_lock);
    s_action_count--;
    portEXIT_CRITICAL(&s_action_lock);
}

static esp_err_t send_wake_packet(const uint8_t mac[6])
{
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t info;
    if (!s_connected || !netif || !esp_netif_is_netif_up(netif) ||
        esp_netif_get_ip_info(netif, &info) != ESP_OK || !info.ip.addr || !info.netmask.addr ||
        ntohl(~info.netmask.addr) <= 1) {
        ESP_LOGW(TAG, "Wake-on-LAN requires a connected IPv4 subnet with broadcast support");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t packet[6 + 16 * 6];
    memset(packet, 0xff, 6);
    for (size_t i = 0; i < 16; i++) memcpy(packet + 6 + i * 6, mac, 6);
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        ESP_LOGE(TAG, "Wake-on-LAN socket failed: %d", errno);
        return ESP_FAIL;
    }
    int enabled = 1;
    esp_err_t result = ESP_FAIL;
    if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &enabled, sizeof(enabled)) != 0) {
        ESP_LOGE(TAG, "Wake-on-LAN broadcast setup failed: %d", errno);
    } else {
        struct sockaddr_in target = {
            .sin_family = AF_INET, .sin_port = htons(9),
            .sin_addr.s_addr = info.ip.addr | ~info.netmask.addr,
        };
        int sent = sendto(fd, packet, sizeof(packet), 0, (struct sockaddr *)&target, sizeof(target));
        if (sent == sizeof(packet)) result = ESP_OK;
        else ESP_LOGE(TAG, "Wake-on-LAN send failed (%d bytes, errno %d)", sent, errno);
    }
    close(fd);
    return result;
}

static void run_macro_action(macro_action_t *macro, macro_client_cache_t *cache)
{
    if (macro->wake_on_lan) {
        esp_err_t err = send_wake_packet(macro->wake_mac);
        action_result(macro, 0, err, 0);
        free(macro);
        action_finished();
        return;
    }
    esp_err_t admission = network_begin(pdMS_TO_TICKS(15000), NETWORK_MACRO);
    if (admission != ESP_OK) {
        ESP_LOGW(TAG, "Macro request deferred: %s", esp_err_to_name(admission));
        action_result(macro, 0, admission, 0);
        free(macro);
        action_finished();
        return;
    }
    esp_http_client_config_t cfg = {
        .url = macro->url,
        .timeout_ms = 10000,
    };
    bool cacheable = false;
#if CONFIG_ESP_TLS_CLIENT_SESSION_TICKETS
    cacheable = cache && macro->certificate[0];
#endif
    if (cacheable && cache->client &&
        (strcmp(cache->url, macro->url) || strcmp(cache->certificate, macro->certificate)))
        macro_client_clear(cache);
    esp_http_client_handle_t client = cacheable ? cache->client : NULL;
    if (!client) {
        if (cacheable) {
            strlcpy(cache->url, macro->url, sizeof(cache->url));
            strlcpy(cache->certificate, macro->certificate, sizeof(cache->certificate));
#if CONFIG_ESP_TLS_CLIENT_SESSION_TICKETS
            cfg.save_client_session = true;
#endif
        }
        client_tls(&cfg, cacheable ? cache->certificate : macro->certificate[0] ? macro->certificate : NULL);
        client = esp_http_client_init(&cfg);
        if (cacheable) cache->client = client;
    }
    if (!client) {
        ESP_LOGE(TAG, "HTTP client allocation failed");
        network_end();
        action_result(macro, 0, ESP_ERR_NO_MEM, 0);
        free(macro);
        action_finished();
        return;
    }
    bool post = !strcmp(macro->method, "POST");
    esp_err_t err = esp_http_client_set_method(client, post ? HTTP_METHOD_POST : HTTP_METHOD_GET);
    if (err == ESP_OK && post) err = esp_http_client_set_header(client, "Content-Type", "application/json");
    if (err == ESP_OK && post)
        err = esp_http_client_set_post_field(client, macro->body, (int)strlen(macro->body));
    if (err == ESP_OK)
        err = macro_optional_header(client, "X-GooPad-Token", macro->secret);
    if (err == ESP_OK)
        err = macro_optional_header(client, "Authorization", macro->bearer);
    if (err == ESP_OK) err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    int tls_error = 0, tls_flags = 0;
    if (err != ESP_OK) {
        esp_http_client_get_and_clear_last_tls_error(client, &tls_error, &tls_flags);
    }
    action_result(macro, status, err, tls_error);
    if (err == ESP_OK) {
        if (status >= 200 && status < 300) ESP_LOGI(TAG, "macro request completed, status=%d", status);
        else ESP_LOGW(TAG, "macro request rejected, status=%d", status);
    } else {
        ESP_LOGW(TAG, "macro request failed: %s", esp_err_to_name(err));
    }
    if (cacheable) {
        char actual_url[sizeof(macro->url)];
        esp_err_t url_result = esp_http_client_get_url(client, actual_url, sizeof(actual_url));
        if (err != ESP_OK || url_result != ESP_OK || strcmp(actual_url, macro->url)) {
            macro_client_clear(cache);
        } else {
            esp_err_t reset = post ? esp_http_client_set_post_field(client, NULL, 0) : ESP_OK;
            if (reset == ESP_OK) reset = esp_http_client_close(client);
            if (reset != ESP_OK) {
                ESP_LOGW(TAG, "Could not retain Companion session: %s", esp_err_to_name(reset));
                macro_client_clear(cache);
            }
        }
    } else {
        esp_http_client_cleanup(client);
    }
    network_end();
    free(macro);
    action_finished();
}

static void macro_task(void *arg)
{
    (void)arg;
    macro_action_t *action;
    macro_client_cache_t *cache = NULL;
#if CONFIG_ESP_TLS_CLIENT_SESSION_TICKETS
    cache = heap_caps_calloc(1, sizeof(*cache), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!cache) ESP_LOGW(TAG, "Companion session cache unavailable");
#endif
    while (true) {
        if (xQueueReceive(s_action_queue, &action, pdMS_TO_TICKS(10000)) == pdTRUE)
            run_macro_action(action, cache);
        else
            macro_client_clear(cache);
    }
}

esp_err_t goopad_diag_test_pc(bool post)
{
    xSemaphoreTake(s_config_lock, portMAX_DELAY);
    cJSON *root = cJSON_Parse(s_config);
    cJSON *macros = root ? cJSON_GetObjectItemCaseSensitive(root, "macros") : NULL;
    cJSON *certificate = root ? cJSON_GetObjectItemCaseSensitive(root, "companionCertificate") : NULL;
    cJSON *target = NULL;
    for (cJSON *item = cJSON_IsArray(macros) ? macros->child : NULL; item; item = item->next) {
        cJSON *mode = cJSON_GetObjectItemCaseSensitive(item, "actionMode");
        cJSON *widget = cJSON_GetObjectItemCaseSensitive(item, "widget");
        if (cJSON_IsString(mode) && !strcmp(mode->valuestring, "widget") &&
            cJSON_IsString(widget) && !strcmp(widget->valuestring, "pc_monitor")) {
            target = item;
            break;
        }
    }
    cJSON *url = target ? cJSON_GetObjectItemCaseSensitive(target, "url") : NULL;
    cJSON *secret = target ? cJSON_GetObjectItemCaseSensitive(target, "secret") : NULL;
    char shared_url[180];
    const char *effective_url = cJSON_IsString(url) ? url->valuestring : NULL;
    const char *effective_secret = cJSON_IsString(secret) ? secret->valuestring : NULL;
    cJSON *shared_pc = target ? cJSON_GetObjectItemCaseSensitive(target, "sharedPc") : NULL;
    if (shared_pc && goopad_companion_resolve(root, target, shared_url, sizeof(shared_url), &effective_secret))
        effective_url = shared_url;
    else if (shared_pc) effective_url = NULL;
    const char *endpoint = effective_url ? strrchr(effective_url, '/') : NULL;
    if (!endpoint || strcmp(endpoint, "/monitor") || !valid_url(effective_url) ||
        strncmp(effective_url, "https://", 8) || !effective_secret ||
        !valid_companion_certificate(certificate)) {
        cJSON_Delete(root);
        xSemaphoreGive(s_config_lock);
        return ESP_ERR_INVALID_STATE;
    }
    portENTER_CRITICAL(&s_action_lock);
    bool busy = !s_action_queue || s_action_count >= 5;
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
        action_finished();
        return ESP_ERR_NO_MEM;
    }
    size_t base = endpoint - effective_url;
    memcpy(action->url, effective_url, base);
    strlcpy(action->url + base, "/status", sizeof(action->url) - base);
    strlcpy(action->method, post ? "POST" : "GET", sizeof(action->method));
    if (post) strlcpy(action->body, "{}", sizeof(action->body));
    strlcpy(action->secret, effective_secret, sizeof(action->secret));
    strlcpy(action->certificate, certificate->valuestring, sizeof(action->certificate));
    action->diagnostic = true;
    action->queued_at = esp_timer_get_time();
    cJSON_Delete(root);
    if (xQueueSend(s_action_queue, &action, 0) != pdTRUE) {
        xSemaphoreGive(s_config_lock);
        free(action);
        action_finished();
        return ESP_ERR_INVALID_STATE;
    }
    xSemaphoreGive(s_config_lock);
    return ESP_OK;
}

static esp_err_t trigger_action(unsigned index, bool navigation, bool skip)
{
    /* Take a configuration snapshot while locked. The worker must not retain
     * cJSON pointers after the tree is freed or perform network I/O under it. */
    xSemaphoreTake(s_config_lock, portMAX_DELAY);
#ifdef GOOPAD_CAPACITY_TEST
    if (s_capacity_config) {
        xSemaphoreGive(s_config_lock);
        return ESP_ERR_INVALID_STATE;
    }
#endif
    cJSON *root = cJSON_Parse(s_config);
    cJSON *macros = root ? cJSON_GetObjectItemCaseSensitive(root, "macros") : NULL;
    cJSON *item = cJSON_IsArray(macros) ? cJSON_GetArrayItem(macros, (int)index) : NULL;
    cJSON *url = item ? cJSON_GetObjectItemCaseSensitive(item, "url") : NULL;
    cJSON *method = item ? cJSON_GetObjectItemCaseSensitive(item, "method") : NULL;
    cJSON *body = item ? cJSON_GetObjectItemCaseSensitive(item, "body") : NULL;
    cJSON *secret = item ? cJSON_GetObjectItemCaseSensitive(item, "secret") : NULL;
    cJSON *sound = item ? cJSON_GetObjectItemCaseSensitive(item, "sound") : NULL;
    cJSON *mode = item ? cJSON_GetObjectItemCaseSensitive(item, "actionMode") : NULL;
    cJSON *wake_mac = item ? cJSON_GetObjectItemCaseSensitive(item, "wakeMac") : NULL;
    cJSON *home_token = item ? cJSON_GetObjectItemCaseSensitive(item, "homeToken") : NULL;
    cJSON *certificate = cJSON_GetObjectItemCaseSensitive(root, "companionCertificate");
    cJSON *shared_pc = item ? cJSON_GetObjectItemCaseSensitive(item, "sharedPc") : NULL;
    char shared_url[180];
    const char *effective_url = cJSON_IsString(url) ? url->valuestring : NULL;
    const char *effective_secret = cJSON_IsString(secret) ? secret->valuestring : NULL;
    if (shared_pc && goopad_companion_resolve(root, item, shared_url, sizeof(shared_url), &effective_secret))
        effective_url = shared_url;
    else if (shared_pc) effective_url = NULL;
    bool page_navigation = cJSON_IsString(mode) && !strcmp(mode->valuestring, "page_navigation");
    if (page_navigation != navigation) {
        cJSON_Delete(root);
        xSemaphoreGive(s_config_lock);
        return ESP_ERR_NOT_SUPPORTED;
    }
    cJSON *widget = item ? cJSON_GetObjectItemCaseSensitive(item, "widget") : NULL;
    bool now_playing = cJSON_IsString(mode) && !strcmp(mode->valuestring, "widget") &&
                       cJSON_IsString(widget) && !strcmp(widget->valuestring, "now_playing");
    if (skip && !now_playing) {
        cJSON_Delete(root);
        xSemaphoreGive(s_config_lock);
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (!now_playing && cJSON_IsString(mode) && !strcmp(mode->valuestring, "widget")) {
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
    if (page_navigation || (cJSON_IsString(mode) && !strcmp(mode->valuestring, "sound"))) {
        cJSON_Delete(root);
        xSemaphoreGive(s_config_lock);
        return sound_err;
    }
#ifdef CONFIG_GOOPAD_HOMEKIT
    if (cJSON_IsString(mode) && !strcmp(mode->valuestring, "homekit")) {
        cJSON *id = cJSON_GetObjectItemCaseSensitive(item, "homekitId");
        esp_err_t err = cJSON_IsString(id) ? goopad_homekit_toggle(id->valuestring) : ESP_ERR_INVALID_ARG;
        cJSON_Delete(root);
        xSemaphoreGive(s_config_lock);
        return err;
    }
#endif
    bool wake = cJSON_IsString(mode) && !strcmp(mode->valuestring, "wake_on_lan");
    uint8_t mac[6];
    if (wake && (!cJSON_IsString(wake_mac) || !parse_wake_mac(wake_mac->valuestring, mac))) {
        cJSON_Delete(root);
        xSemaphoreGive(s_config_lock);
        ESP_LOGW(TAG, "macro %u has an invalid Wake-on-LAN address", index);
        return ESP_ERR_INVALID_ARG;
    }
    const char *media_body = NULL;
    char media_url[180];
    bool media_ready = !now_playing || goopad_companion_media_request(effective_url, skip,
        media_url, sizeof(media_url), &media_body);
    if (now_playing && media_ready) effective_url = media_url;
    if (!wake && (!valid_url(effective_url) || !media_ready ||
        (!now_playing && (!cJSON_IsString(method) || !cJSON_IsString(body))) || !effective_secret)) {
        cJSON_Delete(root);
        xSemaphoreGive(s_config_lock);
        ESP_LOGW(TAG, "macro %u is not configured", index);
        return sound_err != ESP_OK ? sound_err : ESP_ERR_INVALID_ARG;
    }
    bool companion = !wake && (now_playing || pc_action_mode(mode) || companion_target(root, effective_url));
    bool home = cJSON_IsString(mode) && !strcmp(mode->valuestring, "home_assistant");
    if ((companion && (strncmp(effective_url, "https://", 8) ||
                                 !valid_companion_certificate(certificate))) ||
        (home && (strncmp(effective_url, "https://", 8) || !valid_home_token(home_token))) ||
        (!wake && effective_secret[0] && strncmp(effective_url, "https://", 8))) {
        cJSON_Delete(root);
        xSemaphoreGive(s_config_lock);
        ESP_LOGW(TAG, "Refusing insecure or unpaired PC action");
        return ESP_ERR_INVALID_VERSION;
    }
    portENTER_CRITICAL(&s_action_lock);
    bool busy = !s_action_queue || s_action_count >= 5;
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
    action->wake_on_lan = wake;
    if (wake) memcpy(action->wake_mac, mac, sizeof(mac));
    else {
        strlcpy(action->url, effective_url, sizeof(action->url));
        strlcpy(action->method, now_playing ? "POST" : method->valuestring, sizeof(action->method));
        strlcpy(action->body, now_playing ? media_body : body->valuestring, sizeof(action->body));
        strlcpy(action->secret, effective_secret, sizeof(action->secret));
    }
    if (home) snprintf(action->bearer, sizeof(action->bearer), "Bearer %s", home_token->valuestring);
    if (companion) strlcpy(action->certificate, certificate->valuestring, sizeof(action->certificate));
    action->index = index;
    action->queued_at = esp_timer_get_time();
    portENTER_CRITICAL(&s_action_lock);
    action->serial = ++s_action_serial;
    s_last_action_index = (int)index;
    s_last_action_status = s_last_action_tls_error = 0;
    s_last_action_running = true;
    s_last_action_success = false;
    snprintf(s_last_action_message, sizeof(s_last_action_message), "Tile %u: queued saved action...", index + 1);
    portEXIT_CRITICAL(&s_action_lock);
    cJSON_Delete(root);
    if (xQueueSend(s_action_queue, &action, 0) != pdTRUE) {
        xSemaphoreGive(s_config_lock);
        action_result(action, 0, ESP_ERR_INVALID_STATE, 0);
        free(action);
        action_finished();
        ESP_LOGW(TAG, "Macro action queue is full");
        return ESP_ERR_INVALID_STATE;
    }
    xSemaphoreGive(s_config_lock);
    return ESP_OK;
}

esp_err_t goopad_trigger(unsigned index)
{
    return trigger_action(index, false, false);
}

esp_err_t goopad_media_skip(unsigned index)
{
    return trigger_action(index, false, true);
}

esp_err_t goopad_navigation_sound(unsigned index)
{
    return trigger_action(index, true, false);
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
        return httpd_resp_sendstr(req, "The action queue is full or unavailable; try again shortly");
    }
    if (err == ESP_ERR_INVALID_VERSION) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
        "Pair the updated HTTPS Companion in Settings. Tokens cannot be sent over HTTP.");
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
    /* The running slot stays intact until the complete inactive image passes
     * esp_ota_end; only then may the boot selector be changed. */
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
    const size_t buffer_size = 4096;
    uint8_t *buffer = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buffer) {
        esp_ota_abort(handle);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA buffer unavailable");
    }
    size_t remaining = (size_t)req->content_len;
    while (remaining) {
        int want = (int)(remaining > buffer_size ? buffer_size : remaining);
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

static esp_err_t update_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    if (req->method == HTTP_GET) {
        char version[GOOPAD_UPDATE_VERSION_SIZE];
        bool available;
        if (goopad_update_check(version, &available) != ESP_OK)
            return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                      "Could not check for updates. Check GooPad's internet connection and time.");
        char reply[96];
        snprintf(reply, sizeof(reply), "{\"version\":\"%s\",\"available\":%s}",
                 version, available ? "true" : "false");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Cache-Control", "no-store");
        return httpd_resp_sendstr(req, reply);
    }
    char *body = NULL;
    esp_err_t err = receive_body(req, &body);
    if (err != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid update request");
    cJSON *root = cJSON_Parse(body);
    cJSON *version = root ? cJSON_GetObjectItemCaseSensitive(root, "version") : NULL;
    err = cJSON_IsString(version) ? goopad_update_install(version->valuestring) : ESP_ERR_INVALID_ARG;
    cJSON_Delete(root);
    free(body);
    if (err != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Update failed or the release changed. Check again and retry.");
    if (xTaskCreate(restart_task, "ota_restart", 2048, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "could not schedule OTA restart");
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Firmware installed; restart GooPad manually to finish.");
    }
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true,\"restarting\":true}");
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
    cJSON_AddStringToObject(root, "firmwareVersion", esp_app_get_description()->version);
    cJSON_AddStringToObject(root, "board", BSP_BOARD_NAME);
    cJSON_AddNumberToObject(root, "screenWidth", BSP_LCD_H_RES);
    cJSON_AddNumberToObject(root, "screenHeight", BSP_LCD_V_RES);
    cJSON_AddNumberToObject(root, "maxCols", goopad_grid_limit(BSP_LCD_H_RES, 0, BSP_TILE_MIN_PX));
    cJSON_AddNumberToObject(root, "maxRows", goopad_grid_limit(BSP_LCD_V_RES, 84, BSP_TILE_MIN_PX));
    cJSON_AddStringToObject(root, "ssid", s_ssid);
    cJSON_AddBoolToObject(root, "sd", g_app.sd.mounted);
    goopad_unicode_status_t unicode;
    goopad_unicode_status(&unicode);
    cJSON *fonts = cJSON_AddObjectToObject(root, "unicode");
    cJSON_AddBoolToObject(fonts, "available", unicode.available);
    cJSON_AddNumberToObject(fonts, "glyphsPerSize", unicode.glyphs);
    cJSON_AddNumberToObject(fonts, "bitmapCacheBytes", unicode.cache_bytes);
    cJSON_AddNumberToObject(fonts, "bitmapCachePeak", unicode.cache_peak);
    cJSON_AddNumberToObject(fonts, "codepointIndexBytes", unicode.codepoint_index_bytes);
    cJSON_AddNumberToObject(fonts, "metricsCacheBytes", unicode.metrics_cache_bytes);
    cJSON_AddNumberToObject(fonts, "cacheHits", unicode.hits);
    cJSON_AddNumberToObject(fonts, "sdReads", unicode.reads);
    goopad_styled_status_t styled;
    goopad_styled_status(&styled);
    cJSON *styled_fonts = cJSON_AddObjectToObject(root, "styledFonts");
    bool styled_status_ok = styled_fonts &&
        cJSON_AddBoolToObject(styled_fonts, "available", styled.available) &&
        cJSON_AddNumberToObject(styled_fonts, "fonts", styled.fonts) &&
        cJSON_AddNumberToObject(styled_fonts, "glyphs", styled.glyphs) &&
        cJSON_AddNumberToObject(styled_fonts, "indexedMetadataBytes", styled.indexed_metadata_bytes) &&
        cJSON_AddNumberToObject(styled_fonts, "descriptorBytes", styled.descriptor_bytes) &&
        cJSON_AddNumberToObject(styled_fonts, "metadataCacheBytes", styled.metadata_cache_bytes) &&
        cJSON_AddNumberToObject(styled_fonts, "kerningCacheBytes", styled.kerning_cache_bytes) &&
        cJSON_AddNumberToObject(styled_fonts, "kerningPairs", styled.kerning_pairs) &&
        cJSON_AddNumberToObject(styled_fonts, "bitmapCacheBytes", styled.bitmap_cache_bytes) &&
        cJSON_AddNumberToObject(styled_fonts, "bitmapCachePeak", styled.bitmap_cache_peak) &&
        cJSON_AddNumberToObject(styled_fonts, "sharedBitmapCacheBytes", styled.shared_bitmap_cache_bytes) &&
        cJSON_AddNumberToObject(styled_fonts, "bitmapCacheBudget", styled.bitmap_cache_budget) &&
        cJSON_AddNumberToObject(styled_fonts, "cacheHits", styled.hits) &&
        cJSON_AddNumberToObject(styled_fonts, "sdReads", styled.reads);
    if (!styled_status_ok) {
        cJSON_Delete(root);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Font status memory unavailable");
    }
    cJSON_AddBoolToObject(root, "soundAvailable", g_app.results.audio == TEST_PASS);
    cJSON_AddNumberToObject(root, "audioTones", g_app.audio_tones_played);
    cJSON_AddNumberToObject(root, "audioBytes", g_app.audio_bytes_written);
    cJSON_AddNumberToObject(root, "audioPeak", g_app.audio_peak);
    cJSON_AddNumberToObject(root, "uptimeMs", esp_timer_get_time() / 1000);
    cJSON_AddNumberToObject(root, "timeEpoch", time(NULL));
    cJSON_AddNumberToObject(root, "renderedTiles", g_app.rendered_tiles);
    cJSON_AddNumberToObject(root, "renderedPages", g_app.rendered_pages);
    cJSON_AddNumberToObject(root, "emptyTiles", g_app.empty_tiles);
    cJSON_AddNumberToObject(root, "symbolFallbacks", g_app.symbol_fallbacks);
    char action_message[160];
    int action_index, action_status, action_tls_error;
    bool action_running, action_success;
    portENTER_CRITICAL(&s_action_lock);
    strlcpy(action_message, s_last_action_message, sizeof(action_message));
    action_index = s_last_action_index;
    action_status = s_last_action_status;
    action_tls_error = s_last_action_tls_error;
    action_running = s_last_action_running;
    action_success = s_last_action_success;
    portEXIT_CRITICAL(&s_action_lock);
    cJSON *last_action = cJSON_AddObjectToObject(root, "lastAction");
    cJSON_AddNumberToObject(last_action, "index", action_index);
    cJSON_AddNumberToObject(last_action, "httpStatus", action_status);
    cJSON_AddNumberToObject(last_action, "tlsError", action_tls_error);
    cJSON_AddBoolToObject(last_action, "running", action_running);
    cJSON_AddBoolToObject(last_action, "success", action_success);
    cJSON_AddStringToObject(last_action, "message", action_message);
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
        if (i == 6 || i == 7) {
            goopad_pc_stat_text(i, true, monitor, sizeof(monitor));
            cJSON_AddStringToObject(fahrenheit, goopad_pc_stat_key(i), monitor);
        }
    }
    cJSON *live_widgets = cJSON_AddArrayToObject(root, "liveWidgets");
    char live_text[MEDIA_TEXT_BYTES];
    for (unsigned i = 0; i < 32; i++) {
        goopad_live_widget_text(i, live_text, sizeof(live_text));
        cJSON_AddItemToArray(live_widgets, cJSON_CreateString(live_text));
    }
    portENTER_CRITICAL(&s_live_lock);
    uint32_t media_revision = s_media_revision, media_poll = s_media_poll_ms, media_age = s_media_age_ms;
    portEXIT_CRITICAL(&s_live_lock);
    cJSON_AddNumberToObject(root, "mediaRevision", media_revision);
    cJSON_AddNumberToObject(root, "mediaPollMs", media_poll);
    cJSON_AddNumberToObject(root, "mediaSourceAgeMs", media_age);
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
    size_t capacity;
    bool overflow;
} companion_reply_t;

static esp_err_t companion_response(esp_http_client_event_t *event)
{
    companion_reply_t *reply = event->user_data;
    if (event->event_id == HTTP_EVENT_ON_DATA && event->data_len > 0) {
        if ((size_t)event->data_len > reply->capacity - reply->length) reply->overflow = true;
        else {
            memcpy(reply->data + reply->length, event->data, event->data_len);
            reply->length += event->data_len;
        }
    }
    return ESP_OK;
}

void goopad_live_widget_text(unsigned index, char *out, size_t capacity)
{
    if (index >= 32) {
        strlcpy(out, "Invalid widget", capacity);
        return;
    }
    portENTER_CRITICAL(&s_live_lock);
    const char *text = (s_media_mask & (UINT32_C(1) << index)) && s_media_text ? s_media_text : s_live_text[index];
    strlcpy(out, text[0] ? text : "Waiting for data", capacity);
    portEXIT_CRITICAL(&s_live_lock);
}

int goopad_weather_code(unsigned index)
{
    if (index >= 32) return -1;
    portENTER_CRITICAL(&s_live_lock);
    int code = s_weather_code[index] - 1;
    portEXIT_CRITICAL(&s_live_lock);
    return code;
}

static void set_live_text(unsigned index, const char *text)
{
    portENTER_CRITICAL(&s_live_lock);
    strlcpy(s_live_text[index], text, sizeof(s_live_text[index]));
    portEXIT_CRITICAL(&s_live_lock);
}

static void set_live_waiting(unsigned index, const char *text)
{
    portENTER_CRITICAL(&s_live_lock);
    s_weather_code[index] = 0;
    strlcpy(s_live_text[index], text, sizeof(s_live_text[index]));
    portEXIT_CRITICAL(&s_live_lock);
}

static const char *weather_description(int code)
{
    if (code == 0) return "Clear";
    if (code <= 2) return "Partly cloudy";
    if (code == 3) return "Cloudy";
    if (code == 45 || code == 48) return "Fog";
    if (code >= 51 && code <= 67) return "Rain";
    if (code >= 71 && code <= 77) return "Snow";
    if (code >= 80 && code <= 82) return "Showers";
    if (code == 85 || code == 86) return "Snow";
    if (code >= 95 && code <= 99) return "Storm";
    return "Unknown";
}

static bool market_price(const cJSON *value, double *out)
{
    if (!cJSON_IsString(value)) return false;
    char *end;
    double price = strtod(value->valuestring, &end);
    if (end == value->valuestring || *end || !isfinite(price) || price < 0 || price > 1e12) return false;
    *out = price;
    return true;
}

static void live_widget_task(void *arg)
{
    (void)arg;
    char *config_json = heap_caps_malloc(CONFIG_MAX_BYTES + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    char *data = heap_caps_calloc(1, 4097, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    char (*previous)[384] = heap_caps_calloc(32, sizeof(*previous), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    int64_t *next = heap_caps_calloc(32, sizeof(*next), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!config_json || !data || !previous || !next) {
        ESP_LOGE(TAG, "Live widget memory unavailable");
        for (unsigned i = 0; i < 32; i++) set_live_text(i, "Widget memory error");
        free(config_json); free(data); free(previous); free(next);
        vTaskDelete(NULL);
        return;
    }
    bool waiting_for_ready[32] = {false};
    int64_t alert_updated[32] = {0};
    int64_t home_updated[32] = {0};
    int64_t next_nws_request = 0;
    while (true) {
        goopad_get_config(config_json, CONFIG_MAX_BYTES + 1);
        cJSON *root = cJSON_Parse(config_json);
        cJSON *macros = root ? cJSON_GetObjectItemCaseSensitive(root, "macros") : NULL;
        bool fetched = false;
        bool retry_soon = false;
        bool pending_ready = false;
        bool connected = s_connected;
        bool time_ready = time(NULL) >= 1577836800;
        for (unsigned i = 0; i < 32; i++) {
            cJSON *item = cJSON_IsArray(macros) ? cJSON_GetArrayItem(macros, i) : NULL;
            cJSON *mode = item ? cJSON_GetObjectItemCaseSensitive(item, "actionMode") : NULL;
            cJSON *widget = item ? cJSON_GetObjectItemCaseSensitive(item, "widget") : NULL;
            bool enabled = cJSON_IsString(mode) && !strcmp(mode->valuestring, "widget") && cJSON_IsString(widget);
            const char *kind = enabled ? widget->valuestring : "";
            char url[256] = "";
            int interval = 0;
            cJSON *provider = item ? cJSON_GetObjectItemCaseSensitive(item, "stockProvider") : NULL;
            bool finnhub = !strcmp(kind, "stock") && cJSON_IsString(provider) &&
                           !strcmp(provider->valuestring, "finnhub");
            cJSON *finnhub_key = finnhub ? cJSON_GetObjectItemCaseSensitive(item, "finnhubKey") : NULL;
            cJSON *symbol = item ? cJSON_GetObjectItemCaseSensitive(item, "marketSymbol") : NULL;
            cJSON *currency_base = item ? cJSON_GetObjectItemCaseSensitive(item, "currencyBase") : NULL;
            cJSON *currency_quote = item ? cJSON_GetObjectItemCaseSensitive(item, "currencyQuote") : NULL;
            cJSON *home_token = item ? cJSON_GetObjectItemCaseSensitive(item, "homeToken") : NULL;
            bool home_state = !strcmp(kind, "home_state");
            if (!strcmp(kind, "weather")) {
                cJSON *lat = cJSON_GetObjectItemCaseSensitive(item, "weatherLatitude");
                cJSON *lon = cJSON_GetObjectItemCaseSensitive(item, "weatherLongitude");
                cJSON *unit = cJSON_GetObjectItemCaseSensitive(item, "temperatureUnit");
                if (cJSON_IsNumber(lat) && cJSON_IsNumber(lon))
                    snprintf(url, sizeof(url), "https://api.open-meteo.com/v1/forecast?latitude=%.5f&longitude=%.5f&current=temperature_2m,weather_code&temperature_unit=%s",
                             lat->valuedouble, lon->valuedouble, cJSON_IsString(unit) && !strcmp(unit->valuestring, "F") ? "fahrenheit" : "celsius");
                interval = 900;
            } else if (!strcmp(kind, "air_quality")) {
                cJSON *lat = cJSON_GetObjectItemCaseSensitive(item, "weatherLatitude");
                cJSON *lon = cJSON_GetObjectItemCaseSensitive(item, "weatherLongitude");
                if (cJSON_IsNumber(lat) && cJSON_IsNumber(lon))
                    snprintf(url, sizeof(url), "https://air-quality-api.open-meteo.com/v1/air-quality?latitude=%.5f&longitude=%.5f&current=us_aqi",
                             lat->valuedouble, lon->valuedouble);
                interval = 1800;
            } else if (home_state) {
                cJSON *endpoint = cJSON_GetObjectItemCaseSensitive(item, "url");
                if (cJSON_IsString(endpoint) && valid_url(endpoint->valuestring) &&
                    !strncmp(endpoint->valuestring, "https://", 8) && valid_home_token(home_token))
                    strlcpy(url, endpoint->valuestring, sizeof(url));
                interval = 30;
            } else if (!strcmp(kind, "weather_alerts")) {
                cJSON *lat = cJSON_GetObjectItemCaseSensitive(item, "weatherLatitude");
                cJSON *lon = cJSON_GetObjectItemCaseSensitive(item, "weatherLongitude");
                if (cJSON_IsNumber(lat) && cJSON_IsNumber(lon))
                    snprintf(url, sizeof(url), "https://api.weather.gov/alerts/active?point=%.5f,%.5f&status=actual",
                             lat->valuedouble, lon->valuedouble);
                interval = 60;
            } else if (!strcmp(kind, "exchange_rate")) {
                if (cJSON_IsString(currency_base) && cJSON_IsString(currency_quote) &&
                    goopad_currency_code(currency_base->valuestring) && goopad_currency_code(currency_quote->valuestring) &&
                    strcmp(currency_base->valuestring, currency_quote->valuestring))
                    snprintf(url, sizeof(url), "https://api.frankfurter.dev/v2/rate/%s/%s",
                             currency_base->valuestring, currency_quote->valuestring);
                interval = 21600;
            } else if (!strcmp(kind, "crypto") && valid_market_text(symbol, false)) {
                snprintf(url, sizeof(url), "https://api.coinbase.com/v2/prices/%s-USD/spot", symbol->valuestring);
                interval = 300;
            } else if (!strcmp(kind, "stock") && valid_market_text(symbol, false)) {
                if (finnhub) {
                    if (valid_finnhub_key(finnhub_key))
                        snprintf(url, sizeof(url), "https://finnhub.io/api/v1/quote?symbol=%s", symbol->valuestring);
                    interval = 60;
                } else {
                    cJSON *key = cJSON_GetObjectItemCaseSensitive(item, "stockKey");
                    if (valid_market_text(key, true))
                        snprintf(url, sizeof(url), "https://www.alphavantage.co/query?function=GLOBAL_QUOTE&symbol=%s&apikey=%s", symbol->valuestring, key->valuestring);
                    interval = 21600;
                }
            } else if (!strcmp(kind, "github")) {
                cJSON *user = cJSON_GetObjectItemCaseSensitive(item, "githubUser");
                if (valid_github_user(user))
                    snprintf(url, sizeof(url), "https://api.github.com/users/%s", user->valuestring);
                interval = 1800;
            }
            char identity[384];
            snprintf(identity, sizeof(identity), "%s|%s", url,
                     finnhub && valid_finnhub_key(finnhub_key) ? finnhub_key->valuestring : "");
            if (home_state && valid_home_token(home_token)) {
                unsigned char digest[32];
                size_t digest_len = 0;
                if (psa_crypto_init() != PSA_SUCCESS ||
                    psa_hash_compute(PSA_ALG_SHA_256, (const unsigned char *)home_token->valuestring,
                                     strlen(home_token->valuestring), digest, sizeof(digest), &digest_len) != PSA_SUCCESS ||
                    digest_len != sizeof(digest)) {
                    set_live_text(i, "Home state error");
                    continue;
                }
                size_t used = strlen(identity);
                for (unsigned j = 0; j < sizeof(digest); j++)
                    snprintf(identity + used + j * 2, sizeof(identity) - used - j * 2, "%02x", digest[j]);
            }
            if (strcmp(identity, previous[i])) {
                strlcpy(previous[i], identity, sizeof(previous[i]));
                next[i] = 0;
                alert_updated[i] = 0;
                home_updated[i] = 0;
                waiting_for_ready[i] = false;
                set_live_waiting(i, url[0] ? "Updating..." : "Waiting for data");
            }
            int64_t now = esp_timer_get_time() / 1000000;
            if (!strcmp(kind, "weather_alerts") && alert_updated[i] && now - alert_updated[i] > 180)
                set_live_text(i, "NWS alerts stale");
            if (home_state && home_updated[i] && now - home_updated[i] > 90)
                set_live_text(i, "Home state stale");
            if (waiting_for_ready[i] && connected && !time_ready)
                set_live_waiting(i, "Waiting for time");
            goopad_network_resume_waiting(connected && time_ready, &waiting_for_ready[i], &next[i]);
            if (waiting_for_ready[i]) pending_ready = true;
            if (!url[0] || fetched || now < next[i]) continue;
            if (!strcmp(kind, "weather_alerts") && now < next_nws_request) {
                next[i] = next_nws_request;
                continue;
            }
            fetched = true;
            next[i] = now + interval;
            if (!s_connected) {
                set_live_waiting(i, "Wi-Fi unavailable");
                waiting_for_ready[i] = pending_ready = true;
                next[i] = now + 30;
                continue;
            }
            if (time(NULL) < 1577836800) {
                set_live_waiting(i, "Waiting for time");
                waiting_for_ready[i] = pending_ready = true;
                next[i] = now + 10;
                continue;
            }
            if (network_begin_background(NETWORK_WIDGET) != ESP_OK) {
                set_live_waiting(i, "Update deferred");
                next[i] = now + 1;
                retry_soon = true;
                continue;
            }
            if (!strcmp(kind, "weather_alerts")) next_nws_request = now + 30;
            memset(data, 0, 4097);
            esp_http_client_config_t config = {.url = url, .timeout_ms = 250, .is_async = true,
                                               .disable_auto_redirect = finnhub || home_state};
            client_tls(&config, NULL);
            esp_http_client_handle_t client = esp_http_client_init(&config);
            esp_err_t setup = client ? ESP_OK : ESP_ERR_NO_MEM;
            if (setup == ESP_OK && (!strcmp(kind, "github") || !strcmp(kind, "weather_alerts")))
                setup = esp_http_client_set_header(client, "User-Agent", "GooPad (https://github.com/jeromedontdev/GooPad)");
            if (setup == ESP_OK && finnhub)
                setup = esp_http_client_set_header(client, "X-Finnhub-Token", finnhub_key->valuestring);
            char bearer[520];
            if (setup == ESP_OK && home_state) {
                snprintf(bearer, sizeof(bearer), "Bearer %s", home_token->valuestring);
                setup = esp_http_client_set_header(client, "Authorization", bearer);
            }
            bool preempted = false;
            int status = 0;
            bool alerts = !strcmp(kind, "weather_alerts");
            char *alert_data = alerts ? heap_caps_malloc(65537, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : NULL;
            if (alerts && !alert_data && setup == ESP_OK) setup = ESP_ERR_NO_MEM;
            char *response = alerts ? alert_data : data;
            /* Media wins admission, not cancellation of an admitted request:
             * a 500 ms poll must not repeatedly restart a slower TLS handshake. */
            esp_err_t result = setup == ESP_OK ? goopad_network_background_get(client, response, alerts ? 65536 : 4096, 10000, !finnhub && !home_state,
                network_priority_pending, &status, &preempted) : setup;
            if (client) esp_http_client_cleanup(client);
            network_end();
            if (preempted) {
                free(alert_data);
                next[i] = now + 1;
                retry_soon = true;
                continue;
            }
            cJSON *payload = result == ESP_OK && status == 200 ? cJSON_Parse(response) : NULL;
            free(alert_data);
            char text[64] = "Data unavailable";
            bool weather_valid = false;
            if (home_state) {
                strlcpy(text, "Home unavailable", sizeof(text));
                cJSON *state = payload ? cJSON_GetObjectItemCaseSensitive(payload, "state") : NULL;
                if (cJSON_IsString(state) && state->valuestring[0] && strlen(state->valuestring) <= 48) {
                    bool printable = true;
                    for (const unsigned char *p = (const unsigned char *)state->valuestring; *p; p++)
                        if (*p < 32 || *p == 127) printable = false;
                    if (printable) {
                        snprintf(text, sizeof(text), "%s", state->valuestring);
                        home_updated[i] = now;
                    }
                }
                if (!strcmp(text, "Home unavailable")) home_updated[i] = 0;
            } else if (payload && !strcmp(kind, "weather")) {
                cJSON *current = cJSON_GetObjectItemCaseSensitive(payload, "current");
                cJSON *temperature = current ? cJSON_GetObjectItemCaseSensitive(current, "temperature_2m") : NULL;
                cJSON *code = current ? cJSON_GetObjectItemCaseSensitive(current, "weather_code") : NULL;
                cJSON *unit = cJSON_GetObjectItemCaseSensitive(item, "temperatureUnit");
                if (cJSON_IsNumber(temperature) && isfinite(temperature->valuedouble) &&
                    temperature->valuedouble >= -150 && temperature->valuedouble <= 200 &&
                    cJSON_IsNumber(code) && code->valuedouble == code->valueint && code->valueint >= 0 && code->valueint <= 99) {
                    snprintf(text, sizeof(text), "%.0f %s\n%s", temperature->valuedouble,
                             cJSON_IsString(unit) && !strcmp(unit->valuestring, "F") ? "F" : "C",
                             weather_description(code->valueint));
                    portENTER_CRITICAL(&s_live_lock);
                    s_weather_code[i] = code->valueint + 1;
                    portEXIT_CRITICAL(&s_live_lock);
                    weather_valid = true;
                }
            } else if (payload && !strcmp(kind, "air_quality")) {
                cJSON *current = cJSON_GetObjectItemCaseSensitive(payload, "current");
                cJSON *aqi = current ? cJSON_GetObjectItemCaseSensitive(current, "us_aqi") : NULL;
                if (cJSON_IsNumber(aqi) && isfinite(aqi->valuedouble) && aqi->valuedouble >= 0 && aqi->valuedouble <= 1000)
                    snprintf(text, sizeof(text), "Air quality\nAQI %.0f", aqi->valuedouble);
            } else if (alerts) {
                strlcpy(text, "NWS unavailable", sizeof(text));
                if (payload && goopad_nws_alert_text(payload, text, sizeof(text))) alert_updated[i] = now;
            } else if (payload && !strcmp(kind, "exchange_rate")) {
                double rate;
                char date[11];
                if (goopad_frankfurter_rate(payload, currency_base->valuestring, currency_quote->valuestring, &rate, date))
                    snprintf(text, sizeof(text), "%s/%s\n%.6g\n%s", currency_base->valuestring,
                             currency_quote->valuestring, rate, date);
            } else if (payload && !strcmp(kind, "github")) {
                cJSON *repos = cJSON_GetObjectItemCaseSensitive(payload, "public_repos");
                cJSON *followers = cJSON_GetObjectItemCaseSensitive(payload, "followers");
                if (cJSON_IsNumber(repos) && cJSON_IsNumber(followers) &&
                    repos->valuedouble >= 0 && repos->valuedouble <= 10000000 &&
                    followers->valuedouble >= 0 && followers->valuedouble <= 100000000)
                    snprintf(text, sizeof(text), "%d repos\n%d followers", repos->valueint, followers->valueint);
            } else if (payload && cJSON_IsString(symbol)) {
                double amount;
                if (finnhub) {
                    if (goopad_finnhub_quote(payload, &amount))
                        snprintf(text, sizeof(text), "%s\n$%.2f", symbol->valuestring, amount);
                    else strlcpy(text, "No quote\nCheck symbol", sizeof(text));
                } else {
                    cJSON *group = cJSON_GetObjectItemCaseSensitive(payload, !strcmp(kind, "stock") ? "Global Quote" : "data");
                    cJSON *price = group ? cJSON_GetObjectItemCaseSensitive(group, !strcmp(kind, "stock") ? "05. price" : "amount") : NULL;
                    if (market_price(price, &amount))
                        snprintf(text, sizeof(text), "%s\n$%.2f", symbol->valuestring, amount);
                    else if (!strcmp(kind, "stock") && cJSON_IsObject(group))
                        strlcpy(text, "No quote\nCheck symbol", sizeof(text));
                }
            }
            cJSON *note = payload ? cJSON_GetObjectItemCaseSensitive(payload, "Note") : NULL;
            cJSON *information = payload ? cJSON_GetObjectItemCaseSensitive(payload, "Information") : NULL;
            cJSON *provider_error = payload ? cJSON_GetObjectItemCaseSensitive(payload, "Error Message") : NULL;
            const char *stock_error = !finnhub && !strcmp(kind, "stock") ? goopad_stock_error(status,
                cJSON_IsString(note) ? note->valuestring : NULL,
                cJSON_IsString(information) ? information->valuestring : NULL,
                cJSON_IsString(provider_error) ? provider_error->valuestring : NULL) : NULL;
            const char *finnhub_error = finnhub ? goopad_finnhub_error(status, payload) : NULL;
            if (finnhub_error)
                strlcpy(text, finnhub_error, sizeof(text));
            else if (stock_error)
                strlcpy(text, stock_error, sizeof(text));
            else if (status == 429 || (payload && (note || information)))
                strlcpy(text, "Provider limit\nor key error", sizeof(text));
            if (!strcmp(text, "Data unavailable") && (result != ESP_OK || status >= 500))
                next[i] = now + 300;
            if (!strcmp(text, "Data unavailable")) ESP_LOGW(TAG, "Live widget %u unavailable (HTTP %d)", i + 1, status);
            goopad_diag_record(DIAG_WIDGET_RESULT, result, status);
            if (!strcmp(kind, "weather") && !weather_valid) set_live_waiting(i, text);
            else set_live_text(i, text);
            cJSON_Delete(payload);
        }
        cJSON_Delete(root);
        vTaskDelay(pdMS_TO_TICKS(retry_soon || pending_ready ? 1000 : fetched ? 500 : 5000));
    }
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
    cJSON *certificate = root ? cJSON_GetObjectItemCaseSensitive(root, "certificate") : NULL;
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
    valid = valid && !strncmp(url->valuestring, "https://", 8) &&
            valid_companion_certificate(certificate);
    companion_reply_t reply = {.data = heap_caps_calloc(1, 2049, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT), .capacity = 2048};
    esp_http_client_config_t config = {
        .url = valid ? url->valuestring : NULL, .timeout_ms = 4000,
        .disable_auto_redirect = true,
        .event_handler = companion_response, .user_data = &reply,
    };
    client_tls(&config, valid ? certificate->valuestring : NULL);
    esp_err_t admission = !reply.data ? ESP_ERR_NO_MEM :
        valid ? network_begin_foreground(pdMS_TO_TICKS(2000)) : ESP_ERR_INVALID_ARG;
    bool acquired = admission == ESP_OK;
    esp_http_client_handle_t client = acquired ? esp_http_client_init(&config) : NULL;
    esp_err_t err = acquired ? ESP_ERR_NO_MEM : admission;
    int status = 0;
    if (client) {
        esp_http_client_set_header(client, "X-GooPad-Token", token->valuestring);
        err = esp_http_client_perform(client);
        status = esp_http_client_get_status_code(client);
        esp_http_client_cleanup(client);
    }
    if (acquired) network_end();
    cJSON *response = err == ESP_OK && !reply.overflow ? cJSON_Parse(reply.data) : NULL;
    bool ok = status == 200 && cJSON_IsObject(response);
    cJSON_Delete(response);
    cJSON_Delete(root);
    free(body);
    esp_err_t result;
    if (!valid) result = httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
        "Use HTTPS and Copy secure pairing from the updated Companion, including its public certificate.");
    else if (!acquired || !client) {
        ESP_LOGW(TAG, "Companion check deferred: %s", esp_err_to_name(err));
        httpd_resp_set_status(req, "503 Service Unavailable");
        result = httpd_resp_sendstr(req, "GooPad is busy or low on memory. Try the Companion check again.");
    }
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
        "", "CPU", "RAM", "RAM used", "GPU", "GPU RAM", "CPU temp", "GPU temp",
        "Download", "Upload", "Fan"
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
    } else if (index == 6 || index == 7) {
        snprintf(out, capacity, "%s\n%.0f %s", labels[index],
                 fahrenheit ? values[index] * 9 / 5 + 32 : values[index], fahrenheit ? "F" : "C");
    } else if (index == 3 || index == 5) {
        snprintf(out, capacity, "%s\n%.1f GiB", labels[index], values[index] / (1024.0 * 1024 * 1024));
    } else if (index == 8 || index == 9) {
        snprintf(out, capacity, "%s\n%.1f Mbps", labels[index], values[index] * 8 / 1000000.0);
    } else if (index == 10) {
        snprintf(out, capacity, "%s\n%.0f RPM", labels[index], values[index]);
    } else snprintf(out, capacity, "%s\n%.0f%%", labels[index], values[index]);
}

static void monitor_task(void *arg)
{
    (void)arg;
    char *config_json = heap_caps_malloc(CONFIG_MAX_BYTES + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    char *snapshot_config = heap_caps_malloc(CONFIG_MAX_BYTES + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    char *data = heap_caps_calloc(1, 4097, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    char *media_text = heap_caps_calloc(1, MEDIA_TEXT_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    char *published_media = heap_caps_calloc(1, MEDIA_TEXT_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!config_json || !snapshot_config || !data || !media_text || !published_media) {
        free(config_json); free(snapshot_config); free(data); free(media_text); free(published_media);
        ESP_LOGE(TAG, "PC monitor memory unavailable");
        portENTER_CRITICAL(&s_monitor_lock);
        strlcpy(s_monitor_text, "Monitor memory error", sizeof(s_monitor_text));
        portEXIT_CRITICAL(&s_monitor_lock);
        vTaskDelete(NULL);
        return;
    }
    bool have_snapshot = false;
    esp_http_client_handle_t client = NULL;
    char *connection_data = NULL, *cached_url = NULL, *cached_secret = NULL, *cached_certificate = NULL;
    int64_t snapshot_time = 0;
    esp_err_t previous_error = ESP_OK;
    int previous_status = 200, previous_tls_error = 0, previous_tls_flags = 0;
    portENTER_CRITICAL(&s_live_lock);
    s_media_text = published_media;
    portEXIT_CRITICAL(&s_live_lock);
    while (true) {
        int64_t cycle_start = esp_timer_get_time();
        goopad_get_config(config_json, CONFIG_MAX_BYTES + 1);
        cJSON *root = cJSON_Parse(config_json);
        cJSON *macros = root ? cJSON_GetObjectItemCaseSensitive(root, "macros") : NULL;
        cJSON *target = NULL;
        bool media_active = false;
        for (cJSON *item = macros ? macros->child : NULL; item; item = item->next) {
            cJSON *mode = cJSON_GetObjectItemCaseSensitive(item, "actionMode");
            cJSON *widget = cJSON_GetObjectItemCaseSensitive(item, "widget");
            if (cJSON_IsString(mode) && !strcmp(mode->valuestring, "widget") &&
                cJSON_IsString(widget) && (!strcmp(widget->valuestring, "pc_monitor") ||
                                          !strcmp(widget->valuestring, "now_playing"))) {
                if (!target) target = item;
                if (!strcmp(widget->valuestring, "now_playing")) media_active = true;
            }
        }
        char text[64] = "Set PC pairing";
        strlcpy(media_text, "PC unavailable", MEDIA_TEXT_BYTES);
        double values[GOOPAD_PC_STAT_COUNT] = {0};
        bool valid[GOOPAD_PC_STAT_COUNT] = {false}, sampling = false;
        bool deferred = false, connection_needed = false;
        if (!root) strlcpy(text, "Monitor config error", sizeof(text));
        if (target) {
            cJSON *url = cJSON_GetObjectItemCaseSensitive(target, "url");
            cJSON *secret = cJSON_GetObjectItemCaseSensitive(target, "secret");
            cJSON *certificate = cJSON_GetObjectItemCaseSensitive(root, "companionCertificate");
            char shared_url[180];
            const char *effective_url = cJSON_IsString(url) ? url->valuestring : NULL;
            const char *effective_secret = cJSON_IsString(secret) ? secret->valuestring : NULL;
            cJSON *shared_pc = cJSON_GetObjectItemCaseSensitive(target, "sharedPc");
            if (shared_pc && goopad_companion_resolve(root, target, shared_url, sizeof(shared_url), &effective_secret))
                effective_url = shared_url;
            else if (shared_pc) effective_url = NULL;
            bool paired = valid_url(effective_url) && !strncmp(effective_url, "https://", 8) &&
                          effective_secret && valid_companion_certificate(certificate);
            bool time_ready = time(NULL) >= 1577836800;
            connection_needed = paired && s_connected;
            strlcpy(text, paired ? "PC unavailable" : "Pair HTTPS Companion", sizeof(text));
            if (s_connected && paired && !time_ready) {
                strlcpy(text, "Waiting for time", sizeof(text));
                strlcpy(media_text, "Waiting for time", MEDIA_TEXT_BYTES);
            }
            if (s_connected && paired && time_ready && media_active) {
                portENTER_CRITICAL(&s_action_lock);
                s_media_waiting = true;
                portEXIT_CRITICAL(&s_action_lock);
            }
            bool acquired = false;
            if (s_connected && paired && time_ready && !network_priority_pending()) {
                acquired = (media_active ? network_begin(pdMS_TO_TICKS(500), NETWORK_PC) :
                            network_begin_background(NETWORK_PC)) == ESP_OK;
                if (acquired && network_priority_pending()) { network_end(); acquired = false; }
            }
            portENTER_CRITICAL(&s_action_lock);
            s_media_waiting = false;
            portEXIT_CRITICAL(&s_action_lock);
            if (s_connected && paired && time_ready && !acquired) {
                deferred = true;
                strlcpy(text, "Update deferred", sizeof(text));
            }
            if (acquired) {
                memset(data, 0, 4097);
                if (client && (strcmp(cached_url, effective_url) || strcmp(cached_secret, effective_secret) ||
                               strcmp(cached_certificate, certificate->valuestring))) {
                    esp_http_client_cleanup(client);
                    client = NULL;
                    free(connection_data);
                    connection_data = NULL;
                }
                if (!client) {
                    size_t url_bytes = strlen(effective_url) + 1, secret_bytes = strlen(effective_secret) + 1;
                    size_t cert_bytes = strlen(certificate->valuestring) + 1;
                    connection_data = heap_caps_malloc(url_bytes + secret_bytes + cert_bytes,
                                                       MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                    if (connection_data) {
                        cached_url = connection_data;
                        cached_secret = cached_url + url_bytes;
                        cached_certificate = cached_secret + secret_bytes;
                        memcpy(cached_url, effective_url, url_bytes);
                        memcpy(cached_secret, effective_secret, secret_bytes);
                        memcpy(cached_certificate, certificate->valuestring, cert_bytes);
                        esp_http_client_config_t config = {
                            .url = cached_url, .timeout_ms = 250, .is_async = true,
                            .disable_auto_redirect = true,
                        };
                        /* ESP-IDF 6.1 reapplies saved sessions on each async
                         * handshake step, failing with SSL_FEATURE_UNAVAILABLE.
                         * Keep certificate verification, but use fresh TLS. */
                        client_tls(&config, cached_certificate);
                        client = esp_http_client_init(&config);
                        if (!client) { free(connection_data); connection_data = NULL; }
                    }
                }
                if (client) {
                    esp_http_client_set_header(client, "X-GooPad-Token", effective_secret);
                    bool preempted = false;
                    int status = 0;
                    int64_t request_start = esp_timer_get_time();
                    /* Poll cadence is not a TLS deadline: cold verified
                     * handshakes can exceed 1.5 s under render/network load. */
                    esp_err_t err = goopad_network_background_get(client, data, 4096, 8000, false,
                        network_priority_pending, &status, &preempted);
                    int tls_error = 0, tls_flags = 0;
                    esp_http_client_get_and_clear_last_tls_error(client, &tls_error, &tls_flags);
                    esp_err_t close_error = esp_http_client_close(client);
                    if (close_error != ESP_OK) {
                        ESP_LOGW(TAG, "PC monitor connection close failed: %s", esp_err_to_name(close_error));
                        esp_http_client_cleanup(client);
                        client = NULL;
                        free(connection_data);
                        connection_data = NULL;
                    }
                    if (preempted) {
                        deferred = true;
                        strlcpy(text, "Update deferred", sizeof(text));
                    } else {
                        if ((err != ESP_OK || status != 200) &&
                            (err != previous_error || status != previous_status ||
                             tls_error != previous_tls_error || tls_flags != previous_tls_flags))
                            ESP_LOGW(TAG, "PC monitor request failed: %s, HTTP %d, TLS %d, flags %d, %lld ms",
                                     esp_err_to_name(err), status, tls_error, tls_flags,
                                     (esp_timer_get_time() - request_start) / 1000);
                        if (err == ESP_ERR_TIMEOUT) deferred = true;
                        previous_error = err;
                        previous_status = status;
                        previous_tls_error = tls_error;
                        previous_tls_flags = tls_flags;
                    }
                    cJSON *snapshot = err == ESP_OK && status == 200 ? cJSON_Parse(data) : NULL;
                    cJSON *media_status = snapshot ? cJSON_GetObjectItemCaseSensitive(snapshot, "mediaStatus") : NULL;
                    cJSON *media_title = snapshot ? cJSON_GetObjectItemCaseSensitive(snapshot, "mediaTitle") : NULL;
                    cJSON *media_artist = snapshot ? cJSON_GetObjectItemCaseSensitive(snapshot, "mediaArtist") : NULL;
                    if (cJSON_IsString(media_status)) {
                        if ((!strcmp(media_status->valuestring, "Playing") || !strcmp(media_status->valuestring, "Paused")) &&
                            cJSON_IsString(media_title) && cJSON_IsString(media_artist) && media_title->valuestring[0]) {
                            if (strlen(media_title->valuestring) <= 320 && strlen(media_artist->valuestring) <= 320) {
                                snprintf(media_text, MEDIA_TEXT_BYTES, "%s%s%s%s",
                                         !strcmp(media_status->valuestring, "Paused") ? "Paused\n" : "",
                                         media_title->valuestring, media_artist->valuestring[0] ? "\n" : "",
                                         media_artist->valuestring);
                            } else {
                                ESP_LOGW(TAG, "Companion media metadata exceeds UTF-8 byte limits");
                                strlcpy(media_text, "Invalid media metadata", MEDIA_TEXT_BYTES);
                            }
                        } else if (!strcmp(media_status->valuestring, "Media disabled")) strlcpy(media_text, "Media disabled", MEDIA_TEXT_BYTES);
                        else if (!strcmp(media_status->valuestring, "Media unavailable")) strlcpy(media_text, "Media unavailable", MEDIA_TEXT_BYTES);
                        else if (!strcmp(media_status->valuestring, "Sampling media")) strlcpy(media_text, "Sampling media", MEDIA_TEXT_BYTES);
                        else strlcpy(media_text, "No media", MEDIA_TEXT_BYTES);
                        cJSON *revision = cJSON_GetObjectItemCaseSensitive(snapshot, "mediaRevision");
                        cJSON *age = cJSON_GetObjectItemCaseSensitive(snapshot, "mediaAgeMs");
                        portENTER_CRITICAL(&s_live_lock);
                        s_media_poll_ms = (esp_timer_get_time() - request_start) / 1000;
                        s_media_revision = cJSON_IsNumber(revision) && revision->valuedouble >= 0 &&
                            revision->valuedouble <= UINT32_MAX ? (uint32_t)revision->valuedouble : 0;
                        s_media_age_ms = cJSON_IsNumber(age) && age->valuedouble >= 0 &&
                            age->valuedouble <= UINT32_MAX ? (uint32_t)age->valuedouble : 0;
                        portEXIT_CRITICAL(&s_live_lock);
                    }
                    cJSON *memory = snapshot ? cJSON_GetObjectItemCaseSensitive(snapshot, "memoryPercent") : NULL;
                    if (cJSON_IsNumber(memory) && memory->valuedouble >= 0 && memory->valuedouble <= 100) {
                        static const char *fields[GOOPAD_PC_STAT_COUNT] = {
                            "", "cpuPercent", "memoryPercent", "memoryUsedBytes", "gpuPercent",
                            "gpuMemoryUsedBytes", "cpuTemperatureC", "gpuTemperatureC",
                            "downloadBytesPerSec", "uploadBytesPerSec", "fanRpm"
                        };
                        text[0] = '\0';
                        for (unsigned i = 1; i < GOOPAD_PC_STAT_COUNT; i++) {
                            cJSON *value = cJSON_GetObjectItemCaseSensitive(snapshot, fields[i]);
                            double lower = i == 6 || i == 7 ? -50 : 0;
                            double upper = i == 6 || i == 7 ? 200 : i == 3 || i == 5 ? 1e15 :
                                           i == 8 || i == 9 ? 1e12 : i == 10 ? 100000 : 100;
                            valid[i] = cJSON_IsNumber(value) && value->valuedouble >= lower && value->valuedouble <= upper;
                            if (valid[i]) values[i] = value->valuedouble;
                            if (value && !cJSON_IsNull(value) && !valid[i])
                                strlcpy(text, "Invalid PC data", sizeof(text));
                            if (i == 1) sampling = cJSON_IsNull(value);
                        }
                    }
                    else if (snapshot) strlcpy(text, "Invalid PC data", sizeof(text));
                    cJSON_Delete(snapshot);
                } else {
                    if (previous_error != ESP_ERR_NO_MEM) ESP_LOGW(TAG, "PC monitor connection memory unavailable");
                    previous_error = ESP_ERR_NO_MEM;
                    previous_status = 0;
                    strlcpy(text, "PC connection memory error", sizeof(text));
                    strlcpy(media_text, "Media connection memory error", MEDIA_TEXT_BYTES);
                }
                network_end();
            }
        }
        if (!connection_needed && client) {
            esp_http_client_cleanup(client);
            client = NULL;
            free(connection_data);
            connection_data = NULL;
        }
        int64_t now = esp_timer_get_time();
        if (deferred && have_snapshot && !strcmp(snapshot_config, config_json) &&
            now - snapshot_time < 15 * 1000000LL) {
            cJSON_Delete(root);
            vTaskDelay(pdMS_TO_TICKS(media_active ? 250 : 1000));
            continue;
        }
        uint32_t media_mask = 0;
        for (unsigned i = 0; i < 32; i++) {
            cJSON *item = cJSON_IsArray(macros) ? cJSON_GetArrayItem(macros, i) : NULL;
            cJSON *mode = item ? cJSON_GetObjectItemCaseSensitive(item, "actionMode") : NULL;
            cJSON *widget = item ? cJSON_GetObjectItemCaseSensitive(item, "widget") : NULL;
            if (cJSON_IsString(mode) && !strcmp(mode->valuestring, "widget") &&
                cJSON_IsString(widget) && !strcmp(widget->valuestring, "now_playing"))
                media_mask |= UINT32_C(1) << i;
        }
        portENTER_CRITICAL(&s_live_lock);
        strlcpy(published_media, media_text, MEDIA_TEXT_BYTES);
        s_media_mask = media_mask;
        portEXIT_CRITICAL(&s_live_lock);
        if (!text[0]) {
            strlcpy(snapshot_config, config_json, CONFIG_MAX_BYTES + 1);
            snapshot_time = now;
            have_snapshot = true;
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
        int64_t elapsed = (esp_timer_get_time() - cycle_start) / 1000;
        unsigned interval = media_active ? 500 : deferred ? 1000 : 2000;
        if (elapsed < interval) vTaskDelay(pdMS_TO_TICKS(interval - elapsed));
        else vTaskDelay(pdMS_TO_TICKS(10));
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

#ifdef CONFIG_GOOPAD_HOMEKIT
static esp_err_t homekit_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_type(req, "application/json");
    if (req->method == HTTP_GET) {
        char reply[128];
        snprintf(reply, sizeof(reply), "{\"active\":%s,\"starting\":%s,\"error\":\"%s\"}",
                 s_homekit_active ? "true" : "false", s_homekit_starting ? "true" : "false",
                 esp_err_to_name(s_homekit_error));
        return httpd_resp_sendstr(req, reply);
    }
    if (!s_homekit_active)
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "HomeKit bridge is not running; save a switch and restart GooPad");
    char code[11];
    esp_err_t err = goopad_homekit_open_pairing(300, code);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "HomeKit pairing could not be opened: %s", esp_err_to_name(err));
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Pairing unavailable; check whether GooPad is already paired");
    }
    char reply[64];
    snprintf(reply, sizeof(reply), "{\"code\":\"%s\",\"seconds\":300}", code);
    sodium_memzero(code, sizeof(code));
    err = httpd_resp_sendstr(req, reply);
    sodium_memzero(reply, sizeof(reply));
    return err;
}

static void homekit_task(void *arg)
{
    (void)arg;
    goopad_homekit_switch_t switches[GOOPAD_HOMEKIT_MAX_SWITCHES];
    size_t count = 0;
    xSemaphoreTake(s_config_lock, portMAX_DELAY);
    cJSON *root = cJSON_Parse(s_config);
    bool valid = root && homekit_switches(root, switches, &count);
    cJSON_Delete(root);
    xSemaphoreGive(s_config_lock);
    esp_err_t err = valid ? (count ? ESP_OK : ESP_ERR_NOT_FOUND) : ESP_ERR_INVALID_STATE;
    if (err == ESP_OK) err = goopad_homekit_storage_init();
    goopad_homekit_credentials_t credentials = {0};
    if (err == ESP_OK) err = goopad_homekit_credentials_load(&credentials);
    if (err == ESP_OK)
        err = goopad_homekit_start(switches, count, credentials.code, credentials.setup_id);
    sodium_memzero(&credentials, sizeof(credentials));
    s_homekit_error = err;
    s_homekit_active = err == ESP_OK;
    s_homekit_starting = false;
    if (err != ESP_OK && err != ESP_ERR_NOT_FOUND)
        ESP_LOGE(TAG, "HomeKit bridge failed to start: %s", esp_err_to_name(err));
    vTaskDelete(NULL);
}
#endif

static esp_err_t diagnostics_handler(httpd_req_t *req)
{
    esp_err_t auth = auth_required(req);
    if (auth != ESP_OK) return auth;
    char *json = goopad_diag_json();
    if (!json) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Diagnostics unavailable");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    esp_err_t err = httpd_resp_sendstr(req, json);
    cJSON_free(json);
    return err;
}

static esp_err_t start_server(void)
{
    if (s_server) return ESP_OK;
    httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
    config.httpd.max_open_sockets = 6;
    config.httpd.core_id = 1;
    /* SD streaming and TLS must not preempt LVGL (4) or its renderer (3). */
    config.httpd.task_priority = 2;
    const httpd_uri_t routes[] = {
        {.uri = "/", .method = HTTP_GET, .handler = index_handler},
        {.uri = "/font-licenses", .method = HTTP_GET, .handler = font_notice_handler},
        {.uri = "/preview-font", .method = HTTP_GET, .handler = preview_font_handler},
        {.uri = "/api/config", .method = HTTP_GET, .handler = config_get_handler},
        {.uri = "/api/config", .method = HTTP_POST, .handler = config_post_handler},
        {.uri = "/api/password", .method = HTTP_POST, .handler = password_post_handler},
        {.uri = "/api/trigger", .method = HTTP_POST, .handler = trigger_handler},
        {.uri = "/api/wifi/join", .method = HTTP_POST, .handler = wifi_join_handler},
        {.uri = "/api/backup", .method = HTTP_POST, .handler = backup_handler},
        {.uri = "/api/backup", .method = HTTP_GET, .handler = backup_handler},
        {.uri = "/api/status", .method = HTTP_GET, .handler = status_handler},
        {.uri = "/api/diagnostics", .method = HTTP_GET, .handler = diagnostics_handler},
        {.uri = "/api/ota", .method = HTTP_POST, .handler = ota_upload_handler},
        {.uri = "/api/update", .method = HTTP_GET, .handler = update_handler},
        {.uri = "/api/update", .method = HTTP_POST, .handler = update_handler},
        {.uri = "/api/ota/sd", .method = HTTP_POST, .handler = ota_sd_handler},
        {.uri = "/api/icons", .method = HTTP_GET, .handler = icons_handler},
        {.uri = "/api/resources", .method = HTTP_POST, .handler = resource_upload_handler},
        {.uri = "/api/saved-icons", .method = HTTP_GET, .handler = saved_icons_handler},
        {.uri = "/api/icon-match", .method = HTTP_GET, .handler = icon_match_handler},
        {.uri = "/api/icon", .method = HTTP_POST, .handler = icon_upload_handler},
        {.uri = "/api/icon", .method = HTTP_GET, .handler = icon_get_handler},
        {.uri = "/api/icon", .method = HTTP_DELETE, .handler = icon_delete_handler},
        {.uri = "/api/time", .method = HTTP_POST, .handler = time_handler},
        {.uri = "/api/sound", .method = HTTP_POST, .handler = sound_upload_handler},
        {.uri = "/api/sound", .method = HTTP_DELETE, .handler = sound_delete_handler},
        {.uri = "/api/controls", .method = HTTP_POST, .handler = controls_handler},
        {.uri = "/api/background", .method = HTTP_DELETE, .handler = background_delete_handler},
        {.uri = "/api/companion", .method = HTTP_POST, .handler = companion_status_handler},
#ifdef CONFIG_GOOPAD_HOMEKIT
        {.uri = "/api/homekit", .method = HTTP_GET, .handler = homekit_handler},
        {.uri = "/api/homekit", .method = HTTP_POST, .handler = homekit_handler},
#endif
    };
    config.httpd.max_uri_handlers = sizeof(routes) / sizeof(routes[0]);
    esp_err_t err = goopad_tls_configure(&config, s_ip_address);
    if (err == ESP_OK) err = httpd_ssl_start(&s_server, &config);
    if (err != ESP_OK) return err;
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
        wifi_event_sta_disconnected_t *disconnected = data;
        goopad_diag_record(DIAG_WIFI_DISCONNECTED, disconnected ? disconnected->reason : 0, 0);
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
        /* Network services and HomeKit need an assigned address. Start them
         * outside the event callback to avoid blocking the Wi-Fi event loop. */
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
#ifdef CONFIG_GOOPAD_HOMEKIT
            if (!s_homekit_active && !s_homekit_starting) {
                s_homekit_starting = true;
                if (xTaskCreatePinnedToCore(homekit_task, "homekit_start", 12288, NULL, 4, NULL, 1) != pdPASS) {
                    s_homekit_starting = false;
                    s_homekit_error = ESP_ERR_NO_MEM;
                    ESP_LOGE(TAG, "Could not start HomeKit task");
                }
            }
#endif
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
    s_network_lock = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(s_network_lock ? ESP_OK : ESP_ERR_NO_MEM);
    s_action_queue = xQueueCreate(4, sizeof(macro_action_t *));
    if (!s_action_queue) {
        ESP_LOGE(TAG, "Could not create macro action queue");
    } else if (xTaskCreatePinnedToCore(macro_task, "macro_queue", 6144, NULL,
                                       tskIDLE_PRIORITY + 1, NULL, 1) != pdPASS) {
        ESP_LOGE(TAG, "Could not start macro action worker");
        vQueueDelete(s_action_queue);
        s_action_queue = NULL;
    }
    ESP_ERROR_CHECK(goopad_config_storage_init());
    load_config();
    if (xTaskCreatePinnedToCore(monitor_task, "pc_monitor", 6144, NULL, tskIDLE_PRIORITY, NULL, 1) != pdPASS)
        ESP_LOGE(TAG, "Could not start PC monitor worker");
    if (xTaskCreatePinnedToCore(live_widget_task, "live_widgets", 8192, NULL, tskIDLE_PRIORITY, NULL, 1) != pdPASS) {
        ESP_LOGE(TAG, "Could not start live widget worker");
        for (unsigned i = 0; i < 32; i++) set_live_text(i, "Widget worker error");
    }
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
        config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
        config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
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
