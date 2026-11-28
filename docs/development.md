# Developer handoff and operations

Read [architecture.md](architecture.md) for ownership and runtime flow,
[api.md](api.md) for contracts, and the [README](../README.md) for user setup.
These instructions assume Windows/PowerShell and an ESP32-S3-4848S040 board.
Use your own paths and detected serial port.

## First orientation

1. Inspect `git status -sb`, remotes and local changes; preserve work already
   present. The product repository is `jeromedontdev/GooPad`; the original
   hardware-test repository is a different remote.
2. Read the relevant source and tests from the architecture map. Generated
   `build/`, `sdkconfig`, `managed_components/` and release files may belong to
   another build or SDK; do not assume they are reproducible inputs.
3. Identify the change's persistence and security boundaries. Test with
   synthetic configuration first; a real user's paired device and SD card
   contain data that cannot be reconstructed from a config backup alone.

## Build

Install ESP-IDF **6.1** and its tools. In PowerShell, activate its environment
and use a dedicated build directory. Adjust the ESP-IDF installation path:

```powershell
& C:\esp\v6.1\esp-idf\export.ps1
idf.py -B C:\Temp\GooPad-idf-build build
```

The build produces `GooPad.bin` in that build directory. Inspect the size
report: it must fit the 4 MiB app slot. The root `sdkconfig` is local and may
already contain choices. For a fresh build, `sdkconfig.defaults` seeds the
configuration; inspect `CONFIG_GOOPAD_HOMEKIT`, `CONFIG_NVS_BDL_STACK`, flash
and PSRAM settings rather than assuming an old build includes HomeKit.
For the updater's PSRAM-backed TLS, also confirm
`CONFIG_MBEDTLS_HARDWARE_AES` is disabled in the active configuration; existing
`sdkconfig` files do not automatically inherit changed defaults.
For the responsive-network build, also select
`CONFIG_MBEDTLS_COMPILER_OPTIMIZATION_PERF` and
`CONFIG_MBEDTLS_ECP_FIXED_POINT_OPTIM`; these increase app size. Enable
`CONFIG_ESP_TLS_CLIENT_SESSION_TICKETS` for repeated Companion action reuse.
For the PSRAM-first LVGL allocator, select `CONFIG_LV_USE_CUSTOM_MALLOC`;
existing configurations may still select the C library allocator instead.
Select `CONFIG_LV_USE_FONT_COMPRESSED` for the bundled compressed fonts.
Select `CONFIG_LV_GRADIENT_MAX_STOPS=6` and
`CONFIG_LV_USE_DRAW_SW_COMPLEX_GRADIENTS=y` for multi-stop and circular tile
gradients; existing SDK configurations do not inherit these new defaults.
`idf.py set-target` can reset configuration; do not run it casually against an
existing configured workspace. Keep the SDK and the build's `SDKCONFIG`
consistent. `main/CMakeLists.txt` generates the compressed editor and Font
Awesome index and compressed catalog at configure time.

The manifest declares IDF >=5.3, but HomeKit is gated on 6.1/NVS block-device
support and older-IDF combinations have not been comprehensively verified.
Do not claim compatibility based on the manifest alone.

## Safe update, first flash and release

**For a configured device, prefer an app-only OTA update** from the editor or
SD. It writes the inactive slot and leaves the active app and storage intact.
Back up the configuration *and* SD assets separately before testing. The SD
config backup does not include HomeKit identity, Wi-Fi, device TLS identity or
assets; protect it because it can contain tokens. Ensure the exact board,
partition layout, image, and recovery path are known before updating.

The partition table is [`partitions.csv`](../partitions.csv): NVS at `0x9000`
(24 KiB), OTA metadata at `0x10000`, `ota_0` at `0x20000`, `ota_1` at
`0x420000`, and `storage` at `0x820000`. The `storage` tail may contain the
current configuration and the only copy of HomeKit pairing keys. Older firmware
reads the retained pre-migration configuration rather than newer edits; back up
the current configuration before rolling back. Do not erase NVS or format `storage`, and
do not use a full-flash command on an existing device as a shortcut. `idf.py
flash` and a merged image can write bootloader, partition table, OTA selector
and an app slot; they are not equivalent to a safe app-only update.

