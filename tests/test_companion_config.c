#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "goopad_companion.h"

int main(void)
{
    const char *json =
        "{\"companionAddress\":\"https://192.0.2.1:8787\","
        "\"companionToken\":\"abcdefghijklmnopqrstuvwxyz012345\","
        "\"macros\":["
        "{\"actionMode\":\"pc_macro\",\"url\":\"/run\",\"secret\":\"\",\"sharedPc\":true},"
        "{\"actionMode\":\"pc_open_url\",\"url\":\"/open\",\"secret\":\"\",\"sharedPc\":true},"
        "{\"actionMode\":\"widget\",\"widget\":\"now_playing\",\"url\":\"/monitor\",\"secret\":\"\",\"sharedPc\":true},"
        "{\"actionMode\":\"widget\",\"widget\":\"pc_monitor\",\"url\":\"/monitor\",\"secret\":\"\",\"sharedPc\":true}]}";
    cJSON *root = cJSON_Parse(json);
    assert(root);
    cJSON *macros = cJSON_GetObjectItemCaseSensitive(root, "macros");
    const char *paths[] = {"/run", "/open", "/monitor", "/monitor"};
    for (int i = 0; i < 4; i++) {
        char url[180], expected[180];
        const char *token = NULL;
        cJSON *item = cJSON_GetArrayItem(macros, i);
        assert(goopad_companion_resolve(root, item, url, sizeof(url), &token));
        snprintf(expected, sizeof(expected), "https://192.0.2.1:8787%s", paths[i]);
        assert(!strcmp(url, expected) && !strcmp(token, "abcdefghijklmnopqrstuvwxyz012345"));
        assert(!goopad_companion_resolve(root, item, url, 5, &token));
    }
    cJSON *item = cJSON_GetArrayItem(macros, 0);
    char url[180];
    const char *token = NULL;
    const char *body = NULL;
    assert(goopad_companion_media_request("https://192.0.2.1:8787/monitor", false, url, sizeof(url), &body));
    assert(!strcmp(url, "https://192.0.2.1:8787/run"));
    cJSON *request = cJSON_Parse(body);
    assert(request);
    cJSON *step = cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(request, "steps"), 0);
    assert(!strcmp(cJSON_GetObjectItemCaseSensitive(step, "type")->valuestring, "media"));
    assert(!strcmp(cJSON_GetObjectItemCaseSensitive(step, "value")->valuestring, "play_pause"));
    cJSON_Delete(request);
    assert(goopad_companion_media_request("https://192.0.2.1:8787/monitor", true, url, sizeof(url), &body));
    request = cJSON_Parse(body);
    step = cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(request, "steps"), 0);
    assert(!strcmp(cJSON_GetObjectItemCaseSensitive(step, "value")->valuestring, "next"));
    cJSON_Delete(request);
    assert(!goopad_companion_media_request("http://192.0.2.1/monitor", false, url, sizeof(url), &body));
    assert(!goopad_companion_media_request("https://192.0.2.1/monitor?x=1", false, url, sizeof(url), &body));
    assert(!goopad_companion_media_request("https://user@192.0.2.1/monitor", false, url, sizeof(url), &body));
    assert(!goopad_companion_media_request("https://192.0.2.1/run", false, url, sizeof(url), &body));
    assert(!goopad_companion_media_request("https://192.0.2.1/monitor", false, url, 8, &body));
    cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(item, "url"), "/monitor");
    assert(!goopad_companion_resolve(root, item, url, sizeof(url), &token));
    cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(item, "url"), "/run");
    cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(item, "actionMode"), "advanced");
    assert(!goopad_companion_resolve(root, item, url, sizeof(url), &token));
    cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(item, "actionMode"), "pc_macro");
    cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(item, "secret"), "legacy");
    assert(!goopad_companion_resolve(root, item, url, sizeof(url), &token));
    cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(item, "secret"), "");
    cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(root, "companionAddress"), "https://example.test@evil.test");
    assert(!goopad_companion_resolve(root, item, url, sizeof(url), &token));
    cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(root, "companionAddress"), "http://192.0.2.1:8787");
    assert(!goopad_companion_resolve(root, item, url, sizeof(url), &token));
    cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(root, "companionAddress"), "https://192.0.2.1:8787");
    cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(root, "companionToken"), "short");
    assert(!goopad_companion_resolve(root, item, url, sizeof(url), &token));
    cJSON_DeleteItemFromObjectCaseSensitive(item, "sharedPc");
    assert(!goopad_companion_resolve(root, item, url, sizeof(url), &token));
    cJSON_Delete(root);
    puts("Shared Companion configuration tests passed");
    return 0;
}
