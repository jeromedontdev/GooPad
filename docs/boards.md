# Board and display porting

The supported hardware remains the Guition ESP32-S3-4848S040, 480x480.
The backend-selection changes described here are installed/tested in the
development worktree but are not yet part of a source-matched binary release.
Waveshare square, 4-inch and 4.3-inch products are not yet certified: obtain the
exact SKU/revision, schematic, display/touch controllers, resolution, flash and
PSRAM sizes before implementing or flashing a backend.

## Backend boundary

The application, networking, editor and LVGL UI remain shared. Pass an absolute
`GOOPAD_BOARD_DIR` to CMake/ESP-IDF to replace the hardware sources:

```powershell
idf.py -B C:\build\my-board -D SDKCONFIG=C:\build\my-board-sdkconfig -D GOOPAD_BOARD_DIR=C:\boards\my-board reconfigure build
```

The directory must contain independent `board.h`, `board.c`, `hw_display.c`,
`hw_touch.c`, `hw_sdcard.c`, `hw_audio.c` and `hw_gpio.c` implementations.
Missing files cause a configure error rather than silently driving Guition
pins. Use [app.h](../main/app.h) for the existing driver interfaces.
`board.c` owns `g_app` and board initialization/backlight controls.
Keep private helper headers alongside the external sources.

The header must define `BSP_BOARD_NAME`, `BSP_LCD_H_RES`, `BSP_LCD_V_RES`,
`BSP_SD_MOUNT_POINT` and the pin/timing/power constants used by that backend.
Do not copy the selector wrapper from [board.h](../main/board.h) into an
external header. `BSP_TILE_MIN_PX` defaults to 64; raise it when physical
screen size, touch accuracy or readability needs larger tiles.

`hw_display_init()` must return a correctly sized LVGL display. Preserve the
UI lock contract and call `goopad_ui_capture_frame()` only with a compatible
full RGB565 direct-mode framebuffer. A SPI/other buffered renderer may omit
capture (and therefore page-image caching) rather than supplying an incompatible
buffer. Touch coordinates must match the final rotation and display dimensions.
Do not retain Guition initialization commands, timings or power limits merely
because another panel has the same resolution.

For absent peripherals, initialize explicit unavailable results and return
`ESP_ERR_NOT_SUPPORTED` from their operations. Provide all required interfaces;
do not return success for an operation that never happened.

## Layout and assets

Status reports board name, pixel dimensions and `maxCols`/`maxRows`. The limits
use the actual viewport, 48-pixel top gesture area, 36-pixel footer, 10-pixel
outer padding, 8-pixel tile gaps and the backend's minimum tile size. The editor
previews these dimensions, disables overly dense grid choices and rejects
saving an imported layout above the limits. Firmware also validates the limits.
New unsaved configurations choose up to 3x3 within those limits.

The existing layout format supports 2-4 columns/rows and at most 32 positions.
At the default 64-pixel minimum, 240x240 allows 3x2; 480x480 allows 4x4.
A board that cannot fit at least 2x2 needs a layout-format/UI extension before
it is supported. Larger screens remain capped at 4x4 for compatibility.
Resolution is not physical size: calibrate the minimum for the actual panel.
Existing saved layouts are not silently rewritten or rearranged on boot.

Tile assets retain their existing 160x160/GIF formats. Full-screen backgrounds
remain stored as 480x480 RGB565 for compatibility and are scaled to cover the
viewport. Overlay, keyboard, page/GIF visibility and text/tile geometry use
display dimensions; some onboarding/control placements remain tuned for
Guition and must be checked on the target.

## Port validation

Use separate SDK configs/build directories and a board-specific partition
table. Match the flash, PSRAM, bus and panel timing settings to the target,
including its two framebuffer memory requirement. First test display/backlight,
touch mapping, scrolling, onboarding keyboard, SD and optional audio without
user data. Then run configuration/editor tests, verify grid readability,
HTTPS/action response and a short device soak. Do not claim support from a
host layout test or successful Guition build.
