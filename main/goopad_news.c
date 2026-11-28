#include "goopad_news.h"
#include "goopad_utf8.h"
#include "expat.h"
#include <stdlib.h>
#include <string.h>
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif

const char *goopad_news_feed_url(const char *key)
{
    static const char *keys[] = {"bbc_world", "bbc_tech", "npr", "guardian", "nytimes", "ars", "verge", "hackernews"};
    static const char *urls[] = {
        "https://feeds.bbci.co.uk/news/world/rss.xml", "https://feeds.bbci.co.uk/news/technology/rss.xml",
        "https://feeds.npr.org/1001/rss.xml", "https://www.theguardian.com/world/rss",
        "https://rss.nytimes.com/services/xml/rss/nyt/HomePage.xml",
        "https://feeds.arstechnica.com/arstechnica/index", "https://www.theverge.com/rss/index.xml",
        "https://hnrss.org/frontpage"
    };
    for (unsigned i = 0; key && i < sizeof(keys) / sizeof(keys[0]); i++)
        if (!strcmp(key, keys[i])) return urls[i];
    return NULL;
}

typedef struct {
    XML_Parser parser;
    char *out;
    size_t capacity, used, title_size;
    unsigned depth, entry_depth, title_depth, channel_depth, count, entries, kind, limit;
    bool failed, channel, overflow;
    char title[129], headlines[GOOPAD_NEWS_HEADLINES][124];
} news_xml_t;

static void *news_alloc(size_t size)
{
#ifdef ESP_PLATFORM
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    return malloc(size);
#endif
}

static void *news_realloc(void *ptr, size_t size)
{
#ifdef ESP_PLATFORM
    return heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    return realloc(ptr, size);
#endif
}

static const char *xml_local(const char *name)
{
    const char *separator = strrchr(name, '|');
    return separator ? separator + 1 : name;
}

static void XMLCALL news_xml_start(void *data, const char *name, const char **attributes)
{
    news_xml_t *state = data;
    (void)attributes;
    const char *local = xml_local(name);
    if (++state->depth > 32) {
        state->failed = true;
        XML_StopParser(state->parser, XML_FALSE);
        return;
    }
    if (state->depth == 1) {
        if (!strcmp(name, "rss")) state->kind = 1;
        else if (!strcmp(name, "http://www.w3.org/2005/Atom|feed") || !strcmp(name, "feed")) state->kind = 2;
        else if (!strcmp(name, "http://www.w3.org/1999/02/22-rdf-syntax-ns#|RDF")) state->kind = 3;
    }
    if (state->kind != 2 && state->depth == 2 && !strcmp(local, "channel")) {
        state->channel = true; state->channel_depth = state->depth;
    }
    if ((state->kind == 1 && state->channel_depth == 2 && state->depth == 3 && !strcmp(local, "item")) ||
        (state->kind == 2 && state->depth == 2 && !strcmp(local, "entry")) ||
        (state->kind == 3 && state->depth == 2 && !strcmp(local, "item"))) {
        state->entry_depth = state->depth;
        state->entries++;
    }
    if (state->entry_depth && state->depth == state->entry_depth + 1 && !strcmp(local, "title")) {
        state->title_depth = state->depth;
        state->title_size = 0;
        state->overflow = false;
    }
}

static void XMLCALL news_xml_characters(void *data, const char *text, int size)
{
    news_xml_t *state = data;
    if (!state->title_depth || state->count >= state->limit) return;
    for (int i = 0; i < size; i++) {
        unsigned char ch = (unsigned char)text[i];
        if (ch <= 32 || ch == 127) {
            if (!state->title_size || state->title[state->title_size - 1] == ' ') continue;
            ch = ' ';
        }
        if (state->title_size == sizeof(state->title) - 1) state->overflow = true;
        else state->title[state->title_size++] = (char)ch;
    }
}

