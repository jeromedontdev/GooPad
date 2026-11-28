# Architecture and code map

This is the maintainer's map of the current implementation, not a promise that
all hardware, firmware versions, or HomeKit controllers have been tested. Start
with the [developer guide](development.md) for safe builds and troubleshooting
and the [API and data guide](api.md) for wire formats. The [README](../README.md)
is the end-user guide.
For optimization constraints and baseline measurements, see the
[CPU and memory map](cpu-memory-map.md).
The [board porting guide](boards.md) describes the selectable hardware backend,
display-derived grid limits and unverified Waveshare targets.

## System at a glance

```text
 Browser (editor) -- HTTPS + Basic auth --> ESP32-S3 GooPad
                                              | LVGL / RGB display / GT911
                                              | NVS config, TLS identity
                                              | SD assets + config backup
                                              |
  Apple Home <-------- encrypted HAP ---------+ virtual switch bridge
                                              |
  Windows Companion <-- pinned HTTPS + token -+ PC actions / PC statistics
                                              |
  Web / Home Assistant <-- HTTP(S) requests --+ macros and live widgets
```

The browser edits a draft, uploads derived assets, then sends a complete
configuration. The device validates and persists it and the LVGL task rebuilds
the display from that configuration. A tap uses the **saved** macro index, not
unsaved browser edits. The Windows Companion is a separate process, not a
service running on the ESP32.

## Where to work

| Area | Source | Responsibility |
| --- | --- | --- |
| Startup | [`main/main.c`](../main/main.c) | NVS, board, display, touch, SD, Wi-Fi, audio, services and UI initialization; OTA validation. |
| Firmware service | [`main/goopad.c`](../main/goopad.c) | Configuration validation/persistence, Wi-Fi events, HTTPS handlers, action dispatch, OTA, status and widget workers. |
| Daily alarms | [`main/goopad_alarm.c`](../main/goopad_alarm.c), [`main/goopad.c`](../main/goopad.c) | Validate saved schedules and check them in a worker independent of UI refresh and connectivity; deduplication is in RAM. |
| Background network reads | [`main/goopad_network.c`](../main/goopad_network.c) | Bounded streaming HTTPS requests that yield to manual actions. |
| Time and headlines | [`main/goopad_time.c`](../main/goopad_time.c), [`main/goopad_news.c`](../main/goopad_news.c) | Bounded percent formatting, one-to-five separated RSS/RDF/Atom headlines through Expat, and compatible legacy JSON aggregation. |
| Glass backdrop | [`main/goopad_glass.c`](../main/goopad_glass.c), [`main/goopad_ui.c`](../main/goopad_ui.c) | Shared downsampled/blurred RGB565 backdrop, pastel scene and scene-aligned clipped tile surfaces. |
| Layout geometry | [`main/goopad_layout.h`](../main/goopad_layout.h) | Grid/span validation, stacked Arches and legacy ring/arc positions; editor geometry mirrors these helpers. |
| Gradients and emphasis | [`main/goopad_gradient.h`](../main/goopad_gradient.h), [`main/goopad_fonts.c`](../main/goopad_fonts.c), [`main/goopad_text.h`](../main/goopad_text.h) | Shared gradient validation, persistent LVGL stop descriptors, and synthesized glyph weight/slant with expanded fitting metrics. |
| Touch UI | [`main/goopad_ui.c`](../main/goopad_ui.c) | Setup, keyboard, tile layout, swipe and info menu, LVGL widget rendering, action callbacks. |
| Shared interface | [`main/app.h`](../main/app.h) | Driver interfaces, application state, UI/service functions and widget interfaces. It still includes scaffolding-era declarations; inclusion is not proof a module is built. |
| Hardware | [`main/board.h`](../main/board.h), `board.c`, `hw_display.c`, `hw_touch.c`, `hw_wifi.c`, `hw_sdcard.c`, `hw_audio.c`, `hw_gpio.c` in [`main/`](../main/) | Pin map, power caps, panel and touch, networking, SD and audio. The authoritative build list is [`main/CMakeLists.txt`](../main/CMakeLists.txt). |
| TLS | [`main/goopad_tls.c`](../main/goopad_tls.c) | Device HTTPS identity creation and NVS persistence. |
| Updates | [`main/goopad_update.c`](../main/goopad_update.c) | GitHub release check and verified app-only download to the inactive OTA slot. |
| HomeKit | [`main/goopad_homekit.c`](../main/goopad_homekit.c), [`goopad_homekit_storage.c`](../main/goopad_homekit_storage.c), [`goopad_homekit_credentials.c`](../main/goopad_homekit_credentials.c) | Bridge and switch adapter, separate NVS block device, persistent setup ID and pairing codes. |
| Web editor | [`main/index.html`](../main/index.html), [`main/goopad_web.c`](../main/goopad_web.c), [`tools/make_web_resources.py`](../tools/make_web_resources.py) | HTML/CSS/JavaScript, configuration draft and API client. Configuration builds a pinned SD archive; fixed-size reads stream gzip/plain editor and browser fonts. [`main/recovery.html`](../main/recovery.html) stays embedded for missing-card recovery. |
| PC receiver | [`companion/GooPadCompanion.py`](../companion/GooPadCompanion.py), [`windows_startup.py`](../companion/windows_startup.py) | HTTPS/token checks, desktop input, local permissions, sensor caching, GUI/tray and per-user startup. |
| Companion updates | [`companion/companion_update.py`](../companion/companion_update.py), [`tools/companion_installer.iss`](../tools/companion_installer.iss) | Verified release metadata and installer download, per-user installation and update handoff. The executable and adjacent `_internal` runtime are installed/copied together; configuration and TLS identity stay outside the application directory. |
| Tests | [`tests/`](../tests/) | Browser regression, Companion unit/integration tests and opt-in physical-device checks. |
| Build and assets | [`CMakeLists.txt`](../CMakeLists.txt), [`main/CMakeLists.txt`](../main/CMakeLists.txt), [`sdkconfig.defaults`](../sdkconfig.defaults), [`partitions.csv`](../partitions.csv), [`tools/`](../tools/) | ESP-IDF component assembly, flash layout, release/Companion packaging, Font Awesome catalog generation. |
| Third party | [`third_party/`](../third_party/) | Font Awesome and vendored HomeKit SDK. Keep their attribution and license notices when distributing. |

