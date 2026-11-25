#include "goopad_market.h"

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

bool goopad_currency_code(const char *code)
{
    if (!code || strlen(code) != 3) return false;
    for (unsigned i = 0; i < 3; i++)
        if (code[i] < 'A' || code[i] > 'Z') return false;
    return true;
}

bool goopad_frankfurter_rate(const cJSON *payload, const char *base, const char *quote,
                             double *rate, char date[11])
{
    const cJSON *from = cJSON_GetObjectItemCaseSensitive(payload, "base");
    const cJSON *to = cJSON_GetObjectItemCaseSensitive(payload, "quote");
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(payload, "rate");
    const cJSON *day = cJSON_GetObjectItemCaseSensitive(payload, "date");
    if (!goopad_currency_code(base) || !goopad_currency_code(quote) || !strcmp(base, quote) ||
        !cJSON_IsString(from) || strcmp(from->valuestring, base) ||
        !cJSON_IsString(to) || strcmp(to->valuestring, quote) ||
        !cJSON_IsNumber(value) || !isfinite(value->valuedouble) ||
        value->valuedouble <= 0 || value->valuedouble > 1e12 ||
        !cJSON_IsString(day) || strlen(day->valuestring) != 10) return false;
    for (unsigned i = 0; i < 10; i++) {
        char ch = day->valuestring[i];
        if (i == 4 || i == 7) { if (ch != '-') return false; }
        else if (ch < '0' || ch > '9') return false;
    }
    memcpy(date, day->valuestring, 11);
    *rate = value->valuedouble;
    return true;
}

bool goopad_finnhub_quote(const cJSON *payload, double *price)
{
    const cJSON *current = cJSON_GetObjectItemCaseSensitive(payload, "c");
    if (!cJSON_IsNumber(current) || !isfinite(current->valuedouble) ||
        current->valuedouble <= 0 || current->valuedouble > 1e12) return false;
    *price = current->valuedouble;
    return true;
}

const char *goopad_finnhub_error(int status, const cJSON *payload)
{
    if (status == 401 || status == 403) return "Finnhub key\nor access";
    if (status == 429) return "Finnhub\nrate limit";
    if (payload && cJSON_GetObjectItemCaseSensitive(payload, "error")) return "Finnhub error\nCheck settings";
    return NULL;
}

static bool contains_ci(const char *text, const char *needle)
{
    if (!text) return false;
    for (; *text; text++) {
        const char *a = text, *b = needle;
        while (*a && *b && tolower((unsigned char)*a) == tolower((unsigned char)*b)) {
            a++;
            b++;
        }
        if (!*b) return true;
    }
    return false;
}

const char *goopad_stock_error(int status, const char *note, const char *information, const char *error)
{
    if (contains_ci(note, "per day") || contains_ci(note, "daily") ||
        contains_ci(information, "per day") || contains_ci(information, "daily"))
        return "Check key or\nAPI daily limit";
    if (status == 429 || note || contains_ci(information, "rate limit"))
        return "Check key or\nAPI rate limit";
    if (contains_ci(information, "demo") && contains_ci(information, "key"))
        return "Demo key:\nuse a real key";
    if (contains_ci(information, "premium") || contains_ci(error, "premium"))
        return "Premium access\nmay be needed";
    if (status == 401 || status == 403)
        return "Key or access\nrejected";
    if (information || error)
        return "Check symbol\nand API key";
    return NULL;
}
