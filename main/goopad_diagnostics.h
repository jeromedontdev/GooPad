#pragma once

#include <stdbool.h>
#include "esp_err.h"

typedef enum {
    DIAG_BOOT = 1,
    DIAG_NETWORK_DEFERRED,
    DIAG_MACRO_RESULT,
    DIAG_WIDGET_RESULT,
    DIAG_WIFI_DISCONNECTED,
    DIAG_MACRO_LATENCY,
    DIAG_PC_TEST_LATENCY,
} goopad_diag_event_t;

void goopad_diag_init(void);
void goopad_diag_serial_init(void);
void goopad_diag_record(goopad_diag_event_t event, int result, int detail);
char *goopad_diag_json(void);
esp_err_t goopad_diag_test_pc(bool post);
