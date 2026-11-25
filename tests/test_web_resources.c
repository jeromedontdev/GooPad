#include <assert.h>
#include <stdlib.h>
#define GOOPAD_BOARD_HEADER "web_board.h"
#include "../main/goopad_web.c"

typedef struct {
    const char *encoding, *query;
    const char *content_encoding, *content_type;
    int status, fail_after;
    size_t bytes, max_chunk, sends;
} httpd_req_t;

#define HTTPD_400_BAD_REQUEST 400
#define HTTPD_404_NOT_FOUND 404
static const uint8_t recovery_html_start[] = "GooPad recovery";
#define recovery_html_end (recovery_html_start + sizeof(recovery_html_start) - 1)
static esp_err_t httpd_resp_set_type(httpd_req_t *req, const char *type) { req->content_type = type; return ESP_OK; }
static esp_err_t httpd_resp_set_status(httpd_req_t *req, const char *status) { req->status = atoi(status); return ESP_OK; }
static esp_err_t httpd_resp_set_hdr(httpd_req_t *req, const char *key, const char *value)
{
    if (!strcmp(key, "Content-Encoding")) req->content_encoding = value;
    return ESP_OK;
}
static esp_err_t httpd_req_get_hdr_value_str(httpd_req_t *req, const char *key, char *out, size_t size)
{
    (void)key;
    if (!req->encoding || strlen(req->encoding) >= size) return ESP_ERR_NOT_FOUND;
    strcpy(out, req->encoding);
    return ESP_OK;
}
static esp_err_t httpd_req_get_url_query_str(httpd_req_t *req, char *out, size_t size)
{
    if (!req->query || strlen(req->query) >= size) return ESP_ERR_NOT_FOUND;
    strcpy(out, req->query);
    return ESP_OK;
}
static esp_err_t httpd_query_key_value(const char *query, const char *key, char *out, size_t size)
{
    (void)key;
    if (strncmp(query, "name=", 5) || strlen(query + 5) >= size) return ESP_ERR_NOT_FOUND;
    strcpy(out, query + 5);
    return ESP_OK;
}
static esp_err_t httpd_resp_send_chunk(httpd_req_t *req, const char *data, size_t size)
{
    (void)data;
    if (req->fail_after && ++req->sends > (size_t)req->fail_after) return ESP_FAIL;
    req->bytes += size;
    if (size > req->max_chunk) req->max_chunk = size;
    return ESP_OK;
}
static esp_err_t httpd_resp_send(httpd_req_t *req, const char *data, size_t size)
{
    (void)data; req->bytes += size; return ESP_OK;
}
static esp_err_t httpd_resp_sendstr(httpd_req_t *req, const char *text)
{
    return httpd_resp_send(req, text, strlen(text));
}
static esp_err_t httpd_resp_send_err(httpd_req_t *req, int status, const char *text)
{
    req->status = status;
    return httpd_resp_sendstr(req, text);
}
static const char *TAG = "test";
#include "web_handlers.inc"

int main(void)
{
    goopad_web_asset_t asset;
    assert(!goopad_web_known("../../secret"));
    assert(goopad_web_open("../../secret", &asset) == ESP_ERR_NOT_FOUND && !asset.file);
    for (unsigned i = 0; i < sizeof(assets) / sizeof(assets[0]); i++) {
        assert(goopad_web_open(assets[i].name, &asset) == ESP_OK);
        assert(asset.length == assets[i].length);
        httpd_req_t req = {0};
        assert(send_web_asset(&req, &asset) == ESP_OK && !asset.file);
        assert(req.bytes == assets[i].length && req.max_chunk <= 1024);
    }
    assert(hash_handles == 0);
    httpd_req_t gzip = {.encoding = "gzip, deflate"};
    assert(index_handler(&gzip) == ESP_OK && !strcmp(gzip.content_encoding, "gzip"));
    assert(gzip.bytes == assets[0].length && gzip.max_chunk <= 1024);
    httpd_req_t plain = {0};
    assert(index_handler(&plain) == ESP_OK && !plain.content_encoding && plain.bytes == assets[1].length);
    httpd_req_t disabled = {.encoding = "gzip;q=0"};
    assert(index_handler(&disabled) == ESP_OK && !disabled.content_encoding && disabled.bytes == plain.bytes);
    assert(goopad_web_open("index.html", &asset) == ESP_OK);
    httpd_req_t aborted = {.fail_after = 1};
    assert(send_web_asset(&aborted, &asset) == ESP_FAIL && !asset.file && aborted.bytes == 1024);
    httpd_req_t unknown = {.query = "name=../../secret"};
    assert(preview_font_handler(&unknown) == ESP_OK && unknown.status == 404);
    httpd_req_t font = {.query = "name=lobster"};
    assert(preview_font_handler(&font) == ESP_OK && !strcmp(font.content_type, "font/woff2"));
    const char *path = "./goopad/resources/" GOOPAD_WEB_FILE;
    FILE *file = fopen(path, "r+b");
    assert(file && fseek(file, assets[0].offset, SEEK_SET) == 0);
    int original = fgetc(file);
    assert(fseek(file, assets[0].offset, SEEK_SET) == 0 && fputc(original ^ 1, file) != EOF);
    assert(fclose(file) == 0);
    assert(goopad_web_open("index.html.gz", &asset) == ESP_ERR_INVALID_CRC && !asset.file);
    httpd_req_t damaged = {.encoding = "gzip"};
    assert(index_handler(&damaged) == ESP_OK && damaged.status == 503 && !damaged.content_encoding);
    assert(rename(path, "./unavailable.gwp") == 0);
    httpd_req_t missing = {.encoding = "gzip"};
    assert(index_handler(&missing) == ESP_OK && missing.status == 503 && !missing.content_encoding);
    httpd_req_t missing_font = {.query = "name=lobster"};
    assert(preview_font_handler(&missing_font) == ESP_OK && missing_font.status == 503);
    assert(hash_handles == 0);
    assert(rename("./unavailable.gwp", path) == 0);
    file = fopen(path, "r+b");
    assert(file && fseek(file, 0, SEEK_SET) == 0 && fputc('X', file) != EOF && fclose(file) == 0);
    assert(goopad_web_open("index.html", &asset) == ESP_ERR_INVALID_CRC && !asset.file);
    file = fopen(path, "wb");
    assert(file && fwrite("GWP1", 1, 4, file) == 4 && fclose(file) == 0);
    assert(goopad_web_open("index.html", &asset) == ESP_ERR_INVALID_CRC && !asset.file && hash_handles == 0);
    puts("Actual SD web loader/handlers: hashes, gzip/identity, 1 KiB chunks, abort cleanup and 503 recovery passed");
    return 0;
}
