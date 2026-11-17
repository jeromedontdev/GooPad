/*
 * Power: IP5306 PMIC.
 *
 * Per the board schematic, U1 is the plain IP5306 in an ESOP-8 package:
 *
 *   1 VIN   2 LED1   3 LED2   4 LED3
 *   5 KEY   6 BAT    7 SW     8 VOUT   (9 = thermal pad, GND)
 *
 * There is no SDA and no SCL, so this part has no I2C interface. It
 * charges, boosts and protects the cell entirely on its own; the ESP32
 * cannot read charge state or battery level from it, and no bodge wire
 * can change that. Charge state is only visible on the board's LEDs.
 *
 * The probe below therefore always fails, and is kept only so the UI can
 * report "no I2C interface" instead of looking like a wiring fault. What
 * the Power tab shows instead is a power history derived from the reset
 * reason and a persistent boot counter, which is the only power telemetry
 * this hardware can provide.
 */
#include "app.h"
#include "board.h"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "power";

#define IP5306_REG_SYS_CTL0   0x00
#define IP5306_REG_READ0      0x70
#define IP5306_REG_READ1      0x71
#define IP5306_REG_READ2      0x72
#define IP5306_REG_READ3      0x77
#define IP5306_REG_READ4      0x78

#define IP5306_BIT_CHARGING   0x08
#define IP5306_BIT_FULL       0x08
#define IP5306_BIT_BOOST      0x02

static i2c_master_dev_handle_t s_dev;

/* Guards s_dev. The poll task reads it every 2 s while the `pmic` console
 * command can remove and re-add it, so the handle must not be swapped out
 * from under an in-flight transaction. */
static SemaphoreHandle_t s_lock;

static void pmic_lock(void)
{
    if (s_lock) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
    }
}

static void pmic_unlock(void)
{
    if (s_lock) {
        xSemaphoreGive(s_lock);
    }
}

/* Bring up a second I2C bus for the PMIC when bodge pins are configured.
 * Returns the bus to use, or the shared one when no bodge is set up. */
static i2c_master_bus_handle_t pmic_bus(void)
{
    return g_app.i2c_bus;
}

static esp_err_t ip5306_read(uint8_t reg, uint8_t *out)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, out, 1, 100);
}

static esp_err_t ip5306_write(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(s_dev, buf, sizeof(buf), 100);
}

/* The gauge only reports four coarse steps. */
static int level_from_reg78(uint8_t v)
{
    switch (v >> 4) {
        case 0x00: return 100;
        case 0x08: return 75;
        case 0x0C: return 50;
        case 0x0E: return 25;
        default:   return 0;
    }
}

