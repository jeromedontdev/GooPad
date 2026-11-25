#include "goopad_alerts.h"

#include <stdio.h>
#include <string.h>

bool goopad_nws_alert_text(const cJSON *payload, char *out, size_t capacity)
{
    const cJSON *features = cJSON_GetObjectItemCaseSensitive(payload, "features");
    const cJSON *type = cJSON_GetObjectItemCaseSensitive(payload, "type");
    if (!cJSON_IsString(type) || strcmp(type->valuestring, "FeatureCollection") || !cJSON_IsArray(features)) return false;
    int count = cJSON_GetArraySize(features);
    if (count < 0 || count > 999) return false;
    if (!count) {
        snprintf(out, capacity, "No NWS alerts");
        return true;
    }
    const cJSON *feature = cJSON_GetArrayItem(features, 0);
    const cJSON *properties = cJSON_GetObjectItemCaseSensitive(feature, "properties");
    const cJSON *event = cJSON_GetObjectItemCaseSensitive(properties, "event");
    if (!cJSON_IsString(event) || !event->valuestring[0]) return false;
    char name[40];
    size_t i = 0;
    for (const unsigned char *p = (const unsigned char *)event->valuestring; *p && i < sizeof(name) - 1; p++) {
        if (*p < 32 || *p > 126) return false;
        name[i++] = (char)*p;
    }
    name[i] = '\0';
    snprintf(out, capacity, "NWS %d %s\n%s", count, count == 1 ? "alert" : "alerts", name);
    return true;
}
