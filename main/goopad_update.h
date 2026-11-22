#pragma once

#include "esp_err.h"
#include <stdbool.h>

#define GOOPAD_UPDATE_VERSION_SIZE 32

esp_err_t goopad_update_check(char version[GOOPAD_UPDATE_VERSION_SIZE], bool *available);
esp_err_t goopad_update_install(const char *version);
