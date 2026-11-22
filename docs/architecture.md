# Architecture and code map

This is the maintainer's map of the current implementation, not a promise that
all hardware, firmware versions, or HomeKit controllers have been tested. Start
with the [developer guide](development.md) for safe builds and troubleshooting
and the [API and data guide](api.md) for wire formats. The [README](../README.md)
is the end-user guide.

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
| Touch UI | [`main/goopad_ui.c`](../main/goopad_ui.c) | Setup, keyboard, tile layout, swipe and info menu, LVGL widget rendering, action callbacks. |
| Shared interface | [`main/app.h`](../main/app.h) | Driver interfaces, application state, UI/service functions and widget interfaces. It still includes scaffolding-era declarations; inclusion is not proof a module is built. |
| Hardware | [`main/board.h`](../main/board.h), `board.c`, `hw_display.c`, `hw_touch.c`, `hw_wifi.c`, `hw_sdcard.c`, `hw_audio.c`, `hw_gpio.c` in [`main/`](../main/) | Pin map, power caps, panel and touch, networking, SD and audio. The authoritative build list is [`main/CMakeLists.txt`](../main/CMakeLists.txt). |
| TLS | [`main/goopad_tls.c`](../main/goopad_tls.c) | Device HTTPS identity creation and NVS persistence. |
| Updates | [`main/goopad_update.c`](../main/goopad_update.c) | GitHub release check and verified app-only download to the inactive OTA slot. |
| HomeKit | [`main/goopad_homekit.c`](../main/goopad_homekit.c), [`goopad_homekit_storage.c`](../main/goopad_homekit_storage.c), [`goopad_homekit_credentials.c`](../main/goopad_homekit_credentials.c) | Bridge and switch adapter, separate NVS block device, persistent setup ID and pairing codes. |
| Web editor | [`main/index.html`](../main/index.html) | Embedded HTML/CSS/JavaScript, configuration draft, editor preview, asset conversion and API client; there is no separate frontend build. |
| PC receiver | [`companion/GooPadCompanion.py`](../companion/GooPadCompanion.py), [`windows_startup.py`](../companion/windows_startup.py) | HTTPS/token checks, desktop input, local permissions, sensor caching, GUI/tray and per-user startup. |
| Tests | [`tests/`](../tests/) | Browser regression, Companion unit/integration tests and opt-in physical-device checks. |
| Build and assets | [`CMakeLists.txt`](../CMakeLists.txt), [`main/CMakeLists.txt`](../main/CMakeLists.txt), [`sdkconfig.defaults`](../sdkconfig.defaults), [`partitions.csv`](../partitions.csv), [`tools/`](../tools/) | ESP-IDF component assembly, flash layout, release/Companion packaging, Font Awesome catalog generation. |
| Third party | [`third_party/`](../third_party/) | Font Awesome and vendored HomeKit SDK. Keep their attribution and license notices when distributing. |

`main/fa-icons.json` is the bundled icon catalog. `fa-index.json.gz` is generated
by CMake and ignored by Git. `build/`, `managed_components/`, `sdkconfig`,
`dependencies.lock` and `dist/` are local/generated; do not assume a copied
build directory matches the current source, SDK or configuration.

## Startup and data flow

1. `app_main()` initializes NVS without auto-erasing it on error. Display is
   required; failures of other peripherals are logged, and some functionality
   may remain unavailable.
2. `goopad_init()` loads the saved JSON into PSRAM, creates locks and monitoring
   workers, loads device controls and credentials, registers Wi-Fi events and
   starts the station. The display presents onboarding if not connected.
3. On DHCP assignment, `wifi_event()` starts HTTPS and, when enabled, the
   HomeKit startup task. Failed Wi-Fi connections are retried up to the
   configured limit. HTTPS embeds the editor and icon catalogs in the binary.
4. The editor loads `/api/config` and `/api/status`, sets time, and loads asset
   previews. Save calls `prepareMacroAction()`, prepares/upgrades derived icons,
   removes browser-only preview data, and posts the full document.
5. `config_post_handler()` bounds and validates the input, compares HomeKit
   topology, commits the config to NVS, then replaces the locked RAM copy.
   HomeKit membership/name changes schedule a restart; reordering does not.
6. The LVGL refresh timer compares the current config with the rendered snapshot.
   A change rebuilds the grid; widget and switch snapshots update between
   rebuilds. Compact layouts maintain a map back to original macro indices.
7. An actionable tile calls `goopad_trigger()`. Sound and HomeKit have local
   paths; HTTP actions copy their input into a worker-owned structure and run
   asynchronously. `/api/status` exposes the last action result.

The editor restores an SD backup by reading `/api/backup` and posting it
through the normal validated `/api/config` save path. The update check and
download run on the device, avoiding browser cross-origin download failures;
only a newer stable release's app-only asset is accepted.

The editor and firmware each validate input. Browser validation improves UX;
firmware validation is the security and persistence boundary. Keep both in
sync and retain compatibility with existing saved layouts.

## Concurrency and memory

* LVGL is driven by the display task under its mutex. Objects and their
  descriptors are owned by the UI; other tasks publish state through service
  APIs/locked snapshots rather than manipulating LVGL objects.
* `s_config_lock` protects the JSON RAM copy. HTTP actions copy values before
  releasing it; network work runs in separate tasks. `s_action_lock` protects
  action counters/results and allows at most two outbound HTTP macro workers.
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
| NVS `goopad` namespace | JSON config, Wi-Fi credentials, admin credentials, HTTPS certificate/key, brightness and volume. | No automatic erase on init failure. A 12,000-byte request limit does not guarantee a 12,000-byte blob fits in the 24 KiB NVS partition. |
| `storage` partition tail | HomeKit ownership header (4 KiB) and separate NVS block device (64 KiB). | Claimed only if initially erased; other bytes in this partition must remain erased. Do not mount/format it as SPIFFS, including on downgrade. |
| SD `/goopad/` | Icons, backgrounds, GIFs and uploaded audio. | The JSON refers to keys; assets are separate from configuration. |
| SD `/goopad-config.json` | Explicit config-only backup. | Contains secrets from config but not SD assets, Wi-Fi credentials, device TLS key, or HomeKit pairing. Protect it. |
| PC `%LOCALAPPDATA%\GooPad` | Companion config/token, TLS certificate/private key, log and optional startup executable copy. | Keep an existing certificate and key together; do not silently regenerate one half. |
| OTA app slots | Two 4 MiB app slots and OTA selector metadata. | A normal OTA updates the inactive slot. Full/merged flashes may overwrite boot metadata or the working app. |

The claim record uses flash-safe `CLAIMED` and `READY` states plus a marker in
HomeKit NVS. A malformed header, nonblank unrelated partition data or a
missing/mismatched ready marker fails closed. Never recover by erasing storage
without an explicit, informed decision: pairing keys may be irreplaceable.

Neither device NVS nor HomeKit data is encrypted at rest in the documented
development build. Flash encryption/secure boot are not enabled automatically.
See the [security and troubleshooting guide](development.md#security-and-recovery)
before handling dumps, backups, logs, or irreversible eFuse settings.
