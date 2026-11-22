#include <stdint.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <esp_timer.h>
#include <hap.h>

static portMUX_TYPE window_lock = portMUX_INITIALIZER_UNLOCKED;
static int64_t deadline_us;
static uint64_t window_generation;

int hap_set_pair_setup_window(uint32_t seconds)
{
    if (seconds > 600) return HAP_FAIL;
    int64_t deadline = seconds ? esp_timer_get_time() + (int64_t)seconds * 1000000 : 0;
    portENTER_CRITICAL(&window_lock);
    if (seconds && window_generation == UINT64_MAX) {
        portEXIT_CRITICAL(&window_lock);
        return HAP_FAIL;
    }
    if (seconds) window_generation++;
    deadline_us = deadline;
    portEXIT_CRITICAL(&window_lock);
    return HAP_SUCCESS;
}

uint64_t hap_pair_setup_window_generation(void)
{
    portENTER_CRITICAL(&window_lock);
    uint64_t generation = deadline_us && esp_timer_get_time() < deadline_us ? window_generation : 0;
    portEXIT_CRITICAL(&window_lock);
    return generation;
}

bool hap_pair_setup_window_open(void)
{
    return hap_pair_setup_window_generation() != 0;
}
