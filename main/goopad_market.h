#pragma once

#include <stdbool.h>
#include "cJSON.h"

const char *goopad_stock_error(int status, const char *note, const char *information, const char *error);
bool goopad_finnhub_quote(const cJSON *payload, double *price);
const char *goopad_finnhub_error(int status, const cJSON *payload);
bool goopad_currency_code(const char *code);
bool goopad_frankfurter_rate(const cJSON *payload, const char *base, const char *quote,
                            double *rate, char date[11]);
