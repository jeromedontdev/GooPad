/*
 * Display: ST7701S 480x480 panel on an 18-bit RGB parallel bus.
 *
 * The ST7701S has no reset pin on this board and no MIPI/SPI pixel path,
 * so the controller is brought up over a bit-banged 3-wire SPI bus
 * (CS / SCLK / MOSI) and then fed pixels over the RGB bus by the
 * ESP32-S3 LCD_CAM peripheral.
 *
 * LVGL renders straight into the panel's two PSRAM frame buffers
 * (direct mode, double buffered) so there is no extra copy per frame.
 */
#include "app.h"
#include "board.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"

#include <string.h>

static const char *TAG = "display";

#define LVGL_TICK_PERIOD_MS   2
/* LVGL renders and runs every widget callback on this task, so the stack
 * has to cover the deepest draw path plus the UI's own string formatting.
 * 6 KB overflowed once the tabbed UI was added; 12 KB leaves headroom. */
#define LVGL_TASK_STACK       (12 * 1024)
#define LVGL_TASK_PRIO        4
#define LVGL_TASK_MAX_SLEEP   500

static esp_lcd_panel_handle_t s_panel;
static SemaphoreHandle_t s_lvgl_mutex;
static TaskHandle_t s_lvgl_task;
static lv_display_t *s_disp;
static volatile uint32_t s_frame_count;

/* ------------------------------------------------------------------ */
/* ST7701S command bus: hardware SPI, 9-bit frames                    */
/* ------------------------------------------------------------------ */

/* The ST7701S command interface is SPI with one extra leading bit per
 * byte: 0 = command, 1 = data. That is exactly the SPI peripheral's
 * "command phase" (command_bits = 1) followed by an 8-bit data phase, so
 * the whole sequence runs in hardware at 1 MHz instead of being
 * bit-banged. CS is driven manually because the panel needs it held low
 * across a command plus all of its parameters. */
static spi_device_handle_t s_lcd_spi;

static esp_err_t lcd_cmd_bus_init(void)
{
    gpio_config_t cs_cfg = {
        .pin_bit_mask = (1ULL << BSP_LCD_CS_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&cs_cfg), TAG, "lcd cs gpio");
    gpio_set_level(BSP_LCD_CS_GPIO, 1);

    spi_device_interface_config_t dev = {
        .clock_speed_hz = BSP_LCD_SPI_HZ,
        .mode = 0,
        .spics_io_num = -1,   /* CS is driven by hand, see above */
        .queue_size = 1,
        .command_bits = 1,    /* the DC bit */
        .address_bits = 0,
        .dummy_bits = 0,
    };
    return spi_bus_add_device(BSP_SPI_HOST, &dev, &s_lcd_spi);
}

/* Send one byte with its DC bit. */
static esp_err_t lcd_wr_byte(uint8_t data, bool is_data)
{
    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TXDATA,
        .cmd = is_data ? 1 : 0,
        .length = 8,
        .tx_data = {data, 0, 0, 0},
    };
    return spi_device_polling_transmit(s_lcd_spi, &t);
}

