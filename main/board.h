/*
 * Board pin map for the Guition / CYD ESP32-S3-4848S040.
 *
 * 4" 480x480 IPS panel, ST7701S controller, 18-bit RGB parallel bus.
 * GT911 capacitive touch on I2C. IP5306 PMIC (battery + charger).
 * NS4168 I2S class-D amplifier. microSD on SPI2.
 *
 * NOTE: the three relay pins (GPIO1/2/40) and the three I2S pins
 * (BCLK 1, LRCLK 2, DIN 40) are the SAME physical pins. Which function
 * is live depends on three 0-ohm links near the amplifier:
 *   R21/R22/R23 -> speaker (I2S)
 *   R25/R26/R27 -> relays
 * This firmware drives them as I2S and never as relays.
 */
#pragma once

#include "driver/gpio.h"
#include "driver/spi_master.h"

/* ---- Shared SPI2 bus ----
 * The ST7701S command bus and the microSD slot are wired to the same
 * MOSI/SCLK pins, so they share one hardware SPI bus with separate chip
 * selects. The LCD drives its own CS as a plain GPIO because its protocol
 * needs a 9-bit (DC + data) frame that the peripheral cannot express as a
 * normal CS-framed transfer. */
#define BSP_SPI_HOST           SPI2_HOST
#define BSP_SPI_MOSI_GPIO      GPIO_NUM_47
#define BSP_SPI_MISO_GPIO      GPIO_NUM_41
#define BSP_SPI_SCLK_GPIO      GPIO_NUM_48

/* ---- Display: ST7701S command bus (9-bit SPI, manual CS) ---- */
#define BSP_LCD_CS_GPIO        GPIO_NUM_39
#define BSP_LCD_SCLK_GPIO      BSP_SPI_SCLK_GPIO
#define BSP_LCD_MOSI_GPIO      BSP_SPI_MOSI_GPIO
#define BSP_LCD_SPI_HZ         (1 * 1000 * 1000)

/* ---- Display: RGB parallel pixel bus ---- */
#define BSP_LCD_DE_GPIO        GPIO_NUM_18
#define BSP_LCD_VSYNC_GPIO     GPIO_NUM_17
#define BSP_LCD_HSYNC_GPIO     GPIO_NUM_16
#define BSP_LCD_PCLK_GPIO      GPIO_NUM_21

/* Data lines, index 0 = LSB of the 16-bit RGB565 word.
 * The panel is wired 5-6-5 (R0..R4, G0..G5, B0..B4) which is exactly
 * 16 lines, so the RGB565 frame buffer maps straight onto the bus. */
#define BSP_LCD_DATA0_GPIO     GPIO_NUM_4    /* B0 */
#define BSP_LCD_DATA1_GPIO     GPIO_NUM_5    /* B1 */
#define BSP_LCD_DATA2_GPIO     GPIO_NUM_6    /* B2 */
#define BSP_LCD_DATA3_GPIO     GPIO_NUM_7    /* B3 */
#define BSP_LCD_DATA4_GPIO     GPIO_NUM_15   /* B4 */
#define BSP_LCD_DATA5_GPIO     GPIO_NUM_8    /* G0 */
#define BSP_LCD_DATA6_GPIO     GPIO_NUM_20   /* G1 */
#define BSP_LCD_DATA7_GPIO     GPIO_NUM_3    /* G2 */
#define BSP_LCD_DATA8_GPIO     GPIO_NUM_46   /* G3 */
#define BSP_LCD_DATA9_GPIO     GPIO_NUM_9    /* G4 */
#define BSP_LCD_DATA10_GPIO    GPIO_NUM_10   /* G5 */
#define BSP_LCD_DATA11_GPIO    GPIO_NUM_11   /* R0 */
#define BSP_LCD_DATA12_GPIO    GPIO_NUM_12   /* R1 */
#define BSP_LCD_DATA13_GPIO    GPIO_NUM_13   /* R2 */
#define BSP_LCD_DATA14_GPIO    GPIO_NUM_14   /* R3 */
#define BSP_LCD_DATA15_GPIO    GPIO_NUM_0    /* R4 */

#define BSP_LCD_H_RES          480
#define BSP_LCD_V_RES          480
#define BSP_LCD_PCLK_HZ        (12 * 1000 * 1000)

