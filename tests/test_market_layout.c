#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "goopad_layout.h"
#include "goopad_market.h"
#include "goopad_alerts.h"

int main(void)
{
    assert(goopad_grid_limit(480, 0, 64) == 4);
    assert(goopad_grid_limit(480, 84, 64) == 4);
    assert(goopad_grid_limit(240, 0, 64) == 3);
    assert(goopad_grid_limit(240, 84, 64) == 2);
    assert(goopad_grid_limit(800, 0, 80) == 4);
    assert(goopad_grid_limit(480, 0, 120) == 3);
    assert(goopad_grid_limit(480, 84, 120) == 3);
    assert(goopad_grid_limit(64, 84, 64) == 0);
    assert(goopad_grid_limit(480, 0, 0) == 0);
    assert(!strcmp(goopad_stock_error(200, NULL,
        "Our standard API rate limit is 25 requests per day. Please subscribe to premium.", NULL),
        "Check key or\nAPI daily limit"));
    assert(!strcmp(goopad_stock_error(200, NULL,
        "The demo API key is for demo purposes only.", NULL), "Demo key:\nuse a real key"));
    assert(!strcmp(goopad_stock_error(200, NULL, "This is a premium endpoint", NULL),
        "Premium access\nmay be needed"));
    assert(!strcmp(goopad_stock_error(429, NULL, NULL, NULL), "Check key or\nAPI rate limit"));
    assert(!strcmp(goopad_stock_error(200, "Too many calls per minute", NULL, NULL),
        "Check key or\nAPI rate limit"));
    assert(!strcmp(goopad_stock_error(200, NULL, NULL, "Invalid API call"),
        "Check symbol\nand API key"));
    assert(goopad_stock_error(200, NULL, NULL, NULL) == NULL);

    double price = 0;
    cJSON *quote = cJSON_Parse("{\"c\":123.45,\"pc\":120}");
    assert(quote && goopad_finnhub_quote(quote, &price) && price == 123.45);
    cJSON_Delete(quote);
    const char *invalid_quotes[] = {"{}", "{\"c\":0}", "{\"c\":-1}", "{\"c\":1e13}",
                                    "{\"c\":\"123\"}", "{\"c\":null}", "[]", "{\"c\":1e999}"};
    for (size_t i = 0; i < sizeof(invalid_quotes) / sizeof(invalid_quotes[0]); ++i) {
        quote = cJSON_Parse(invalid_quotes[i]);
        assert(quote && !goopad_finnhub_quote(quote, &price));
        cJSON_Delete(quote);
    }
    assert(!cJSON_Parse("{\"c\":"));
    quote = cJSON_Parse("{\"error\":\"private-token-in-provider-response\"}");
    assert(!strcmp(goopad_finnhub_error(401, NULL), "Finnhub key\nor access"));
    assert(!strcmp(goopad_finnhub_error(403, NULL), "Finnhub key\nor access"));
    assert(!strcmp(goopad_finnhub_error(429, NULL), "Finnhub\nrate limit"));
    assert(!strcmp(goopad_finnhub_error(200, quote), "Finnhub error\nCheck settings"));
    assert(goopad_finnhub_error(500, NULL) == NULL);
    cJSON_Delete(quote);

    char date[11];
    quote = cJSON_Parse("{\"date\":\"2026-11-24\",\"base\":\"USD\",\"quote\":\"EUR\",\"rate\":0.91079}");
    assert(quote && goopad_frankfurter_rate(quote, "USD", "EUR", &price, date));
    assert(price == 0.91079 && !strcmp(date, "2026-11-24"));
    assert(!goopad_frankfurter_rate(quote, "USD", "GBP", &price, date));
    assert(!goopad_frankfurter_rate(quote, "USD", "USD", &price, date));
    cJSON_Delete(quote);
    const char *invalid_rates[] = {"{}", "{\"base\":\"USD\",\"quote\":\"EUR\",\"rate\":0,\"date\":\"2026-11-24\"}",
        "{\"base\":\"USD\",\"quote\":\"EUR\",\"rate\":1,\"date\":\"bad\"}",
        "{\"base\":\"USD\",\"quote\":\"EUR\",\"rate\":\"1\",\"date\":\"2026-11-24\"}"};
    for (size_t i = 0; i < sizeof(invalid_rates) / sizeof(invalid_rates[0]); i++) {
        quote = cJSON_Parse(invalid_rates[i]);
        assert(quote && !goopad_frankfurter_rate(quote, "USD", "EUR", &price, date));
        cJSON_Delete(quote);
    }
    assert(!goopad_currency_code("usd") && !goopad_currency_code("USD/") && goopad_currency_code("USD"));
    char alert[64];
    quote = cJSON_Parse("{\"type\":\"FeatureCollection\",\"features\":[]}");
    assert(quote && goopad_nws_alert_text(quote, alert, sizeof(alert)) && !strcmp(alert, "No NWS alerts"));
    cJSON_Delete(quote);
    quote = cJSON_Parse("{\"type\":\"FeatureCollection\",\"features\":[{\"properties\":{\"event\":\"Flood Warning\"}}]}");
    assert(quote && goopad_nws_alert_text(quote, alert, sizeof(alert)) && !strcmp(alert, "NWS 1 alert\nFlood Warning"));
    cJSON_Delete(quote);
    quote = cJSON_Parse("{\"type\":\"FeatureCollection\",\"features\":[{}]}");
    assert(quote && !goopad_nws_alert_text(quote, alert, sizeof(alert)));
    cJSON_Delete(quote);
    quote = cJSON_Parse("{\"features\":{}}");
    assert(quote && !goopad_nws_alert_text(quote, alert, sizeof(alert)));
    cJSON_Delete(quote);

    assert(goopad_widget_image_top(8, 40, 160) == -52);
    uint8_t alpha[64] = {0};
    for (int y = 2; y < 6; y++) for (int x = 0; x < 8; x++) alpha[y * 8 + x] = 255;
    goopad_cover_t cover = goopad_image_cover(alpha, 8, 120, 120);
    assert(cover.scale == 7680 && cover.x == 0 && cover.y == 0);
    cover = goopad_image_cover(alpha, 8, 160, 80);
    assert(cover.scale == 5120 && cover.x == 0 && cover.y == 0);
    memset(alpha, 0, sizeof(alpha));
    for (int y = 1; y < 5; y++) for (int x = 2; x < 6; x++) alpha[y * 8 + x] = 255;
    cover = goopad_image_cover(alpha, 8, 80, 80);
    assert(cover.scale == 5120 && cover.x == 0 && cover.y == 20);
    memset(alpha, 0, sizeof(alpha));
    cover = goopad_image_cover(alpha, 8, 80, 120);
    assert(cover.scale == 3840 && cover.x == 0 && cover.y == 0);
    assert(goopad_widget_image_top(28, 48, 32) == 36);
    assert(goopad_widget_image_top(8, 40, 80) == -12);
    uint32_t area;
    assert(goopad_layout_area(3, 3, 0, 2, 2, &area) && area == 0x1b);
    assert(goopad_layout_area(3, 3, 9, 2, 2, &area) && area == (0x1bU << 9));
    assert(!goopad_layout_area(3, 3, 2, 2, 1, &area));
    assert(!goopad_layout_area(3, 3, 6, 1, 2, &area));
    assert(!goopad_layout_area(3, 3, 30, 2, 2, &area));
    uint8_t widths[32], heights[32];
    memset(widths, 1, sizeof(widths));
    memset(heights, 1, sizeof(heights));
    widths[0] = heights[0] = 2;
    assert(goopad_layout_valid(3, 3, false, 9, widths, heights, 0x1e5));
    assert(!goopad_layout_valid(3, 3, false, 9, widths, heights, 0x1e7));
    assert(!goopad_layout_valid(3, 3, true, 9, widths, heights, 0x1e5));
    widths[1] = 2;
    assert(!goopad_layout_valid(3, 3, false, 9, widths, heights, 1));
    puts("PASS: safe provider errors and centered scaled widget image bounds");
    return 0;
}
