#include "goopad_time.h"
#include "goopad_utf8.h"
#include <string.h>

bool goopad_time_format_valid(const char *format)
{
    if (!format || !*format || strlen(format) > GOOPAD_TIME_FORMAT_BYTES ||
        !goopad_utf8_valid(format, strlen(format))) return false;
    for (const unsigned char *p = (const unsigned char *)format; *p; p++) {
        if (*p < 32 && *p != '\n' && *p != '\t') return false;
        if (*p == 127) return false;
        if (*p != '%') continue;
        p++;
        bool plain = *p == '-';
        if (plain) p++;
        if (!*p || !strchr(plain ? "dHmIMSjy" : "%aAbBdeHImMSpPjuwYyFTRDrnt", *p)) return false;
    }
    return true;
}

const char *goopad_widget_time_format(bool date, const char *key)
{
    static const char *keys[2][6] = {
        {"24h", "12h", "24h_seconds", "12h_seconds", "12h_plain", "12h_plain_seconds"},
        {"iso", "mdy", "dmy", "month_short"}
    };
    static const char *formats[2][6] = {
        {"%H:%M", "%-I:%M %p", "%H:%M:%S", "%-I:%M:%S %p", "%-I:%M", "%-I:%M:%S"},
        {"%Y-%m-%d", "%m/%d/%Y", "%d/%m/%Y", "%b %d"}
    };
    for (unsigned i = 0; key && i < 6; i++)
        if (keys[date][i] && !strcmp(key, keys[date][i])) return formats[date][i];
    return key && goopad_time_format_valid(key) ? key : NULL;
}

bool goopad_time_text(char *out, size_t capacity, const char *format, const struct tm *time)
{
    if (!out || !capacity) return false;
    out[0] = '\0';
    if (!time || !goopad_time_format_valid(format)) return false;
    size_t used = 0;
    for (const char *p = format; *p; p++) {
        char token[64] = {0};
        if (*p != '%') {
            token[0] = *p;
        } else {
            p++;
            bool plain = *p == '-';
            if (plain) p++;
            if (*p == '%') strcpy(token, "%");
            else if (*p == 'n') strcpy(token, "\n");
            else if (*p == 't') strcpy(token, "\t");
            else if (*p == 'P') strcpy(token, time->tm_hour < 12 ? "am" : "pm");
            else {
                char spec[] = {'%', *p, '\0'};
                if (!strftime(token, sizeof(token), spec, time)) return false;
            }
            if (plain) {
                size_t skip = 0;
                while ((token[skip] == '0' || token[skip] == ' ') && token[skip + 1]) skip++;
                memmove(token, token + skip, strlen(token + skip) + 1);
            }
        }
        size_t size = strlen(token);
        if (size >= capacity - used) { out[0] = '\0'; return false; }
        memcpy(out + used, token, size + 1);
        used += size;
    }
    return true;
}