static esp_err_t lcd_wr_cmd(uint8_t cmd, const uint8_t *params, size_t len, uint32_t delay_ms)
{
    gpio_set_level(BSP_LCD_CS_GPIO, 0);
    esp_err_t err = lcd_wr_byte(cmd, false);
    for (size_t i = 0; err == ESP_OK && i < len; i++) {
        err = lcd_wr_byte(params[i], true);
    }
    gpio_set_level(BSP_LCD_CS_GPIO, 1);
    if (delay_ms) {
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
    return err;
}

/* Vendor bring-up sequence for this panel. Any other sequence leaves the
 * panel black, so this is copied verbatim from Guition's reference. */
typedef struct {
    uint8_t cmd;
    uint8_t len;
    uint8_t delay_ms;
    uint8_t data[16];
} st7701_cmd_t;

static const st7701_cmd_t s_init_seq[] = {
    {0x01, 0, 120, {0}},  /* software reset; this panel has no reset pin */
    {0xFF, 5, 0, {0x77, 0x01, 0x00, 0x00, 0x10}},
    {0xC0, 2, 0, {0x3B, 0x00}},
    {0xC1, 2, 0, {0x0D, 0x02}},
    {0xC2, 2, 0, {0x31, 0x05}},
    {0xCD, 1, 0, {0x00}},
    /* Positive voltage gamma */
    {0xB0, 16, 0, {0x00, 0x11, 0x18, 0x0E, 0x11, 0x06, 0x07, 0x08,
                   0x07, 0x22, 0x04, 0x12, 0x0F, 0xAA, 0x31, 0x18}},
    /* Negative voltage gamma */
    {0xB1, 16, 0, {0x00, 0x11, 0x19, 0x0E, 0x12, 0x07, 0x08, 0x08,
                   0x08, 0x22, 0x04, 0x11, 0x11, 0xA9, 0x32, 0x18}},
    /* Page 1: power rails */
    {0xFF, 5, 0, {0x77, 0x01, 0x00, 0x00, 0x11}},
    {0xB0, 1, 0, {0x60}},   /* Vop = 4.7375 V */
    {0xB1, 1, 0, {0x32}},   /* VCOM */
    {0xB2, 1, 0, {0x07}},   /* VGH = 15 V */
    {0xB3, 1, 0, {0x80}},
    {0xB5, 1, 0, {0x49}},   /* VGL = -10.17 V */
    {0xB7, 1, 0, {0x85}},
    {0xB8, 1, 0, {0x21}},   /* AVDD = 6.6 V, AVCL = -4.6 V */
    {0xC1, 1, 0, {0x78}},
    {0xC2, 1, 0, {0x78}},
    {0xE0, 3, 0, {0x00, 0x1B, 0x02}},
    {0xE1, 11, 0, {0x08, 0xA0, 0x00, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x44, 0x44}},
    {0xE2, 12, 0, {0x11, 0x11, 0x44, 0x44, 0xED, 0xA0, 0x00, 0x00, 0xEC, 0xA0, 0x00, 0x00}},
    {0xE3, 4, 0, {0x00, 0x00, 0x11, 0x11}},
    {0xE4, 2, 0, {0x44, 0x44}},
    {0xE5, 16, 0, {0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0,
                   0x0E, 0xED, 0xD8, 0xA0, 0x10, 0xEF, 0xD8, 0xA0}},
    {0xE6, 4, 0, {0x00, 0x00, 0x11, 0x11}},
    {0xE7, 2, 0, {0x44, 0x44}},
    {0xE8, 16, 0, {0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0,
                   0x0D, 0xEC, 0xD8, 0xA0, 0x0F, 0xEE, 0xD8, 0xA0}},
    {0xEB, 7, 0, {0x02, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x40}},
    {0xEC, 2, 0, {0x3C, 0x00}},
    {0xED, 16, 0, {0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF,
                   0xFF, 0xFF, 0xFF, 0x20, 0x45, 0x67, 0x98, 0xBA}},
    /* VAP & VAN */
    {0xFF, 5, 0, {0x77, 0x01, 0x00, 0x00, 0x13}},
    {0xE5, 1, 0, {0xE4}},
    {0xFF, 5, 0, {0x77, 0x01, 0x00, 0x00, 0x00}},
    /* 0x70 = RGB888, 0x60 = RGB666, 0x50 = RGB565.
     * The bus is 16 lines, so the panel must be told RGB666 and the
     * driver pads each 5/6/5 component to 6 bits. */
    {0x3A, 1, 10, {0x60}},
    {0x11, 0, 120, {0}},    /* sleep out */
    {0x29, 0, 0, {0}},      /* display on */
};

static esp_err_t st7701_send_init(void)
{
    for (size_t i = 0; i < sizeof(s_init_seq) / sizeof(s_init_seq[0]); i++) {
        const st7701_cmd_t *c = &s_init_seq[i];
        ESP_RETURN_ON_ERROR(lcd_wr_cmd(c->cmd, c->data, c->len, c->delay_ms),
                            TAG, "init cmd 0x%02X", c->cmd);
    }
    return ESP_OK;
}

/* ------------------------------------------------------------------ */
/* RGB panel                                                          */
/* ------------------------------------------------------------------ */

static esp_err_t rgb_panel_create(void)
{
    esp_lcd_rgb_panel_config_t cfg = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .data_width = 16,
        .in_color_format = LCD_COLOR_FMT_RGB565,
        .out_color_format = LCD_COLOR_FMT_RGB565,
        .num_fbs = 2,
        .bounce_buffer_size_px = 10 * BSP_LCD_H_RES,
        .dma_burst_size = 64,
        .hsync_gpio_num = BSP_LCD_HSYNC_GPIO,
        .vsync_gpio_num = BSP_LCD_VSYNC_GPIO,
        .de_gpio_num = BSP_LCD_DE_GPIO,
        .pclk_gpio_num = BSP_LCD_PCLK_GPIO,
        .disp_gpio_num = GPIO_NUM_NC,
        .data_gpio_nums = {
            BSP_LCD_DATA0_GPIO,  BSP_LCD_DATA1_GPIO,
            BSP_LCD_DATA2_GPIO,  BSP_LCD_DATA3_GPIO,
            BSP_LCD_DATA4_GPIO,  BSP_LCD_DATA5_GPIO,
            BSP_LCD_DATA6_GPIO,  BSP_LCD_DATA7_GPIO,
            BSP_LCD_DATA8_GPIO,  BSP_LCD_DATA9_GPIO,
            BSP_LCD_DATA10_GPIO, BSP_LCD_DATA11_GPIO,
            BSP_LCD_DATA12_GPIO, BSP_LCD_DATA13_GPIO,
            BSP_LCD_DATA14_GPIO, BSP_LCD_DATA15_GPIO,
        },
        .timings = {
            .pclk_hz = BSP_LCD_PCLK_HZ,
            .h_res = BSP_LCD_H_RES,
            .v_res = BSP_LCD_V_RES,
            .hsync_pulse_width = 8,
            .hsync_back_porch = 20,
            .hsync_front_porch = 10,
            .vsync_pulse_width = 8,
            .vsync_back_porch = 10,
            .vsync_front_porch = 10,
            .flags = {
                .pclk_active_neg = false,
                .hsync_idle_low = false,
                .vsync_idle_low = false,
            },
        },
        .flags = {
            .fb_in_psram = true,
            .double_fb = true,
        },
    };

    return esp_lcd_new_rgb_panel(&cfg, &s_panel);
}

