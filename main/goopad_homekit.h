#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <esp_err.h>

#define GOOPAD_HOMEKIT_MAX_SWITCHES 32

typedef struct {
    char id[16];
    char name[25];
} goopad_homekit_switch_t;

esp_err_t goopad_homekit_start(const goopad_homekit_switch_t *switches, size_t count,
                               const char *setup_code, const char *setup_id);
esp_err_t goopad_homekit_toggle(const char *id);
esp_err_t goopad_homekit_state(const char *id, bool *state);
esp_err_t goopad_homekit_open_pairing(uint32_t seconds, char code[11]);
