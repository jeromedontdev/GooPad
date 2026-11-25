#include "goopad_network.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void goopad_network_resume_waiting(bool ready, bool *waiting, int64_t *next)
{
    if (ready && *waiting) {
        *waiting = false;
        *next = 0;
    }
}

static esp_err_t background_ready(int64_t deadline, bool (*priority_pending)(void), bool *preempted)
{
    if (priority_pending()) {
        *preempted = true;
        return ESP_ERR_INVALID_STATE;
    }
    return esp_timer_get_time() >= deadline ? ESP_ERR_TIMEOUT : ESP_OK;
}

/* Streaming reads permit priority checks between short socket waits without
 * treating a slow response as a complete body. The caller owns the client. */
esp_err_t goopad_network_background_get(esp_http_client_handle_t client, char *data,
                                        size_t capacity, int timeout_ms, bool redirects,
                                        bool (*priority_pending)(void), int *status, bool *preempted)
{
    int64_t deadline = esp_timer_get_time() + (int64_t)timeout_ms * 1000;
    *preempted = false;
    *status = 0;
    for (unsigned redirect = 0; ; redirect++) {
        esp_err_t result;
        do {
            result = background_ready(deadline, priority_pending, preempted);
            if (result != ESP_OK) return result;
            result = esp_http_client_open(client, 0);
            if (result == ESP_ERR_HTTP_EAGAIN) vTaskDelay(pdMS_TO_TICKS(10));
        } while (result == ESP_ERR_HTTP_EAGAIN);
        if (result != ESP_OK) return result;

        int64_t headers;
        do {
            result = background_ready(deadline, priority_pending, preempted);
            if (result != ESP_OK) return result;
            headers = esp_http_client_fetch_headers(client);
            if (headers == -ESP_ERR_HTTP_EAGAIN) vTaskDelay(pdMS_TO_TICKS(10));
        } while (headers == -ESP_ERR_HTTP_EAGAIN);
        if (headers < 0) return ESP_ERR_HTTP_FETCH_HEADER;
        *status = esp_http_client_get_status_code(client);
        if (redirects && (*status == 301 || *status == 302 || *status == 303 ||
                          *status == 307 || *status == 308)) {
            if (redirect >= 10) return ESP_ERR_HTTP_MAX_REDIRECT;
            result = esp_http_client_set_redirection(client);
            if (result != ESP_OK) return result;
            result = esp_http_client_close(client);
            if (result != ESP_OK) return result;
            result = esp_http_client_clear_response_buffer(client);
            if (result != ESP_OK) return result;
            continue;
        }
        size_t length = 0;
        while (true) {
            result = background_ready(deadline, priority_pending, preempted);
            if (result != ESP_OK) return result;
            char extra;
            int count = esp_http_client_read(client, length < capacity ? data + length : &extra,
                                             length < capacity ? capacity - length : 1);
            if (count == -ESP_ERR_HTTP_EAGAIN) {
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }
            if (count < 0) return ESP_ERR_HTTP_INCOMPLETE_DATA;
            if (!count) {
                if (!esp_http_client_is_complete_data_received(client)) return ESP_ERR_HTTP_INCOMPLETE_DATA;
                data[length] = '\0';
                return ESP_OK;
            }
            if (length == capacity) return ESP_ERR_INVALID_SIZE;
            length += count;
        }
    }
}