/* ------------------------------------------------------------------ */
/* LVGL glue                                                          */
/* ------------------------------------------------------------------ */

static bool on_frame_buf_complete(esp_lcd_panel_handle_t panel,
                                  const esp_lcd_rgb_panel_event_data_t *edata,
                                  void *user_ctx)
{
    (void)panel;
    (void)edata;
    (void)user_ctx;
    BaseType_t yield = pdFALSE;
    if (s_lvgl_task) {
        vTaskNotifyGiveFromISR(s_lvgl_task, &yield);
    }
    return yield == pdTRUE;
}

static void lvgl_flush_wait_cb(lv_display_t *disp)
{
    /* In direct mode LVGL reuses the frame buffer it just handed to the
     * panel, so it must wait until the panel has switched away from it. */
    if (lv_display_flush_is_last(disp)) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
    lv_display_flush_ready(disp);
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    if (!lv_display_flush_is_last(disp)) {
        lv_display_flush_ready(disp);
        return;
    }
    /* Direct mode: the whole frame buffer is already up to date. */
    ulTaskNotifyTake(pdTRUE, 0);
    esp_lcd_panel_draw_bitmap(s_panel, 0, 0, BSP_LCD_H_RES, BSP_LCD_V_RES, px_map);
    s_frame_count++;
}

static void lvgl_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static void lvgl_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "LVGL task started");
    uint32_t last_report = 0;
    while (1) {
        uint32_t wait_ms;
        if (xSemaphoreTake(s_lvgl_mutex, portMAX_DELAY) == pdTRUE) {
            wait_ms = lv_timer_handler();
            xSemaphoreGive(s_lvgl_mutex);
        } else {
            wait_ms = 10;
        }
        if (wait_ms < 1) {
            wait_ms = 1;
        }
        if (wait_ms > LVGL_TASK_MAX_SLEEP) {
            wait_ms = LVGL_TASK_MAX_SLEEP;
        }
        vTaskDelay(pdMS_TO_TICKS(wait_ms));

        /* Prove the panel is really being fed: log the flush count every 10 s. */
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000000);
        if (now - last_report >= 10) {
            last_report = now;
            ESP_LOGI(TAG, "frames flushed: %lu (uptime %lus)",
                     (unsigned long)s_frame_count, (unsigned long)now);
        }
    }
}

bool ui_lock(uint32_t timeout_ms)
{
    if (!s_lvgl_mutex) {
        return false;
    }
    TickType_t ticks = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(s_lvgl_mutex, ticks) == pdTRUE;
}

