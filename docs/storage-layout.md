# Storage layout and application-space ledger

## What the 31 KiB warning means

The tested Guition has **16 MiB flash**, but each firmware image must fit in
one **4 MiB OTA slot**. The installed Unicode development build uses
**4,162,208 bytes**, leaving **32,096 bytes (31.34 KiB, 0.77%)** in its slot.
This is not a warning that the whole flash chip, configuration store or SD
card is full.

Measured on 2026-11-24 from the final ESP-IDF 6.1 HomeKit build's `GooPad.bin`
and `GooPad.map`. The preceding deployment verified the active image
descriptor/ELF digest and unchanged configuration. This is a development
worktree build, not a firmware release matching a clean source commit.
Binary SHA-256:

```text
cca8dceb46a23605a490e75027f0d2af52d372dad5004226b30b27806a0c60e1
```

KiB means 1,024 bytes; MiB means 1,048,576 bytes. All percentages below use the
4,194,304-byte app slot, not the whole flash chip.

## Physical flash layout

The [partition table](../partitions.csv) and deployment's verified live table
agree. Ranges are start-inclusive, end-exclusive. Reserved ranges are not
measurements of occupied bytes.

| Region | Start | End | Reserved capacity | Purpose |
| --- | --- | --- | --- | --- |
| Bootloader region | `0x000000` | `0x008000` | 32 KiB | Bootloader starts at `0x0`; the region includes any unused padding. |
| Partition table sector | `0x008000` | `0x009000` | 4 KiB | Flash layout metadata. |
| `nvs` | `0x009000` | `0x00F000` | 24 KiB | Wi-Fi/admin/TLS identity, settings, retained legacy config and migration marker. |
| `phy_init` | `0x00F000` | `0x010000` | 4 KiB | Radio initialization data. |
| `otadata` | `0x010000` | `0x012000` | 8 KiB | OTA selection/validation metadata. |
| Alignment gap | `0x012000` | `0x020000` | 56 KiB | Outside any declared partition; not an asset store. |
| `ota_0` | `0x020000` | `0x420000` | 4 MiB | One complete firmware image. |
| `ota_1` | `0x420000` | `0x820000` | 4 MiB | The other complete image for safe OTA/recovery. |
| `storage` | `0x820000` | `0x1000000` | 7.875 MiB | Legacy SPIFFS partition type, **not a mounted SPIFFS filesystem**. See below. |

The two app slots are alternatives, not an 8 MiB combined application.
An update writes the inactive slot; the other slot provides recovery.
The second slot's actual contents/length may differ from the installed image.
Do not add both slot capacities to the measured application's size.

### The misleadingly named `storage` partition

[goopad_homekit_storage.c](../main/goopad_homekit_storage.c) owns its tail:

| Subregion | Start | End | Capacity | Current use |
| --- | --- | --- | --- | --- |
| Unassigned prefix | `0x820000` | `0xFEF000` | 8,187,904 bytes / 7.81 MiB | Must remain erased under the current ownership checks. |
| Ownership header | `0xFEF000` | `0xFF0000` | 4 KiB | Claim/ready state and signature. |
| Shared NVS block device | `0xFF0000` | `0x1000000` | 64 KiB | Current configuration and HomeKit persistence/metadata. |

The 64 KiB is **shared capacity**, not 64 KiB of usable JSON. NVS page/record
overhead and HomeKit records consume space. Actual remaining NVS entries
depend on saved data; this report does not inspect or publish private records.
The current application also limits configuration JSON to **12,000 UTF-8
bytes**, independently of store capacity; see the validation limit in
[goopad.c](../main/goopad.c).

The current guard checks for unrelated data outside the reserved tail and
fails closed. The large unassigned prefix is therefore not a drop-in asset
store. Reusing it requires an explicit ownership/layout migration with safe
upgrade/downgrade handling. Never format it, overwrite it, resize app slots or
erase pairing/configuration merely to increase headroom.

## What occupies the installed app slot

These are **linked, initialized section contributions**, not source-file
lengths or runtime heap allocations. BSS/no-init/reserved RTC memory are
excluded because they do not occupy image payload. Asset rows include their
linked object overhead. Shared constants and unattributed bytes are separated
so the ledger reconciles exactly with the actual binary length.

