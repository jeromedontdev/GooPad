/*
 * Board bring-up: I2C bus, backlight PWM, and the I2C address scan.
 */
#include "app.h"
#include "board.h"

#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "board";

app_state_t g_app;

#define BL_LEDC_MODE    LEDC_LOW_SPEED_MODE
#define BL_LEDC_TIMER   LEDC_TIMER_0
#define BL_LEDC_CHANNEL LEDC_CHANNEL_0
#define BL_LEDC_RES     LEDC_TIMER_10_BIT

static esp_err_t backlight_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = BL_LEDC_MODE,
        .duty_resolution = BL_LEDC_RES,
        .timer_num = BL_LEDC_TIMER,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer), TAG, "ledc timer");

    ledc_channel_config_t ch = {
        .gpio_num = BSP_LCD_BL_GPIO,
        .speed_mode = BL_LEDC_MODE,
        .channel = BL_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = BL_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    return ledc_channel_config(&ch);
}

static int s_backlight_pct = BSP_BACKLIGHT_DEFAULT_PCT;

void board_backlight_set(int percent)
{
    if (percent < 0) {
        percent = 0;
    }
    if (percent > 100) {
        percent = 100;
    }
    s_backlight_pct = percent;
    /* The user-facing range is 0-100%, but 100% maps to the safe maximum
     * duty rather than full duty. The cap is invisible: the slider still
     * goes to 100 and that is simply as bright as this supply can drive.
     * See the power budget in board.h. */
    uint32_t max_duty = (1u << BL_LEDC_RES) - 1;
    uint32_t duty = (max_duty * (uint32_t)percent * BSP_BACKLIGHT_MAX_PCT) / 10000u;
    ledc_set_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL, duty);
    ledc_update_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL);
}

int board_backlight_get(void)
{
    return s_backlight_pct;
}

esp_err_t board_init(void)
{
    ESP_LOGI(TAG, "initialising board");

    /* Backlight starts off so the panel does not show garbage during init. */
    ESP_RETURN_ON_ERROR(backlight_init(), TAG, "backlight init");
    board_backlight_set(0);

    /* One SPI2 bus serves both the ST7701S command interface and the SD
     * card; they share MOSI/SCLK and have separate chip selects. */
    spi_bus_config_t spi_cfg = {
        .mosi_io_num = BSP_SPI_MOSI_GPIO,
        .miso_io_num = BSP_SPI_MISO_GPIO,
        .sclk_io_num = BSP_SPI_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(BSP_SPI_HOST, &spi_cfg, SPI_DMA_CH_AUTO),
                        TAG, "spi bus");

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = BSP_I2C_SDA_GPIO,
        .scl_io_num = BSP_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &g_app.i2c_bus), TAG, "i2c bus");

    g_app.power.level_pct = -1;
    return ESP_OK;
}

void board_i2c_scan(void)
{
    g_app.i2c_found_count = 0;
    ESP_LOGI(TAG, "scanning I2C bus 0 (SDA=%d SCL=%d)", BSP_I2C_SDA_GPIO, BSP_I2C_SCL_GPIO);

    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        /* 200 ms: the IP5306 stretches the clock far longer than a normal
         * device, so a short probe timeout reports it as absent. */
        if (i2c_master_probe(g_app.i2c_bus, addr, 200) == ESP_OK) {
            ESP_LOGI(TAG, "  found device at 0x%02X", addr);
            if (g_app.i2c_found_count < (int)sizeof(g_app.i2c_found)) {
                g_app.i2c_found[g_app.i2c_found_count++] = addr;
            }
        }
    }
    ESP_LOGI(TAG, "I2C scan done, %d device(s)", g_app.i2c_found_count);
}
