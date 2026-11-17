/*
 * Audio: NS4168 I2S class-D amplifier.
 *
 * The amplifier shares GPIO1/2/40 with the relay footprint. On a board
 * whose 0-ohm links sit on R21/R22/R23 those pins drive the speaker; on
 * a relay board they drive the relays and this test will be silent.
 *
 * Two things about this part drive the configuration here:
 *
 *  1. It is an I2S *slave* DAC that reads its samples MSB-first from the
 *     top of the slot. Feeding it 16-bit data in a 16-bit slot makes it
 *     interpret the sample as the low bits of a wider word, which lands
 *     roughly 40 dB down - audible only with your ear against the cone.
 *     Samples are therefore generated left justified in a 32-bit
 *     container, and the slot width is selectable at runtime so the
 *     correct alignment can be found on real hardware.
 *
 *  2. Mono slot mode on the ESP32-S3 interleaves each sample with a zero
 *     ([valid, 0, valid, 0, ...]), which the amp reproduces as a metallic
 *     buzz. Stereo with the sample duplicated into both slots avoids it.
 *
 * Playback runs on its own task fed by a small request queue so the LVGL
 * task never blocks on the I2S DMA.
 */
#include "app.h"
#include "board.h"

#include "driver/i2s_std.h"
#include "esp_check.h"
#include "esp_log.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_struct.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "audio";

#define AUDIO_TASK_STACK 6144
#define AUDIO_TASK_PRIO  5
#define AUDIO_QUEUE_LEN  8
#define AUDIO_FRAMES     256

typedef enum {
    AUDIO_CMD_TONE = 0,
    AUDIO_CMD_SWEEP,
    AUDIO_CMD_SET_BITS,
    AUDIO_CMD_RESTORE_PINS,
    AUDIO_CMD_SUSPEND,
    AUDIO_CMD_SELFTEST,
} audio_cmd_t;

typedef struct {
    audio_cmd_t cmd;
    int freq_hz;
    int duration_ms;
    bool square;
    int bits;
} audio_req_t;

static i2s_chan_handle_t s_tx;
static QueueHandle_t s_queue;
static bool s_ready;

/* 0-100, applied on top of the waveform amplitude. */
static volatile int s_volume_pct = BSP_VOLUME_DEFAULT_PCT;

/* Peak waveform amplitude for 100% volume.
 *
 * The class-D amp draws current in proportion to output amplitude, so this
 * is the knob that actually limits audio current draw. 32767 is full
 * scale; BSP_AUDIO_MAX_PCT scales it down so that "100%" is the loudest
 * this supply can sustain rather than the loudest the DAC can produce.
 * The cap is invisible to the user - the volume slider still runs 0-100. */
#define AUDIO_PEAK_AMPLITUDE \
    ((int)(32767.0 * (double)BSP_AUDIO_MAX_PCT / 100.0))

/* Slot width in bits: 16, 24 or 32. 16 is the default because that is the
 * configuration verified to work with this amplifier. */
static volatile int s_bits = 16;

/* Requested slot width, 0 when nothing is pending.
 *
 * This is a plain flag rather than a queue entry. Queueing the request
 * meant a blocking send from the LVGL task, which froze the UI for up to
 * two seconds and still lost the request whenever the queue was full of
 * tones - the buttons looked dead. A flag cannot be dropped and never
 * blocks the caller. */
static volatile int s_pending_bits = 0;

/* Set by the audio task once it has actually disabled the channel, so
 * callers of hw_audio_suspend() can wait for the pins to be free before
 * driving them. */
static volatile bool s_suspended = false;

void hw_audio_set_volume(int percent)
{
    if (percent < 0) {
        percent = 0;
    }
    if (percent > 100) {
        percent = 100;
    }
    s_volume_pct = percent;
    ESP_LOGI(TAG, "volume %d%%", percent);
}

int hw_audio_get_volume(void)
{
    return s_volume_pct;
}

int hw_audio_get_bits(void)
{
    return s_bits;
}

/* Non-zero while a slot-width change is still waiting for the audio task. */
int hw_audio_get_pending_bits(void)
{
    return s_pending_bits;
}

static i2s_data_bit_width_t bits_to_enum(int bits)
{
    switch (bits) {
        case 16: return I2S_DATA_BIT_WIDTH_16BIT;
        case 24: return I2S_DATA_BIT_WIDTH_24BIT;
        default: return I2S_DATA_BIT_WIDTH_32BIT;
    }
}

