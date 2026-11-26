#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cJSON.h"

#define GOOPAD_ALARM_LIMIT 32

typedef struct {
    char id[16];
    char sound[6];
    unsigned index;
    unsigned minute;
    bool enabled;
    int64_t fired_day;
    int64_t next_attempt_us;
} goopad_alarm_entry_t;

typedef struct {
    goopad_alarm_entry_t entries[GOOPAD_ALARM_LIMIT];
    size_t count;
    int offset_minutes;
} goopad_alarm_schedule_t;

bool goopad_alarm_id_valid(const char *id);
bool goopad_alarm_time_valid(const char *text, unsigned *minute);
bool goopad_alarm_sound_valid(const char *sound);
bool goopad_alarm_configure(goopad_alarm_schedule_t *schedule, const cJSON *root);
void goopad_alarm_tick(goopad_alarm_schedule_t *schedule, int64_t epoch,
                       int64_t monotonic_us,
                       bool (*play)(unsigned index, const char *sound, void *context),
                       void *context);