`main/fa-icons.json` is the source icon catalog. CMake generates a compressed
name index and individually compressed drawings embedded in the firmware.
Search inflates only matching drawings, using temporary PSRAM. `fa-index.json.gz`
is ignored by Git. `build/`, `managed_components/`, `sdkconfig`,
`dependencies.lock` and `dist/` are local/generated; do not assume a copied
build directory matches the current source, SDK or configuration.

## Startup and data flow

1. `app_main()` initializes NVS without auto-erasing it on error. Display is
   required; failures of other peripherals are logged, and some functionality
   may remain unavailable.
2. `goopad_init()` loads the saved JSON into PSRAM, creates locks and monitoring
   workers, loads device controls and credentials, registers Wi-Fi events and
   starts the station. Connections scan all channels and prefer the strongest
   matching access point, including when joining a network during onboarding.
   The display presents onboarding if not connected.
3. On DHCP assignment, `wifi_event()` starts HTTPS and, when enabled, the
   HomeKit startup task. Failed Wi-Fi connections are retried up to the
   configured limit. HTTPS embeds the editor and icon catalogs in the binary.
4. The editor loads `/api/config` and `/api/status`, sets time, and loads asset
   previews. Save calls `prepareMacroAction()`, prepares/upgrades derived icons,
   looks up matching SD assets by content hash before uploading new ones,
   removes browser-only preview data, and posts the full document.
5. `config_post_handler()` bounds and validates the input, compares HomeKit
   topology, commits the config to NVS, then replaces the locked RAM copy.
   HomeKit membership/name changes schedule a restart; reordering does not.
6. The LVGL refresh timer compares the current config with the rendered snapshot.
   A change rebuilds the grid and restores the nearest page, clamped to the new
   page count. A reboot starts on page one. Compact layouts maintain a map back
   to original macro indices. GIF decoding and widget/switch updates pause
   while pages scroll and refresh at scroll end.
   Glass backdrop/tint children precede filled visuals, which precede headings
   and captions. Multi-stop gradient descriptors and emphasized-font wrappers
   are PSRAM-owned and freed only after their LVGL objects are deleted.
7. An actionable tile calls `goopad_trigger()`. Sound and HomeKit have local
   paths; HTTP and Wake-on-LAN actions copy their input into a worker-owned structure and run
   asynchronously. `/api/status` exposes the last action result.
   Displaying HTTP actions additionally capture bounded, revision-tagged text
   into a per-macro PSRAM cache. They do not reuse session-cached clients whose
   callback would retain a freed action. Send-only webhook behavior is retained.