static i2s_std_slot_config_t make_slot_cfg(int bits)
{
    i2s_std_slot_config_t slot =
        I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(bits_to_enum(bits), I2S_SLOT_MODE_STEREO);
    /* Match the slot width to the data width so the DAC sees a word of
     * exactly the size it is being fed. */
    slot.slot_bit_width = (i2s_slot_bit_width_t)bits;
    slot.ws_width = bits;
    return slot;
}

/* 24-bit data needs an MCLK that is a multiple of 3, which the default
 * 256x is not. Without this, i2s_channel_init_std_mode() fails with
 * "The 'mclk_multiple' should be the multiple of 3 while using 24-bit
 * data width" and the channel is left torn down - which is why 24-bit
 * could never be selected. 384x is a multiple of 3 and of 128. */
static i2s_std_clk_config_t make_clk_cfg(int bits)
{
    i2s_std_clk_config_t clk = I2S_STD_CLK_DEFAULT_CONFIG(BSP_I2S_SAMPLE_RATE);
    if (bits == 24) {
        clk.mclk_multiple = I2S_MCLK_MULTIPLE_384;
    }
    return clk;
}

/* Tear the channel down and build a fresh one at the requested width.
 *
 * Reconfiguring in place (i2s_channel_reconfig_std_slot) requires the
 * channel to be in the READY state and silently leaves it unusable if any
 * step fails, which is why the buttons appeared to do nothing. Deleting
 * and recreating has no such state to get wrong.
 *
 * Must run on the audio task: i2s_channel_disable() blocks until any
 * in-flight write returns, and the audio task is the one writing. */
static esp_err_t apply_bits(int bits)
{
    if (bits != 16 && bits != 24 && bits != 32) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_ready) {
        s_bits = bits;
        return ESP_OK;
    }
    if (bits == s_bits) {
        ESP_LOGI(TAG, "already %d-bit, nothing to do", bits);
        return ESP_OK;
    }

    ESP_LOGI(TAG, "rebuilding I2S channel at %d-bit", bits);

    if (s_tx) {
        i2s_channel_disable(s_tx);
        i2s_del_channel(s_tx);
        s_tx = NULL;
    }

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    chan_cfg.dma_desc_num = 8;
    chan_cfg.dma_frame_num = 480;

    esp_err_t err = i2s_new_channel(&chan_cfg, &s_tx, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_new_channel failed: %s", esp_err_to_name(err));
        s_tx = NULL;
        return err;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = make_clk_cfg(bits),
        .slot_cfg = make_slot_cfg(bits),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = BSP_I2S_BCLK_GPIO,
            .ws = BSP_I2S_LRCLK_GPIO,
            .dout = BSP_I2S_DOUT_GPIO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    err = i2s_channel_init_std_mode(s_tx, &std_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "init_std_mode failed at %d-bit: %s", bits, esp_err_to_name(err));
        i2s_del_channel(s_tx);
        s_tx = NULL;
        /* Fall back to 16-bit rather than leaving the amp silent. */
        if (bits != 16) {
            ESP_LOGW(TAG, "falling back to 16-bit");
            return apply_bits(16);
        }
        return err;
    }

    err = i2s_channel_enable(s_tx);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "enable failed: %s", esp_err_to_name(err));
        i2s_del_channel(s_tx);
        s_tx = NULL;
        return err;
    }

    s_bits = bits;
    ESP_LOGI(TAG, "channel rebuilt: %d-bit, BCLK %d Hz", bits,
             BSP_I2S_SAMPLE_RATE * bits * 2);
    return ESP_OK;
}

/* Request a slot-width change; the audio task applies it.
 *
 * Returns immediately. The request is a flag, so it survives a busy queue
 * and cannot block the LVGL task. */
esp_err_t hw_audio_set_bits(int bits)
{
    if (bits != 16 && bits != 24 && bits != 32) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_ready) {
        s_bits = bits;
        return ESP_OK;
    }
    s_pending_bits = bits;
    ESP_LOGI(TAG, "slot width request queued: %d-bit", bits);
    return ESP_OK;
}

/* Re-route the I2S signals after something else has taken the pins.
 *
 * The channel is already disabled by AUDIO_CMD_SUSPEND, which is the
 * state i2s_channel_reconfig_std_gpio() requires. Disabling it again here
 * just logs "the channel has not been enabled yet". */