| Category | Image bytes | KiB | Slot % |
| --- | ---: | ---: | ---: |
| SDK/libraries, excluding fonts and the shared string pool | 1,705,914 | 1,665.93 | 40.67 |
| Font Awesome catalog and search index | 859,770 | 839.62 | 20.50 |
| New Unicode fallback fonts: ten sizes | 686,561 | 670.47 | 16.37 |
| Styled tile fonts: 15 families, four sizes each | 312,963 | 305.63 | 7.46 |
| Built-in Montserrat: eight sizes | 187,473 | 183.08 | 4.47 |
| Shared merged string pool | 167,537 | 163.61 | 3.99 |
| Other GooPad application objects, excluding pooled strings | 114,607 | 111.92 | 2.73 |
| Browser WOFF2 fonts: nine files | 55,592 | 54.29 | 1.33 |
| Hosted editor: gzip HTML | 52,657 | 51.42 | 1.26 |
| Embedded font notices | 5,765 | 5.63 | 0.14 |
| Unattributed linker bytes plus image envelope/padding | 13,369 | 13.06 | 0.32 |
| **Installed image** | **4,162,208** | **4,064.66** | **99.23** |
| **Remaining in one app slot** | **32,096** | **31.34** | **0.77** |
| **App-slot capacity** | **4,194,304** | **4,096.00** | **100.00** |

**Device fonts plus the icon catalog occupy 2,046,767 bytes, about 49% of the
slot.** These are bundled features, not copies of uploaded user images.

The compiler/linker merges strings across objects. In this map, the
167,537-byte `.rodata.esp_stdio_register.str1.4` pool is attributed to
`stdio_vfs.c.obj` despite its pre-relaxation size being only 126 bytes.
It must not be interpreted as 164 KiB of console-specific strings or as an
accurate per-feature allocation. Other GooPad/SDK strings can reside there.
The remaining object attribution also does not predict exact savings from
disabling a feature, because dependencies, relaxation and alignment change.

The SDK bucket includes LVGL rendering/layout, Wi-Fi/radio, lwIP, TLS,
cryptography, HomeKit, mDNS, HTTP, storage drivers and the operating runtime.
Examples of **subsets already counted above**, not additional allocations:

- LVGL excluding its Montserrat objects: 349,158 bytes.
- Radio `net80211`/`pp`/`phy` and WPA libraries: 343,998 bytes; Wi-Fi wrappers
  and other network components are additional parts of the SDK bucket.
- Three linked TLS/PSA/mbedTLS archives: 247,112 bytes, including the public
  certificate bundle. libsodium is another 103,032 bytes.
- lwIP: 101,213 bytes; HomeKit core: 31,095 bytes. Their shared dependencies
  are counted in the other libraries, not attributed exclusively to HomeKit.

### Why the last update consumed almost all the remaining room

The prior punctuation build was `0x3515d0` (3,478,992 bytes). The Unicode build
is `0x3f82a0` (4,162,208 bytes): **683,216 bytes / 667.20 KiB net growth**.
The new fallbacks alone contribute 686,561 bytes, replacing much smaller
punctuation-only assets; minor validation/build differences also affect the
net delta. Adding extended glyphs at every selected size is the dominant
reason headroom fell from approximately 17% to less than 1%.

| Unicode fallback size | Linked image bytes |
| --- | ---: |
| 8 px | 24,843 |
| 10 px | 28,041 |
| 12 px | 31,026 |
| 14 px | 36,104 |
| 16 px | 41,064 |
| 20 px | 54,473 |
| 24 px | 70,011 |
| 28 px | 94,003 |
| 36 px | 138,439 |
| 40 px | 168,557 |
| **Total** | **686,561** |

## SD assets, configuration and build files are different budgets

- User icons, backgrounds, GIFs and audio live on SD under `/goopad/`.
  Their file bytes do **not** grow the app image. Displaying them can consume
  PSRAM. Exact SD used/free capacity has not been remeasured for this report.
- Configuration stores SD asset references, action settings and shared
  Companion data in the persistent store, not in the firmware image.
  `/goopad-config.json` is an explicit config-only SD backup; it can contain
  secrets and does not back up identities, pairing or all referenced assets.