static void XMLCALL news_xml_end(void *data, const char *name)
{
    news_xml_t *state = data;
    (void)name;
    if (state->title_depth == state->depth && state->count < state->limit) {
        size_t length = state->title_size;
        while (length && state->title[length - 1] == ' ') length--;
        state->title[length] = '\0';
        size_t size = length > 120 ? 120 : length;
        while (size && ((unsigned char)state->title[size] & 0xc0) == 0x80) size--;
        if (size) {
            if (!goopad_utf8_valid(state->title, size)) state->failed = true;
            char headline[124];
            memcpy(headline, state->title, size);
            if (state->overflow || size < length) { memcpy(headline + size, "...", 3); size += 3; }
            headline[size] = '\0';
            bool duplicate = false;
            for (unsigned i = 0; i < state->count; i++) if (!strcmp(headline, state->headlines[i])) duplicate = true;
            if (!state->failed && !duplicate) {
                size_t separator = state->count ? sizeof(GOOPAD_NEWS_SEPARATOR) - 1 : 0;
                if (state->used + separator + size >= state->capacity) state->failed = true;
                else {
                    if (separator) {
                        memcpy(state->out + state->used, GOOPAD_NEWS_SEPARATOR, separator);
                        state->used += separator;
                    }
                    memcpy(state->out + state->used, headline, size + 1);
                    state->used += size;
                    strcpy(state->headlines[state->count], headline);
                    state->count++;
                }
            }
        }
    }
    if (state->title_depth == state->depth) state->title_depth = 0;
    if (state->entry_depth == state->depth) state->entry_depth = 0;
    if (state->channel_depth == state->depth) state->channel_depth = 0;
    state->depth--;
}

static void XMLCALL news_xml_doctype(void *data, const char *name, const char *system, const char *public_id, int internal)
{
    news_xml_t *state = data;
    (void)name; (void)system; (void)public_id; (void)internal;
    state->failed = true;
    XML_StopParser(state->parser, XML_FALSE);
}

static int XMLCALL news_xml_external(XML_Parser parser, const char *context, const char *base, const char *system, const char *public_id)
{
    (void)parser; (void)context; (void)base; (void)system; (void)public_id;
    return XML_STATUS_ERROR;
}

bool goopad_news_xml_text(const char *feed, size_t size, char *out, size_t capacity)
{
    return goopad_news_xml_text_limit(feed, size, out, capacity, 3);
}

bool goopad_news_xml_text_limit(const char *feed, size_t size, char *out, size_t capacity, unsigned limit)
{
    if (!out || !capacity) return false;
    out[0] = '\0';
    if (!feed || !size || size > GOOPAD_NEWS_XML_BYTES || limit < 1 || limit > GOOPAD_NEWS_HEADLINES) return false;
    news_xml_t *state = news_alloc(sizeof(*state));
    if (!state) return false;
    memset(state, 0, sizeof(*state));
    XML_Memory_Handling_Suite memory = {news_alloc, news_realloc, free};
    state->parser = XML_ParserCreate_MM(NULL, &memory, "|");
    bool valid = false;
    if (state->parser) {
        state->out = out; state->capacity = capacity; state->limit = limit;
        XML_SetUserData(state->parser, state);
        XML_SetElementHandler(state->parser, news_xml_start, news_xml_end);
        XML_SetCharacterDataHandler(state->parser, news_xml_characters);
        XML_SetStartDoctypeDeclHandler(state->parser, news_xml_doctype);
        XML_SetExternalEntityRefHandler(state->parser, news_xml_external);
        XML_SetParamEntityParsing(state->parser, XML_PARAM_ENTITY_PARSING_NEVER);
        valid = XML_Parse(state->parser, feed, (int)size, XML_TRUE) == XML_STATUS_OK &&
                !state->failed && state->kind && (state->kind == 2 || state->channel);
        if (valid && !state->entries && capacity >= sizeof("No headlines")) strcpy(out, "No headlines");
        else if (!state->count) valid = false;
        XML_ParserFree(state->parser);
    }
    free(state);
    if (!valid) out[0] = '\0';
    return valid;
}

bool goopad_news_text(const cJSON *feed, char *out, size_t capacity)
{
    return goopad_news_text_limit(feed, out, capacity, 3);
}

bool goopad_news_text_limit(const cJSON *feed, char *out, size_t capacity, unsigned limit)
{
    if (!out || capacity < 4) return false;
    out[0] = '\0';
    if (limit < 1 || limit > GOOPAD_NEWS_HEADLINES) return false;
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
    for (const cJSON *item = items->child; item && count < limit; item = item->next) {
        const cJSON *title = cJSON_GetObjectItemCaseSensitive(item, "title");
        if (!cJSON_IsString(title) || !title->valuestring[0] ||
            (previous && !strcmp(previous, title->valuestring))) continue;
        size_t length = strlen(title->valuestring);
        if (!goopad_utf8_valid(title->valuestring, length)) return false;
        size_t size = length > 120 ? 120 : length;
        while (size && ((unsigned char)title->valuestring[size] & 0xc0) == 0x80) size--;
        size_t extra = size < length ? 3 : 0;
        size_t separator = count ? sizeof(GOOPAD_NEWS_SEPARATOR) - 1 : 0;
        if (used + separator + size + extra >= capacity) return false;
        if (separator) { memcpy(out + used, GOOPAD_NEWS_SEPARATOR, separator); used += separator; }
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
