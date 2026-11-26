#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "goopad_alarm.h"

typedef struct {
    unsigned calls;
    unsigned index;
    char sound[6];
    bool available;
} player_t;

static bool play(unsigned index, const char *sound, void *context)
{
    player_t *player = context;
    player->calls++;
    player->index = index;
    strcpy(player->sound, sound);
    return player->available;
}

static bool configure(goopad_alarm_schedule_t *schedule, const char *json)
{
    cJSON *root = cJSON_Parse(json);
    assert(root);
    bool result = goopad_alarm_configure(schedule, root);
    cJSON_Delete(root);
    return result;
}

int main(void)
{
    unsigned minute = 0;
    assert(goopad_alarm_time_valid("00:00", &minute) && minute == 0);
    assert(goopad_alarm_time_valid("23:59", &minute) && minute == 1439);
    assert(!goopad_alarm_time_valid("24:00", NULL));
    assert(!goopad_alarm_time_valid("7:00", NULL));
    assert(!goopad_alarm_time_valid("07:60", NULL));
    assert(goopad_alarm_id_valid("a0123456789abcd"));
    assert(!goopad_alarm_id_valid("a0123456789abcD"));
    assert(goopad_alarm_sound_valid("chime") && !goopad_alarm_sound_valid("none"));

    const char *config =
        "{\"timeOffsetMinutes\":-300,\"macros\":["
        "{\"actionMode\":\"widget\",\"widget\":\"alarm\",\"alarmId\":\"a0123456789abcd\","
        "\"alarmTime\":\"07:00\",\"alarmEnabled\":true,\"alarmSound\":\"chime\"}]}";
    goopad_alarm_schedule_t schedule = {0};
    assert(configure(&schedule, config) && schedule.count == 1);
    const int64_t day = 20000 * INT64_C(86400);
    const int64_t due = day + 12 * 3600;
    player_t player = {.available = false};
    goopad_alarm_tick(&schedule, 1000, 0, play, &player);
    goopad_alarm_tick(&schedule, due - 1, 0, play, &player);
    assert(player.calls == 0);
    goopad_alarm_tick(&schedule, due, 1000000, play, &player);
    assert(player.calls == 1 && player.index == 0 && !strcmp(player.sound, "chime"));
    goopad_alarm_tick(&schedule, due + 2, 3000000, play, &player);
    assert(player.calls == 1);
    player.available = true;
    goopad_alarm_tick(&schedule, due + 5, 6000000, play, &player);
    assert(player.calls == 2);
    goopad_alarm_tick(&schedule, due + 20, 21000000, play, &player);
    assert(player.calls == 2);

    const char *moved =
        "{\"timeOffsetMinutes\":-300,\"macros\":[{},"
        "{\"actionMode\":\"widget\",\"widget\":\"alarm\",\"alarmId\":\"a0123456789abcd\","
        "\"alarmTime\":\"07:00\",\"alarmEnabled\":true,\"alarmSound\":\"beep\"}]}";
    assert(configure(&schedule, moved));
    goopad_alarm_tick(&schedule, due + 25, 26000000, play, &player);
    assert(player.calls == 2);
    goopad_alarm_tick(&schedule, due - 86400, 30000000, play, &player);
    assert(player.calls == 2);
    goopad_alarm_tick(&schedule, due + 86400, 86401000000, play, &player);
    assert(player.calls == 3 && player.index == 1 && !strcmp(player.sound, "beep"));
    goopad_alarm_tick(&schedule, due + 2 * 86400 + 60, 172861000000, play, &player);
    assert(player.calls == 3);

    const char *disabled =
        "{\"timeOffsetMinutes\":-300,\"macros\":["
        "{\"actionMode\":\"widget\",\"widget\":\"alarm\",\"alarmId\":\"a0123456789abcd\","
        "\"alarmTime\":\"07:00\",\"alarmEnabled\":false,\"alarmSound\":\"alert\"}]}";
    assert(configure(&schedule, disabled));
    goopad_alarm_tick(&schedule, due + 2 * 86400, 172801000000, play, &player);
    assert(player.calls == 3);

    const char *midnight =
        "{\"timeOffsetMinutes\":120,\"macros\":["
        "{\"actionMode\":\"widget\",\"widget\":\"alarm\",\"alarmId\":\"a11111111111111\","
        "\"alarmTime\":\"00:00\",\"alarmEnabled\":true,\"alarmSound\":\"alert\"},"
        "{\"actionMode\":\"widget\",\"widget\":\"alarm\",\"alarmId\":\"a22222222222222\","
        "\"alarmTime\":\"00:00\",\"alarmEnabled\":true,\"alarmSound\":\"beep\"}]}";
    assert(configure(&schedule, midnight));
    player.available = false;
    goopad_alarm_tick(&schedule, day + 22 * 3600 - 1, 200000000000, play, &player);
    assert(player.calls == 3);
    goopad_alarm_tick(&schedule, day + 22 * 3600, 200001000000, play, &player);
    assert(player.calls == 5);
    player.available = true;
    goopad_alarm_tick(&schedule, day + 22 * 3600 + 5, 200006000000, play, &player);
    assert(player.calls == 7 && player.index == 1 && !strcmp(player.sound, "beep"));
    goopad_alarm_tick(&schedule, day + 22 * 3600 + 10, 200011000000, play, &player);
    assert(player.calls == 7);
    assert(configure(&schedule, disabled));

    const char *invalid[] = {
        "{\"timeOffsetMinutes\":841,\"macros\":[]}",
        "{\"macros\":[{\"actionMode\":\"widget\",\"widget\":\"alarm\",\"alarmId\":\"a0123456789abcd\",\"alarmTime\":\"07:00\",\"alarmSound\":\"beep\"}]}",
        "{\"macros\":[{\"actionMode\":\"widget\",\"widget\":\"alarm\",\"alarmId\":\"a0123456789abcd\",\"alarmTime\":\"25:00\",\"alarmEnabled\":true,\"alarmSound\":\"beep\"}]}",
        "{\"macros\":[{\"actionMode\":\"widget\",\"widget\":\"alarm\",\"alarmId\":\"a0123456789abcd\",\"alarmTime\":\"07:00\",\"alarmEnabled\":true,\"alarmSound\":\"none\"}]}",
        "{\"macros\":[{\"actionMode\":\"widget\",\"widget\":\"alarm\",\"alarmId\":\"a0123456789abcd\",\"alarmTime\":\"07:00\",\"alarmEnabled\":true,\"alarmSound\":\"beep\"},{\"actionMode\":\"widget\",\"widget\":\"alarm\",\"alarmId\":\"a0123456789abcd\",\"alarmTime\":\"08:00\",\"alarmEnabled\":true,\"alarmSound\":\"beep\"}]}"
    };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        assert(!configure(&schedule, invalid[i]));
        assert(schedule.count == 1 && !schedule.entries[0].enabled);
    }
    puts("Alarm scheduling tests passed");
    return 0;
}
