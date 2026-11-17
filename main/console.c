/*
 * Serial console for testing without the touch UI.
 *
 * The on-screen buttons depend on the touch panel and on the LVGL task
 * being healthy, which makes them a poor way to isolate an audio fault.
 * These commands drive the same code paths directly from the UART, so a
 * result here is independent of the display stack.
 */
#include "app.h"
#include "board.h"

#include "esp_console.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "console";

static int cmd_tone(int argc, char **argv)
{
    int freq = (argc > 1) ? atoi(argv[1]) : 1000;
    int ms = (argc > 2) ? atoi(argv[2]) : 500;
    bool square = (argc > 3) && (strcmp(argv[3], "sq") == 0);

    ESP_LOGI(TAG, "tone %d Hz %d ms %s", freq, ms, square ? "square" : "sine");
    if (square) {
        hw_audio_play_square(freq, ms);
    } else {
        hw_audio_play_tone(freq, ms);
    }
    return 0;
}

static int cmd_bits(int argc, char **argv)
{
    if (argc < 2) {
        printf("current: %d-bit\n", hw_audio_get_bits());
        return 0;
    }
    int bits = atoi(argv[1]);
    esp_err_t err = hw_audio_set_bits(bits);
    if (err != ESP_OK) {
        printf("set %d-bit -> %s\n", bits, esp_err_to_name(err));
        return 0;
    }
    /* The change is applied asynchronously by the audio task, so wait for
     * it to settle before reporting - otherwise this prints the old width
     * and looks like the request was ignored. */
    for (int i = 0; i < 100 && hw_audio_get_pending_bits() != 0; i++) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelay(pdMS_TO_TICKS(50));
    printf("set %d-bit -> now %d-bit\n", bits, hw_audio_get_bits());
    return 0;
}

static int cmd_vol(int argc, char **argv)
{
    if (argc < 2) {
        printf("current: %d%%\n", hw_audio_get_volume());
        return 0;
    }
    hw_audio_set_volume(atoi(argv[1]));
    printf("volume now %d%%\n", hw_audio_get_volume());
    return 0;
}

static int cmd_selftest(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("playing 1 beep at 32-bit, 2 at 24-bit, 3 at 16-bit\n");
    hw_audio_selftest();
    return 0;
}

static int cmd_relay(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("toggling GPIO 1/2/40 as plain outputs\n");
    hw_audio_suspend();
    const gpio_num_t pins[3] = {GPIO_NUM_1, GPIO_NUM_2, GPIO_NUM_40};
    for (int i = 0; i < 3; i++) {
        gpio_set_direction(pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(pins[i], 0);
    }
    for (int n = 0; n < 3; n++) {
        for (int i = 0; i < 3; i++) {
            gpio_set_level(pins[i], 1);
        }
        vTaskDelay(pdMS_TO_TICKS(150));
        for (int i = 0; i < 3; i++) {
            gpio_set_level(pins[i], 0);
        }
        vTaskDelay(pdMS_TO_TICKS(150));
    }
    hw_audio_restore_pins();
    printf("done, I2S restored\n");
    return 0;
}

static int cmd_i2c(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    board_i2c_scan();
    printf("%d device(s) found\n", g_app.i2c_found_count);
    for (int i = 0; i < g_app.i2c_found_count; i++) {
        printf("  0x%02X\n", g_app.i2c_found[i]);
    }
    return 0;
}

static int cmd_pmic(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    esp_err_t err = hw_power_init();
    if (err == ESP_OK) {
        hw_power_poll();
        printf("found: charging=%d full=%d level=%d%%\n",
               g_app.power.charging, g_app.power.charge_full,
               g_app.power.level_pct);
        printf("regs 70=%02X 71=%02X 72=%02X 77=%02X 78=%02X\n",
               g_app.power.reg70, g_app.power.reg71, g_app.power.reg72,
               g_app.power.reg77, g_app.power.reg78);
    } else if (err == ESP_ERR_NOT_SUPPORTED) {
        printf("no I2C interface: U1 is the plain IP5306 (ESOP-8)\n");
        printf("  pins: VIN LED1 LED2 LED3 KEY BAT SW VOUT - no SDA/SCL\n");
        printf("  charge state is only visible on the board LEDs\n");
    } else {
        printf("not found (%s)\n", esp_err_to_name(err));
    }
    return 0;
}

static int cmd_chan(int argc, char **argv)
{
    static const char *names[3] = {"both", "left", "right"};
    if (argc < 2) {
        printf("current: %s\n", names[hw_audio_get_channel()]);
        return 0;
    }
    int ch;
    if (strcmp(argv[1], "both") == 0) {
        ch = 0;
    } else if (strcmp(argv[1], "left") == 0) {
        ch = 1;
    } else if (strcmp(argv[1], "right") == 0) {
        ch = 2;
    } else {
        printf("usage: chan [both|left|right]\n");
        return 0;
    }
    hw_audio_set_channel(ch);
    printf("channel now %s\n", names[ch]);
    return 0;
}

static int cmd_bl(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: bl <0-100>\n");
        return 0;
    }
    int pct = atoi(argv[1]);
    board_backlight_set(pct);
    printf("backlight %d%%\n", pct);
    return 0;
}

