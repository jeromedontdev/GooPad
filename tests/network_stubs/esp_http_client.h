#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef int esp_err_t;
typedef void *esp_http_client_handle_t;
#define ESP_OK 0
#define ESP_ERR_INVALID_STATE 1
#define ESP_ERR_TIMEOUT 2
#define ESP_ERR_INVALID_SIZE 3
#define ESP_ERR_HTTP_EAGAIN 4
#define ESP_ERR_HTTP_FETCH_HEADER 5
#define ESP_ERR_HTTP_MAX_REDIRECT 6
#define ESP_ERR_HTTP_INCOMPLETE_DATA 7

esp_err_t esp_http_client_open(esp_http_client_handle_t client, int length);
int64_t esp_http_client_fetch_headers(esp_http_client_handle_t client);
int esp_http_client_get_status_code(esp_http_client_handle_t client);
esp_err_t esp_http_client_set_redirection(esp_http_client_handle_t client);
esp_err_t esp_http_client_close(esp_http_client_handle_t client);
esp_err_t esp_http_client_clear_response_buffer(esp_http_client_handle_t client);
int esp_http_client_read(esp_http_client_handle_t client, char *buffer, int length);
bool esp_http_client_is_complete_data_received(esp_http_client_handle_t client);
