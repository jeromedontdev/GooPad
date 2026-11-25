#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

esp_err_t goopad_config_storage_init(void);
esp_err_t goopad_config_storage_load(char *out, size_t capacity, const char *legacy,
                                    bool legacy_available);
esp_err_t goopad_config_storage_save(const char *json);