`app_main()` marks an OTA app valid after initialization. Rollback protects
against failure to reach that point, not every later bug or network failure.
Observe boot logs and verify the actual feature, persisted config and paired
bridge after the update. If HTTPS is unreachable, distinguish Wi-Fi/DHCP,
LAN routing and certificate problems from a failed boot; do not erase data.
HTTPS can become responsive before initialization and OTA validation finish.
Wait for initialization/font-canary completion before disruptive serial
bootloader checks; resetting an unvalidated candidate can trigger rollback.
For a version-changing upgrade, HomeKit deliberately updates `hap_main/fw_rev`
and advances `hap_main/config_num` once (wrapping 65535 to 1). Validate those
exact transitions separately from the reboot advertisement `state_num`;
all other logical pairing/configuration records and the ownership marker
must remain identical. Do not ignore arbitrary NVS differences.
Manual serial recovery must first identify the live device, partition table,
active selector and known-good slot; never assume a COM port or slot.

For a **fresh/disposable board** or an explicitly approved recovery with
backups, [`tools/make_release.ps1`](../tools/make_release.ps1) creates a merged
first-flash image and an app-only OTA binary:

```powershell
.\tools\make_release.ps1 -BuildDir C:\Temp\GooPad-idf-build
```

The script's `-Port` option writes the merged image at flash offset 0. It is
not the normal update procedure. It reads resolved flash parameters from the
selected build's `flasher_args.json` and rejects a different project/version;
the workspace's local `sdkconfig` is not used for packaging.
The packager also produces `GooPad-<version>-NOTICE.txt`; distribute it with
the firmware. The existing embedded `/font-licenses` notice resource includes
the Expat MIT license as well as font licenses.
Commit `dependencies.lock` and build releases from a clean checkout with a fresh
configuration seeded by `sdkconfig.defaults`. Record the source commit, SDK
revision, tool versions and production diagnostic flags with the artifacts.
Review version, image contents,
licenses, release assets and signing/security expectations before distribution.
[`tools/make_companion.ps1`](../tools/make_companion.ps1) packages the Windows
Companion ZIP and the per-user Inno Setup installer; see the README for its
requirements and bundled notices. Install Inno Setup and pass
`-InnoCompiler <path-to-ISCC.exe>` if `ISCC.exe` is not on PATH. The installer
and executable use `VERSION` from `companion/companion_update.py`; increment it
for a release and publish `GooPadCompanion-Setup-<version>.exe` with matching
stable release tag and GitHub SHA-256 metadata. Release the matching firmware
asset too: firmware update checks expect it on the latest GooPad release.
The installer is not code-signed, and release digests are not signatures.
Use a fresh Companion output directory. Packaging uses PyInstaller `--onedir`
and `--noupx`; the installer and per-user startup copy must include `_internal`,
not just the executable. Keep legacy `PYINSTALLER_RESET_ENVIRONMENT` handling
for updates from older self-extracting versions. Do not remove sharing or
weaken authentication to address an antivirus detection.
Before publication, scan the final directory, ZIP and installer with Defender
enabled, and record definition version, hashes and results. A clean source
rebuild alone does not establish why an older binary was flagged; dependencies,
archive layout and antivirus reputation can also differ. Never use exclusions,
obfuscation or unnecessary dependency padding to make a scan pass.

The frozen updater and installer must request an independent PyInstaller
runtime with `PYINSTALLER_RESET_ENVIRONMENT=1`. The installer sets it too,
because an older caller does not: otherwise restarting at the same executable
path can reuse a temporary Python directory already deleted on exit.
Do not repair this by copying DLLs or deleting pairing/configuration.
For unattended installation checks, use the installer's `Process.WaitForExit()`
rather than PowerShell `Start-Process -Wait`, which also waits for descendants
including the restarted Companion. Inno Setup can reuse an earlier install
directory; use an explicit `/DIR` for a controlled canonical-location repair
and verify the actual listening process path, not just the default location.