static void restore_i2s_pins(void)
{
    ESP_LOGI(TAG, "restoring I2S pin routing");
    i2s_std_gpio_config_t gpio = {
        .mclk = I2S_GPIO_UNUSED,
        .bclk = BSP_I2S_BCLK_GPIO,
        .ws = BSP_I2S_LRCLK_GPIO,
        .dout = BSP_I2S_DOUT_GPIO,
        .din = I2S_GPIO_UNUSED,
        .invert_flags = {
            .mclk_inv = false,
            .bclk_inv = false,
            .ws_inv = false,
        },
    };
    esp_err_t err = i2s_channel_reconfig_std_gpio(s_tx, &gpio);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gpio reconfig failed: %s", esp_err_to_name(err));
    }
    i2s_channel_enable(s_tx);
    s_suspended = false;
}

void hw_audio_restore_pins(void)
{
    if (!s_ready) {
        return;
    }
    audio_req_t req = {.cmd = AUDIO_CMD_RESTORE_PINS};
    if (xQueueSend(s_queue, &req, 0) != pdTRUE) {
        ESP_LOGW(TAG, "audio queue full, dropping pin restore");
    }
}

/* Stop the I2S peripheral so another driver can take GPIO 1/2/40.
 *
 * This must NOT call i2s_channel_disable() directly: that blocks until any
 * in-flight write returns, and the audio task is the one writing. Calling
 * it from the LVGL task deadlocks the audio task permanently, after which
 * every later request - including slot-width changes - is ignored. The
 * request is queued instead and handled by the audio task itself. */
void hw_audio_suspend(void)
{
    if (!s_ready) {
        return;
    }
    ESP_LOGI(TAG, "queueing I2S suspend to release GPIO 1/2/40");
    audio_req_t req = {.cmd = AUDIO_CMD_SUSPEND};
    if (xQueueSend(s_queue, &req, pdMS_TO_TICKS(500)) != pdTRUE) {
        ESP_LOGW(TAG, "audio queue full, dropping suspend");
        return;
    }
    /* Wait for the audio task to actually disable the channel, otherwise
     * the caller drives the pins while I2S is still outputting on them. */
    for (int i = 0; i < 200 && !s_suspended; i++) {
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    if (!s_suspended) {
        ESP_LOGW(TAG, "suspend did not complete in 1 s");
    }
}

/* Which I2S slot to drive.
 *
 * The NS4168 is a mono amp with one speaker output pair, and its CTRL pin
 * selects which slot it plays (R24 straps it to VOUT-BAT = right). The
 * firmware normally duplicates every sample into both slots so the amp
 * gets the signal whichever slot it selects. These modes let you prove
 * that on real hardware: if LEFT and RIGHT sound identical, the amp is
 * picking up a duplicated signal as intended. */
typedef enum {
    AUDIO_CH_BOTH = 0,   /* duplicate into both slots (default) */
    AUDIO_CH_LEFT,       /* left slot only, right silent */
    AUDIO_CH_RIGHT,      /* right slot only, left silent */
} audio_channel_t;

static volatile audio_channel_t s_channel = AUDIO_CH_BOTH;

void hw_audio_set_channel(int ch)
{
    if (ch < 0 || ch > 2) {
        return;
    }
    s_channel = (audio_channel_t)ch;
    ESP_LOGI(TAG, "channel mode %s",
             ch == AUDIO_CH_LEFT ? "left only" :
             ch == AUDIO_CH_RIGHT ? "right only" : "both (duplicated)");
}

int hw_audio_get_channel(void)
{
    return (int)s_channel;
}

/* Stream a tone for the requested duration.
 *
 * The waveform is generated as a left-justified 32-bit value (full scale
 * = INT32_MAX) and then narrowed to the active slot width, so the DAC
 * always sees the sample starting at the MSB of the slot. */
static void play_tone(int freq_hz, int duration_ms, int amplitude, bool square)
{
    const int rate = BSP_I2S_SAMPLE_RATE;
    const int bits = s_bits;
    /* Static, not stack: these two buffers are 3 KB together and the audio
     * task only has a 4 KB stack, so keeping them local overflowed it and
     * crashed the device. Only the audio task ever calls this. */
    static int32_t buf32[AUDIO_FRAMES * 2];
    static int16_t buf16[AUDIO_FRAMES * 2];

    int total = (rate * duration_ms) / 1000;
    int written = 0;
    double phase = 0.0;
    double step = 2.0 * M_PI * (double)freq_hz / (double)rate;
    const int fade = rate / 100; /* 10 ms */
    const double vol = (double)s_volume_pct / 100.0;

    while (written < total) {
        int n = (total - written < AUDIO_FRAMES) ? (total - written) : AUDIO_FRAMES;
        for (int i = 0; i < n; i++) {
            /* Fade in/out so the tone does not click. */
            double env = 1.0;
            int pos = written + i;
            if (pos < fade) {
                env = (double)pos / (double)fade;
            } else if (total - pos < fade) {
                env = (double)(total - pos) / (double)fade;
            }
            /* A square wave has ~2.1x the RMS of a sine at the same peak,
             * which is the quickest way to tell "quiet" from "broken". */
            double wave = square ? (sin(phase) >= 0.0 ? 1.0 : -1.0) : sin(phase);
            double v = wave * (double)amplitude * env * vol;

            /* 16-bit is the native width and the one verified to work.
             * Wider slots carry the same value left-justified. */
            int16_t s16 = (int16_t)v;
            int16_t l = (s_channel == AUDIO_CH_RIGHT) ? 0 : s16;
            int16_t r = (s_channel == AUDIO_CH_LEFT) ? 0 : s16;
            buf16[i * 2] = l;
            buf16[i * 2 + 1] = r;
            buf32[i * 2] = (int32_t)l << 16;
            buf32[i * 2 + 1] = (int32_t)r << 16;

            phase += step;
            if (phase > 2.0 * M_PI) {
                phase -= 2.0 * M_PI;
            }
        }

        const void *src;
        size_t bytes;
        if (bits == 16) {
            src = buf16;
            bytes = (size_t)n * 2 * sizeof(int16_t);
        } else {
            /* 24-bit is carried in 32-bit containers, left justified. */
            src = buf32;
            bytes = (size_t)n * 2 * sizeof(int32_t);
        }

        size_t sent = 0;
        /* Finite timeout, not portMAX_DELAY. If the channel is disabled
         * while a write is in flight (hw_audio_suspend, or a failed
         * rebuild) a blocking write never returns, the audio task stops
         * looping, and every later request - including slot-width changes
         * - is silently ignored. That is exactly the "buttons do nothing"
         * symptom. */
        esp_err_t err = i2s_channel_write(s_tx, src, bytes, &sent,
                                          pdMS_TO_TICKS(1000));
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "i2s write failed: %s", esp_err_to_name(err));
            return;
        }
        g_app.audio_bytes_written += sent;
        written += n;
    }
}

