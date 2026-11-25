# CPU and memory map

Use this map to plan optimizations, not as a substitute for profiling the
running device. Stack sizes are reservations in bytes, not measured usage;
heap availability, task placement and render cost depend on configuration,
assets and concurrent network activity. Check the active `sdkconfig` and
measure on the target before changing priorities, affinity or allocation.

## CPU and ownership

The ESP32-S3 has two 240 MHz cores. With the verified ESP-IDF 6.1 build,
Wi-Fi is pinned to core 0; lwIP's TCP/IP task is unpinned. The application
creates these tasks (see the linked sources for creation and lifecycle):

| Work | Core / priority | Stack | Responsibility |
| --- | --- | ---: | --- |
| [`lvgl`](../main/hw_display.c) | 1 / 4 | 12 KiB | LVGL timers, touch callbacks, layout and display refresh under the LVGL mutex. |
| [`audio`](../main/hw_audio.c) | 1 / 5 | 8 KiB | Queued audio and I2S output, independent of LVGL. |
| [`pc_monitor`, `live_widgets`](../main/goopad.c) | 1 / 0 | 6 KiB, 8 KiB | Network polling; publish snapshots for the UI and yield their shared TLS slot to pending manual work. Idle-priority time slicing lets the idle watchdog run during software TLS. |
| HTTPS server | 1 / 2 | SDK default | Authenticated editor/API; the short-lived `https_start` task uses 12 KiB at priority 4. |
| HomeKit | SDK/worker dependent | Worker 4 KiB | Bridge work and queued toggles; the short-lived startup task is core 1, priority 4, 12 KiB. See [`goopad_homekit.c`](../main/goopad_homekit.c). |
| Macro actions | core 1 / 1 | 6 KiB | One worker processes a bounded FIFO of up to four waiting actions. |
| Wi-Fi scan / SD update | unpinned / 4 or 5 | 8 KiB / 6 KiB | Temporary tasks, created on demand by [`goopad_ui.c`](../main/goopad_ui.c). |
| Serial diagnostics | unpinned / 2 | 6 KiB | Restricted physical serial commands; reports numeric, allowlisted diagnostics. |

The baseline enables one LVGL software draw unit; its configured draw-thread
stack is 8 KiB at priority 3. Increasing draw units also increases memory
and competes with radio/network work. A previous two-unit experiment did not
demonstrate a reliable gain. LVGL objects and draw state are not freely
thread-safe: worker threads must publish data, not mutate the UI without its
lock. Do not hold that lock over network or SD operations.

## Memory and bandwidth

| Resource | Current design / budget |
| --- | --- |
| PSRAM | 8 MiB physical on the tested board, octal at 80 MHz; actual free heap is lower after code/rodata mapping and allocations. |
| Tile fonts | Fifteen styled families at four sizes stream from SD, with embedded Montserrat/essential fallback fonts. Multilingual glyphs also stream from SD. The loaders share a bounded 32 KiB bitmap cache; styled descriptors, metrics and kerning use additional bounded PSRAM. Code/rodata is copied from flash to PSRAM in the selected config. Keep the single draw unit unless compressed-font concurrency is validated. See the measured [font/storage ledger](storage-layout.md) before adding assets. |
| Panel frame buffers | Two 480 x 480 RGB565 buffers in PSRAM, 460,800 bytes each; RGB DMA also uses two 10-line bounce buffers. |
| Static icon storage | Each loaded icon allocates its actual size in PSRAM; the former fixed reservation was 2,457,600 bytes. |
| Page cache | Up to three 480 x 396 RGB565 pages, 380,160 bytes each, allocated on demand in PSRAM with a 1 MiB free-space threshold. Missing or invalid pages use the live renderer. |
| Background | Optional 480 x 480 RGB565 image, 460,800 bytes in PSRAM; it is stationary while pages move. |
| GIF / config | GIF buffers and 12,001-byte config copies use PSRAM; PC monitoring retains one additional copy to invalidate cached readings after config changes. Live widgets retain 32 request identities of 384 bytes each in PSRAM, including any provider keys, to notice key changes. Decoded GIF and LVGL allocations add variable costs. |
| Diagnostic ring | 48 fixed numeric records in RTC memory, surviving some soft resets; no raw log strings or configuration. |
| Allocator | Normal `malloc` prefers internal memory for allocations below 16 KiB, with 32 KiB reserved for internal-only allocations. cJSON uses PSRAM; LVGL uses a PSRAM-first allocator, with a logged normal-heap fallback. |
| TLS | TLS allocation uses PSRAM and software AES. Performance-optimized mbedTLS and fixed-point ECP tables reduce handshake latency; certificate verification remains enabled. The macro worker retains one pinned Companion TLS session briefly for reuse, closing the socket between requests. These options increase firmware size. |
| Editor | The HTML is stored as gzip and served compressed to browsers. Clients that do not advertise gzip temporarily allocate the uncompressed editor and decompressor state in PSRAM, avoiding the HTTP task's limited stack; allocation or decompression failures return an explicit HTTP error. |
| Flash | Two 4 MiB app slots; NVS, OTA metadata, storage and HomeKit data are separate. See the [storage ledger](storage-layout.md) and [`development.md`](development.md) before any write. |

