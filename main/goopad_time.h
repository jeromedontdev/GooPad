#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#define GOOPAD_TIME_FORMAT_BYTES 96
bool goopad_time_format_valid(const char *format);
const char *goopad_widget_time_format(bool date, const char *key);
bool goopad_time_text(char *out, size_t capacity, const char *format, const struct tm *time);