esp_err_t hw_power_init(void)
{
    /* The plain IP5306 has no I2C interface (see the file header), so this
     * probe can never succeed. It is kept so the failure is reported as a
     * hardware fact rather than looking like a bus or wiring fault. */
    if (!BSP_IP5306_HAS_I2C) {
        ESP_LOGW(TAG, "IP5306 has no I2C interface on this board");
        ESP_LOGW(TAG, "  U1 is the plain ESOP-8 part: VIN/LED1-3/KEY/BAT/SW/VOUT");
        ESP_LOGW(TAG, "  there is no SDA or SCL, so charge state cannot be read");
        ESP_LOGW(TAG, "  charge state is only shown by the board's own LEDs");
        g_app.power.present = false;
        g_app.power.level_pct = -1;
        g_app.results.power = TEST_SKIP;
        return ESP_ERR_NOT_SUPPORTED;
    }

    static const uint8_t addrs[] = {0x75, 0x74, 0x76};
    uint8_t found_addr = 0;
    esp_err_t probe = ESP_FAIL;
    i2c_master_bus_handle_t bus = pmic_bus();

    if (!s_lock) {
        s_lock = xSemaphoreCreateMutex();
    }

    for (size_t a = 0; a < sizeof(addrs) / sizeof(addrs[0]) && found_addr == 0; a++) {
        for (int attempt = 0; attempt < 3; attempt++) {
            probe = i2c_master_probe(bus, addrs[a], 500);
            if (probe == ESP_OK) {
                found_addr = addrs[a];
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

    if (found_addr == 0) {
        ESP_LOGW(TAG, "IP5306 not responding at 0x74/0x75/0x76 (%s)",
                 esp_err_to_name(probe));
        ESP_LOGW(TAG, "  the PMIC is fitted but its SDA/SCL are not routed to the ESP32");
        ESP_LOGW(TAG, "  charge state cannot be read in firmware on this unit");
        ESP_LOGW(TAG, "  to fix: bodge SDA/SCL to free pins and set");
        ESP_LOGW(TAG, "  BSP_PMIC_I2C_SDA_GPIO / BSP_PMIC_I2C_SCL_GPIO in board.h");
        g_app.power.present = false;
        g_app.power.level_pct = -1;
        g_app.results.power = TEST_FAIL;
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "IP5306 ACKed at 0x%02X", found_addr);

    /* Re-probing (the `pmic` console command) must not leak a device
     * handle each time, and must not swap the handle out from under the
     * poll task mid-transaction. */
    pmic_lock();
    if (s_dev) {
        i2c_master_bus_rm_device(s_dev);
        s_dev = NULL;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = found_addr,
        .scl_speed_hz = BSP_I2C_FREQ_HZ,
    };
    esp_err_t add_err = i2c_master_bus_add_device(bus, &dev_cfg, &s_dev);
    pmic_unlock();
    if (add_err != ESP_OK) {
        ESP_LOGE(TAG, "add device failed: %s", esp_err_to_name(add_err));
        return add_err;
    }

    /* Keep the boost converter alive so the board does not brown out when
     * running from the battery with no USB attached. */
    uint8_t ctl0 = 0;
    pmic_lock();
    bool ctl_ok = (ip5306_read(IP5306_REG_SYS_CTL0, &ctl0) == ESP_OK);
    if (ctl_ok) {
        ESP_LOGI(TAG, "SYS_CTL0 = 0x%02X (boost %s, charger %s)",
                 ctl0,
                 (ctl0 & IP5306_BIT_BOOST) ? "on" : "off",
                 (ctl0 & 0x10) ? "on" : "off");
        if (!(ctl0 & IP5306_BIT_BOOST)) {
            ESP_LOGI(TAG, "enabling boost output");
            ip5306_write(IP5306_REG_SYS_CTL0, ctl0 | IP5306_BIT_BOOST);
        }
    }
    pmic_unlock();

    g_app.power.present = true;
    g_app.results.power = TEST_PASS;
    ESP_LOGI(TAG, "IP5306 ready");
    hw_power_poll();
    return ESP_OK;
}

void hw_power_poll(void)
{
    pmic_lock();
    if (!s_dev) {
        pmic_unlock();
        return;
    }

    uint8_t r0 = 0, r1 = 0, r2 = 0, r3 = 0, r4 = 0;
    bool ok = true;
    ok &= (ip5306_read(IP5306_REG_READ0, &r0) == ESP_OK);
    ok &= (ip5306_read(IP5306_REG_READ1, &r1) == ESP_OK);
    ok &= (ip5306_read(IP5306_REG_READ2, &r2) == ESP_OK);
    ok &= (ip5306_read(IP5306_REG_READ3, &r3) == ESP_OK);
    ok &= (ip5306_read(IP5306_REG_READ4, &r4) == ESP_OK);
    pmic_unlock();

    if (!ok) {
        ESP_LOGW(TAG, "IP5306 read failed");
        g_app.results.power = TEST_FAIL;
        return;
    }

    g_app.power.reg70 = r0;
    g_app.power.reg71 = r1;
    g_app.power.reg72 = r2;
    g_app.power.reg77 = r3;
    g_app.power.reg78 = r4;

    bool charging = (r0 & IP5306_BIT_CHARGING) != 0;
    bool full = (r1 & IP5306_BIT_FULL) != 0;

    g_app.power.charge_full = full;
    /* "Charging" means current is flowing: the charger is active and the
     * cell is not already full. */
    g_app.power.charging = charging && !full;
    g_app.power.level_pct = level_from_reg78(r4);
    g_app.results.power = TEST_PASS;
}
