#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "cJSON.h"

bool goopad_news_text(const cJSON *feed, char *out, size_t capacity);
