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
`idf.py set-target` can reset configuration; do not run it casually against an
existing configured workspace. Keep the SDK and the build's `SDKCONFIG`
consistent. `main/CMakeLists.txt` embeds the editor and generates the compressed
Font Awesome index at configure time.

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
only copy of HomeKit pairing keys. Do not erase NVS or format `storage`, and
do not use a full-flash command on an existing device as a shortcut. `idf.py
flash` and a merged image can write bootloader, partition table, OTA selector
and an app slot; they are not equivalent to a safe app-only update.

`app_main()` marks an OTA app valid after initialization. Rollback protects
against failure to reach that point, not every later bug or network failure.
Observe boot logs and verify the actual feature, persisted config and paired
bridge after the update. If HTTPS is unreachable, distinguish Wi-Fi/DHCP,
LAN routing and certificate problems from a failed boot; do not erase data.
Manual serial recovery must first identify the live device, partition table,
active selector and known-good slot; never assume a COM port or slot.

For a **fresh/disposable board** or an explicitly approved recovery with
backups, [`tools/make_release.ps1`](../tools/make_release.ps1) creates a merged
first-flash image and an app-only OTA binary:

```powershell
.\tools\make_release.ps1 -BuildDir C:\Temp\GooPad-idf-build
```

The script's `-Port` option writes the merged image at flash offset 0. It is
not the normal update procedure. It reads flash parameters from the root
`sdkconfig`; ensure that file matches the build being packaged, especially
when the build uses a different `SDKCONFIG`. Review version, image contents,
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

## Tests

Run the smallest relevant tests first, then hardware validation when behavior
depends on the board:

* **Companion:** create an isolated venv, install
  [`companion/requirements.txt`](../companion/requirements.txt), then run:

  ```powershell
  .\build\companion-test-venv\Scripts\python.exe -m unittest discover -s tests -p test_companion.py
  ```

  The tests exercise an actual local HTTPS server with a test certificate and
  mocked OS actions. They do not prove physical Windows input, hardware sensors,
  tray behavior or the packaged executable on every system.
* **Editor:** open local `main/index.html` as a `file:` URL, load
  `tests/test_editor.js` as a classic script and run
  `await runGooPadEditorTests()` in the browser console. The suite uses
  synthetic layouts and mocked saves and refuses a live device URL. It covers
  UI/config behavior, not the ESP32 renderer or network stack.
* **Device:** [`tests/Test-GooPadDevice.ps1`](../tests/Test-GooPadDevice.ps1)
  requires PowerShell 7, device HTTPS and credentials. `-NonMutating` avoids
  layout/power changes but still sends rejected validation requests. The
  default and `-TileVisibilityOnly` temporarily modify configuration and
  restore it; run only with permission and a backup, and do not run while
  someone else is editing. The script uses `-SkipCertificateCheck`, so it is
  a controlled-LAN diagnostic, **not** a TLS identity-verification test.
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
