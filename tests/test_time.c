#include "goopad_time.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct tm time = {.tm_year = 126, .tm_mon = 10, .tm_mday = 7, .tm_hour = 14,
                      .tm_min = 3, .tm_sec = 9, .tm_wday = 6, .tm_yday = 310};
    char out[256];
    assert(goopad_time_text(out, sizeof(out), "%Y-%m-%d%n%-I:%M:%S %p %%", &time));
    assert(!strcmp(out, "2026-11-07\n2:03:09 PM %"));
    assert(goopad_time_text(out, sizeof(out), "%A %B %-d %j %u %w %P %F %T %R %D", &time));
    assert(!strcmp(out, "Saturday November 7 311 6 6 pm 2026-11-07 14:03:09 14:03 11/07/26"));
    for (unsigned hour = 0; hour < 24; hour++) {
        time.tm_hour = hour;
        assert(goopad_time_text(out, sizeof(out), goopad_widget_time_format(false, "12h"), &time));
        char expected[32];
        snprintf(expected, sizeof(expected), "%u:03 %s", hour % 12 ? hour % 12 : 12, hour < 12 ? "AM" : "PM");
        assert(!strcmp(out, expected));
    }
    assert(goopad_time_text(out, sizeof(out), goopad_widget_time_format(true, "month_short"), &time));
    assert(!strcmp(out, "Nov 07"));
    const char *invalid[] = {"", "%", "%Q", "%-A", "%100Y", "%z", "bad\rformat", "\xc0\xaf"};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        assert(!goopad_time_format_valid(invalid[i]));
        assert(!goopad_time_text(out, sizeof(out), invalid[i], &time));
    }
    char long_format[98];
    memset(long_format, 'x', sizeof(long_format) - 1);
    long_format[sizeof(long_format) - 1] = '\0';
    assert(!goopad_time_format_valid(long_format));
    assert(!goopad_time_text(out, 3, "%Y", &time) && !out[0]);
    puts("Time format tests passed");
    return 0;
}
