#include "goopad_news.h"
#include "goopad_layout.h"
#include "goopad_glass.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    char text[384];
    if (argc == 2 && !strcmp(argv[1], "--feed")) {
        char *xml = malloc(GOOPAD_NEWS_XML_BYTES + 1);
        assert(xml);
        size_t bytes = fread(xml, 1, GOOPAD_NEWS_XML_BYTES + 1, stdin);
        bool valid = goopad_news_xml_text(xml, bytes, text, sizeof(text));
        unsigned headlines = valid ? 1 : 0;
        for (const char *p = text; *p; p++) if (*p == '\n') headlines++;
        printf("Feed bytes=%zu valid=%d headlines=%u\n", bytes, valid, headlines);
        free(xml);
        return valid ? 0 : 1;
    }
    cJSON *feed = cJSON_Parse("{\"hits\":[{\"title\":\"First\"},{\"title\":\"First\"},{\"title\":null},{\"title\":\"Second\"},{\"title\":\"Third\"},{\"title\":\"Fourth\"}]}");
    assert(goopad_news_text(feed, text, sizeof(text)) && !strcmp(text, "First\nSecond\nThird"));
    cJSON_Delete(feed);
    const char *rss = "<rss><channel><title>Channel title</title>"
        "<item><title><![CDATA[First & plain]]></title></item>"
        "<item><title>Second &amp; &#x20ac;</title></item>"
        "<item><title><![CDATA[First & plain]]></title></item>"
        "<item><title>Third</title></item><item><title>Fourth</title></item></channel></rss>";
    assert(goopad_news_xml_text(rss, strlen(rss), text, sizeof(text)));
    assert(!strcmp(text, "First & plain\nSecond & \xe2\x82\xac\nThird"));
    assert(!goopad_news_xml_text(rss, strlen(rss), text, 10) && !text[0]);
    const char *atom = "<a:feed xmlns:a=\"http://www.w3.org/2005/Atom\"><a:title>Feed title</a:title>"
        "<a:entry><a:title type=\"xhtml\"><div xmlns=\"http://www.w3.org/1999/xhtml\">"
        "Nested <b>Atom</b> title</div></a:title></a:entry></a:feed>";
    assert(goopad_news_xml_text(atom, strlen(atom), text, sizeof(text)) && !strcmp(text, "Nested Atom title"));
    const char *rdf = "<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" "
        "xmlns=\"http://purl.org/rss/1.0/\"><channel><title>Channel</title></channel>"
        "<item><title>RSS 1</title></item></rdf:RDF>";
    assert(goopad_news_xml_text(rdf, strlen(rdf), text, sizeof(text)) && !strcmp(text, "RSS 1"));
    const char *bad_xml[] = {
        "<rss><channel><item><title>Truncated", "<html><title>Not a feed</title></html>",
        "<rss><channel><item><title>&unknown;</title></item></channel></rss>",
        "<!DOCTYPE rss [<!ENTITY x \"expansion\">]><rss><channel><item><title>&x;</title></item></channel></rss>",
        "<!DOCTYPE rss SYSTEM \"file:///private\"><rss><channel/></rss>",
        "<rss><channel><item><title>\xc0\xaf</title></item></channel></rss>",
        ("<rss><channel><item><title>One</title></item><item><title>Two</title></item>"
         "<item><title>Three</title></item></channel></rss><broken>"),
        "<rss><channel><item><description>No title</description></item></channel></rss>"
    };
    for (unsigned i = 0; i < sizeof(bad_xml) / sizeof(bad_xml[0]); i++) {
        assert(!goopad_news_xml_text(bad_xml[i], strlen(bad_xml[i]), text, sizeof(text)) && !text[0]);
    }
    char *bounded = malloc(GOOPAD_NEWS_XML_BYTES + 1);
    assert(bounded);
    memset(bounded, ' ', GOOPAD_NEWS_XML_BYTES + 1);
    memcpy(bounded, "<rss><channel/></rss>", strlen("<rss><channel/></rss>"));
    assert(goopad_news_xml_text(bounded, GOOPAD_NEWS_XML_BYTES, text, sizeof(text)) && !strcmp(text, "No headlines"));
    assert(!goopad_news_xml_text(bounded, GOOPAD_NEWS_XML_BYTES + 1, text, sizeof(text)) && !text[0]);
    free(bounded);
    char nested[2048] = "<rss><channel>";
    for (unsigned i = 0; i < 33; i++) strcat(nested, "<nested>");
    for (unsigned i = 0; i < 33; i++) strcat(nested, "</nested>");
    strcat(nested, "</channel></rss>");
    assert(!goopad_news_xml_text(nested, strlen(nested), text, sizeof(text)));
    char long_xml[300] = "<rss><channel><item><title>";
    size_t prefix = strlen(long_xml);
    memset(long_xml + prefix, 'x', 119);
    strcpy(long_xml + prefix + 119, "\xe2\x82\xac rest</title></item></channel></rss>");
    assert(goopad_news_xml_text(long_xml, strlen(long_xml), text, sizeof(text)) && strlen(text) == 122 &&
           !strcmp(text + 119, "..."));
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
    for (int cols = 2; cols <= 4; cols++) for (int rows = 2; rows <= 4; rows++)
        for (int width = 240; width <= 480; width += 120) {
            int height = width - 84;
            double center_y = 0;
            for (int slot = 0; slot < cols * rows; slot++) {
                goopad_orbit_t tile = goopad_arches_position(width, height, cols, rows, slot);
                center_y += tile.y + tile.side / 2.0;
                assert(tile.side > 0);
                for (int x = 0; x <= 1; x++) for (int y = 0; y <= 1; y++) {
                    double dx = tile.x + x * tile.side - width / 2.0;
                    double dy = tile.y + y * tile.side - height / 2.0;
                    double radius = height / 2.0 - 10;
                    assert(dx * dx + dy * dy <= radius * radius);
                }
                for (int other = 0; other < slot; other++) {
                    goopad_orbit_t neighbor = goopad_arches_position(width, height, cols, rows, other);
                    assert(tile.x + tile.side + 6 <= neighbor.x || neighbor.x + neighbor.side + 6 <= tile.x ||
                           tile.y + tile.side + 6 <= neighbor.y || neighbor.y + neighbor.side + 6 <= tile.y);
                }
            }
            assert(fabs(center_y / (cols * rows) - height / 2.0) <= 1);
            if (cols >= 3) assert(goopad_arches_position(width, height, cols, rows, 1).y <
                                  goopad_arches_position(width, height, cols, rows, 0).y);
        }
    static uint16_t background[480 * 480], pixels[GOOPAD_GLASS_SIDE * GOOPAD_GLASS_SIDE],
                    scratch[GOOPAD_GLASS_SIDE * GOOPAD_GLASS_SIDE];
    for (int y = 0; y < 480; y++) for (int x = 0; x < 480; x++)
        background[y * 480 + x] = x < 240 ? 0 : 0xffff;
    goopad_glass_fill(pixels, background, 0, false);
    assert(pixels[59] == 0 && pixels[60] == 0xffff);
    goopad_glass_blur(pixels, scratch);
    assert(pixels[58] > 0 && pixels[61] < 0xffff);
    assert(pixels[0] == 0 && pixels[GOOPAD_GLASS_SIDE - 1] == 0xffff);
    for (int y = 0; y < 480; y++) for (int x = 0; x < 480; x++)
        background[y * 480 + x] = (x + y) % 2 ? 0xffff : 0;
    goopad_glass_fill(pixels, background, 0, false);
    assert(pixels[0] == 0x7bef && pixels[GOOPAD_GLASS_SIDE * GOOPAD_GLASS_SIDE - 1] == 0x7bef);
    puts("JSON/RSS/Atom, bounded XML, Arches/ring geometry and glass blur tests passed");
    return 0;
}
