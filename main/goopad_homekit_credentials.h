#pragma once

#include <esp_err.h>

typedef struct {
    char code[11];
    char setup_id[5];
} goopad_homekit_credentials_t;

esp_err_t goopad_homekit_credentials_load(goopad_homekit_credentials_t *credentials);
esp_err_t goopad_homekit_generate_code(char code[11]);
