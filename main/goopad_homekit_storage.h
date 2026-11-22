#pragma once

#include "esp_err.h"

#define GOOPAD_HOMEKIT_NVS_PARTITION "goopad_hap"

esp_err_t goopad_homekit_storage_init(void);
