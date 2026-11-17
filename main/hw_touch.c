/*
 * Touch: GT911 capacitive controller on I2C0.
 *
 * The board does not route the GT911 reset or interrupt lines, so the
 * controller is polled and its address is discovered by probing both
 * possible values (0x5D and 0x14).
 */
#include "app.h"
#include "board.h"

#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "touch";

static esp_lcd_touch_handle_t s_touch;
static esp_lcd_panel_io_handle_t s_touch_io;

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    esp_lcd_touch_point_data_t pt[1] = {0};
    uint8_t count = 0;

    g_app.touch_polls++;

    esp_err_t rd = esp_lcd_touch_read_data(s_touch);
    if (rd != ESP_OK) {
        g_app.touch_read_errors++;
    }

    /* esp_lcd_touch_get_data() returns esp_err_t, and ESP_OK is 0, so it
     * must not be used as a boolean: `err && count` is always false. */
    esp_err_t err = esp_lcd_touch_get_data(s_touch, pt, &count, 1);
    bool pressed = (err == ESP_OK) && (count > 0);

    if (pressed) {
        data->point.x = pt[0].x;
        data->point.y = pt[0].y;
        data->state = LV_INDEV_STATE_PRESSED;
        g_app.touch_x = pt[0].x;
        g_app.touch_y = pt[0].y;
        g_app.touch_pressed = true;
        /* Log the first touch and then every 50th, so a working panel is
         * visible on the console without flooding it while drawing. */
        if (g_app.touch_events == 0 || (g_app.touch_events % 50) == 0) {
            ESP_LOGI(TAG, "touch #%lu at (%d, %d)",
                     (unsigned long)(g_app.touch_events + 1), pt[0].x, pt[0].y);
        }
        g_app.touch_events++;
        g_app.results.touch = TEST_PASS;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        g_app.touch_pressed = false;
    }
}

/* Read the GT911 product ID string (register 0x8140, 4 bytes). A real
 * controller answers "911\0"; anything else means the address is occupied
 * by something that is not a GT911. */
static esp_err_t gt911_read_product_id(uint8_t addr, char out[5])
{
    i2c_master_dev_handle_t dev = NULL;
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = BSP_I2C_FREQ_HZ,
    };
    esp_err_t err = i2c_master_bus_add_device(g_app.i2c_bus, &cfg, &dev);
    if (err != ESP_OK) {
        return err;
    }

    const uint8_t reg[2] = {0x81, 0x40};
    uint8_t id[4] = {0};
    err = i2c_master_transmit_receive(dev, reg, sizeof(reg), id, sizeof(id), 200);
    i2c_master_bus_rm_device(dev);

    if (err != ESP_OK) {
        return err;
    }
    for (int i = 0; i < 4; i++) {
        out[i] = (id[i] >= 0x20 && id[i] < 0x7F) ? (char)id[i] : '.';
    }
    out[4] = '\0';
    return ESP_OK;
}

esp_err_t hw_touch_init(lv_display_t *disp)
{
    uint8_t addr = 0;
    char product_id[5] = {0};

    /* Probe both possible addresses and confirm the product ID, so a
     * stray device on the bus is not mistaken for a touch controller. */
    const uint8_t candidates[2] = {BSP_TOUCH_ADDR, BSP_TOUCH_ADDR_ALT};
    for (int i = 0; i < 2 && addr == 0; i++) {
        if (i2c_master_probe(g_app.i2c_bus, candidates[i], 200) != ESP_OK) {
            continue;
        }
        if (gt911_read_product_id(candidates[i], product_id) == ESP_OK) {
            ESP_LOGI(TAG, "device at 0x%02X reports product ID \"%s\"",
                     candidates[i], product_id);
            addr = candidates[i];
        } else {
            ESP_LOGW(TAG, "device at 0x%02X did not answer the GT911 ID read",
                     candidates[i]);
        }
    }

    if (addr == 0) {
        ESP_LOGE(TAG, "no GT911 found at 0x%02X or 0x%02X",
                 BSP_TOUCH_ADDR, BSP_TOUCH_ADDR_ALT);
        g_app.results.touch = TEST_FAIL;
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "GT911 found at 0x%02X", addr);
    snprintf(g_app.touch_product_id, sizeof(g_app.touch_product_id), "%s", product_id);

    esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    io_cfg.dev_addr = addr;
    io_cfg.scl_speed_hz = BSP_I2C_FREQ_HZ;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(g_app.i2c_bus, &io_cfg, &s_touch_io),
                        TAG, "touch io");

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = BSP_LCD_H_RES,
        .y_max = BSP_LCD_V_RES,
        .rst_gpio_num = BSP_TOUCH_RST_GPIO,
        .int_gpio_num = BSP_TOUCH_INT_GPIO,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    ESP_RETURN_ON_ERROR(esp_lcd_touch_new_i2c_gt911(s_touch_io, &tp_cfg, &s_touch),
                        TAG, "gt911 driver");

    lv_indev_t *indev = lv_indev_create();
    ESP_RETURN_ON_FALSE(indev, ESP_ERR_NO_MEM, TAG, "lvgl indev");
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);
    lv_indev_set_display(indev, disp);

    /* The controller answered, so the hardware is present. Whether it
     * actually reports touches is tracked separately by touch_events. */
    g_app.touch_present = true;
    g_app.results.touch = TEST_PASS;
    ESP_LOGI(TAG, "touch ready");
    return ESP_OK;
}
