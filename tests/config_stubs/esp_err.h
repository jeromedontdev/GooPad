#pragma once
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL 1
#define ESP_ERR_INVALID_ARG 2
#define ESP_ERR_INVALID_STATE 3
#define ESP_ERR_INVALID_SIZE 4
#define ESP_ERR_NO_MEM 5
#define ESP_ERR_NVS_NOT_FOUND 6
#define ESP_ERR_NVS_NOT_ENOUGH_SPACE 7
const char *esp_err_to_name(esp_err_t err);
