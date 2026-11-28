#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_http_client.h"

void goopad_network_resume_waiting(bool ready, bool *waiting, int64_t *next);
typedef esp_err_t (*goopad_network_chunk_fn)(void *context, const char *data, size_t size);
esp_err_t goopad_network_background_get(esp_http_client_handle_t client, char *data,
                                        size_t capacity, int timeout_ms, bool redirects,
                                        bool (*priority_pending)(void), int *status, bool *preempted);
esp_err_t goopad_network_background_stream(esp_http_client_handle_t client, char *buffer,
                                           size_t buffer_size, size_t max_size,
                                           goopad_network_chunk_fn consume, void *context,
                                           int timeout_ms, bool redirects,
                                           bool (*priority_pending)(void), int *status, bool *preempted);
