#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "cJSON.h"

bool goopad_companion_resolve(const cJSON *root, const cJSON *item,
                              char *url, size_t capacity, const char **token);
bool goopad_companion_media_request(const char *monitor, bool skip,
                                    char *url, size_t capacity, const char **body);