## Tests

* **Time, headlines, glass blur and radial geometry:** after IDF has installed
  cJSON and Expat, configure Expat once to generate a **host** configuration:

  ```powershell
  zig cc -Wall -Wextra -Werror -I main main\goopad_time.c tests\test_time.c -o build\test_time.exe
  .\build\test_time.exe
  cmake -S managed_components\espressif__expat\expat\expat -B build\host-expat -G Ninja -DCMAKE_C_COMPILER=zig -DCMAKE_C_COMPILER_ARG1=cc -DEXPAT_SHARED_LIBS=OFF -DEXPAT_BUILD_TOOLS=OFF -DEXPAT_BUILD_EXAMPLES=OFF -DEXPAT_BUILD_TESTS=OFF -DEXPAT_BUILD_DOCS=OFF -DEXPAT_ENABLE_INSTALL=OFF
  $xml = 'managed_components\espressif__expat\expat\expat\lib'
  zig cc -Wall -Wextra -Werror -DXML_STATIC '-DXML_DTD=' -DXML_GE=1 `
    -I main -I managed_components\espressif__cjson\cJSON -I $xml -I build\host-expat `
    main\goopad_news.c main\goopad_glass.c managed_components\espressif__cjson\cJSON\cJSON.c `
    "$xml\xmlparse.c" "$xml\xmlrole.c" "$xml\xmltok.c" "$xml\xcs.c" "$xml\random_rand_s.c" `
    tests\test_news_layout.c -o build\test_news_layout.exe
  .\build\test_news_layout.exe
  node tests\test_request_response.js
  node tests\test_asset_deletion.js
  node tests\test_visual_layers.js
  ```

  The Node harnesses compile actual firmware functions against host mocks.
  They cover bounded HTTP responses, redirect reset, stale-layout suppression,
  and saved image/background deletion protection. Radial tests verify exact
  non-overlap and square-corner circle bounds, not merely viewport containment.
  Editor tests cover the corresponding controls and previews. None substitutes
  for live HTTPS, physical touch/rendering or the five-minute device soak.
  This Windows host command compiles Expat directly, matching the generated
  host features rather than reusing the ESP target configuration or archiver.
  `test_news_layout.exe --feed` accepts an XML feed on stdin and prints only
  byte count, validity and headline count; use it to verify preset responses
  without copying headline content into reports. Battery-percentage validation
  needs a supported telemetry backend, which the current Guition lacks.

* **Font emphasis:** uses the actual LVGL glyph-selection API with synthetic
  regular/fallback bitmaps to verify bold/italic pixels, expanded metrics,
  in-place stride expansion and raw/decoded buffer ownership:

  ```powershell
  $lv = 'managed_components\lvgl__lvgl'
  $fonts = @(8,10,12,14,16,20,28,40) | ForEach-Object { "-DLV_FONT_MONTSERRAT_$_=1" }
  zig cc -std=c11 -O0 -ffunction-sections -fdata-sections -DLV_CONF_SKIP `
    -DCONFIG_LV_USE_FONT_PLACEHOLDER=1 @fonts `
    -I "$lv\include\lvgl" -I "$lv\include" -I "$lv\src" `
    -I tests\unicode_stubs -I main tests\test_font_emphasis.c main\goopad_fonts.c `
    "$lv\src\font\lv_font.c" '-Wl,--gc-sections' -o build\test_font_emphasis.exe
  .\build\test_font_emphasis.exe
  ```

  The visual harness extracts actual firmware rendering blocks, without
  claiming physical frame playback. Browser controls cover two-to-six gradient
  stops and drag continuity, directions, uniform select heights, theme reset,
  font emphasis and headline counts. No live layout fixtures are required.

Run the smallest relevant tests first, then hardware validation when behavior
depends on the board:

