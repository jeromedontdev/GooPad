#include "goopad_news.h"
#include "goopad_layout.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    char text[384];
    cJSON *feed = cJSON_Parse("{\"hits\":[{\"title\":\"First\"},{\"title\":\"First\"},{\"title\":null},{\"title\":\"Second\"},{\"title\":\"Third\"},{\"title\":\"Fourth\"}]}");
    assert(goopad_news_text(feed, text, sizeof(text)) && !strcmp(text, "First\nSecond\nThird"));
    cJSON_Delete(feed);
    feed = cJSON_Parse("{\"items\":[{\"title\":\"JSON Feed\\nheadline\"}]}");
    assert(goopad_news_text(feed, text, sizeof(text)) && !strcmp(text, "JSON Feed headline"));
    assert(!goopad_news_text(feed, text, 5));
    cJSON_Delete(feed);
    feed = cJSON_Parse("{\"items\":[]}");
    assert(goopad_news_text(feed, text, sizeof(text)) && !strcmp(text, "No headlines"));
    cJSON_Delete(feed);
    feed = cJSON_Parse("{\"items\":[{\"content_html\":\"not a title\"}]}");
    assert(!goopad_news_text(feed, text, sizeof(text)));
    cJSON_Delete(feed);
    feed = cJSON_Parse("{\"error\":\"provider failure\"}");
    assert(!goopad_news_text(feed, text, sizeof(text)));
    cJSON_Delete(feed);
    char title[200];
    memset(title, 'x', sizeof(title) - 1); title[sizeof(title) - 1] = 0;
    feed = cJSON_CreateObject();
    cJSON *items = cJSON_AddArrayToObject(feed, "items"), *item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "title", title); cJSON_AddItemToArray(items, item);
    assert(goopad_news_text(feed, text, sizeof(text)) && strlen(text) == 123 && !strcmp(text + 120, "..."));
    cJSON_Delete(feed);
    memset(title, 'x', 119);
    memcpy(title + 119, "\xe2\x82\xac", 4);
    feed = cJSON_CreateObject();
    items = cJSON_AddArrayToObject(feed, "items"); item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "title", title); cJSON_AddItemToArray(items, item);
    assert(goopad_news_text(feed, text, sizeof(text)) && strlen(text) == 122 && !strcmp(text + 119, "..."));
    cJSON_ReplaceItemInObject(item, "title", cJSON_CreateString("\xc0\xaf"));
    assert(!goopad_news_text(feed, text, sizeof(text)));
    cJSON_Delete(feed);
    for (int arc = 0; arc <= 1; arc++) for (int count = 4; count <= 16; count++) {
        for (int width = 240; width <= 480; width += 240) {
            int height = width - 84;
            for (int slot = 0; slot < count; slot++) {
                goopad_orbit_t tile = goopad_orbit_position(width, height, count, slot, arc);
                assert(tile.x >= 0 && tile.y >= 0 && tile.side > 0);
                assert(tile.x + tile.side <= width && tile.y + tile.side <= height);
                for (int x = 0; x <= 1; x++) for (int y = 0; y <= 1; y++) {
                    double dx = tile.x + x * tile.side - width / 2.0;
                    double dy = tile.y + y * tile.side - height / 2.0;
                    double radius = (width < height ? width : height) / 2.0 - 9;
                    assert(dx * dx + dy * dy <= radius * radius);
                }
                for (int other = 0; other < slot; other++) {
                    goopad_orbit_t neighbor = goopad_orbit_position(width, height, count, other, arc);
                    assert(tile.x + tile.side <= neighbor.x || neighbor.x + neighbor.side <= tile.x ||
                           tile.y + tile.side <= neighbor.y || neighbor.y + neighbor.side <= tile.y);
                }
            }
        }
    }
    puts("News parsing and non-overlapping ring/arch layout tests passed");
    return 0;
}
