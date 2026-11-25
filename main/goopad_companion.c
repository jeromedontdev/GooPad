#include "goopad_companion.h"

#include <stdio.h>
#include <string.h>

bool goopad_companion_media_request(const char *monitor, bool skip,
                                    char *url, size_t capacity, const char **body)
{
    if (!monitor || !url || !body || strncmp(monitor, "https://", 8)) return false;
    const char *path = strchr(monitor + 8, '/');
    if (!path || path == monitor + 8 || strcmp(path, "/monitor")) return false;
    for (const unsigned char *p = (const unsigned char *)monitor + 8;
         p < (const unsigned char *)path; p++)
        if (*p <= 32 || *p >= 127 || *p == '@' || *p == '?' || *p == '#' || *p == '\\') return false;
    size_t authority = path - monitor;
    if (authority + sizeof("/run") > capacity) return false;
    memcpy(url, monitor, authority);
    memcpy(url + authority, "/run", sizeof("/run"));
    *body = skip ? "{\"steps\":[{\"type\":\"media\",\"value\":\"next\"}]}" :
                   "{\"steps\":[{\"type\":\"media\",\"value\":\"play_pause\"}]}";
    return true;
}

bool goopad_companion_resolve(const cJSON *root, const cJSON *item,
                              char *url, size_t capacity, const char **token)
{
    const cJSON *shared = cJSON_GetObjectItemCaseSensitive(item, "sharedPc");
    const cJSON *mode = cJSON_GetObjectItemCaseSensitive(item, "actionMode");
    const cJSON *widget = cJSON_GetObjectItemCaseSensitive(item, "widget");
    const cJSON *path = cJSON_GetObjectItemCaseSensitive(item, "url");
    const cJSON *secret = cJSON_GetObjectItemCaseSensitive(item, "secret");
    const cJSON *address = cJSON_GetObjectItemCaseSensitive(root, "companionAddress");
    const cJSON *pairing = cJSON_GetObjectItemCaseSensitive(root, "companionToken");
    if (!url || !capacity || !token || !cJSON_IsTrue(shared) || !cJSON_IsString(mode) ||
        !cJSON_IsString(path) || !cJSON_IsString(secret) || secret->valuestring[0] ||
        !cJSON_IsString(address) || !cJSON_IsString(pairing)) return false;
    const char *suffix = NULL;
    if (!strcmp(mode->valuestring, "pc_macro")) suffix = "/run";
    else if (!strcmp(mode->valuestring, "pc_open_url")) suffix = "/open";
    else if (!strcmp(mode->valuestring, "widget") && cJSON_IsString(widget) &&
             (!strcmp(widget->valuestring, "pc_monitor") || !strcmp(widget->valuestring, "now_playing")))
        suffix = "/monitor";
    if (!suffix || strcmp(path->valuestring, suffix) || strncmp(address->valuestring, "https://", 8) ||
        !address->valuestring[8]) return false;
    for (const unsigned char *p = (const unsigned char *)address->valuestring + 8; *p; p++)
        if (*p <= 32 || *p >= 127 || *p == '/' || *p == '?' || *p == '#' || *p == '@' || *p == '\\')
            return false;
    size_t length = strlen(pairing->valuestring);
    if (length < 32 || length > 64) return false;
    for (const unsigned char *p = (const unsigned char *)pairing->valuestring; *p; p++)
        if (*p < 32 || *p > 126) return false;
    int written = snprintf(url, capacity, "%s%s", address->valuestring, suffix);
    if (written < 0 || (size_t)written >= capacity || written >= 180) return false;
    *token = pairing->valuestring;
    return true;
}