- The bundled Font Awesome search/catalog remains compiled into the app.
  [main/CMakeLists.txt](../main/CMakeLists.txt) embeds it, the compressed
  editor, web fonts and notices, and compiles the device font C files.
- The source TTF, generated C text, raw icon JSON, ELF debug information and
  linker map are host-side build/source files. They are **not copied wholesale
  to the device**. For example, the 24 MB ELF is not a 24 MB firmware image.
- Internal RAM and 8 MiB PSRAM are volatile working memory, not app storage.

### Correction: flash-backed data still consumes PSRAM in this build

The selected SDK config has both `CONFIG_SPIRAM_FETCH_INSTRUCTIONS=y` and
`CONFIG_SPIRAM_RODATA=y`. ESP-IDF copies flash instructions and read-only data
into PSRAM at startup for execution/access during flash operations.

Consequently, the font tables and embedded catalog **occupy flash and also
contribute to the boot-time PSRAM reservation**. There is no separate
application-level allocation to load each complete font, but that does not
make their PSRAM cost zero. The earlier assertion that the expanded fonts
were not loaded into PSRAM was incorrect for this configuration.

The size summary reports 1,480,312 bytes of flash code and 2,552,980 bytes of
flash data, roughly 3.85 MiB combined before MMU/page alignment. This is not
an exact measured PSRAM reservation: startup mapping/alignment and section
selection matter. Frame buffers, cached pages, GIFs, TLS and heap allocations
use further memory. The most recent five-minute soak saw approximately
1.83-1.92 million bytes free PSRAM. That is an observed free-heap range, not
the physical capacity or a per-feature allocation measurement.

## Sensible space-recovery targets

These are candidates, **not implemented changes or guaranteed savings**:

1. Offload the bundled icon catalog/index to SD with an explicit absent-card
   behavior. Its current linked budget is about 840 KiB.
2. Keep a small essential Unicode fallback in flash and put extended font
   packs on SD, loaded on demand. Preserve usable text without SD and measure
   glyph latency, PSRAM, cache behavior and corruption handling.
3. Offer fewer optional font families/sizes as an intentional UI/build
   choice. Merely substituting a different-sized fallback changes appearance.
4. Web assets are already compressed and are comparatively small. Removing
   those first will not recover as much space as the catalog/fonts.

Changing PSRAM execution options may recover RAM but **does not remove the
stored flash image**, and can affect display/flash-operation performance.
Do not turn off TLS verification, pairing protection or OTA recovery to save
space. Enlarging slots needs a separately reviewed migration, not an app-only
OTA shortcut.

## Reproducing and maintaining this ledger

Use the installed ESP-IDF Python environment and the map/binary from the
**same successful build**. `$build` is your verified production build directory:

```powershell
python -m esp_idf_size --no-color "$build\GooPad.map"
python -m esp_idf_size --format json2 --files --output-file "$build\size-files.json" "$build\GooPad.map"
python -m esp_idf_size --format json2 --archives --output-file "$build\size-archives.json" "$build\GooPad.map"
(Get-Item -LiteralPath "$build\GooPad.bin").Length
Get-FileHash -Algorithm SHA256 -LiteralPath "$build\GooPad.bin"
```

For each object, sum its `memory_types.*.sections.*.size`, excluding sections
whose `abbrev_name` is `.bss`, `.rtc_noinit` or `.rtc_reserved`. Group Unicode,
other device fonts, Montserrat and embedded assets by object name, then other
`libmain.a` objects and remaining libraries. Inspect merged-string attribution
in the map before labelling library totals as feature costs.

For this snapshot, object payload attribution sums to 4,148,839 bytes;
the size tool's estimated image is 4,162,098 bytes. Keep the 13,259-byte
unattributed linker difference and 110-byte binary-vs-estimate difference
explicit rather than forcing them into a feature bucket. The binary length,
not ELF length or linker address-window capacity, determines app-slot fit.

Update this dated ledger after material asset/layout changes. Verify the live
partition table before writes; never publish credentials, raw configuration,
flash dumps or private serial captures as size-report evidence. Documentation
updates alone need no device flash or soak.
