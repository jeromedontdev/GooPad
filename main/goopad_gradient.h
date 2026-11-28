#pragma once
#include "cJSON.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define GOOPAD_GRADIENT_STOPS 6
typedef struct {
    uint32_t colors[GOOPAD_GRADIENT_STOPS];
    uint8_t positions[GOOPAD_GRADIENT_STOPS];
    unsigned count;
    enum { GOOPAD_GRADIENT_VERTICAL, GOOPAD_GRADIENT_HORIZONTAL, GOOPAD_GRADIENT_CIRCLE } direction;
} goopad_gradient_t;

static inline bool goopad_gradient_parse(const cJSON *macro, goopad_gradient_t *out)
{
    memset(out, 0, sizeof(*out));
    const cJSON *direction = cJSON_GetObjectItemCaseSensitive(macro, "gradientDirection");
    if (direction) {
        if (!cJSON_IsString(direction)) return false;
        if (!strcmp(direction->valuestring, "horizontal")) out->direction = GOOPAD_GRADIENT_HORIZONTAL;
        else if (!strcmp(direction->valuestring, "circle")) out->direction = GOOPAD_GRADIENT_CIRCLE;
        else if (strcmp(direction->valuestring, "vertical")) return false;
    }
    const cJSON *stops = cJSON_GetObjectItemCaseSensitive(macro, "gradientStops");
    if (!stops) return true;
    if (!cJSON_IsArray(stops)) return false;
    int count = cJSON_GetArraySize(stops);
    if (count < 2 || count > GOOPAD_GRADIENT_STOPS) return false;
    for (int i = 0; i < count; i++) {
        const cJSON *stop = cJSON_GetArrayItem(stops, i);
        const cJSON *color = cJSON_GetObjectItemCaseSensitive(stop, "color");
        const cJSON *position = cJSON_GetObjectItemCaseSensitive(stop, "position");
        if (!cJSON_IsObject(stop) || !cJSON_IsString(color) || strlen(color->valuestring) != 6 ||
            strspn(color->valuestring, "0123456789abcdefABCDEF") != 6 ||
            !cJSON_IsNumber(position) || position->valuedouble != position->valueint ||
            position->valueint < 0 || position->valueint > 100 ||
            (i && position->valueint <= out->positions[i - 1])) return false;
        out->colors[i] = (uint32_t)strtoul(color->valuestring, NULL, 16);
        out->positions[i] = position->valueint;
    }
    if (out->positions[0] != 0 || out->positions[count - 1] != 100) return false;
    out->count = count;
    return true;
}
