#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "goopad_network.h"

enum scenario { STREAM, PREEMPT, PREEMPT_OPEN, PREEMPT_HEADERS, TIMEOUT, REDIRECT,
                OVERFLOW, INCOMPLETE, REDIRECT_ERROR };
static enum scenario scenario;
static int64_t now;
static int opens, headers, reads, redirects, closes, clears, position;
static bool priority;

int64_t esp_timer_get_time(void) { return now; }
void vTaskDelay(int ticks) { now += ticks * 1000; }
static bool priority_pending(void) { return priority; }

esp_err_t esp_http_client_open(esp_http_client_handle_t client, int length)
{
    (void)client; (void)length;
    if (scenario == PREEMPT_OPEN && opens == 0) priority = true;
    return opens++ == 0 ? ESP_ERR_HTTP_EAGAIN : ESP_OK;
}

int64_t esp_http_client_fetch_headers(esp_http_client_handle_t client)
{
    (void)client;
    now += 250000;
    if (scenario == PREEMPT_HEADERS && headers == 0) priority = true;
    return headers++ == 0 ? -ESP_ERR_HTTP_EAGAIN : 5;
}

int esp_http_client_get_status_code(esp_http_client_handle_t client)
{
    (void)client;
    return (scenario == REDIRECT || scenario == REDIRECT_ERROR) && !redirects ? 302 : 200;
}

esp_err_t esp_http_client_set_redirection(esp_http_client_handle_t client)
{
    (void)client;
    if (scenario == REDIRECT_ERROR) return ESP_ERR_INVALID_STATE;
    redirects++;
    return ESP_OK;
}
esp_err_t esp_http_client_close(esp_http_client_handle_t client) { (void)client; closes++; return ESP_OK; }
esp_err_t esp_http_client_clear_response_buffer(esp_http_client_handle_t client) { (void)client; clears++; return ESP_OK; }

int esp_http_client_read(esp_http_client_handle_t client, char *buffer, int length)
{
    (void)client;
    now += 250000;
    reads++;
    if (scenario == PREEMPT) { priority = true; return -ESP_ERR_HTTP_EAGAIN; }
    if (scenario == TIMEOUT) return -ESP_ERR_HTTP_EAGAIN;
    if (scenario == INCOMPLETE) return 0;
    if (reads == 2) return -ESP_ERR_HTTP_EAGAIN;
    int remaining = 5 - position;
    if (!remaining) return 0;
    int amount = remaining < length ? remaining : length;
    if (amount > 3) amount = 3;
    memcpy(buffer, &"hello"[position], amount);
    position += amount;
    return amount;
}
bool esp_http_client_is_complete_data_received(esp_http_client_handle_t client)
{
    (void)client;
    return position == 5;
}

static esp_err_t run(enum scenario which, size_t capacity, int timeout, bool follow,
                     char *data, int *status, bool *preempted)
{
    scenario = which;
    now = opens = headers = reads = redirects = closes = clears = position = 0;
    priority = false;
    return goopad_network_background_get(NULL, data, capacity, timeout, follow,
                                         priority_pending, status, preempted);
}

int main(void)
{
    bool waiting = true;
    int64_t next = 30;
    goopad_network_resume_waiting(false, &waiting, &next);
    assert(waiting && next == 30);
    goopad_network_resume_waiting(true, &waiting, &next);
    assert(!waiting && next == 0);
    next = 900;
    goopad_network_resume_waiting(true, &waiting, &next);
    assert(!waiting && next == 900);
    char data[8] = {0};
    int status;
    bool preempted;
    assert(run(STREAM, 5, 4000, true, data, &status, &preempted) == ESP_OK);
    assert(!preempted && status == 200 && !strcmp(data, "hello") && reads == 4);
    assert(run(PREEMPT, 5, 4000, true, data, &status, &preempted) == ESP_ERR_INVALID_STATE);
    assert(preempted && reads == 1);
    assert(run(PREEMPT_OPEN, 5, 4000, true, data, &status, &preempted) == ESP_ERR_INVALID_STATE);
    assert(preempted && headers == 0);
    assert(run(PREEMPT_HEADERS, 5, 4000, true, data, &status, &preempted) == ESP_ERR_INVALID_STATE);
    assert(preempted && reads == 0);
    assert(run(TIMEOUT, 5, 1000, true, data, &status, &preempted) == ESP_ERR_TIMEOUT);
    assert(!preempted);
    assert(run(REDIRECT, 5, 4000, true, data, &status, &preempted) == ESP_OK);
    assert(status == 200 && redirects == 1 && closes == 1 && clears == 1);
    assert(run(REDIRECT, 5, 4000, false, data, &status, &preempted) == ESP_OK);
    assert(status == 302 && redirects == 0 && closes == 0 && clears == 0);
    assert(run(REDIRECT_ERROR, 5, 4000, true, data, &status, &preempted) == ESP_ERR_INVALID_STATE);
    assert(!preempted && closes == 0);
    assert(run(OVERFLOW, 4, 4000, true, data, &status, &preempted) == ESP_ERR_INVALID_SIZE);
    assert(run(INCOMPLETE, 5, 4000, true, data, &status, &preempted) == ESP_ERR_HTTP_INCOMPLETE_DATA);
    puts("PASS: streamed reads, priority preemption, timeout, redirects and response bounds");
    return 0;
}