/* ---- Backlight (LEDC PWM, active high) ---- */
#define BSP_LCD_BL_GPIO        GPIO_NUM_38
#define BSP_LCD_BL_ON_LEVEL    1

/* ---- Power budget ----
 *
 * The board has no bulk capacitance on its 5 V rail and no current
 * limiting on the P1 input (Q1 is a permanently-on P-FET), so the supply
 * is whatever the adapter gives. Audio and backlight are the two loads
 * that can be controlled from firmware, and both are scaled here so the
 * rail is never asked for more than it can give.
 *
 * The scaling is deliberately conservative because WiFi and Bluetooth are
 * expected to run as well: an ESP32-S3 radio burst is ~240 mA on its own,
 * on top of ~120 mA for the chip and display logic. Reserving that
 * headroom is what keeps an audio burst from collapsing the rail.
 *
 * These are invisible to the user: 100% on the volume slider and 100% on
 * the brightness slider map to these values, so the controls still run
 * their full range. Raise them only if you are powering from a supply
 * that can genuinely deliver 2 A. */
#define BSP_AUDIO_MAX_PCT      60   /* of full-scale waveform amplitude */
#define BSP_BACKLIGHT_MAX_PCT  70   /* of full LEDC duty */

/* Brightness applied at boot when nothing is saved, as a percentage of the
 * user-facing range. */
#define BSP_BACKLIGHT_DEFAULT_PCT 50

/* Volume applied at boot when nothing is saved. */
#define BSP_VOLUME_DEFAULT_PCT    50

/* ---- Touch: GT911 on I2C ---- */
#define BSP_I2C_SDA_GPIO       GPIO_NUM_19
#define BSP_I2C_SCL_GPIO       GPIO_NUM_45
/* 100 kHz rather than 400 kHz: the IP5306 on this bus stretches the clock,
 * which the GT911 tolerates but the PMIC does not at higher rates. */
#define BSP_I2C_FREQ_HZ        100000
#define BSP_TOUCH_ADDR         0x5D
#define BSP_TOUCH_ADDR_ALT     0x14
#define BSP_TOUCH_RST_GPIO     GPIO_NUM_NC
#define BSP_TOUCH_INT_GPIO     GPIO_NUM_NC

/* ---- Power: IP5306 PMIC ----
 *
 * IMPORTANT: this board uses the plain IP5306, NOT the IP5306-I2C variant.
 * Per the schematic, U1 is an ESOP-8 with these pins:
 *
 *   1 VIN   2 LED1   3 LED2   4 LED3
 *   5 KEY   6 BAT    7 SW     8 VOUT   (9 = thermal pad, GND)
 *
 * There is no SDA and no SCL. The chip has no I2C interface at all, so it
 * can never be probed, read or configured from the ESP32 - no bodge wire
 * can change that. Charge state is only visible on the board's own LEDs,
 * which are driven by LED1/LED2/LED3 as outputs.
 *
 * The address below is kept only so the probe fails with a clear message
 * rather than looking like a wiring fault. */
#define BSP_IP5306_ADDR        0x75
#define BSP_IP5306_HAS_I2C     0   /* plain IP5306: no I2C interface */

/* ---- microSD on the shared SPI2 bus ---- */
#define BSP_SD_CS_GPIO         GPIO_NUM_42
#define BSP_SD_MISO_GPIO       BSP_SPI_MISO_GPIO
#define BSP_SD_MOSI_GPIO       BSP_SPI_MOSI_GPIO
#define BSP_SD_SCLK_GPIO       BSP_SPI_SCLK_GPIO
#define BSP_SD_MOUNT_POINT     "/sdcard"

/* ---- Audio: NS4168 I2S amplifier ---- */
#define BSP_I2S_BCLK_GPIO      GPIO_NUM_1
#define BSP_I2S_LRCLK_GPIO     GPIO_NUM_2
#define BSP_I2S_DOUT_GPIO      GPIO_NUM_40
/* 22050 Hz, 16-bit stereo: this is the configuration that is known to
 * work with the NS4168 on ESP32-S3 under ESP-IDF 5/6. */
#define BSP_I2S_SAMPLE_RATE    22050
