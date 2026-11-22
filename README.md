# GooPad

GooPad turns the Guition / CYD **ESP32-S3-4848S040** (4-inch, 480×480)
touch display into a Wi-Fi macro pad. It is built with ESP-IDF 6.1 and LVGL
9; the existing display, GT911 touch, SD-card, and board drivers are retained.

![GooPad logo](web/logo.svg)

For maintainers: [architecture and code map](docs/architecture.md),
[development and recovery](docs/development.md), and
[API and data contracts](docs/api.md). Future coding agents should also read
[AGENTS.md](AGENTS.md).

## First boot

1. Power the board and use its touch-screen keyboard to enter Wi-Fi details.
   **Scan networks** fills the SSID list; selecting a network populates its
   name. The password field uses the same on-screen keyboard.
2. After joining, the device shows its IP address and unique web-admin
   password. Open `https://<device-ip>/` on a computer or phone on that Wi-Fi.
   The device creates its own self-signed certificate: accept the browser's
   certificate warning for your GooPad before signing in.
   New devices use an eight-character initial password without ambiguous
   letters. Changing the password never changes the initial password shown
   in Info, and the new password is never displayed. Older firmware did not
   preserve the initial password separately; upgrades keep your login intact
   and omit the password from Info rather than expose a changed password.
3. Sign in as `admin` with the password on the display. Create macros, choose
   a 2–4 column and row layout, colors, theme, and tile shape, then save.
4. Tap a configured tile on GooPad to run its action. Unconfigured tiles stay
   inactive, so you can save a layout before adding every action. Swipe between
   pages; the page dots show the current page.

The browser editor is served from the device and remains available at its
current DHCP address. Reserve an address for GooPad in your router if you want
a stable bookmark.
If nonvolatile storage cannot be initialized, startup stops rather than erasing
Wi-Fi credentials and configuration. Check the serial log and recover a backup
before deliberately reinitializing storage.

## Macros and customization

