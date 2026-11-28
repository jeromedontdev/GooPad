#include "goopad_news.h"
#include "goopad_utf8.h"
#include <string.h>

bool goopad_news_text(const cJSON *feed, char *out, size_t capacity)
{
    if (!out || capacity < 4) return false;
    out[0] = '\0';
    const cJSON *items = cJSON_GetObjectItemCaseSensitive(feed, "items");
    if (!items) items = cJSON_GetObjectItemCaseSensitive(feed, "hits");
    if (!cJSON_IsArray(items)) return false;
    if (!items->child) {
        if (capacity < sizeof("No headlines")) return false;
        strcpy(out, "No headlines");
        return true;
    }
    size_t used = 0;
    unsigned count = 0;
    const char *previous = NULL;
    for (const cJSON *item = items->child; item && count < 3; item = item->next) {
        const cJSON *title = cJSON_GetObjectItemCaseSensitive(item, "title");
        if (!cJSON_IsString(title) || !title->valuestring[0] ||
            (previous && !strcmp(previous, title->valuestring))) continue;
        size_t length = strlen(title->valuestring);
        if (!goopad_utf8_valid(title->valuestring, length)) return false;
        size_t size = length > 120 ? 120 : length;
        while (size && ((unsigned char)title->valuestring[size] & 0xc0) == 0x80) size--;
        size_t extra = size < length ? 3 : 0;
        if (used + (count ? 1 : 0) + size + extra >= capacity) return false;
        if (count) out[used++] = '\n';
        for (size_t i = 0; i < size; i++) {
            unsigned char c = (unsigned char)title->valuestring[i];
            out[used++] = c < 32 || c == 127 ? ' ' : (char)c;
        }
        if (extra) { memcpy(out + used, "...", 3); used += 3; }
        out[used] = '\0';
        previous = title->valuestring;
        count++;
    }
    return count != 0;
}