void ui_unlock(void)
{
    if (s_lvgl_mutex) {
        xSemaphoreGive(s_lvgl_mutex);
    }
}

esp_err_t hw_display_init(lv_display_t **out_disp){
    ESP_LOGI(TAG, "initialising ST7701S command bus (hardware SPI, 9-bit frames)");
    ESP_RETURN_ON_ERROR(lcd_cmd_bus_init(), TAG, "cmd bus");

    ESP_LOGI(TAG, "sending panel init sequence");
    ESP_RETURN_ON_ERROR(st7701_send_init(), TAG, "init sequence");

    ESP_LOGI(TAG, "creating RGB panel (480x480, PSRAM frames, 10-line DMA bounce buffers)");
    ESP_RETURN_ON_ERROR(rgb_panel_create(), TAG, "rgb panel");

    void *fb0 = NULL;
    void *fb1 = NULL;
    ESP_RETURN_ON_ERROR(esp_lcd_rgb_panel_get_frame_buffer(s_panel, 2, &fb0, &fb1),
                        TAG, "get frame buffers");
    memset(fb0, 0, BSP_LCD_H_RES * BSP_LCD_V_RES * sizeof(uint16_t));
    memset(fb1, 0, BSP_LCD_H_RES * BSP_LCD_V_RES * sizeof(uint16_t));

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel), TAG, "panel reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel), TAG, "panel init");

    /* This board has no DISP enable line, so the RGB driver reports
     * ESP_ERR_NOT_SUPPORTED here. The panel is already on from the 0x29
     * command in the init sequence, so that is not an error. */
    esp_err_t disp_err = esp_lcd_panel_disp_on_off(s_panel, true);
    if (disp_err != ESP_OK && disp_err != ESP_ERR_NOT_SUPPORTED) {
        ESP_RETURN_ON_ERROR(disp_err, TAG, "panel on");
    }

    ESP_LOGI(TAG, "initialising LVGL");
    lv_init();

    s_lvgl_mutex = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(s_lvgl_mutex, ESP_ERR_NO_MEM, TAG, "lvgl mutex");

    s_disp = lv_display_create(BSP_LCD_H_RES, BSP_LCD_V_RES);
    ESP_RETURN_ON_FALSE(s_disp, ESP_ERR_NO_MEM, TAG, "lvgl display");
    lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565);

    lv_display_set_buffers(s_disp, fb0, fb1,
                           BSP_LCD_H_RES * BSP_LCD_V_RES * sizeof(uint16_t),
                           LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(s_disp, lvgl_flush_cb);
    lv_display_set_flush_wait_cb(s_disp, lvgl_flush_wait_cb);

    esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_frame_buf_complete = on_frame_buf_complete,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_rgb_panel_register_event_callbacks(s_panel, &cbs, s_disp),
                        TAG, "rgb callbacks");

    const esp_timer_create_args_t tick_args = {
        .callback = lvgl_tick_cb,
        .name = "lvgl_tick",
    };
    esp_timer_handle_t tick_timer = NULL;
    ESP_RETURN_ON_ERROR(esp_timer_create(&tick_args, &tick_timer), TAG, "tick timer");
    ESP_RETURN_ON_ERROR(esp_timer_start_periodic(tick_timer, LVGL_TICK_PERIOD_MS * 1000),
                        TAG, "tick start");

    BaseType_t ok = xTaskCreatePinnedToCore(lvgl_task, "lvgl", LVGL_TASK_STACK, NULL,
                                LVGL_TASK_PRIO, &s_lvgl_task, 1);
    ESP_RETURN_ON_FALSE(ok == pdPASS, ESP_ERR_NO_MEM, TAG, "lvgl task");

    /* Start at a moderate brightness rather than 100%.
     *
     * The backlight is by far the largest load on the 3.3 V rail (the LED
     * driver U5 boosts from VOUT-BAT). On a marginal USB-A supply, full
     * brightness plus an audio burst is enough to collapse the rail and
     * reset the chip. 60% leaves headroom and is still perfectly usable;
     * raise it with the `bl` console command or the Display tab. */
    board_backlight_set(BSP_BACKLIGHT_DEFAULT_PCT);

    g_app.disp = s_disp;
    g_app.results.display = TEST_PASS;
    if (out_disp) {
        *out_disp = s_disp;
    }
    ESP_LOGI(TAG, "display ready");
    return ESP_OK;
}