The historical pre-offload Unicode ESP-IDF 6.1 HomeKit build was 4,162,208
bytes, leaving 32,096 bytes in each app slot. The subsequently installed
SD-resource/PC-repair build was 2,316,192 bytes. The clean 0.7.3 release build
is 2,316,320 bytes, leaving 1,877,984 bytes (44.77%); consult the storage ledger
for source, SDK and image identities.
An earlier build's linked DIRAM was 154,214 of 341,760
reported bytes; that is a **link-time** figure, not free runtime heap.
Rerun the size tool for the active build. Radio, DMA, task stacks, TLS and draw
tasks use additional runtime memory; a large free-PSRAM figure does not imply
there is enough internal memory for Wi-Fi or lwIP.

## Profiling and safety

In a short on-device capacity probe, an opt-in diagnostic firmware substituted
temporary two-page layouts in RAM; it did not save them. With automatic
scroll requests and a concurrent diagnostic Companion `/status` request,
the device rendered 32 second-updating analog clocks, loaded 32 copies of an
existing 160 x 160 alpha image, decoded four copies of an existing GIF, and
rendered 16 copies of a paired PC-monitor widget. The 32-PC fixture exceeded
the existing configuration-size limit, so it was not a completed load test.
Each step ran for roughly 20 seconds, not a long-duration stability test.
After the 32-image step, approximately 629 KiB PSRAM and 49 KiB internal
heap were free; page caches can be disabled under PSRAM pressure. Four GIFs
left approximately 1.68 MiB PSRAM and 49 KiB internal heap free. These
figures depend on the particular assets, active services and background load;
they do not establish safe limits for combinations or larger GIFs.
The diagnostic Companion requests observed in those steps took 469–1,461 ms;
these are round trips, not touch-to-action latency measurements.
In a separate 113-second run with 16 weather widgets, 12 provider requests
completed with HTTP 200; three diagnostic Companion requests completed in
569, 1,873 and 569 ms. The remaining widgets had not all been fetched by the
end of the observation. This shows a throughput limitation, not a measured
maximum number of safe widgets or a guarantee about provider availability.

On one subsequent real-layout reboot, four internet-widget requests completed
successfully by 36 seconds of uptime after the connection-aware startup change.
In an earlier observation, two had completed by 34 seconds. Wi-Fi association,
time synchronization, provider response and competing PC requests vary, so
these single runs are not a controlled benchmark or a guaranteed startup time.
After a further reboot, all four widgets reported data by 50 seconds.
The existing 32-position and four-GIF limits remain; no speculative visual
budget or per-provider count is imposed.

Compare identical layouts, page counts, backgrounds and network load. Record
animation updates per second, snapshot/copy latency, heap free and largest
block (internal and PSRAM), minimum internal free heap, idle overhead,
watchdogs and uptime. A synthetic LVGL scroll is not a physical-finger test.
Test dynamic widgets, GIFs, HomeKit changes, background images, allocation
failure and page rebuilds, not just a static page.

On the tested two-page layout, instrumented warmed frame-cache runs measured
medians of about 16 and 22 synthetic updates/s versus roughly 9/s before the
redesign. The variation reflects individual runs, not a guaranteed rate.
The first visit to an uncached page still uses live rendering. On one run, the
largest full-page copy took 32 ms and total copies were about 25.6 MB over the
first 50 seconds, including ten test animations; this is not a continuous CPU
utilization measurement. One observation reported roughly 76 KiB free internal
heap after the test, versus 53 KiB in an earlier run using the normal LVGL
allocator. The lifetime minimum reported 16 bytes in both runs; it is a
sum of per-region minima and is not necessarily a simultaneous low watermark.
These are individual observations, not guaranteed headroom or proof of
stability under every workload.

On the corrected page-container build, the synthetic swipe and subsequent
click passed, the live fallback passed, and 11 framebuffer comparisons found
no mismatches. These checks do not establish physical touch behavior.

Earlier synchronous page-snapshot experiments reached about 13–16 synthetic
updates/s but aborted on internal allocation failures during concurrent
radio/network activity. A later update also observed an allocation abort around
the restart boundary; the responsible image was not established. The old
firmware logged task-watchdog warnings while receiving OTA uploads. Avoid
shifting memory or work between cores merely on assumption; verify stability
with HomeKit and network operations active. Preserve the active OTA slot, NVS,
pairing and SD assets when testing.