static int cmd_wifi(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("scanning...\n");
    esp_err_t err = hw_wifi_scan();
    if (err != ESP_OK) {
        printf("scan failed: %s\n", esp_err_to_name(err));
        return 0;
    }
    printf("%d network(s):\n", g_app.wifi.ap_count);
    for (int i = 0; i < g_app.wifi.ap_count; i++) {
        printf("  %-24s %4d dBm  ch%2d  auth %d\n",
               g_app.wifi.aps[i].ssid, g_app.wifi.aps[i].rssi,
               g_app.wifi.aps[i].channel, g_app.wifi.aps[i].auth);
    }
    return 0;
}

static int cmd_ble(int argc, char **argv)
{
    int ms = (argc > 1) ? atoi(argv[1]) : 3000;
    printf("scanning for %d ms...\n", ms);
    esp_err_t err = hw_ble_scan(ms);
    if (err != ESP_OK) {
        printf("scan failed: %s\n", esp_err_to_name(err));
        return 0;
    }
    printf("%d device(s)\n", g_app.ble.device_count);
    for (int i = 0; i < g_app.ble.device_count; i++) {
        printf("  %-24s %4d dBm\n",
               g_app.ble.devices[i].name, g_app.ble.devices[i].rssi);
    }
    return 0;
}

static int cmd_status(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("display=%d touch=%d power=%d sd=%d audio=%d i2c=%d wifi=%d ble=%d\n",
           g_app.results.display, g_app.results.touch, g_app.results.power,
           g_app.results.sdcard, g_app.results.audio, g_app.results.i2c,
           g_app.results.wifi, g_app.results.ble);
    printf("audio: %d-bit, vol %d%%, %lu tones, %lu bytes\n",
           hw_audio_get_bits(), hw_audio_get_volume(),
           (unsigned long)g_app.audio_tones_played,
           (unsigned long)g_app.audio_bytes_written);
    static const char *chn[3] = {"both", "left", "right"};
    printf("audio: channel %s\n", chn[hw_audio_get_channel()]);
    printf("touch: present=%d id=\"%s\" events=%lu polls=%lu errors=%lu\n",
           g_app.touch_present, g_app.touch_product_id,
           (unsigned long)g_app.touch_events,
           (unsigned long)g_app.touch_polls,
           (unsigned long)g_app.touch_read_errors);
    printf("power: present=%d charging=%d level=%d%%\n",
           g_app.power.present, g_app.power.charging, g_app.power.level_pct);
    printf("power: last reset=%d boots=%lu brownouts=%lu\n",
           g_app.power.last_reset,
           (unsigned long)g_app.power.boot_count,
           (unsigned long)g_app.power.brownout_count);
    return 0;
}

void console_init(void)
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "hwtest>";
    repl_cfg.max_cmdline_length = 128;

    esp_console_dev_uart_config_t uart_cfg = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    if (esp_console_new_repl_uart(&uart_cfg, &repl_cfg, &repl) != ESP_OK) {
        ESP_LOGW(TAG, "console init failed");
        return;
    }

    const esp_console_cmd_t cmds[] = {
        {.command = "tone", .help = "tone <hz> [ms] [sq]", .func = cmd_tone},
        {.command = "bits", .help = "bits [16|24|32]", .func = cmd_bits},
        {.command = "vol", .help = "vol [0-100]", .func = cmd_vol},
        {.command = "chan", .help = "chan [both|left|right]", .func = cmd_chan},
        {.command = "bl", .help = "bl <0-100> backlight brightness", .func = cmd_bl},
        {.command = "selftest", .help = "beep at each slot width", .func = cmd_selftest},
        {.command = "relay", .help = "toggle GPIO 1/2/40", .func = cmd_relay},
        {.command = "i2c", .help = "rescan the I2C bus", .func = cmd_i2c},
        {.command = "pmic", .help = "probe the IP5306 and dump registers", .func = cmd_pmic},
        {.command = "wifi", .help = "scan for WiFi access points", .func = cmd_wifi},
        {.command = "ble", .help = "ble [ms] scan for BLE devices", .func = cmd_ble},
        {.command = "status", .help = "print subsystem state", .func = cmd_status},
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        esp_console_cmd_register(&cmds[i]);
    }

    esp_console_start_repl(repl);
    ESP_LOGI(TAG, "console ready: tone, bits, vol, selftest, relay, i2c, status");
}
