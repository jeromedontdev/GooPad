#include "goopad_alarm.h"

#include <string.h>

bool goopad_alarm_id_valid(const char *id)
{
    if (!id || strlen(id) != 15 || id[0] != 'a') return false;
    for (size_t i = 1; i < 15; i++)
        if (!((id[i] >= '0' && id[i] <= '9') || (id[i] >= 'a' && id[i] <= 'f'))) return false;
    return true;
}

bool goopad_alarm_time_valid(const char *text, unsigned *minute)
{
    if (!text || strlen(text) != 5 || text[2] != ':' ||
        text[0] < '0' || text[0] > '2' || text[1] < '0' || text[1] > '9' ||
        text[3] < '0' || text[3] > '5' || text[4] < '0' || text[4] > '9') return false;
    unsigned hour = (unsigned)(text[0] - '0') * 10 + (unsigned)(text[1] - '0');
    if (hour > 23) return false;
    if (minute) *minute = hour * 60 + (unsigned)(text[3] - '0') * 10 + (unsigned)(text[4] - '0');
    return true;
}

bool goopad_alarm_sound_valid(const char *sound)
{
    return sound && (!strcmp(sound, "alert") || !strcmp(sound, "chime") || !strcmp(sound, "beep"));
}

bool goopad_alarm_configure(goopad_alarm_schedule_t *schedule, const cJSON *root)
{
    const cJSON *macros = cJSON_GetObjectItemCaseSensitive(root, "macros");
    const cJSON *offset = cJSON_GetObjectItemCaseSensitive(root, "timeOffsetMinutes");
    if (!cJSON_IsArray(macros) || (offset && (!cJSON_IsNumber(offset) ||
        offset->valuedouble != offset->valueint || offset->valueint < -840 || offset->valueint > 840))) return false;
    goopad_alarm_schedule_t next = {.offset_minutes = offset ? offset->valueint : 0};
    unsigned index = 0;
    for (const cJSON *item = macros->child; item; item = item->next, index++) {
        const cJSON *mode = cJSON_GetObjectItemCaseSensitive(item, "actionMode");
        const cJSON *widget = cJSON_GetObjectItemCaseSensitive(item, "widget");
        if (!cJSON_IsString(mode) || strcmp(mode->valuestring, "widget") ||
            !cJSON_IsString(widget) || strcmp(widget->valuestring, "alarm")) continue;
        const cJSON *id = cJSON_GetObjectItemCaseSensitive(item, "alarmId");
        const cJSON *time = cJSON_GetObjectItemCaseSensitive(item, "alarmTime");
        const cJSON *enabled = cJSON_GetObjectItemCaseSensitive(item, "alarmEnabled");
        const cJSON *sound = cJSON_GetObjectItemCaseSensitive(item, "alarmSound");
        unsigned minute;
        if (next.count >= GOOPAD_ALARM_LIMIT || !cJSON_IsString(id) || !goopad_alarm_id_valid(id->valuestring) ||
            !cJSON_IsString(time) || !goopad_alarm_time_valid(time->valuestring, &minute) ||
            !cJSON_IsBool(enabled) || !cJSON_IsString(sound) || !goopad_alarm_sound_valid(sound->valuestring)) return false;
        for (size_t i = 0; i < next.count; i++)
            if (!strcmp(next.entries[i].id, id->valuestring)) return false;
        goopad_alarm_entry_t *entry = &next.entries[next.count++];
        memcpy(entry->id, id->valuestring, sizeof(entry->id));
        strcpy(entry->sound, sound->valuestring);
        entry->index = index;
        entry->minute = minute;
        entry->enabled = cJSON_IsTrue(enabled);
        entry->fired_day = -1;
        for (size_t i = 0; i < schedule->count; i++) {
            const goopad_alarm_entry_t *old = &schedule->entries[i];
            if (!strcmp(old->id, entry->id) && old->minute == minute) {
                entry->fired_day = old->fired_day;
                entry->next_attempt_us = old->next_attempt_us;
                break;
            }
        }
    }
    *schedule = next;
    return true;
}

void goopad_alarm_tick(goopad_alarm_schedule_t *schedule, int64_t epoch,
                       int64_t monotonic_us,
                       bool (*play)(unsigned index, const char *sound, void *context),
                       void *context)
{
    if (epoch < 1577836800 || !play) return;
    int64_t local = epoch + (int64_t)schedule->offset_minutes * 60;
    int64_t day = local / 86400;
    unsigned minute = (unsigned)((local % 86400) / 60);
    for (size_t i = 0; i < schedule->count; i++) {
        goopad_alarm_entry_t *entry = &schedule->entries[i];
        if (!entry->enabled || entry->minute != minute || day <= entry->fired_day ||
            monotonic_us < entry->next_attempt_us) continue;
        if (play(entry->index, entry->sound, context)) entry->fired_day = day;
        else entry->next_attempt_us = monotonic_us + 5000000;
    }
}