Time format strings belong to the UI snapshot, not the temporary parsed JSON.
Multi-source news cycles through one primary and up to three preset feeds,
fetching one per minute; each request retains the existing shared-network
admission and bounded response path. Single-source polling is unchanged.
Headlines run in the existing background-priority serialized network worker.
XML responses are parsed incrementally from the shared 4 KiB scratch buffer
and capped at 1 MiB; legacy JSON uses a 64 KiB PSRAM response buffer freed after
aggregation. Expat's parser and working state use a PSRAM allocation suite;
DTD/external entities and nesting over 32 are rejected.
Glass uses one 120x120 RGB565 backdrop (28,800 bytes), shared by every tile;
custom backgrounds temporarily require another 28,800 bytes for a separable
seven-tap blur after 4x4 downsampling. Buffers/cache entries are freed only after
their LVGL objects are deleted. Allocation failure is logged and displays
`Glass unavailable`. The native surface is scene-aligned at rest, not a
real-time optical refraction simulation. About and the visual picker share
the same RGB565/RGB565A8/GIF thumbnail loader and explicit HTTP/image errors.
Live strings and request-result caches are PSRAM-backed and allocated before
workers/server startup. The optional battery label consumes existing board
telemetry only; it does not introduce gauge detection or speculative wiring.

The editor restores an SD backup by reading `/api/backup` and posting it
through the normal validated `/api/config` save path. The update check and
download run on the device, avoiding browser cross-origin download failures;
only a newer stable release's app-only asset is accepted. The HTTP transmit
buffer must accommodate the full bounded GitHub asset redirect URL and request
line; signed redirect URLs can exceed ESP-IDF's default buffer.
TLS allocates in PSRAM and uses software AES: hardware AES requires temporary
internal DMA buffers that can fail under concurrent display and network load.
Neither setting disables certificate or image verification.

The editor and firmware each validate input. Browser validation improves UX;
firmware validation is the security and persistence boundary. Keep both in
sync and retain compatibility with existing saved layouts.

## Concurrency and memory

* LVGL is driven by the display task under its mutex. Objects and their
  descriptors are owned by the UI; other tasks publish state through service
  APIs/locked snapshots rather than manipulating LVGL objects.
* `s_config_lock` protects the JSON RAM copy. HTTP and Wake-on-LAN actions copy values before
  releasing it; network work runs in separate tasks. `s_action_lock` protects
  action counters/results and the foreground-waiter count.
* PC and live-widget workers maintain cached snapshots under critical sections;
  the HTTP server and LVGL read those snapshots. Companion sensor collection
  similarly refreshes in a background thread. Avoid blocking a server handler
  or LVGL task with a slow sensor, network call or audio playback.
* HomeKit has a mutex for switch state and a queue/worker that submits local
  toggles to the HAP server's work queue. The persistent pairing-request object,
  deadline, lock and completion semaphore protect against late server work.
  Do not replace it with a stack object or silently lose queued presses.
* Display frames, icon buffers, GIFs, config snapshots and several network
  buffers use PSRAM. RGB scanout uses double frame buffers and internal DMA
  bounce buffers; the display code coordinates frame handoff with VSYNC.
  Allocation failures and socket limits are real design constraints.
* For solid backgrounds, the display flush updates up to three PSRAM page
  images from already-rendered, settled frames. During a swipe, valid cached
  pages replace their live object trees; uncached or invalid pages continue
  rendering live. Widget and HomeKit changes invalidate their page, and GIFs
  pause during movement. The stationary custom image background always uses
  live rendering. Cache entries are discarded on grid rebuild; no snapshots or
  SD reads are performed at gesture start.
* LVGL allocation prefers PSRAM, including small draw tasks. A logged fallback
  to the normal allocator remains available if PSRAM is exhausted; internal
  DMA, radio and RTOS allocations still require their own headroom.
* Macro HTTP, Companion checks/monitoring and live-widget HTTP requests share
  an outbound mutex and check internal heap before starting. Pending manual
  actions and Companion checks take priority: background HTTPS polls use short
  streaming socket waits and abandon their client when foreground work is
  pending, then retry. DNS resolution and an already-running foreground request
  cannot be preempted. PC monitoring retains the last valid snapshot for at
  most 15 seconds during deferral or timeout and retries after one second; it
  otherwise waits two seconds between polls. This does not reserve RAM against other
  tasks or serialize the firmware updater.
  The live-widget worker waits 500 ms after processing a due tile, one second
  during a priority deferral or while awaiting Wi-Fi/time, and five seconds
  when idle. A widget blocked on startup becomes due as soon as both Wi-Fi
  and time are ready; previously fetched widgets keep their provider intervals.
  Requests remain serialized and manual actions retain priority.
  Button actions are handled in order by one worker with a bounded queue of
  four waiting actions; a full queue is reported rather than silently dropping
  an action. For repeated requests to the same pinned Companion URL and
  certificate, that worker can reuse a TLS session; it closes sockets between
  requests and discards the cache after ten seconds of worker inactivity, on
  error, or when another pinned endpoint or certificate is used. JSON trees
  use PSRAM to preserve internal radio and DMA memory.
