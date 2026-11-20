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
   password. Open `http://<device-ip>/` on a computer or phone on that Wi-Fi.
3. Sign in as `admin` with the password on the display. Create macros, choose
   a 2–4 column and row layout, colors, theme, and tile shape, then save.
4. Tap a configured tile on GooPad to run its action. Unconfigured tiles stay
   inactive, so you can save a layout before adding every action. Swipe between
   pages; the page dots show the current page.

The browser editor is served from the device and remains available at its
current DHCP address. Reserve an address for GooPad in your router if you want
a stable bookmark.

## Macros and customization

Each tile supports a name, color, icon/glyph, and a plain-language action.
**Open a link or trigger a service** needs only the complete link supplied by
that service. **Open a website on my Windows PC** uses the Companion app and
asks for its displayed address/token and the website; it opens only HTTP(S)
links in the PC's default browser. **Custom web request (Advanced)** retains
the optional `GET`/`POST`, target URL, JSON body, and token controls. Requests
from GooPad validate HTTPS certificates. The searchable icon picker includes 275+
symbols, and the icon field accepts other short icon codes or glyphs. Common
symbols map to the built-in LVGL icon font on the panel; unsupported glyphs
fall back to the button label's initial. The editor also bundles 2,163
searchable Font Awesome Free solid, regular, and brand icons. Font Awesome
selections and uploaded images are converted to 32×32 RGB565 and saved under
`/goopad` on the SD card; a card is required to use them. The **Settings** page
contains device themes, background color, website appearance, backups, firmware
updates, and admin-password changes. The device's information panel also has
theme and SD firmware-update controls. The display has Midnight, Ocean, Forest,
and Light accents, custom background colors, and rounded, square, or circular
tile corners. Up to 32 macros are stored locally; grid size controls how many
appear on each page.

The bundled Font Awesome icon data is version 7.3.1; attribution and license
details are in [third_party/fontawesome/NOTICE.txt](./third_party/fontawesome/NOTICE.txt).
To regenerate the catalog from the official npm packages, unpack the matching
`free-solid-svg-icons`, `free-regular-svg-icons`, and `free-brands-svg-icons`
packages into one directory and run
`python tools/generate_fa_catalog.py <packages-directory> main/fa-icons.json`.

### Windows companion

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
The device currently serves plain HTTP, so use it only on a trusted Wi-Fi
network; do not port-forward it or use an untrusted public hotspot. Wi-Fi
credentials and the admin password are stored in NVS and are not flash-
encrypted by this development build.

Macros intentionally make requests to the URLs you configure. Treat editor
access as privileged and avoid targets that you would not trust the device to
contact. The Windows companion uses a separate random token and does not
accept arbitrary commands from request bodies.

## Hardware and pins

| Part | Detail |
|---|---|
| MCU | ESP32-S3-WROOM-1U-N16R8, dual core, 240 MHz |
| Flash / PSRAM | 16 MB / 8 MB octal |
| Display | ST7701S, 480×480 RGB |
| Touch | GT911, I²C at `0x5D` |
| SD | microSD over SPI2 |
| Wi-Fi | 2.4 GHz station |

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
