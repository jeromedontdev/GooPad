# ESP32-S3-4848S040 hardware test firmware

[![Release](https://img.shields.io/github/v/release/jeromedontdev/esp32-s3-4848s040-test-suite)](https://github.com/jeromedontdev/esp32-s3-4848s040-test-suite/releases/latest)
[![Licence](https://img.shields.io/github/license/jeromedontdev/esp32-s3-4848s040-test-suite)](LICENSE)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v6.1-blue)](https://github.com/espressif/esp-idf)

Interactive self-test firmware for the Guition / CYD **ESP32-S3-4848S040**
4" 480×480 wall panel. It brings up every peripheral on the board and
presents an LVGL UI so each one can be exercised and judged by eye.

Built with **ESP-IDF v6.1** (no Arduino layer).

## Quick start

### Flash a prebuilt image

Download the merged binary from the
[latest release](https://github.com/jeromedontdev/esp32-s3-4848s040-test-suite/releases/latest)
and write it to offset `0x0`:

```powershell
esptool --chip esp32s3 -p COM4 -b 460800 write-flash 0x0 esp32s3_4848s040_hwtest-1.0.0.bin
```

### Build from source

```powershell
git clone https://github.com/jeromedontdev/esp32-s3-4848s040-test-suite.git
cd esp32-s3-4848s040-test-suite

# One-time: point at your ESP-IDF install
C:\esp\v6.1\esp-idf\export.bat

idf.py set-target esp32s3
idf.py build
idf.py -p COM4 flash monitor
```

`sdkconfig.defaults` carries every non-default setting, so no manual
`sdkconfig` editing is required. The first build downloads LVGL 9,
`esp_lvgl_port` and `esp_lcd_touch_gt911` from the ESP component registry.

### Making a release image

```powershell
.\tools\make_release.ps1              # build + merge into dist/
.\tools\make_release.ps1 -Port COM4   # ...and flash it
```

The script reads the flash mode, frequency and size from `sdkconfig`
rather than hardcoding them — hardcoding `qio` produced an image that
boot-looped, because `CONFIG_ESPTOOLPY_FLASHMODE` is `dio` on this board
even though `FLASHMODE_QIO` is also set.

## Hardware

| Part | Detail |
| --- | --- |
| MCU | ESP32-S3-WROOM-1U-N16R8, 2 cores @ 240 MHz |
| Flash | 16 MB (QIO, 80 MHz) |
| PSRAM | 8 MB octal @ 80 MHz |
| Display | Sitronix ST7701S, 480×480, 18-bit RGB parallel bus |
| Touch | Goodix GT911, I2C @ 0x5D |
| PMIC | Injoinic IP5306 (plain ESOP-8, **no I2C interface**) |
| Audio | NS4168 I2S class-D amplifier (mono) |
| Storage | microSD on SPI2 |
| Radio | WiFi 2.4 GHz + Bluetooth LE (NimBLE) |

## Pin map

Verified against the board schematic (ESP32-S3-WROOM-1U-N16R8):

```
LCD RGB      DE 18   VSYNC 17   HSYNC 16   PCLK 21
LCD data     B0-B4  4, 5, 6, 7, 15
             G0-G5  8, 20, 3, 46, 9, 10
             R0-R4  11, 12, 13, 14, 0
LCD cmd      CS 39   SCLK 48   MOSI 47      (3-wire SPI, init only)
Backlight    38 (LEDC PWM, active high)
Touch        SDA 19  SCL 45   (GT911 @ 0x5D, no RST/INT)
microSD      CS 42   MISO 41   MOSI 47   SCLK 48
Audio        BCLK 1  LRCLK 2   DOUT 40
PMIC         IP5306, no I2C (see Power section)
```

The schematic also shows `SD_MDI` (the amp's CTRL pin) pulled to
`VOUT-BAT` through R24 (1 MΩ), which selects the right channel and keeps
the amp out of shutdown — so no GPIO is needed for amp enable.

### Shared pins

* **GPIO 47 / 48** are used by both the ST7701S command bus and the SD
  card SPI bus. They share a single `SPI2_HOST` bus with separate chip
  selects, so there is no pin reconfiguration between the two.
* **GPIO 1 / 2 / 40** each fan out to *two* 0-ohm links, of which only one
  of each pair is fitted:
  * `R21`/`R22`/`R23` → speaker (I2S: DIN / LRCLK / BCLK)
  * `R25`/`R26`/`R27` → relays (`relay1`/`relay2`/`relay3`)

  This firmware always drives them as I2S. If the relay side is fitted the
  amp inputs float and audio is effectively silent. See
  [Audio: the 0-ohm link matrix](#audio-the-0-ohm-link-matrix).

  The **Relay click** button on the Audio tab toggles those pins as plain
  outputs; a relay board clicks audibly, which tells the two variants
  apart.

## Build and flash

See [Quick start](#quick-start) above. The only board-specific detail is the
port: use whichever COM port the side USB-C connector enumerates as (COM4
on the machine this was developed on). `P1` is a UART header, not USB — see
[Which port to use](#which-port-to-use).

## Serial console

The firmware also starts an `esp_console` REPL on the same UART, so every
subsystem can be exercised without touching the screen. This is the
fastest way to tell a UI problem from a driver problem.

### Which port to use

The board has **two** USB-serial paths:

| Connector | Chip | Notes |
| --- | --- | --- |
| USB-C on the side | CH340 | Full flash + monitor. This is the one to use. |
| `P1` 4-pin HC-1.25 | none | **UART header only** — VIN / TXD0 / RXD0 / GND |

`P1` is a bare UART breakout, not USB. A USB-A cable cannot work there:
there is no USB-serial chip on those pins. To use it you need a 3.3 V
USB-serial adapter wired **crossed** (adapter TX → P1 RXD0, adapter RX →
P1 TXD0) plus GND.

Note that `P1` pin 1 is `VIN`, which feeds the IP5306's input — so it
expects roughly 5 V, not 3.3 V.

### Powering from P1 (USB-A / UART) is marginal

Per the schematic, both inputs feed the *same* 5 V rail:

```
P1 pin 1 VIN ──> Q1 (AO3401 P-FET, gate tied to GND via R31 47k = always on) ──┐
                                                                                ├──> 5V rail
USB-C VBUS ────> D1 (1N5819W Schottky) ────────────────────────────────────────┘
                                                                                │
                                     D3 (SMBJ6.0CA TVS) ── GND ─────────────────┘
                                                                                │
                                     IP5306 VIN ──> VOUT-BAT ──> U3 buck ──> 3.3V
```

**P1 is the weaker input, and it is the one that misbehaves.** With only
the USB-A/UART cable connected, the board resets repeatedly and crashes on
audio. With only USB-C connected it is stable.

The reason is the path itself:

* **Q1 is a P-FET with its gate tied to GND through R31**, so it is
  permanently on and has no current limiting, no soft-start and no
  reverse blocking. Whatever the USB-A adapter delivers goes straight onto
  the 5 V rail.
* **D1 (1N5819W) is a plain Schottky**, so the USB-C path drops ~0.3 V but
  is otherwise the same.
* **There is no bulk capacitance on the 5 V rail** — only D3, a TVS diode
  for transient clamping. Nothing holds the rail up when the load steps.

Audio is the load step: a full-scale tone draws a burst of current from
the class-D amp, which is fed from `VOUT-BAT`. On a weak USB-A adapter
(especially a 500 mA port or a long thin cable) that burst pulls the 5 V
rail down, the IP5306's input sags, and the chip resets. The reset is a
**power-on reset, not a brownout** — the rail drops faster than the
detector can trip, which is why `brownouts` stays at 0 while `boots`
climbs.

#### What to do

1. **Prefer USB-C for power.** It is stable on this board.
2. **If you must use P1**, use a supply that can actually deliver the
   current — a 5 V 2 A adapter, not a laptop's 500 mA port — and keep the
   cable short and thick.
3. **Turn the backlight down.** It is the largest steady load. The
   firmware now boots at 60 % rather than 100 %, and the Display tab has a
   brightness slider. `hwtest> bl 30` from the console does the same.
4. **Do not power both connectors at once.** Q1 blocks nothing, so the two
   supplies would be wired together through Q1 and D1 and back-feed each
   other. Unplug one fully before connecting the other.

If you need the console while on a bench supply, power the board from
USB-C and leave P1's VIN pin disconnected — use P1 only for TXD0/RXD0/GND.

```
hwtest> help
hwtest> status                 # pass/fail, audio state, touch counters
hwtest> tone 1000 500          # 1 kHz sine for 500 ms
hwtest> tone 1000 500 sq       # same, square wave (~2.1x the RMS)
hwtest> bits 24                # switch slot width: 16, 24 or 32
hwtest> vol 60                 # volume 0-100
hwtest> chan left              # drive left slot only (both|left|right)
hwtest> bl 30                  # backlight brightness 0-100
hwtest> wifi                   # scan for WiFi access points
hwtest> ble 4000               # scan for BLE devices for 4000 ms
hwtest> selftest               # beep at each slot width in turn
hwtest> relay                  # toggle GPIO 1/2/40 as plain outputs
hwtest> i2c                    # rescan the I2C bus
hwtest> pmic                   # report the IP5306's (absent) I2C interface
```

`chan` is a diagnostic for the mono amp: with `chan both` (the default)
every sample is duplicated into both slots, so the amp plays it whichever
slot its CTRL pin selects. If `chan left` and `chan right` sound identical,
the amp is picking up the duplicated signal as intended; if only one of
them is audible, that tells you which slot R24 has selected.

## What the UI tests

| Tab | What it exercises |
| --- | --- |
| **Status** | Pass/fail summary for all six subsystems, I2C device list, uptime and free heap |
| **Display** | Backlight brightness slider, colour bars, a black→white gradient (banding), and text at two sizes |
| **Touch** | A drawing canvas — drag a finger to leave a trail; live X/Y, event count, controller ID, poll and error counters |
| **Power** | Reports that the IP5306 has no I2C interface, plus a power history (last reset reason, boot count, brownout count) — the only power telemetry this hardware provides |
| **SD** | Card name, capacity, sector size, root file count, write test, remount button |
| **Audio** | 440 Hz / 880 Hz / 1.5 kHz sine tones, 440 Hz / 1 kHz square waves, a 200 Hz→3 kHz sweep, a volume slider, 16/24/32-bit slot selection, an auto-test that beeps at each slot width, a relay-click test, and I2S diagnostics (bytes written, GPIO matrix routing) |
| **Radio** | WiFi scan (SSID, RSSI, channel, auth) and BLE scan (name, RSSI) — both prove the radio works without needing credentials |
| **Info** | Full pin map and build info |

## Source layout

```
main/
  board.h        pin map and board constants
  app.h          shared state and module interfaces
  board.c        I2C bus, backlight PWM, I2C scan
  hw_display.c   ST7701S 9-bit SPI init + RGB panel + LVGL glue
  hw_touch.c     GT911 driver + LVGL input device
  hw_power.c     IP5306 register access
  hw_sdcard.c    SPI2 microSD mount, write test, remount
  hw_audio.c     I2S sine generator on its own task
  hw_gpio.c      chip / flash / PSRAM / heap reporting
  hw_wifi.c      WiFi station init + access-point scan
  hw_ble.c       NimBLE init + BLE device scan
  settings.c     persisted volume / backlight in NVS
  console.c      esp_console REPL for UI-independent testing
  ui.c           LVGL tabbed test UI
  main.c         bring-up sequence
```

## Notes on this board

* **The ST7701S init sequence is critical.** This panel only lights up
  with the vendor "type 9" sequence; any other leaves it black. It is
  reproduced verbatim in `hw_display.c`.
* **The command bus runs on hardware SPI, not bit-banged.** The ST7701S
  protocol is SPI with one extra leading DC bit per byte, which maps
  exactly onto the peripheral's command phase (`command_bits = 1`). CS is
  driven manually because the panel needs it held low across a command
  and all of its parameters.
* **No display enable pin.** `esp_lcd_panel_disp_on_off()` returns
  `ESP_ERR_NOT_SUPPORTED`; the panel is turned on by the `0x29` command
  in the init sequence instead.
* **No touch reset or interrupt pin.** The GT911 is polled, and both
  possible addresses (0x5D and 0x14) are probed at init. Presence is
  confirmed by reading the product ID register (0x8140), which must
  return `911`, so a stray device on the bus is not mistaken for a touch
  controller. The Touch tab reports the ID, the poll count and the I2C
  error count, which distinguishes "not fitted" from "fitted but not
  reporting".
* **The IP5306 has no I2C interface on this board.** Per the schematic, U1
  is the plain ESOP-8 part (VIN/LED1-3/KEY/BAT/SW/VOUT) — there is no SDA
  or SCL, so it can never be queried and no bodge wire can change that. A
  full bus scan returns only `0x14` and `0x5D` (both GT911 touch
  addresses). The firmware reports `power=SKIP` rather than a misleading
  `FAIL`, and shows a **power history** (last reset reason, boot count,
  brownout count) instead. See
  [Power: why there is no battery reading](#power-why-there-is-no-battery-reading).
* **The panel has no brightness command.** Dimming is PWM on GPIO 38.
* **I2S must be stereo, not mono.** On the ESP32-S3, mono slot mode
  interleaves each sample with a zero (`[valid, 0, valid, 0, ...]`), which
  the NS4168 reproduces as a metallic buzz. Samples are duplicated into
  both slots instead.
* **The NS4168 needs MSB-aligned samples.** It is an I²S slave DAC that
  reads each sample from the top of the slot. Samples are generated
  left-justified so the DAC always sees the sample starting at the MSB.
  The Audio tab has 16/24/32-bit slot buttons so the alignment can be
  auditioned on real hardware; 16-bit is the default.
* **24-bit needs `mclk_multiple` to be a multiple of 3.** The default
  256× is not, so `i2s_channel_init_std_mode()` fails with *"The
  'mclk_multiple' should be the multiple of 3 while using 24-bit data
  width"* and leaves the channel torn down. That is why the 24-bit button
  could never be selected. 384× is used for 24-bit; a failed rebuild now
  falls back to 16-bit instead of leaving the amp silent.
* **The NS4168 CTRL pin is multi-level, not a logic pin.** 1.5 V–VDD
  selects the right channel, 0.9–1.15 V the left, and 0–0.4 V shuts the
  amp down. A GPIO high selects right; low silences it. If the amp is
  silent regardless of firmware, check this pin is not strapped low.
* **Square-wave test tones** carry ~2.1× the RMS of a sine at the same
  peak, which is the fastest way to tell "too quiet" from "not working".
* **The relay-click test must not reset the pins.** `gpio_reset_pin()`
  clears the GPIO matrix output selector, which is what routes the I2S
  peripheral onto GPIO 1/2/40 — calling it leaves the amp permanently
  silent. The test suspends I2S, drives the pins as plain outputs, then
  re-routes them with `i2s_channel_reconfig_std_gpio()`.
* **The audio task stack must not hold the sample buffers.** The 16-bit and
  32-bit buffers together are 3 KB, which overflowed a 4 KB task stack and
  crashed the device on the first tone. They are static now.
* **Slot-width changes are a flag, not a queue entry.** The audio queue is
  a few entries deep and is usually full of pending tones, so queueing the
  change either dropped it (zero-timeout send) or froze the UI for two
  seconds (blocking send). A volatile flag cannot be dropped and never
  blocks the caller; the audio task checks it before every command.
* **Every I2S channel operation must run on the audio task.**
  `i2s_channel_disable()` blocks until any in-flight write returns, and the
  audio task is the one writing. Calling it from the LVGL task (which is
  what `hw_audio_suspend()` used to do) deadlocks the audio task
  permanently — after which *every* later request, including slot-width
  changes, is silently ignored. That was the real reason the bit-width
  buttons appeared dead while the serial console worked: the console never
  touched the relay test, so it never triggered the deadlock. Suspend is
  now a queued command, and `hw_audio_suspend()` waits for the audio task
  to confirm before the caller drives the pins.
* **`i2s_channel_write()` must not use `portMAX_DELAY`.** If the channel is
  disabled mid-write the blocking call never returns and the audio task
  stops looping, with the same "everything after this is ignored" effect.
  It uses a 1 s timeout.
* **LVGL must not use its built-in allocator on this board.** With WiFi and
  BLE linked in, the internal DRAM is too tight: LVGL's default 64 KB
  static pool plus the radio stacks overflowed `dram0_0_seg`, and raising
  the pool to 256 KB failed to link outright. The fix is
  `CONFIG_LV_USE_CLIB_MALLOC=y` (with `CONFIG_LV_USE_BUILTIN_MALLOC=n`), so
  LVGL allocates from the normal heap, which `CONFIG_SPIRAM_USE_MALLOC=y`
  backs with PSRAM. Without this the UI panics in `lv_draw_add_task` as
  soon as a scan runs.
* **Audio and backlight are scaled, not clamped.** `BSP_AUDIO_MAX_PCT` and
  `BSP_BACKLIGHT_MAX_PCT` in [board.h](main/board.h) define what "100%"
  means for each control, so the sliders still run their full range while
  the actual current draw stays within what the supply can deliver. The
  caps are invisible to the user. They are set conservatively (60% audio,
  70% backlight) because WiFi and Bluetooth bursts add ~240 mA on top.
* **Labels need an explicit width to wrap.** A plain `lv_label` has no
  width constraint, so long body text runs off the right edge of its card
  instead of wrapping. `make_body_label()` sets `lv_pct(100)` width and
  `LV_LABEL_LONG_WRAP`; use it for anything longer than a short caption.
  Do not use it inside a flex row, where a 100% width squeezes the
  siblings out.
* **`SPACE_BETWEEN` plus `flex_grow` overflows.** The volume and backlight
  rows used `LV_FLEX_ALIGN_SPACE_BETWEEN` with a growing slider, which
  pushed the slider's right edge a few pixels off screen. Use
  `LV_FLEX_ALIGN_START`, a fixed-width caption, and `flex_grow` on the
  slider with `lv_obj_set_width(slider, 0)`.
* **Volume and backlight persist in NVS.** `settings.c` stores the
  user-facing 0-100 values (not the scaled duty/amplitude, so the power
  budget can change without invalidating them). Writes are skipped when
  the value has not changed since the last save, so dragging a slider
  costs one write when it settles rather than one per pixel.
* **Never call `ui_refresh()` from an LVGL event handler.** The LVGL mutex
  is a plain (non-recursive) `xSemaphoreCreateMutex()`, and event callbacks
  already run with it held, so re-taking it self-deadlocks the UI. Updating
  widget styles directly from the callback is fine — you are already inside
  the LVGL context.
* **Buttons need a visible selected state.** The 16/24/32-bit slot buttons
  always worked, but nothing showed which width was active, so pressing
  them looked like a no-op. `bits_highlight()` repaints them on tap and
  from the refresh timer (so console changes show up too).
* **P1 is the marginal power input; USB-C is stable.** Both feed the same
  5 V rail: P1 VIN through Q1 (an AO3401 P-FET whose gate is tied to GND
  via R31, so it is permanently on with no current limiting or soft-start),
  and USB-C VBUS through D1 (1N5819W). There is **no bulk capacitance on
  the 5 V rail** — only D3, a TVS clamp. Audio is a load step, so on a weak
  USB-A adapter the rail sags and the chip resets. The reset is a
  **power-on reset, not a brownout** (the drop is too fast for the detector
  to trip), which is why `brownouts` stays 0 while `boots` climbs. See
  [Powering from P1 (USB-A / UART) is marginal](#powering-from-p1-usb-a--uart-is-marginal).
* **Power behaviour depends on whether a cell is fitted.** With the battery
  in, unplugging USB caused a reset followed by a clean restart on battery
  — a transient dip during the IP5306's USB→battery handover. With the
  cell removed the board runs entirely from USB 5 V, so any sag on that
  rail (weak cable, hub, marginal port) goes straight through to the 3.3 V
  regulator. The brownout detector is **enabled** at level 7 (2.44 V, the
  lowest threshold) so a sag gives a clean reset with a recorded reason,
  and the brownout tally is persisted in NVS so it survives the reset it
  counts. See
  [Power without a battery fitted](#power-without-a-battery-fitted).
* **`sdkconfig.defaults` is ignored once `sdkconfig` exists.** Editing it
  has no effect on an existing build tree; the value has to be changed in
  `sdkconfig` itself (or the file deleted to force regeneration).
* **The sample rate must be a standard audio rate.** The NS4168 datasheet
  specifies fS = 48 kHz as its reference condition and its clock recovery
  only supports the standard rates. 22050 Hz is not one of them, and
  running there leaves the DAC at the wrong speed — which presents as very
  low output.
* **Touch coordinates are screen coordinates.** The drawing canvas is
  nested inside a card inside a tab, so `canvas_dot()` subtracts the
  canvas origin before plotting; without that every dot lands offset from
  the finger.

## Power: why there is no battery reading

The board schematic settles this. **U1 is the plain IP5306 in an ESOP-8
package, not the IP5306-I2C variant.** Its pins are:

```
1 VIN    2 LED1    3 LED2    4 LED3
5 KEY    6 BAT     7 SW      8 VOUT      (9 = thermal pad, GND)
```

There is **no SDA and no SCL**. The part has no I2C interface at all, so
it can never be probed, read or configured from the ESP32 — and no bodge
wire can change that. This is not a routing problem or a missing pull-up;
the pins simply do not exist.

The IP5306 charges, boosts and protects the cell entirely on its own. The
ESP32 has no visibility into it. Charge state is shown only by the board's
own LEDs, driven from LED1/LED2/LED3 as outputs.

The firmware therefore reports `power=SKIP` (not `FAIL`) and shows a
**power history** instead — last reset reason, boot count and brownout
count — which is the only power telemetry this hardware can provide.

### Power without a battery fitted

**With the cell removed the board runs entirely from the USB 5 V rail.**
That changes the failure mode: there is no boost converter to hand over to,
so any sag on USB goes straight through to the 3.3 V regulator. A weak
cable, a marginal host port, or a hub is now the most likely cause of
resets and of the "takes a couple of cycles to boot" behaviour.

The brownout detector is **enabled** (`CONFIG_ESP_BROWNOUT_DET=y`, level 7
= 2.44 V, the lowest threshold the ESP32-S3 offers) so a sag produces a
clean reset with a recorded reason instead of undefined behaviour. The
brownout tally is stored in NVS, so it survives the reset it is counting:

```
hwtest> status
power: last reset=1 boots=66 brownouts=0
```

If `brownouts` climbs, the supply is the problem, not the firmware. Try a
short, thick USB-C cable and a port that is not a hub.

#### If you refit the battery

The original symptom was a reset on unplug, then the board came back up on
battery — a *transient* dip during the USB→battery handover. The rail that
collapses is **`VOUT-BAT`**, the IP5306's boost output, which feeds the U3
buck (pin 5, `IN`). The board has almost nothing on that rail: **C25 and
C26** (next to the amp) are the only bulk capacitance, and 3.3 V has just
C16 + C11 (10 µF each) and C12 (0.1 µF).

**Best place to add bulk capacitance — across `VOUT-BAT` to GND, at the U3
buck input.** That is the node that actually sags, and it is upstream of
the regulator, so the buck keeps regulating through the dip:

* **+** → `VOUT-BAT` (U3 pin 5, or the `VOUT-BAT` side of C25/C26)
* **−** → GND (the ground side of C25/C26)

**Second choice — across 3.3 V to GND**, near the ESP32 module. Easier to
reach (the 3.3 V test points S3/S4 are right there) but less effective,
because it only buffers the *output* of the buck rather than the sagging
input.

Doing both is reasonable: one cap on `VOUT-BAT` and one on 3.3 V.

#### Is 470 µF enough?

Yes. The load is small — roughly 1 W total, so about **240 mA** drawn from
`VOUT-BAT` through the buck. The buck needs its input to stay above about
3.3 V, so there is ~1.5 V of headroom to sag into. At 240 mA:

| Dip duration | Capacitance needed (1.5 V sag) |
| --- | --- |
| 1 ms | 160 µF |
| 2 ms | 320 µF |
| 3 ms | 480 µF |
| 5 ms | 800 µF |

So **470 µF covers a dip of roughly 3 ms**, which is the right order of
magnitude for a boost-converter handover. If it is not quite enough, a
second 470 µF in parallel doubles the ride-through.

#### Do you need low-ESR?

**No — a plain aluminum electrolytic is fine here.** ESR only matters when
the current is high. At 240 mA, even a mediocre 0.3 Ω ESR contributes just
72 mV of drop, which is negligible against 1.5 V of headroom. This is a
low-current rail, so the cheap radial cap in your parts drawer is the
correct part.

Practical notes:

* **Watch the voltage rating.** `VOUT-BAT` is the cell voltage, up to
  ~4.2 V, so a 6.3 V or 10 V part is fine. On 3.3 V use 6.3 V or higher.
* **Keep leads short.** Long leads add inductance and defeat the purpose.
  Keep them under ~2 cm and twist them together.
* **Mind the polarity** on electrolytics — reversed, they vent.
* **Aluminum electrolytics age.** If the cap has been sitting for years,
  it may need reforming; a fresh one is more predictable.

#### Other mitigations

1. **Cell internal resistance / state of charge.** A small or nearly flat
   cell sags under the boost converter's inrush. Try a known-good, charged
   18650.
2. **Reduce the load at the moment of handover.** The backlight is the
   biggest load on 3.3 V. Dimming it during bring-up (the firmware already
   starts at 0 % and ramps) reduces the dip.

A reset here is survivable — the device recovers on battery. If you need
it seamless, the capacitance fix is the answer.

## Audio: the 0-ohm link matrix

The schematic shows IO40/IO2/IO1 each fanning out to **two** 0-ohm links,
of which only one of each pair is fitted:

| ESP32 pin | Relay side | Amp side | Amp signal |
| --- | --- | --- | --- |
| IO40 | R25 → `relay1` | R21 → `DIN` | data |
| IO2  | R26 → `relay2` | R22 → `LRCLK` | word clock |
| IO1  | R27 → `relay3` | R23 → `BCLK` | bit clock |

If the relay side is fitted, the NS4168's inputs float and the amp is
effectively silent — a faint tone may couple capacitively into the
high-gain class-D input, which is audible only with your ear against the
cone. Move the three links to the amp side (R21/R22/R23) to fix it.

Note also that `SD_MDI` (the amp's CTRL pin) is pulled to `VOUT-BAT`
through **R24 (1 MΩ)**, which selects the right channel and keeps the amp
out of shutdown. That is why no GPIO is needed for amp enable.

### Is audio stuck on the right channel?

**Yes — the amp is a single-channel (mono) amplifier, and R24 straps it to
the right slot.** The NS4168 has one speaker output pair (`VOP`/`VON`), so
it can only ever drive one speaker. `CTRL` selects *which I2S slot* it
plays, not a second output.

This is not a problem in practice, because **the firmware already
duplicates every sample into both slots**:

```c
buf16[i * 2]     = s16;   /* left  */
buf16[i * 2 + 1] = s16;   /* right */
```

So whichever slot the amp selects, it receives the full signal. Nothing is
lost, and there is no "missing channel" to recover.

#### If you want to switch channels at runtime

`CTRL` is multi-level, not a logic pin:

| CTRL voltage | Selected channel |
| --- | --- |
| 1.5 V – VDD | right |
| 0.9 – 1.15 V | left |
| 0 – 0.4 V | shutdown |

R24 pulls it to `VOUT-BAT` (right). To make it switchable you would need a
resistor divider to land in the 0.9–1.15 V window for "left", plus a GPIO
to select between the divider and the pull-up — and **there are no free
GPIOs on this board** (see below). Since both slots already carry the same
signal, there is nothing to gain.

#### If you want true stereo

You would need a second amp and a second speaker. The I2S bus already
carries independent left and right data, so the firmware change is small
(remove the duplication and write `buf[i*2] = left; buf[i*2+1] = right;`),
but the hardware does not exist for it.

#### A note on the "mux" idea

There is no audio mux on this board to reconfigure. The only routing
choice is the three 0-ohm links in the table above, and those select
between the amp and the relays — not between channels.

## GPIO availability on this board

The module is an **ESP32-S3-WROOM-1U-N16R8** (16 MB flash, 8 MB *octal*
PSRAM). That leaves very little free:

* GPIO 26–32 — internal flash, unusable
* GPIO 33–37 — octal PSRAM (IO35/36/37 are marked NC on the schematic)
* GPIO 22–25 — **do not exist on ESP32-S3** (the range skips them)
* Everything else is used by the LCD, touch, SD, audio or USB

In practice there are **no free GPIOs** on this board. Any expansion has to
share an existing pin or use an external I/O expander.

## Licence

MIT — see [LICENSE](LICENSE).

This is test firmware for a specific board. It is not a product and comes
with no warranty; in particular the power budget in
[board.h](main/board.h) is tuned for the supply this was developed against.
