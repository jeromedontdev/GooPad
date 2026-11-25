#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_http_client.h"

void goopad_network_resume_waiting(bool ready, bool *waiting, int64_t *next);
esp_err_t goopad_network_background_get(esp_http_client_handle_t client, char *data,
                                        size_t capacity, int timeout_ms, bool redirects,
                                        bool (*priority_pending)(void), int *status, bool *preempted);
