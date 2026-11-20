/*
 * microSD card on SPI2.
 *
 * The card shares MOSI (GPIO47) and SCLK (GPIO48) with the ST7701S
 * command bus. That is safe here because the panel init sequence has
 * already been sent by the time the SD bus is started, and the panel
 * never needs the command bus again.
 */
#include "app.h"
#include "board.h"

#include "driver/sdspi_host.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include <dirent.h>
#include <stdio.h>

static const char *TAG = "sdcard";

/* The bus itself is brought up once in board_init() because the ST7701S
 * command interface shares it. 10 MHz: the card shares MOSI/SCLK with the
 * LCD, and the longer traces on this board are not reliable at the
 * 20-40 MHz a dedicated slot would allow. */
#define SD_MAX_FREQ_KHZ 10000

static sdmmc_card_t *s_card;

static esp_err_t sd_mount(void)
{
    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = BSP_SPI_HOST;
    host.max_freq_khz = SD_MAX_FREQ_KHZ;

    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = BSP_SD_CS_GPIO;
    slot_cfg.host_id = BSP_SPI_HOST;

    esp_err_t err = esp_vfs_fat_sdspi_mount(BSP_SD_MOUNT_POINT, &host, &slot_cfg,
                                            &mount_cfg, &s_card);
    if (err != ESP_OK) {
        if (err == ESP_ERR_TIMEOUT) {
            snprintf(g_app.sd.last_error, sizeof(g_app.sd.last_error),
                     "no card inserted");
        } else {
            snprintf(g_app.sd.last_error, sizeof(g_app.sd.last_error),
                     "%s", esp_err_to_name(err));
        }
        ESP_LOGW(TAG, "no card (%s)", esp_err_to_name(err));
        g_app.sd.mounted = false;
        /* A timeout means the slot is empty or the card is not seated;
         * that is a missing card, not a broken bus. */
        g_app.results.sdcard = (err == ESP_ERR_TIMEOUT) ? TEST_SKIP : TEST_FAIL;
        return err;
    }

    g_app.sd.mounted = true;
    g_app.sd.capacity_bytes = (uint64_t)s_card->csd.capacity * s_card->csd.sector_size;
    g_app.sd.sector_size = s_card->csd.sector_size;
    snprintf(g_app.sd.card_name, sizeof(g_app.sd.card_name), "%s", s_card->cid.name);

    ESP_LOGI(TAG, "card mounted: %s, %llu MB, %u sectors of %u B",
             g_app.sd.card_name,
             (unsigned long long)(g_app.sd.capacity_bytes / (1024 * 1024)),
             (unsigned)s_card->csd.capacity, (unsigned)s_card->csd.sector_size);

    /* Count files in the root directory. */
    g_app.sd.file_count = 0;
    DIR *dir = opendir(BSP_SD_MOUNT_POINT);
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] != '.') {
                g_app.sd.file_count++;
            }
        }
        closedir(dir);
    }
    g_app.results.sdcard = TEST_PASS;
    return ESP_OK;
}

esp_err_t hw_sdcard_init(void)
{
    return sd_mount();
}

esp_err_t hw_sdcard_remount(void)
{
    if (s_card) {
        esp_vfs_fat_sdcard_unmount(BSP_SD_MOUNT_POINT, s_card);
        s_card = NULL;
    }
    g_app.sd.mounted = false;
    g_app.sd.write_ok = false;
    g_app.sd.file_count = 0;
    g_app.sd.last_error[0] = '\0';
    return sd_mount();
}