Each tile supports an optional label, color, visual, and a plain-language action.
Choose **Icon / emoji**, **Font Awesome**, **Image**, or **Animated GIF** from its visual-type
dropdown. Blank labels stay blank; the visual is enlarged and centered.
The live preview is the editor's centerpiece: select any tile, including an
empty slot, to edit it. Choose the button type at the top of its settings.
Only that tile's controls are shown. You can configure
positions in any order. **Button label** is optional on-screen text; tiles have
no separate editor name.
**Clear tile** leaves other positions untouched. Drag a preview tile onto an
empty position to move it, or onto another tile to swap them. Hold a dragged
tile over a navigation arrow, or near the preview's left/right edge, to change pages. Mouse swiping starts
in the space between tiles; touch scrolling remains available.
The preview-only arrows navigate without dragging. **+** inserts a page after
the current page and leaves all tiles unselected; **−** removes the current page, confirming if it contains
tiles. Other pages shift with their contents. At least one page remains, with
up to 32 positions total; insertion is rejected if it would discard configured
tiles beyond that limit. Turn off **Compact configured tiles** to edit pages.
Page changes apply when you save.
Enable **Hide empty tiles (keep positions)** above the preview to remove empty
boxes without shifting configured tiles or removing pages. The preview's empty
spaces remain selectable for editing and accept dragged tiles. Alternatively,
**Compact configured tiles** packs configured tiles together and hides unused
pages. Existing compact layouts retain their behavior. Save to apply either option.
**Save to device** is below the live preview.
Save progress, confirmation, and errors appear only there. **Save appearance**
returns to the preview so its result is visible. **About & help** includes
downloads, setup instructions, examples, troubleshooting, and source links.
**Custom web request** sends a GET or POST from GooPad and allows a JSON body
and optional `X-GooPad-Token` header. Existing service-link tiles remain GET
requests. To open a website on your
PC, choose **Windows PC macro** and the **Open a website** starter or step.
It uses the Companion pairing in Settings and opens HTTP(S) links in the PC's
default browser. Existing website tiles keep working. Requests from GooPad
validate HTTPS certificates.
The searchable icon picker includes 275+
symbols, and the icon field accepts other short icon codes or glyphs.
**Full-color emoji** renders the browser's emoji as a transparent 160×160
image on save and requires SD; unchanged saved emoji images are reused.
New tiles use this mode. Existing tiles retain **Built-in device symbol (no SD)**,
which maps common emoji to monochrome LVGL symbols rather than a full emoji font.
Gear, play and other mapped symbols accept either text or emoji variation selectors.
Unsupported built-in glyphs fall back to the label's initial or `?` and are logged;
choose full-color mode or Font Awesome for these. The editor also bundles 2,163
searchable Font Awesome Free solid, regular, and brand icons. Enter Font
Awesome codes such as `fa-server` or `fa-brands fa-github`, or search the
bundled catalog. The picker first downloads a small compressed name index and
only fetches matching drawings. Selection progress stays inside the picker;
direct-code progress uses the main page. Repeated selections reuse uploaded
assets, and lossless run-length compression reduces icon-upload traffic.
Image/GIF upload status uses a bright, bold bordered notice beneath its picker,
distinct from the field description; errors are red.
Font Awesome selections and uploaded images are converted to
160×160 RGB565 with an 8-bit alpha plane and saved under
`/goopad` on the SD card; a card is required to use them. The **Settings** page
contains preset themes plus Custom background controls, website appearance, backups, firmware
updates, and admin-password changes. Swipe downward from the top edge of the
device to slide open its information menu; the old information icon is removed.
Start within the top 32 pixels and pull down at least 64 pixels.
The menu's Settings has display-brightness, sound-volume, and SD firmware-update
controls. Appearance and themes are configured only from the website. Brightness
and volume changes take effect immediately; **Save settings** persists them.
Website themes are independent of device themes and are saved in the current browser.
The display has Midnight, Ocean, Forest, Light, Slate, Violet, Ember, Berry,
Teal, and Black palettes, custom background colors, and per-macro font colors and
rounded, square, circle, pill, or soft-corner tile shapes. Up to 32 macros are stored locally;
grid size controls how many appear on each page. Each macro can choose an
outline color or remove its outline. Action tiles also support a custom
two-color vertical gradient and three icon/image sizes; set both colors alike
for a solid tile. GIFs set to fill the tile ignore the visual-size setting.
**Icon color** independently tints built-in symbols and Font Awesome icons.
Full-color emoji, uploaded images and GIFs retain their original colors. Use
**Use default color** to restore the previous appearance.
Existing page-navigation tiles continue to work until cleared or changed, but
new ones cannot be created. Preset themes always use their own
background; choose **Custom** for your own background. Custom also accepts an
uploaded image (up to 15 MB input, SD required). The browser center-crops it to
480x480 RGB565 and fills transparent pixels with the selected background color.
The device loads the 460,800-byte image into PSRAM. Preset themes ignore the
stored image; switching back to Custom restores it. Removing it from the layout
requires saving. Images are stored separately on SD, so a config-only backup
does not include the assets.