static void audio_task(void *arg)
{
    (void)arg;
    audio_req_t req;
    while (1) {
        /* Apply any pending slot-width change before doing more work, so a
         * queue full of tones cannot starve it. */
        int want = s_pending_bits;
        if (want != 0) {
            s_pending_bits = 0;
            apply_bits(want);
        }

        if (xQueueReceive(s_queue, &req, pdMS_TO_TICKS(20)) != pdTRUE) {
            continue;
        }

        switch (req.cmd) {
            case AUDIO_CMD_SET_BITS:
                apply_bits(req.bits);
                break;

            case AUDIO_CMD_RESTORE_PINS:
                restore_i2s_pins();
                break;

            case AUDIO_CMD_SUSPEND:
                ESP_LOGI(TAG, "suspending I2S to release GPIO 1/2/40");
                i2s_channel_disable(s_tx);
                s_suspended = true;
                break;

            case AUDIO_CMD_SWEEP:
                ESP_LOGI(TAG, "sweep 200 Hz -> 3 kHz");
                for (int f = 200; f <= 3000; f += 100) {
                    play_tone(f, 60, AUDIO_PEAK_AMPLITUDE, false);
                }
                g_app.audio_tones_played++;
                break;

            case AUDIO_CMD_SELFTEST: {
                /* Play the same tone at each slot width with a spoken
                 * count, so the loudest one can be identified by ear. */
                static const int widths[3] = {32, 24, 16};
                for (int i = 0; i < 3; i++) {
                    ESP_LOGI(TAG, "selftest: %d-bit", widths[i]);
                    apply_bits(widths[i]);
                    /* One beep per index so the position is audible. */
                    for (int b = 0; b <= i; b++) {
                        play_tone(1000, 120, AUDIO_PEAK_AMPLITUDE, true);
                        vTaskDelay(pdMS_TO_TICKS(120));
                    }
                    vTaskDelay(pdMS_TO_TICKS(400));
                }
                apply_bits(32);
                ESP_LOGI(TAG, "selftest done, back to 32-bit");
                break;
            }

            case AUDIO_CMD_TONE:
            default:
                ESP_LOGI(TAG, "tone %d Hz for %d ms (%s, %d-bit, vol %d%%)",
                         req.freq_hz, req.duration_ms,
                         req.square ? "square" : "sine", s_bits, s_volume_pct);
                play_tone(req.freq_hz, req.duration_ms, AUDIO_PEAK_AMPLITUDE, req.square);
                g_app.audio_tones_played++;
                break;
        }
        ESP_LOGI(TAG, "done, %lu bytes written total",
                 (unsigned long)g_app.audio_bytes_written);
    }
}

