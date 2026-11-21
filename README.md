# GooPad

GooPad turns the Guition / CYD **ESP32-S3-4848S040** (4-inch, 480×480)
touch display into a Wi-Fi macro pad. It is built with ESP-IDF 6.1 and LVGL
9; the existing display, GT911 touch, SD-card, and board drivers are retained.

![GooPad logo](web/logo.svg)

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

## Macros and customization

Each tile supports an optional label, color, visual, and a plain-language action.
Choose **Icon / emoji**, **Font Awesome**, **Image**, or **Animated GIF** from its visual-type
dropdown. Blank labels stay blank; the visual is enlarged and centered.
The live preview is the editor's centerpiece: select any tile, including an
empty slot, to edit it. Only that tile's controls are shown. You can configure
positions in any order. **Tile name** names a tile in the editor without adding
text to its visual; **Button label** remains optional on-screen text.
**Clear tile** leaves other positions untouched. Drag a preview tile onto an
empty position to move it, or onto another tile to swap them. Hold a dragged
tile near the preview's left/right edge to change pages. Mouse swiping starts
in the space between tiles; touch scrolling remains available.
**Add page** reveals more positions, up to 32 total.
**Save to device** is below the live preview.
Save progress, confirmation, and errors appear only there. **Save appearance**
returns to the preview so its result is visible. **About & help** includes
downloads, setup instructions, examples, troubleshooting, and source links.
**Open a link or trigger a service** needs only the complete link supplied by
that service. **Open a website on my Windows PC** uses the Companion app and
asks for its displayed address/token and the website; it opens only HTTP(S)
links in the PC's default browser. **Custom web request (Advanced)** retains
the optional `GET`/`POST`, target URL, JSON body, and token controls. Requests
from GooPad validate HTTPS certificates. The searchable icon picker includes 275+
symbols, and the icon field accepts other short icon codes or glyphs. Common
symbols map to the built-in LVGL icon font on the panel; unsupported glyphs
fall back to the button label's initial. The editor also bundles 2,163
searchable Font Awesome Free solid, regular, and brand icons. Enter Font
Awesome codes such as `fa-server` or `fa-brands fa-github`, or search the
bundled catalog. The picker first downloads a small compressed name index and
only fetches matching drawings. Selection progress stays inside the picker;
direct-code progress uses the main page. Repeated selections reuse uploaded
assets, and lossless run-length compression reduces icon-upload traffic.
Font Awesome selections and uploaded images are converted to
160×160 RGB565 with an 8-bit alpha plane and saved under
`/goopad` on the SD card; a card is required to use them. The **Settings** page
contains preset themes plus Custom background controls, website appearance, backups, firmware
updates, and admin-password changes. The device's information panel also has
theme, display-brightness, sound-volume, and SD firmware-update controls. Brightness
and volume changes take effect immediately; **Save settings** persists them.
The display has Midnight, Ocean, Forest,
and Light palettes, custom background colors, and per-macro font colors and
rounded, square, circle, pill, or soft-corner tile shapes. Up to 32 macros are stored locally;
grid size controls how many appear on each page. Each macro can choose an
outline color or remove its outline. Preset themes always use their own
background; choose **Custom** for your own background. Custom also accepts an
uploaded image (up to 15 MB input, SD required). The browser center-crops it to
480x480 RGB565 and fills transparent pixels with the selected background color.
The device loads the 460,800-byte image into PSRAM. Preset themes ignore the
stored image; switching back to Custom restores it. Removing it from the layout
requires saving. Images are stored separately on SD, so a config-only backup
does not include the assets.

Choose **Live widget** for a live clock, date, or **PC stats**.
Widgets are display-only on the device; select them in the editor to configure
them. Time format offers 12/24-hour displays, with optional seconds; the default
remains 24-hour without seconds. Date format offers year-month-day, month/day/year,
day/month/year, and short month/day; the default remains year-month-day. Date preview text
scales to the tile's dimensions. Clock/date use network time or time
synced when the website opens; Settings includes a fixed UTC offset, initially
matched to the browser. Update that offset after daylight-saving changes.
PC stats use the paired Companion. Choose CPU/RAM combined, CPU usage,
RAM percentage or used GiB, GPU usage, dedicated GPU RAM used, CPU temperature,
or GPU temperature per tile. Temperature tiles support Celsius/Fahrenheit.
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
open multiple HTTPS connections.
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

### Windows companion

Download **GooPadCompanion-Windows.zip** from the site's **About & help** tab or
[the latest release](https://github.com/jeromedontdev/GooPad/releases/latest).
Extract it and run `GooPadCompanion.exe` as a normal Windows user. No Python
installation is needed for that app. Closing quits and minimizing uses the
taskbar by default. You can independently change either button to **Hide in tray**;
the receiver keeps running while hidden. Its two Copy buttons provide the PC address and a private
pairing token. Paste these under website **Settings > Windows companion**, check
the connection, then save. The last-result check also shows errors and whether
PowerShell is enabled. Allow TCP port 8787 only on a trusted Private network.

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

The device talks to Companion over token-authenticated **HTTP on the LAN**;
unlike the editor link, this link is not encrypted. Never port-forward it or
use an untrusted Wi-Fi network.

`companion\Start-GooPadCompanion.cmd` starts a standard-library Python
webhook receiver on the Windows PC. On first run it creates
`%LOCALAPPDATA%\GooPad\companion.json` with a random shared token. Add an
action entry that maps a short ID to a program and argument list, then set a
GooPad macro URL to `http://<pc-ip>:8787/hook/<id>` and copy the token into
that macro's companion-token field. For the simple **Open a website on my
Windows PC** action, use the displayed Companion address and token; no JSON
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
* **Upload firmware** accepts an ESP-IDF application `.bin`.
* **GitHub release** downloads the latest GooPad release's `GooPad-ota-*.bin`
  asset in the browser and uploads it to the device.
* **Install SD image** installs `/goopad.bin` from the mounted card. The same
  app-only image can be installed from the device's info-panel Settings page.
* **Change admin password** is available in website Settings. Use 8-64
  printable ASCII characters; the browser asks you to sign in again after
  saving.

The OTA partition layout supports rollback if a new image fails to boot.
Initial installation must flash the full merged image because it also installs
the OTA partition table. Later updates use an application-only image.

## Build and flash

Install ESP-IDF 6.1 and its tools, then from PowerShell in this directory:

```powershell
& C:\esp\v6.1\esp-idf\export.bat
idf.py set-target esp32s3
idf.py build
idf.py -p COM9 flash monitor
```

Use the USB-C/CH340 serial port shown by Windows (COM9 on the development
machine). The board is an ESP32-S3-WROOM-1U-N16R8: 16 MB flash, 8 MB octal
PSRAM, ST7701S RGB display, GT911 touch, and microSD slot.

Run the existing Companion tests with `python -m unittest tests.test_companion`.
The console receiver and Windows startup use the standard library.
The desktop tray uses pystray and Pillow; install source-GUI dependencies with
`python -m pip install -r companion\requirements.txt`.
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

To create a full-flash image and an app-only OTA image:

```powershell
.\tools\make_release.ps1
```

To also flash the merged image:

```powershell
.\tools\make_release.ps1 -Port COM4
```

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