Choose **Live widget** for a live clock, date, or **PC stats**.
Widgets are display-only on the device; select them in the editor to configure
them. Time format offers 12/24-hour displays, with optional seconds and optional
AM/PM for 12-hour clocks; the default
remains 24-hour without seconds. Date format offers year-month-day, month/day/year,
day/month/year, and short month/day; the default remains year-month-day. Date preview text
scales to the tile's dimensions. Clock/date use network time or time
synced when the website opens; Settings includes a fixed UTC offset, initially
matched to the browser. Update that offset after daylight-saving changes.
Each widget has 25 color presets, including Midnight glass, Cloud,
Lavender, Mint, Aurora, Ember, Arctic, and Berry, or Custom to use your own tile, gradient
end, text, outline and graphic-accent colors. Switching away from Custom
preserves those colors for switching back. The optional **Modern card** layout
adds a compact heading and accent to larger rounded or square tiles, and falls
back to the classic layout on smaller tiles and other shapes to keep readings
legible. Existing widgets retain their original appearance until changed.
Choose text-only or an icon badge, a live analog clock, a calendar card, or a
PC usage bar/ring. Usage graphics support CPU, RAM and GPU percentages; the
combined PC summary graphic shows CPU usage. Missing readings dim the gauge
and show an unavailable message rather than a fabricated zero reading.
Text size supports Auto or 12, 14, 16, 20, 28 and 40 device pixels, fitting down
to keep readings inside the tile. Tiny tiles simplify graphics to text when
necessary. Device graphics use lightweight LVGL vectors, not uploaded assets.
PC stats use the paired Companion. Choose CPU/RAM combined, CPU usage,
RAM percentage or used GiB, GPU usage, dedicated GPU RAM used, CPU temperature,
or GPU temperature, download/upload speed, or fastest reported fan RPM per tile.
Temperature tiles support Celsius/Fahrenheit. Network speed sums non-loopback
interfaces and fan readings require LibreHardwareMonitor or OpenHardwareMonitor.
CPU/RAM refresh every five seconds; cached GPU/sensor readings update about
every ten seconds, without blocking incoming PC commands.
The web preview reads the device's cached result without blocking its web
server on a second PC request. Save pairing/widget changes to update that result.
After updating firmware, reload open editor tabs: older direct monitor-proxy
requests return a reload message rather than tying up the upload server.
It requires the same LAN reachability as PC commands and shows an explicit
unavailable/sampling message when no reading is available.
GPU usage is the busiest Windows GPU engine (summed across its processes);
dedicated GPU memory sums adapters, not shared system RAM. GPU counters require
supported Windows drivers. NVIDIA GPU temperature uses the driver's
`nvidia-smi.exe` when available. For CPU temperatures and other GPU temperature
sensors, optionally run the official
[LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/releases)
Windows Forms app alongside Companion, keeping its WMI sensor provider available.
Some sensors require **that sensor app** to run as administrator; keep GooPad
Companion running as your normal user. No remote web server is needed.
Unsupported sensors display **Unavailable**, not a fake zero; CPU package/GPU
core readings are preferred, with the highest matching temperature used if needed.

Additional widgets include countdown (an absolute date/time; synchronized clock
required) and stopwatch. Tap the stopwatch to start/pause, hold to reset; it
resets when the layout reloads or the device restarts.