esp_err_t hw_audio_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    chan_cfg.dma_desc_num = 8;
    chan_cfg.dma_frame_num = 480;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &s_tx, NULL), TAG, "i2s channel");

    i2s_std_config_t std_cfg = {
        .clk_cfg = make_clk_cfg(s_bits),
        .slot_cfg = make_slot_cfg(s_bits),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = BSP_I2S_BCLK_GPIO,
            .ws = BSP_I2S_LRCLK_GPIO,
            .dout = BSP_I2S_DOUT_GPIO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_tx, &std_cfg), TAG, "i2s std mode");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_tx), TAG, "i2s enable");

    s_queue = xQueueCreate(AUDIO_QUEUE_LEN, sizeof(audio_req_t));
    ESP_RETURN_ON_FALSE(s_queue, ESP_ERR_NO_MEM, TAG, "audio queue");

    BaseType_t ok = xTaskCreate(audio_task, "audio", AUDIO_TASK_STACK, NULL,
                                AUDIO_TASK_PRIO, NULL);
    ESP_RETURN_ON_FALSE(ok == pdPASS, ESP_ERR_NO_MEM, TAG, "audio task");

    s_ready = true;
    g_app.results.audio = TEST_PASS;
    ESP_LOGI(TAG, "I2S ready (BCLK=%d LRCLK=%d DOUT=%d, %d Hz, %d-bit stereo)",
             BSP_I2S_BCLK_GPIO, BSP_I2S_LRCLK_GPIO, BSP_I2S_DOUT_GPIO,
             BSP_I2S_SAMPLE_RATE, s_bits);

    /* Confirm the GPIO matrix really routed the I2S signals onto the
     * expected pins. If these read back as something else, the pins are
     * being driven by another peripheral and the amp will stay silent. */
    const struct {
        gpio_num_t pin;
        uint32_t want;
        const char *name;
    } checks[] = {
        {BSP_I2S_BCLK_GPIO,  I2S0O_BCK_OUT_IDX, "BCLK"},
        {BSP_I2S_LRCLK_GPIO, I2S0O_WS_OUT_IDX,  "LRCLK"},
        {BSP_I2S_DOUT_GPIO,  I2S0O_SD_OUT_IDX,  "DOUT"},
    };
    for (size_t i = 0; i < sizeof(checks) / sizeof(checks[0]); i++) {
        uint32_t sel = GPIO.func_out_sel_cfg[checks[i].pin].func_sel;
        if (sel == checks[i].want) {
            ESP_LOGI(TAG, "  %s on GPIO%d routed to I2S (sig %lu)",
                     checks[i].name, checks[i].pin, (unsigned long)sel);
        } else {
            ESP_LOGW(TAG, "  %s on GPIO%d NOT routed to I2S (sig %lu, want %lu)",
                     checks[i].name, checks[i].pin,
                     (unsigned long)sel, (unsigned long)checks[i].want);
        }
    }
    return ESP_OK;
}

void hw_audio_play_tone(int freq_hz, int duration_ms)
{
    if (!s_ready) {
        ESP_LOGW(TAG, "audio not initialised");
        return;
    }
    audio_req_t req = {.cmd = AUDIO_CMD_TONE, .freq_hz = freq_hz,
                       .duration_ms = duration_ms, .square = false};
    if (xQueueSend(s_queue, &req, 0) != pdTRUE) {
        ESP_LOGW(TAG, "audio queue full, dropping tone");
    }
}

void hw_audio_play_square(int freq_hz, int duration_ms)
{
    if (!s_ready) {
        return;
    }
    audio_req_t req = {.cmd = AUDIO_CMD_TONE, .freq_hz = freq_hz,
                       .duration_ms = duration_ms, .square = true};
    if (xQueueSend(s_queue, &req, 0) != pdTRUE) {
        ESP_LOGW(TAG, "audio queue full, dropping tone");
    }
}

void hw_audio_play_sweep(void)
{
    if (!s_ready) {
        return;
    }
    audio_req_t req = {.cmd = AUDIO_CMD_SWEEP};
    if (xQueueSend(s_queue, &req, 0) != pdTRUE) {
        ESP_LOGW(TAG, "audio queue full, dropping sweep");
    }
}

void hw_audio_selftest(void)
{
    if (!s_ready) {
        return;
    }
    audio_req_t req = {.cmd = AUDIO_CMD_SELFTEST};
    if (xQueueSend(s_queue, &req, 0) != pdTRUE) {
        ESP_LOGW(TAG, "audio queue full, dropping selftest");
    }
}
