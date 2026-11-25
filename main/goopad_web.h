#pragma once

#include <stdbool.h>
#include <stdio.h>
#include "esp_err.h"

typedef struct {
    FILE *file;
    size_t length;
} goopad_web_asset_t;

bool goopad_web_known(const char *name);
esp_err_t goopad_web_open(const char *name, goopad_web_asset_t *asset);