* The shareable diagnostic report is an allowlist of numeric events and memory
  metrics, not a filtered copy of SDK logs. Its bounded RTC-memory ring survives
  some resets, but not loss of power; raw serial messages may contain private data.

Task names, stack sizes, pinning and actual lock usage are defined in the
source, not a general scheduling guarantee. Before adding a worker, identify
who owns its data, how it stops, how errors reach the UI, and what happens on
allocation failure or reconnect.

## HomeKit bridge

Each `homekit` macro has a generated stable `homekitId` and a user-facing
`homekitName`. At startup the adapter sorts by ID, creates a bridge and a child
switch per tile, reuses persisted accessory IDs, and synchronizes a topology
hash. A tile press queues a toggle; a controller write updates the same state
and the UI polls it. State starts off after reboot; it is not the state of the
light controlled by the Apple Home automation.

Pairing is closed by default. The authenticated settings endpoint opens a
five-minute window only when the bridge is active and has no paired
controller. A setup ID is persistent; a fresh setup code is generated.
Pairing identity and controller data are in the separate HomeKit store, not
the SD config backup. Changing a switch's stable ID or deleting pairing data
can require Home setup/automation repair.
The advertised HomeKit firmware revision comes from the ESP-IDF app
description's `PROJECT_VER`, set in the top-level CMake file.

The vendored SDK is selected for ESP-IDF 6.1+; `CONFIG_GOOPAD_HOMEKIT` also
requires NVS block-device support. The component manifest's lower IDF floor
does **not** establish that the HomeKit feature or every older-IDF
configuration has been validated. Apple Home interoperability beyond the
reported user pairing/switch test remains limited.

## Persistence and recovery boundaries

| Location | Contents | Important invariant |
| --- | --- | --- |
| NVS `goopad` namespace | Legacy JSON config, migration marker, Wi-Fi credentials, admin credentials, HTTPS certificate/key, brightness and volume. | No automatic erase on init failure; the legacy config is retained for older firmware. |
| `storage` partition tail | Ownership header (4 KiB) and separate NVS block device (64 KiB) for configuration and HomeKit. | Claimed only if initially erased; other bytes in this partition must remain erased. Do not mount/format it as SPIFFS, including on downgrade. |
| SD `/goopad/` | Icons, backgrounds, GIFs and uploaded audio. | The JSON refers to keys; assets are separate from configuration. |
| SD `/goopad-config.json` | Explicit config-only backup. | Contains secrets from config but not SD assets, Wi-Fi credentials, device TLS key, or HomeKit pairing. Protect it. |
| PC `%LOCALAPPDATA%\GooPad` | Companion config/token, TLS certificate/private key, log and optional startup executable copy. | Keep an existing certificate and key together; do not silently regenerate one half. |
| OTA app slots | Two 4 MiB app slots and OTA selector metadata. | A normal OTA updates the inactive slot. Full/merged flashes may overwrite boot metadata or the working app. |

On the first successful save, the config is committed to the larger store
before its migration marker is written to the original NVS. Later saves use
the larger store, retaining the original config for older firmware. Older
firmware will see that older snapshot, not subsequent edits; if it edits the
legacy config, newer firmware detects the change and loads the edited legacy
version. Back up the current config before rolling back.

The editor stores a shared Companion address and token once at the root and
uses `sharedPc` with an endpoint path for compatible PC tiles. The firmware
resolves these only for the corresponding PC action or widget and checks the
paired certificate. Legacy per-tile values remain readable; firmware without
`sharedPc` support cannot run the compacted PC tiles after rollback.

The claim record uses flash-safe `CLAIMED` and `READY` states plus a marker in
the separate NVS. A malformed header, nonblank unrelated partition data or a
missing/mismatched ready marker fails closed. Never recover by erasing storage
without an explicit, informed decision: configuration and pairing keys may
be irreplaceable.

Neither device NVS nor HomeKit data is encrypted at rest in the documented
development build. Flash encryption/secure boot are not enabled automatically.
See the [security and troubleshooting guide](development.md#security-and-recovery)
before handling dumps, backups, logs, or irreversible eFuse settings.
