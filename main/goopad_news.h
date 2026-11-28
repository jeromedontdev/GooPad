#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "cJSON.h"

bool goopad_news_text(const cJSON *feed, char *out, size_t capacity);
bool goopad_news_xml_text(const char *feed, size_t size, char *out, size_t capacity);
const char *goopad_news_feed_url(const char *key);
#define GOOPAD_NEWS_XML_BYTES (256U * 1024U)
#define GOOPAD_NEWS_HEADLINES 5
#define GOOPAD_NEWS_TEXT_BYTES 640
#define GOOPAD_NEWS_SEPARATOR "\n---\n"
bool goopad_news_xml_text_limit(const char *feed, size_t size, char *out, size_t capacity, unsigned limit);
bool goopad_news_text_limit(const cJSON *feed, char *out, size_t capacity, unsigned limit);
