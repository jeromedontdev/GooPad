#include "goopad_diagnostics.h"
#include "app.h"

#include "cJSON.h"
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "driver/uart.h"
#include "driver/usb_serial_jtag.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIAG_CAPACITY 48
#define DIAG_MAGIC UINT64_C(0x474f4f5041444431)

typedef struct {
    uint32_t uptime;
    int32_t event, result, detail;
} diag_entry_t;

typedef struct {
    uint64_t magic;
    uint32_t next, count;
    diag_entry_t entries[DIAG_CAPACITY];
} diag_store_t;

static RTC_NOINIT_ATTR diag_store_t s_store;
static portMUX_TYPE s_diag_lock = portMUX_INITIALIZER_UNLOCKED;
static const char *TAG = "goopad_diag";
static bool s_ready;

void goopad_diag_record(goopad_diag_event_t event, int result, int detail)
{
    if (!s_ready || event < DIAG_BOOT || event > DIAG_PC_TEST_LATENCY) return;
    diag_entry_t entry = {(uint32_t)(esp_timer_get_time() / 1000000), event, result, detail};
    portENTER_CRITICAL(&s_diag_lock);
    s_store.entries[s_store.next] = entry;
    s_store.next = (s_store.next + 1) % DIAG_CAPACITY;
    if (s_store.count < DIAG_CAPACITY) s_store.count++;
    portEXIT_CRITICAL(&s_diag_lock);
}

char *goopad_diag_json(void)
{
    diag_store_t snapshot;
    portENTER_CRITICAL(&s_diag_lock);
    snapshot = s_store;
    portEXIT_CRITICAL(&s_diag_lock);
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;
    if (!cJSON_AddNumberToObject(root, "format", 1) ||
        !cJSON_AddNumberToObject(root, "uptimeSeconds", esp_timer_get_time() / 1000000) ||
        !cJSON_AddNumberToObject(root, "internalFree", heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)) ||
        !cJSON_AddNumberToObject(root, "internalLargest", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)) ||
        !cJSON_AddNumberToObject(root, "psramFree", heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) ||
        !cJSON_AddNumberToObject(root, "resetReason", esp_reset_reason())) {
        cJSON_Delete(root);
        return NULL;
    }
    cJSON *events = cJSON_AddArrayToObject(root, "events");
    if (!events) {
        cJSON_Delete(root);
        return NULL;
    }
    for (unsigned i = 0; i < snapshot.count; i++) {
        diag_entry_t *entry = &snapshot.entries[(snapshot.next + DIAG_CAPACITY - snapshot.count + i) % DIAG_CAPACITY];
        if (entry->event < DIAG_BOOT || entry->event > DIAG_PC_TEST_LATENCY) continue;
        cJSON *item = cJSON_CreateIntArray((int[]){(int)entry->uptime, entry->event, entry->result, entry->detail}, 4);
        if (!item || !cJSON_AddItemToArray(events, item)) {
            cJSON_Delete(item);
            cJSON_Delete(root);
            return NULL;
        }
    }
    char *text = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return text;
}

#if CONFIG_ESP_CONSOLE_UART || CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
static int serial_read(char *ch)
{
#if CONFIG_ESP_CONSOLE_UART
    return uart_read_bytes(CONFIG_ESP_CONSOLE_UART_NUM, ch, 1, pdMS_TO_TICKS(100));
#else
    return usb_serial_jtag_read_bytes(ch, 1, pdMS_TO_TICKS(100));
#endif
}

static void serial_task(void *arg)
{
    (void)arg;
    char command[24];
    size_t length = 0;
    bool overflow = false;
    while (true) {
        char ch;
        if (serial_read(&ch) <= 0) continue;
        if (ch != '\r' && ch != '\n') {
            if (length + 1 < sizeof(command)) command[length++] = ch;
            else overflow = true;
            continue;
        }
        if (!length && !overflow) continue;
        command[length] = '\0';
#ifdef GOOPAD_CAPACITY_TEST
        if (!overflow && !strncmp(command, "cap ", 4)) {
            char kind[12];
            unsigned count = 0;
            int fields = sscanf(command + 4, "%11s %u", kind, &count);
            esp_err_t result = fields >= 1 ? goopad_capacity_set(kind, count) : ESP_ERR_INVALID_ARG;
            printf("GOOPAD_CAP result=%d\n", (int)result);
        } else
#endif
        if (!overflow && (!strcmp(command, "diag") || !strcmp(command, "status"))) {
            char *json = goopad_diag_json();
            if (json) {
                printf("GOOPAD_DIAG %s\n", json);
                cJSON_free(json);
            } else printf("GOOPAD_DIAG unavailable\n");
        } else if (!overflow && !strcmp(command, "help")) {
            printf("GOOPAD_COMMANDS help status diag test pc [post] run <1-32>\n");
        } else if (!overflow && (!strcmp(command, "test pc") || !strcmp(command, "test pc post"))) {
            printf("GOOPAD_PC_TEST result=%d\n", (int)goopad_diag_test_pc(!strcmp(command, "test pc post")));
        } else if (!overflow && !strncmp(command, "run ", 4)) {
            char *end;
            long index = strtol(command + 4, &end, 10);
            if (*end || index < 1 || index > 32) printf("GOOPAD_RUN invalid\n");
            else printf("GOOPAD_RUN %ld result=%d\n", index, (int)goopad_trigger((unsigned)index - 1));
        } else printf("GOOPAD_COMMAND invalid\n");
        length = 0;
        overflow = false;
    }
}
#endif

void goopad_diag_init(void)
{
    if (s_store.magic != DIAG_MAGIC || s_store.next >= DIAG_CAPACITY || s_store.count > DIAG_CAPACITY) {
        memset(&s_store, 0, sizeof(s_store));
        s_store.magic = DIAG_MAGIC;
    }
    s_ready = true;
    goopad_diag_record(DIAG_BOOT, esp_reset_reason(), 0);
}

void goopad_diag_serial_init(void)
{
#if CONFIG_ESP_CONSOLE_UART
    esp_err_t err = uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0);
#elif CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    usb_serial_jtag_driver_config_t config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    esp_err_t err = usb_serial_jtag_driver_install(&config);
#else
    ESP_LOGW(TAG, "Serial commands unavailable on the configured console");
    return;
#endif
#if CONFIG_ESP_CONSOLE_UART || CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Serial commands unavailable: %s", esp_err_to_name(err));
    } else if (xTaskCreate(serial_task, "serial_diag", 6144, NULL, 2, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Could not start serial diagnostic task");
#if CONFIG_ESP_CONSOLE_UART
        uart_driver_delete(CONFIG_ESP_CONSOLE_UART_NUM);
#else
        usb_serial_jtag_driver_uninstall();
#endif
    }
#endif
}