* **Release packaging:** `.\tests\Test-ReleasePackaging.ps1` uses isolated fake
  build artifacts and a mocked esptool; it never flashes a device.
* **Configuration migration:** with Zig:

  ```powershell
  zig cc -Wall -Wextra -Werror -I tests\config_stubs -I main main\goopad_config_storage.c tests\test_config_storage.c -o build\test_config_storage.exe
  .\build\test_config_storage.exe
  ```

  These tests use mock NVS; they do not prove flash behavior on a device.
* **Network priority:** with an existing C compiler, for example Zig:

  ```powershell
  zig cc -Wall -Wextra -Werror -I tests\network_stubs -I main main\goopad_network.c tests\test_network.c -o build\test_network.exe
  .\build\test_network.exe
  ```

  These host tests simulate streaming, preemption, timeouts and redirects;
  they do not exercise ESP-IDF's network stack or physical input.
* **Stock errors and widget image placement:**

  ```powershell
  zig cc -Wall -Wextra -Werror -I main -I managed_components\espressif__cjson\cJSON main\goopad_market.c main\goopad_alerts.c managed_components\espressif__cjson\cJSON\cJSON.c tests\test_market_layout.c -o build\test_market_layout.exe
  .\build\test_market_layout.exe
  ```

  The provider-message checks use synthetic data, not saved API keys.
* **Daily alarms:** with Zig and the managed cJSON dependency:

  ```powershell
  zig cc -Wall -Wextra -Werror -I main -I managed_components\espressif__cjson\cJSON main\goopad_alarm.c managed_components\espressif__cjson\cJSON\cJSON.c tests\test_alarm.c -o build\test_alarm.exe
  .\build\test_alarm.exe
  ```

  These host tests exercise the scheduler, not physical audio or timekeeping.
* **Companion:** create an isolated venv, install
  [`companion/requirements.txt`](../companion/requirements.txt), then run:

  ```powershell
  .\build\companion-test-venv\Scripts\python.exe -m unittest discover -s tests -p test_companion.py
  ```

  The tests exercise an actual local HTTPS server with a test certificate and
  mocked OS actions. They do not prove physical Windows input, hardware sensors,
  tray behavior or the packaged executable on every system.
  After packaging, also exercise the **actual frozen update/restart chain**:

  ```powershell
  .\build\companion-test-venv\Scripts\python.exe tests\companion_installer_smoke.py --package-directory C:\Temp\GooPad-release\GooPadCompanion-Windows --inno-compiler C:\Tools\InnoSetup\ISCC.exe --seconds 300 --report C:\Temp\GooPad-companion-soak.json
  ```

  This builds a small frozen legacy updater, replaces it at the **same path**
  through the installer, and checks the actual packaged app over trusted HTTPS
  after the original Python runtime has disappeared. It uses a temporary
  profile, private test identity, random port, no uninstall registration and
  app-local shortcuts; it does not install over the real Companion or change
  Windows startup. It checks monitoring, authentication, saved permissions and
  pairing-file preservation, copies the complete app for startup, moves the
  original install out of reach, and soaks the copied executable. It then
  closes the test app and removes fixtures.
  A shorter `--seconds 15` run is useful while iterating, not a device soak.
  GUI regressions also exercise the single **Enable Sharing and PC Controls**
  toggle, disable-all, cancellation/save errors and legacy mixed permissions.
