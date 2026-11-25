#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "cJSON.h"

bool goopad_nws_alert_text(const cJSON *payload, char *out, size_t capacity);