Weather uses latitude/longitude and Celsius or Fahrenheit. Weather data by
[Open-Meteo.com](https://open-meteo.com/), [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/):
temperatures are rounded and weather codes summarized. The free endpoint is for
non-commercial use and transmits the coordinates to Open-Meteo. Weather refreshes
every 15 minutes. Crypto uses Coinbase USD spot prices every five minutes.
Stock prices use an Alpha Vantage key and refresh every six hours per tile.
Alpha Vantage's free quota is 25 requests per day shared by the key: several
tiles or frequent reboots may exhaust it, and quotes may be delayed. The stock
key is stored in device configuration and backups; protect both. Live widgets
use verified HTTPS and show unavailable on errors or provider limits. They are
informational, not financial advice.

Home Assistant switch/light tiles support toggle, on and off through its REST
API. Enter a long-lived access token and an HTTPS address with a publicly
trusted certificate; HTTP and self-signed certificates are not supported. The
token is stored in the device configuration and backups. These controls do not
show live switch state. This action only controls entities available to Home
Assistant.

### Apple Home bridge

With the ESP-IDF 6.1 HomeKit build, choose **Apple Home switch** for a tile,
enter a switch name, and save. Adding, removing, or renaming a switch restarts
GooPad; moving a switch preserves its HomeKit identity. In Settings, open
pairing and use the displayed code in the Apple Home app's **More options**.
The code expires after five minutes. Keep it private. Create an automation in
Apple Home to make the virtual switch control a light or another accessory;
the switch does not directly control the light. Switch states start off after
a restart. On the tile, a filled green dot means on and a hollow circle means
off; a gray circle means the state is unavailable. The editor shows an example
indicator, not the live state. With no switch tiles, the bridge is not running.

HomeKit uses its own encrypted protocol on TCP port 80; the GooPad editor
continues to use HTTPS. Pairing and accessory identity are stored separately
from the configuration and are not included in SD configuration backups.
Without flash encryption, someone with physical flash access can read pairing
keys. This build does not enable flash encryption or secure boot.

HomeKit claims 68 KB at the end of the `storage` SPIFFS partition only if the
partition is initially erased; otherwise it refuses to start rather than
overwriting existing data. The rest of that partition must remain erased.
Do not mount or format it as SPIFFS while using HomeKit, including after a
downgrade; doing so could destroy pairing data. Existing NVS configuration and
SD assets are separate. Older ESP-IDF builds do not enable this bridge; they
have not been validated with configurations containing HomeKit tiles.

The bridge has been tested with a software HomeKit controller; a user has also
confirmed Apple Home pairing and switch operation. Broader compatibility testing
remains limited.

Animated GIFs require SD and are limited to 160×160 pixels, 128 KB, 240 frames,
and four GIF tiles per layout. Device frame delays are clamped to at least
100 ms (10 fps); animations pause on hidden pages and behind the Info panel.
The browser preview may animate faster than the device. Resize or compress
larger GIFs before uploading. Upload progress and errors appear beside the
GIF file chooser; saving a GIF without a successfully uploaded asset is rejected.
Choose **GIF size → Fill whole tile** to zoom proportionally and crop to the
tile shape, with any button label overlaid. **Fit inside tile** preserves the
existing centered-icon sizing. This changes display size, not upload limits.
TLS allocations use PSRAM to avoid exhausting internal RAM when browsers
open multiple HTTPS connections. The HTTPS session pool is bounded at six
connections, leaving socket capacity for server control and outbound commands.
Unconfigured device tiles have no plus sign and cannot be tapped.
**Hide unconfigured tiles** compacts
configured actions and widgets without changing their action assignments.
Turn it off in the editor to access empty positions.
An existing SD configuration backup can be downloaded with an authenticated
`GET /api/backup`. This is read-only and never replaces the backup.

Each button can play a click, beep, chime, or alert through the board's I2S
speaker, either alongside its web action or with **Play a sound only**.
Select **Uploaded WAV / MP3** to upload a sound up to 30 seconds (15 MB input).
The browser decodes and resamples it to 22050 Hz mono PCM WAV; the device
streams it from SD on a separate task. Repeat taps while playback is busy
are rejected rather than accumulating sounds or web actions.
The board must have a speaker and the speaker routing links fitted.
All sounds, including uploads, retain the original 60% waveform-amplitude
cap; backlight brightness retains its 70% PWM cap. The user-facing sliders
still range from 0–100. These caps do not replace an adequate USB supply.

New icons retain transparency even when the tile color changes. Saving
automatically upgrades older Font Awesome icons to the higher resolution.
Re-upload older custom
images once: their original alpha was discarded by the previous firmware
and cannot be reconstructed reliably.

The editor and device API now use HTTPS on port 443 only. Each device generates
its own P-256 key and self-signed certificate and saves them in NVS; firmware
updates and reboots preserve them. The certificate includes the initial IP
address, so reserve that address in your router. Browsers will warn because
the certificate is self-signed (and if the IP changes). Only accept it for
your own device on a trusted LAN; do not expose GooPad to the internet.

The bundled Font Awesome icon data is version 7.3.1; attribution and license
details are in [third_party/fontawesome/NOTICE.txt](./third_party/fontawesome/NOTICE.txt).
To regenerate the catalog from the official npm packages, unpack the matching
`free-solid-svg-icons`, `free-regular-svg-icons`, and `free-brands-svg-icons`
packages into one directory and run
`python tools/generate_fa_catalog.py <packages-directory> main/fa-icons.json`.

The HomeKit SDK's attribution and license notices are in
[third_party/esp-homekit-sdk/NOTICE.txt](./third_party/esp-homekit-sdk/NOTICE.txt).
Include these notices and the referenced licenses when distributing firmware.

### Windows companion

Download **GooPadCompanion-Windows.zip** from the site's **About & help** tab or
[the latest release](https://github.com/jeromedontdev/GooPad/releases/latest).
Extract it and run `GooPadCompanion.exe` as a normal Windows user. No Python
installation is needed for that app. Closing quits and minimizing uses the
taskbar by default. You can independently change either button to **Hide in tray**;
the receiver keeps running while hidden. Choose **Copy secure pairing**, paste
the code under website **Settings > Windows companion**, and select **Apply secure pairing**.
This imports the HTTPS address, private token and public certificate. Check
the HTTPS connection, then save. The last-result check also shows errors and whether
PowerShell is enabled. Allow TCP port 8787 only on a trusted Private network.
Secure pairing is the only pairing UI: there are no separate address/token/
certificate fields or Copy buttons. Existing saved pairing is retained when
saving unrelated appearance changes. The Companion executable, window,
taskbar and tray share the original GooPad grid icon.

Choose **Windows PC macro (guided builder)** on a tile. Starters include Copy,
Paste, Screenshot, Play/Pause and Mute. Add/reorder/remove up to 12 steps:
hotkeys, media keys, typed text, waits, HTTP(S) websites, PowerShell commands,
and locally approved app actions. No webhook or JSON knowledge is required.
Total waits are limited to 10 seconds; each serialized sequence is limited to
3072 UTF-8 bytes, and all device configuration must fit 12 KB.
Sequences do not overlap: repeated taps while a PC sequence runs are rejected.
Keyboard/text steps target the foreground application, not a chosen window.
Windows blocks input into elevated apps, locked screens, and some games.

PowerShell is **off by default**. Enable it locally using the Companion checkbox
only on a trusted network. Anyone holding the token can then execute commands
as your Windows user. Each command is limited to 512 characters and 10 seconds;
a failed step stops the sequence and is reported in the app/log and last-result
check. Do not run Companion as Administrator. Its config and log are in
`%LOCALAPPDATA%\GooPad`.

PC power and monitor controls are separately disabled by default and must be
enabled locally in Companion. They include lock, monitor off, sleep, hibernate,
shutdown, restart, and cancelling a scheduled shutdown/restart. Shutdown and
restart wait 60 seconds and then force-close applications; unsaved work can be
lost. Sleep may hibernate depending on Windows settings. Anyone with the paired
token can invoke enabled system controls, so only enable them on trusted networks.

#### Manual or automatic Windows startup

| Mode | Starts | Stops | Available actions |
| --- | --- | --- | --- |
| Open the app manually | When you open it | Quit Companion or sign out | All desktop actions |
| **Start with Windows** checkbox | When this user signs in, minimized or hidden in tray | Quit Companion or sign out | Same desktop actions |

**Desktop startup is recommended** for everyday macro-pad use. Hotkeys, media
keys and typed text require your awake, logged-in, unlocked desktop. Minimize
the app to keep it running, or select tray behavior for Close/Minimize.
**Hide in the tray on Windows startup** starts without a taskbar window.
Use the tray's **Open GooPad Companion** item to restore it and **Quit** to
stop the listener, regardless of the Close preference. Windows startup uses
only this user's `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` entry.
For the packaged app it copies the executable into
`%LOCALAPPDATA%\GooPad\bin`, so moving the extracted download does not break
startup. Disabling the checkbox removes the startup entry, retaining that copy.
After downloading an update, **Quit** the old app, open the new executable, and
uncheck/recheck this option to refresh its startup copy. Remove any old manual
`shell:startup` shortcut to avoid launching two copies. When running from Python
source, keep the source directory and interpreter at their registered paths.

Both launch methods support PowerShell when enabled locally, running as your
normal Windows user. No administrator prompt or Windows service is needed.
The app does not run before sign-in or after sign-out. Windows service support
is intentionally not included.

The device talks to Companion over token-authenticated **verified HTTPS** on
port 8787. Each installation has a unique persistent certificate and private key.
GooPad trusts the public certificate copied directly from your PC and verifies
the fixed identity `companion.goopad`, even when connecting by IP.
Neither endpoint falls back to plaintext HTTP. Never port-forward either app.
Keep the pairing code private; never copy a private key into GooPad.
Certificates and keys remain in `%LOCALAPPDATA%\GooPad`; back them up together
with Companion configuration. An incomplete, invalid or expired identity fails
explicitly rather than silently replacing an already paired identity.

**Upgrading from 0.5.x:** replace Companion, import its secure pairing code,
check the connection and save Settings, then save your PC macros to regenerate
their HTTPS URLs. Legacy HTTP macros remain stored but are blocked from sending
tokens until migrated. Advanced `/hook/` actions targeting the paired Companion
also use its trusted certificate. Public HTTPS webhooks use the public CA bundle.
**Test saved action** actually runs the saved macro after confirmation.
Live preview reports running, HTTP success/rejection, or transport/TLS failure;
HTTP 204 is a successful webhook response.
Firmware enables certificate validity-date checks and cross-signed CA-bundle
verification, including the public chain used by Discord webhooks.

`companion\Start-GooPadCompanion.cmd` starts a Python
webhook receiver on the Windows PC. On first run it creates
`%LOCALAPPDATA%\GooPad\companion.json` with a random shared token. Add an
action entry that maps a short ID to a program and argument list, then set a
GooPad macro URL to `https://<pc-ip>:8787/hook/<id>` and copy the token into
that macro's companion-token field. For the simple **Open a website on my
Windows PC** action, use secure pairing in Settings; no JSON
or action entry is needed. The companion opens only HTTP(S) links in the
default browser. Requests are token-checked and private-LAN only. Configured
program actions launch only explicitly allowed programs with `shell=False`.
For SSH, map an action to Windows OpenSSH (`ssh.exe`) with a preconfigured
host/key and fixed argument list; the macro cannot supply or alter commands.
Allow port 8787 only on a trusted/private Windows network when prompted.

For example, an action can open a fixed web page:

```json
{
  "token": "the-generated-token",
  "actions": {
    "dashboard": {
      "program": "C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe",
      "arguments": ["http://192.168.1.50"]
    }
  }
}
```

For SSH, use the same structure with `program` set to
`C:\\Windows\\System32\\OpenSSH\\ssh.exe` and `arguments` set to a fixed key,
host, and remote command. Prefer key-based authentication and validate host
keys with the OpenSSH `known_hosts` file.

### Backup and firmware update

* **Backup to SD** writes `goopad-config.json` to the mounted card.
* **Restore from SD** replaces the saved configuration with that backup after
  confirmation, discarding unsaved editor changes. Images, sounds, Wi-Fi
  credentials and Apple Home pairing are not included; protect the backup
  because it may contain tokens.
* **Upload firmware** accepts an ESP-IDF application `.bin`.
* **Check for Update** asks GooPad to check the latest stable GitHub release.
  If a newer version is available, confirm to download and install its
  app-only image. GooPad needs internet access and an accurate clock for
  certificate verification. It checks the release's SHA-256 digest and image
  version before switching slots.
* On the **device's** info-panel Settings page, install `/goopad.bin` from
  the mounted SD card.
* **Change admin password** is available in website Settings. Use 8-64
  printable ASCII characters; the browser asks you to sign in again after
  saving.

The OTA partition layout supports rollback if a new image fails to boot.
Initial installation must flash the full merged image because it also installs
the OTA partition table. Later updates use an application-only image.

## Build and flash

Install ESP-IDF 6.1 and its tools, then build from PowerShell in this directory:

```powershell
& C:\esp\v6.1\esp-idf\export.ps1
idf.py -B C:\Temp\GooPad-idf-build build
```

For an existing configured device, use the app-only OTA image; do not casually
erase or full-flash its NVS or HomeKit storage. See the
[developer guide](docs/development.md#safe-update-first-flash-and-release)
for build configuration, backups, first installation and recovery. The board
is an ESP32-S3-WROOM-1U-N16R8: 16 MB flash, 8 MB octal
PSRAM, ST7701S RGB display, GT911 touch, and microSD slot.

Run Companion tests in an isolated Python environment:

```powershell
python -m venv build\companion-test-venv
.\build\companion-test-venv\Scripts\python.exe -m pip install -r companion\requirements.txt
.\build\companion-test-venv\Scripts\python.exe -m unittest discover -s tests -p test_companion.py
```

Isolation avoids globally installed `pip_system_certs` injecting client
certificate verification into the test HTTPS server. It does not change the
system Python installation. The desktop tray uses pystray and Pillow.
Build the Windows download with
`.\tools\make_companion.ps1`; PyInstaller must also be installed. The download
includes dependency licenses and pystray source for replacing/rebuilding that
LGPL library. The source GUI can be launched with
`python companion\GooPadCompanion.py --gui --hidden`.
For authenticated hardware regression checks (PowerShell 7), run
`.\tests\Test-GooPadDevice.ps1 -Device https://<device-ip> -Credential (Get-Credential admin)`.
This temporarily exercises nine grid sizes, five shapes, power clamps,
uploaded WAV playback and rapid sound taps, then restores configuration
and power settings and deletes the test sound even if a check fails.
Add `-NonMutating` to check HTTPS, authentication, icon catalog and rejection
of invalid tile settings without replacing the layout or changing power controls.
Add `-TileVisibilityOnly` instead to run the focused native empty-tile fixtures
with snapshot restoration and built-in emoji checks, without the broader audio/power/asset tests.
These checks require firmware 0.6.0 or later and refuse older schemas before
sending invalid-input tests that older firmware might otherwise accept.
They skip TLS certificate verification and are intended only for a trusted LAN;
they do not verify device identity.
Browser editor regressions are in [tests/test_editor.js](tests/test_editor.js).
Open the local [editor](main/index.html), load that script as a classic script,
and call `runGooPadEditorTests()` in its console. It uses synthetic layouts and
mocked saves, refuses device URLs, and checks page editing, drag-hover navigation,
widget controls, empty-slot hiding and drag/drop, shared website pairing, and
actual text bounds across all grids, shapes and text sizes.

To create a full-flash image and an app-only OTA image:

```powershell
.\tools\make_release.ps1 -BuildDir C:\Temp\GooPad-idf-build
```

The script reads flash parameters from the root `sdkconfig`, which must match
the selected build. Its `-Port` option writes the merged image and is intended
for fresh boards or explicitly planned recovery, not routine updates.

## Security notes

The editor and its API require HTTP Basic authentication using a random,
device-specific password. That password is displayed on the touchscreen.
HTTPS encrypts editor credentials and configuration in transit. The certificate
is self-signed rather than publicly trusted. Use a trusted Wi-Fi network; do
not port-forward it or use an untrusted public hotspot. Wi-Fi credentials,
the TLS private key, and the admin password are stored in NVS and are not
flash-encrypted by this development build.

Macros intentionally make requests to the URLs you configure. Treat editor
access as privileged and avoid targets that you would not trust the device to
contact. The Windows companion uses a separate random token and
accepts only validated PC steps or locally configured app actions. PowerShell
from a macro is accepted only after explicit local opt-in; treat that token as
full access to your Windows account when enabled.

Build the Windows ZIP on Windows with `python -m pip install pyinstaller`,
then `.\tools\make_companion.ps1`. The ZIP contains the self-contained app,
Python source, launcher, and this README. No signing certificate is included
in this development build; Windows may display an unsigned-app warning.

## Hardware and pins

| Part | Detail |
|---|---|
| MCU | ESP32-S3-WROOM-1U-N16R8, dual core, 240 MHz |
| Flash / PSRAM | 16 MB / 8 MB octal |
| Display | ST7701S, 480×480 RGB |
| Touch | GT911, I²C at `0x5D` |
| SD | microSD over SPI2 |
| Wi-Fi | 2.4 GHz station |

The RGB display uses two PSRAM frames and internal 10-line DMA bounce buffers.
VSYNC DMA restart prevents permanent scanout shifts after underruns; PSRAM
instruction/rodata execution keeps the cache available during NVS/OTA writes.
Rendering and HTTPS run on core 1, away from the core-0 scanout interrupt.

```text
LCD RGB: DE 18, VSYNC 17, HSYNC 16, PCLK 21
LCD data: B0-B4 4,5,6,7,15; G0-G5 8,20,3,46,9,10; R0-R4 11,12,13,14,0
LCD command: CS 39, SCLK 48, MOSI 47
Backlight: 38
Touch: SDA 19, SCL 45
microSD: CS 42, MISO 41, MOSI 47, SCLK 48
```

## Current scope

This initial GooPad firmware implements Wi-Fi onboarding, the local editor,
persisted HTTP/webhook macros, grid/layout/theme controls, SD config backup,
and file/GitHub/SD application OTA. SSH can be delegated to a predefined
Windows companion action; there is no SSH client on the ESP32 itself.
The ESP32 itself does not include an SSH client; SSH actions are delegated to
the allowlisted Windows companion. The source retains the original hardware
diagnostics files as reference, but GooPad no longer launches the test UI or
writes hardware-test files to the SD card.