* **Editor:** open local `main/index.html` as a `file:` URL, load
  `tests/test_editor.js` as a classic script and run
  `await runGooPadEditorTests()` in the browser console. The suite uses
  synthetic layouts and mocked saves and refuses a live device URL. It covers
  UI/config behavior, not the ESP32 renderer or network stack. In an automated
  runner, disable the `updateWidgetPreview` and `refreshMonitorPreview`
  background intervals before loading the page so periodic requests do not
  race the suite's fetch mocks. Leave gesture/debounce timeouts enabled.
  Run `node tests/test_gif_normalization.js` with Zig installed to compare
  the editor's GIF normalization against the compiled firmware routine.
  Run `zig cc -Wall -Wextra -Werror -I main -I managed_components\espressif__cjson\cJSON main\goopad_companion.c managed_components\espressif__cjson\cJSON\cJSON.c tests\test_companion_config.c -o build\test_companion_config.exe`
  and then `.\build\test_companion_config.exe` to test shared PC pairing
  resolution and now-playing request construction, including invalid references.
  Run `node tests/test_media_gestures.js` with Zig installed to compile the
  actual LVGL media callback against event/action mocks and check tap, hold,
  release suppression, cancelled presses and explicit action-error logging.
  It does not prove physical touch or Windows player behavior.
  Wake-on-LAN validation should also be checked on the device; a controlled UDP
  receiver on the same subnet can verify the exact magic packet without waking
  an actual computer. Packet transmission does not prove a target can wake.
* **Device:** [`tests/Test-GooPadDevice.ps1`](../tests/Test-GooPadDevice.ps1)
  requires PowerShell 7, device HTTPS and credentials. `-NonMutating` avoids
  layout/power changes but still sends rejected validation requests. The
  default and `-TileVisibilityOnly` temporarily modify configuration and
  restore it; run only with permission and a backup, and do not run while
  someone else is editing. The script uses `-SkipCertificateCheck`, so it is
  a controlled-LAN diagnostic, **not** a TLS identity-verification test.
  An isolated test build may define `GOOPAD_RENDER_DIAGNOSTIC` for the
  firmware's two-page scroll, framebuffer, fallback and synthetic-touch checks.
  It automatically scrolls the device and generates touch events; never enable
  it in a production build. These checks do not replace physical touch tests.
  For example, with a separate build directory and SDK config, pass
  `-D CMAKE_C_FLAGS=-DGOOPAD_RENDER_DIAGNOSTIC` to `idf.py`. Check the serial
  `RENDER_DIAG` results, and replace the test image with a normal build afterward.
* **Read-only soak:** [`tests/Test-GooPadSoak.ps1`](../tests/Test-GooPadSoak.ps1)
  polls authenticated HTTPS status and diagnostics for five minutes by default
  after relevant changes, not every chat session. Request a longer duration
  explicitly, or extend it when investigating resets, leaks or timing problems.
  Supply a SHA-256 fingerprint of the device's **public certificate**, verified
  through a trusted channel. For example, after exporting that certificate as
  PEM and verifying it belongs to your device:

  ```powershell
  $cert = [System.Security.Cryptography.X509Certificates.X509Certificate2]::CreateFromPemFile('C:\path\goopad-public-cert.pem')
  $pin = $cert.GetCertHashString([System.Security.Cryptography.HashAlgorithmName]::SHA256)
  $credential = Get-Credential -UserName admin -Message 'GooPad admin password'
  .\tests\Test-GooPadSoak.ps1 -Device 'https://goopad.local' -Credential $credential -CertificateSha256 $pin -DurationSeconds 300 -ReportPath 'C:\Temp\goopad-soak.jsonl'
  ```

  The hostname/IP must match the certificate. The optional JSONL report is
  created only if it does not already exist and is flushed after each record.
  It contains numeric health metrics and error categories, not credentials,
  configuration, SSIDs, URLs or widget text. A nonzero exit means an error was
  observed; warnings include Wi-Fi disconnects and network deferrals. The
  optional `-RequireCompanion` switch additionally fails a sample unless PC
  monitoring has a valid CPU/RAM snapshot, catching "PC unavailable" and
  "Update deferred" rather than treating a healthy HTTPS server as a working
  Companion connection. Use it on a paired device after widgets finish startup.
  Reports include only Companion availability and the number of deferred
  widgets, never their text. It does not assert that a song is currently playing.
  Add `-MaxWidgetDeferralSeconds 15` to fail on a widget continuously deferred
  for more than 15 seconds; the default zero disables that optional limit.
  The report records its duration without logging widget contents.
  The first diagnostic ring is a baseline, so earlier events are not reported.
  The device retains only 48 events without sequence IDs; events can be missed
  between samples, especially identical events, and obvious gaps are reported.
  This does not capture raw serial logs, continuously verify rendering or
  exercise touch or actions. A short test validates setup, not long-term
  stability. To test the harness using a synthetic local HTTPS server, run
  `python -m unittest discover -s tests -p test_soak.py -v` in an environment
  with the Companion's `cryptography` dependency and PowerShell 7.
  Reports include separate status and diagnostics request latency, as well as
  combined polling latency; these measure the client/network/HTTPS path, not
  physical tap-to-action response time. Deferred-network events now identify
  their source in `detail`: 1 macro, 2 manual PC diagnostic, 3 live widget,
  4 PC monitor/now-playing. Result 263 means the shared network lock was busy;
  result 257 indicates the internal-memory admission threshold was reached.
  Older builds record detail 0, which cannot identify the requesting worker.

## Other boards

See the [board porting guide](boards.md) before targeting another ESP32.
Never use a Guition pin map, partition layout or merged image on unverified
Waveshare hardware. Screen/controller names alone do not establish compatibility.

## Unicode fallback fonts

The current development build uses compact compiled essential Latin/punctuation
and a matching SD-backed Lato/Noto pack with 51,647 glyphs per size.
Read [multilingual fonts/resources](unicode-fonts.md) for the format, licenses,
offline behavior, installation and native cache/bitmap regression.
Startup checks printable ASCII and representative multilingual mappings in
all 68 font variants. This does not prove universal script shaping.

Regenerate using an installed `lv_font_conv` 1.5.3 and Node:

```powershell
.\tools\make_multilingual_resources.ps1 -ConverterPath C:\font-tools\node_modules\lv_font_conv\lv_font_conv.js -NodePath C:\node\node.exe -CacheDir C:\font-cache -OutputDir C:\goopad-resources
node tests\test_multilingual_pack.js C:\goopad-resources
```

Regeneration updates the firmware's filename/SHA manifest; build and install
matching resources together, retaining the old filenames for recovery.
Run production `reconfigure build` and check actual headroom before deployment;
never expand/erase live partitions to force a font build to fit.
The old full-Lato generator/test remain historical asset tooling.

## SD editor and styled-font resources

Production configuration generates the web archive and its compiled asset
manifest from the editor and browser fonts. Gzip and identity HTTP clients
receive their respective stored representation; neither loads the whole page
into PSRAM. An embedded recovery page remains available without SD.

```powershell
python tools\make_web_resources.py main C:\goopad-resources main\goopad_web_manifest.h
python -m unittest discover -s tests -p test_web_resources.py
```

The archive normalizes gzip timestamps/OS metadata for reproducible immutable
filenames. Native tests execute the actual loader and HTTP handlers, including
gzip/identity, missing/corrupt/truncated files, interrupted sends, 1 KiB chunks
and recovery responses. Browser recovery tests must mock requests or run only
on a local file page; never exercise uploads/actions against user fixtures.

Styled-pack tooling parses the 60 generated font C inputs and preserves their
pixels, metrics and kerning; those reference sources and licenses are required
to reproduce the pack. Production compiles only essential fallbacks, not those
styled C inputs. Run the focused native suite with the matching Unicode pack:

```powershell
.\tests\run_styled_native_tests.ps1 -ResourceDirectory C:\goopad-resources -NodeExe C:\node\node.exe -ZigExe C:\zig\zig.exe -UnicodePack C:\goopad-resources\text-371771469d5d.gfp
```

It covers original-source parity, actual glyph decoding, warm lookups, shared
32 KiB LRU/pin lifetime and independent missing/corrupt-pack failures.
Install the matching `web` and `styled` resources after app-only OTA, reboot
for font initialization, and verify configuration/pairing plus PSRAM on-device.
* **HomeKit:** separately check paired controller discovery, local and remote
  switch state/notifications, reconnect, reboot and topology changes with
  actual Apple Home hardware where available. Do not delete a user's pairing
  or change its topology as a routine test. The browser and Companion tests
  do not prove Apple Home interoperability.

Do not claim a hardware, certificate, Apple Home, or packaged-app test passed
unless it was actually run. Capture test commands and outcomes in the change
description without leaking credentials, pairing material or private configs.

## Adding features

### A tile or action

1. Define the saved representation and compatibility with old configs.
2. Add editor controls, preview, and `prepareMacroAction()` conversion in
   `main/index.html`; consider both newly created and existing tiles.
3. Add bounds and cross-field validation in `validate_config()`. Update
   `macro_configured()`/`macroConfigured()` if the new mode affects visibility.
4. Add dispatch in `trigger_action()` and, if needed, device rendering/callbacks
   in `goopad_ui.c`. Copy data before leaving the config lock; avoid network
   and LVGL cross-thread access.
5. Add focused editor and device validation tests. For Companion actions also
   update `validate_steps()`, `execute_steps()` and Companion tests; enforce
   permissions on the PC, not only in the browser.
6. Document user behavior, failure modes, secrets and upgrade implications.

### A widget

Add editor options and preview, firmware validation and UI rendering. For data
from an external provider, use bounded asynchronous fetching and a cache;
decide refresh cadence, quotas, unavailable states and privacy implications.
Keep LVGL rendering on its thread, and test small tiles, all relevant shapes,
missing data, time changes and reconnect.

### A HomeKit change

Preserve `homekitId` and accessory IDs across reorder/rename; compare topology
and restart only as appropriate. Do not modify the dedicated store without a
power-loss and downgrade plan. Test pairing window timeout, local and remote
writes, notifications, restart persistence and real Apple Home behavior using
a disposable pairing. Never assume a config backup restores HomeKit keys.

## Security and recovery

* Editor access is privileged. Basic auth is protected by device HTTPS; the
  self-signed certificate must be verified/accepted for the correct device.
  Avoid exposing device or Companion ports to the internet.
* PC pairing includes a token and public certificate; the private key stays on
  the PC. Token holders can invoke allowed actions; enabling PowerShell or
  system controls increases the consequences. Companion runs as the signed-in
  user and cannot reliably send input to locked/elevated desktops.
* Device flash and NVS are unencrypted in the development setup. Anyone with
  physical flash access can read credentials and pairing keys. Do not enable
  irreversible eFuses without an explicit security/recovery plan.
* Logs, SD backups, configuration snapshots, serial captures and test fixtures
  may contain secrets. Sanitize before sharing; do not commit real material.
  The About page's diagnostic download is separately constructed from an
  allowlist of numeric fields; use it instead of forwarding raw serial output.
  USB serial commands at 115200 baud are `help`, `status`, `diag`,
  `test pc [post]`, and `run <1-32>`; `run` performs the real saved action. `test pc`
  queues a read-only `/status` request to the paired PC monitor through the
  same worker as a macro; the `post` variant sends `{}` to that read-only path
  and normally receives HTTP 404. Event 7 records its result and latency without
  changing the saved configuration or last action. Only the `GOOPAD_DIAG`
  JSON record is designed for sharing, not surrounding SDK/serial messages.
  Physical serial access permits action execution and remains privileged.
* If NVS initialization fails, inspect the serial error and preserve it. If
  HomeKit reports unavailable, inspect `/api/homekit`, switch config and storage
  logs first; a claimed/nonblank partition must not be blindly formatted.
* If HTTPS times out but serial reports an IP or mDNS discovery works, verify
  actual TCP reachability, Wi-Fi signal, route, subnet and firewall. Discovery
  is not proof HTTPS or HAP connections work. Check certificate identity and
  clock separately from transport reachability.
* If an image/GIF is unavailable, check SD mount, referenced key and file
  format. If a widget is unavailable, inspect its provider/Companion status,
  certificate, clock and quota; do not replace errors with invented values.
