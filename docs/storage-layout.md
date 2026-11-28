# Storage layout and application-space ledger

## 0.7.12 editor development build (2026-11-28)

The control redesign build is **2,425,328 bytes**, leaving **1,768,976 bytes**
in its 4 MiB app slot. The production ESP-IDF 6.1 build used the existing
dedicated SDK configuration; its binary SHA-256 is
`6e453986133d9a19bc6ff3cb7abdeed927aabae9bfdc7632dd50f5c64ea729cb`.
Its matching editor pack is `web-6197e40dc00c.gwp` (343,415 bytes,
SHA-256 `6197e40dc00c875c4e8bb2752382feb1205c908b99c9c7a37412d1f5111b443e`).
This worktree build was installed app-only on COM9 with its matching editor;
0.7.11 remains in the recovery slot. Configuration, primary NVS and protected
HomeKit records were verified. The five-minute soak reported a widget timeout;
see the [change log](change-log.md). It was not released.

## 0.7.13 controls development build (2026-11-28)

The further controls build is **2,425,328 bytes**, leaving **1,768,976 bytes**
in its 4 MiB app slot. The ESP-IDF 6.1 production worktree build SHA-256 is
`ed6b97d099d91faab74be6a3ba6003571342afe882de205233dbdcb75213ccad`.
Its matching editor pack is `web-b762a640bc27.gwp` (346,250 bytes,
SHA-256 `b762a640bc27dd8887747ce6b22d7569b8a30a4dda9bfaa65347425a50bdf50a`).
This build is not installed or released; app-slot headroom is not a measurement
of runtime memory.

## 0.7.13 committed-source build (2026-11-28)

The ESP-IDF 6.1 production build from `316d6a0` is **2,425,328 bytes**, leaving
**1,768,976 bytes** in the 4 MiB app slot. Its SHA-256 is
`77594e02446f6f394a16c406da07abe2c3e37b2d1b71382c181e61a883b5fe6e`.
The matching editor pack is `web-9cee602ba7fc.gwp` (346,373 bytes, SHA-256
`9cee602ba7fc2dab857007891d7933e8437c6cdc87853d9fc2d7c76f6c35b014`).
App-slot headroom is not a measurement of runtime memory. See the
[change log](change-log.md) for deployment and validation status.

## 0.7.14 development build (2026-11-28)

The ESP-IDF 6.1 production worktree build is **2,426,480 bytes**, leaving
**1,767,824 bytes** in the 4 MiB app slot. Its SHA-256 is
`4cf4119cc8fb59f00da34b3b2459ce7e379f6c376101f009867aba0b9fb69140`.
The matching editor pack is `web-d0ea1b93c60a.gwp` (350,510 bytes, SHA-256
`d0ea1b93c60ab29cc8ea1bfaa124e3bb5718a7ae64d679557013563c692f6e92`).
App-slot headroom is not runtime memory. See the [change log](change-log.md)
for deployment and validation status.

The subsequent production reconfigure/build from committed source `94c9346`
has the same size, headroom and matching editor pack. Its firmware SHA-256 is
`8e0d5256d7481761a132fcda6ffd416c0442d7cc2111a5ed3d8288225594489a`;
this is the image installed on the device, not the earlier worktree image.

## 0.7.15 committed-source build (2026-11-28)

The ESP-IDF 6.1 production build from `ed48be2` is **2,427,328 bytes**, leaving
**1,766,976 bytes** in its 4 MiB app slot. Its SHA-256 is
`9262f4c40e91abaabc193476a3ce724f4e5048c19c1a6ef8b9a21d1f08f44886`.
The matching editor pack is `web-571557f77ce5.gwp` (350,505 bytes, SHA-256
`571557f77ce52d8ceaec99af69c628e5c0f112d903ceb5ce5c994c93b87ae4d7`).
The image was installed app-only on COM9 with 0.7.14 in the recovery slot.
App-slot headroom is not runtime memory; see the [change log](change-log.md)
for preservation, device soak and publication status.

## What the 31 KiB warning means

The tested Guition has **16 MiB flash**, but each firmware image must fit in
one **4 MiB OTA slot**. The preceding full-Lato Unicode development build used
**4,162,208 bytes**, leaving **32,096 bytes (31.34 KiB, 0.77%)** in its slot.
This is not a warning that the whole flash chip, configuration store or SD
card is full.

That historical snapshot was measured on 2026-11-24 from the ESP-IDF 6.1 HomeKit build's `GooPad.bin`
and `GooPad.map`. The preceding deployment verified the active image
descriptor/ELF digest and unchanged configuration. This is a development
worktree build, not a firmware release matching a clean source commit.
Binary SHA-256:

```text
cca8dceb46a23605a490e75027f0d2af52d372dad5004226b30b27806a0c60e1
```

KiB means 1,024 bytes; MiB means 1,048,576 bytes. All percentages below use the
4,194,304-byte app slot, not the whole flash chip.

## Published 0.7.6 build

The clean-source 0.7.6 build installed on COM9 is **2,318,928 bytes**, leaving
**1,875,376 bytes** in its app slot. Its SHA-256 is
`1af7df81a9baa9d06ed8dfc9cc814e7131be38a99b491e83e082fba6ae5bd65a`.
The active slot was compared with this known, SHA-256-verified image using
device-side flash MD5; the preceding clean-source image remains in the
recovery slot. That image was also **2,318,928 bytes**, with SHA-256
`0b8d4ebdc77c932bc6c2c7a8b0bcb57e0001bc8bbb0d27770e2e379e4ae11b2f`.
The preceding image passed a five-minute soak; the final label-only build was
not soaked again at the user's request. This image is published in
[v0.7.6](https://github.com/jeromedontdev/GooPad/releases/tag/v0.7.6).
See the [change log](change-log.md) for deployment and validation status.

## 0.7.7 development build (2026-11-27)

The tile/widget additions build is **2,326,784 bytes**, leaving
**1,867,520 bytes / 44.53% free** in the unchanged 4 MiB app slot.
SHA-256: `50dfd9a638036a6bb910214224eebac7861da611175b3a9fa819f2190a1c3425`.
ESP-IDF 6.1 `reconfigure build` used a fresh SDK configuration seeded from
`sdkconfig.defaults`, with HomeKit, NVS block device, PSRAM-first custom LVGL
allocation, compressed fonts and TLS session/performance options enabled;
hardware AES and render/capacity diagnostics are off. The matching SD editor
archive is `web-25b659fcb2c2.gwp` (315,446 bytes).

This measures the development worktree, not a released clean-source artifact.
An earlier local-config build accidentally used the C-library LVGL allocator;
the final measurement above is the corrected fresh-defaults build.
The subsequent COM9 application-only installation verified the live partition
table, active/recovery images, pinned HTTPS and preserved configuration/pairing,
and installed the matching editor archive. The image is active in `ota_0`,
with the preceding 0.7.6 image retained in `ota_1`. A five-minute read-only
soak completed with zero errors/reboots and background-deferral warnings.
See the [change log](change-log.md) for warning assessment, measured runtime
memory and physical-validation limits. No binary release is published.
Time-format storage and response/headline caches add runtime memory use;
app-slot headroom is not a measured free-PSRAM budget.

## 0.7.8 display/RSS development build (2026-11-27)

The corrected thumbnails, centered stacked Arches, Material containers, frosted
Glass and RSS/RDF/Atom build is **2,419,600 bytes**, leaving **1,774,704 bytes /
42.31% free** in the unchanged 4 MiB slot. SHA-256:
`173bf33e31eef903ef2ccf0e6d1f762003e1409f752382fbdb605259043880fc`.
The matching editor is `web-bb90c1f9f5e9.gwp`, **322,258 bytes**, SHA-256
`bb90c1f9f5e99aad393b30b400530f89ad56fc54afbfabbf9566943bb451d18b`.
Production ESP-IDF 6.1 settings retain the custom LVGL allocator, compressed
fonts, HomeKit/NVS block device and TLS performance/session options; hardware
AES and render/capacity diagnostics remain disabled. The user's root local
SDK configuration is unchanged.

Glass adds one shared **28,800-byte PSRAM** backdrop, not one buffer per tile.
Custom-background blurring temporarily adds another 28,800 bytes. RSS/Atom
responses can use 256 KiB plus Expat's PSRAM working allocations; legacy JSON
retains its 64 KiB cap. These are runtime allocations, not app-slot costs.
Installation/soak status is recorded in the change log; this development build
is not a published binary release.

Subsequent COM9 pinned application-only deployment verified this image active
in `ota_1`, with the exact 0.7.7 image retained in `ota_0`. Source commit
`2f456463da925b95375cb87699e607482747f674` is pushed to the product remote;
the matching editor is installed. Saved configuration, primary NVS, TLS
identity and HomeKit pairing were preserved.

The 300.183-second read-only soak collected 21 samples with zero errors or
reboots and 25 assessed startup/background-deferral warnings. Internal free
memory rose from 19,535 bytes at startup to 38,071 bytes at completion.
Largest internal block was at least 8,192 bytes (lower than the previous
15,360-byte soak minimum, but above the 4,096-byte network-admission floor).
After the initial PSRAM drop from 4,506,376 to 3,411,528 bytes, samples stayed
between 3,375,844 and 3,436,772 bytes, ending at 3,408,388 bytes without a
sustained decline. These measurements cover the preserved configuration,
not every new RSS/Glass layout. See the [change log](change-log.md) for warning
counts and remaining physical-validation/release-packaging limits.

## 0.7.9 rendering/editor development build (2026-11-27)

The Circles/Arches, GIF layering, multi-stop gradients, font emphasis and
selectable-headline build is **2,425,328 bytes**, leaving **1,768,976 bytes /
42.18% free** in the unchanged 4 MiB slot. SHA-256:
`a242af7b193e65b197462b5da311dcd23e37a67bb6bbf076cbcf49870e689aca`.
The matching editor is `web-755d62f9147c.gwp`, **331,501 bytes**, SHA-256
`755d62f9147c5e0859dbd2482c159fc92bd303790dcb4b63dcd092cb0a087c25`.
Production ESP-IDF 6.1 reconfiguration/build retains the preceding settings,
with six LVGL gradient stops and complex gradients enabled. Compile-time
guards reject existing configurations without that capacity/support.

The live-text cache grows from 384 to 640 bytes per position: **8,192 additional
PSRAM bytes** across 32 positions. Gradient descriptors are allocated together
in PSRAM only when needed; emphasized-font wrappers are cached by base font and
style in PSRAM. They remain alive until their LVGL objects are deleted.
Glyph transformation uses the caller's enlarged A8 buffer in place; raw bitmap
requests allocate a temporary owned draw buffer released with the glyph.
These are runtime costs, not storage or app-slot headroom. No SD font pack
changes are required. Installation/soak and publication status are recorded
in the change log; this is not a published binary release.

Subsequent certificate-pinned application-only COM9 deployment verified
0.7.9 active in `ota_0`, with exact 0.7.8 retained in `ota_1` and the matching
editor installed. Source commit
`a831845fddc9593b2d346323852e336e5f552c30` is pushed to the product remote.
Configuration, primary NVS, TLS identity and HomeKit pairing were preserved.
The 300.139-second read-only soak collected 24 samples with zero errors/reboots
and 34 assessed startup/background-deferral warnings. Internal free memory
rose from 17,379 to 37,915 bytes; the largest block rose from 7,680 to 15,360.
After 30 seconds, PSRAM ranged 3,380,052-3,425,204 bytes, without a sustained
decline; the final sample was 3,380,052 bytes. These are bounds on the preserved
configuration, not a measured budget for every new gradient/emphasis layout.

## 0.7.10 wording development build (2026-11-28)

The editor and touchscreen wording build is **2,425,328 bytes**, leaving
**1,768,976 bytes / 42.18% free** in a 4 MiB app slot. SHA-256:
`5a1a190fcf348362d713e63e8f08a7b1c14c33564507a177d53b17216b24467d`.
The matching editor `web-638980b80cea.gwp` is **329,963 bytes**, SHA-256
`638980b80cea68c99af7eb3c768f9bf1d5e88f210794305883f9bb4cbd6e758b`.
The production ESP-IDF 6.1 build used the preceding HomeKit, PSRAM, TLS
and gradient configuration. It is installed in `ota_1` on the verified COM9
device, with the exact preceding 0.7.9 app retained in `ota_0` as recovery.
See the change log for verification and remaining release work. These are
development artifacts, not a published binary release.

## 0.7.11 editor workspace development build (2026-11-28)

The production ESP-IDF 6.1 app is **2,425,328 bytes**, leaving **1,768,976
bytes / 42.18% free** in a 4 MiB app slot. SHA-256:
`d30cd7827caddc36bdaa63e995c78954e4f3e8469694b75ba13f3bccbc590366`.
Its matching `web-d149ada81c45.gwp` editor archive is **336,765 bytes**,
SHA-256 `d149ada81c45c4ab98af2bc8330e1c3072f503222b805df507c2faca9bb82fe9`.
The app and editor were installed on COM9 with 0.7.10 retained for recovery,
but a later primary NVS comparison reported a difference and the verification
was stopped. See the change log before any further device operation; no
versioned binary release is published.

## Companion-only 0.7.4 release

The [0.7.4 release](https://github.com/jeromedontdev/GooPad/releases/tag/v0.7.4)
includes a matching version-only firmware build from clean source
`bda4ca7f1ed28e5a75c6ab44cb8e240c8358e5c0`: **2,316,320 bytes**, still
**1,877,984 bytes / 44.77% free**. Its SHA-256 is
`fecbc0173cbf34650a31e5d4aa6b905f473fdea5a93bf4dbe42dc1ff4eca15a6`.
Resolved SDK settings, embedded behavior, actual SD resources and PSRAM
budgets are unchanged. This repairs the Windows Companion; the hardware was
not reflashed and its verified active image remains the 0.7.3 image below.

## Complete 0.7.3 release build

The clean production build from source commit
`34fd9f80e609505403d5127b659fdcf2674cc70e` is **2,316,320 bytes**, leaving
**1,877,984 bytes / 44.77% free** in its unchanged 4 MiB OTA slot. SHA-256:

```text
c8a52ecde58835a373dbd6dc392b29ad30662f56917f1b36ca4f79e945168207
```

It was built in a separate clean checkout with a fresh configuration seeded
by the committed defaults and component lockfile, using ESP-IDF 6.1 commit
`fff9895c82d744c7237be8847347bdd1b07c6643`. Production render/capacity
diagnostics are OFF. The resolved configuration matches the preceding
verified production configuration. The release includes the matching SD
resources, dependency/font notices and build-input/checksum records; these
record provenance, not a promise of bit-identical output across toolchains,
build paths, platforms or timestamps.

The [published 0.7.3 release](https://github.com/jeromedontdev/GooPad/releases/tag/v0.7.3)
matches the verified active app-only installation. The final five-minute
functional soak observed 56/56 available PC snapshots and no errors/reboots.
Both soak reports are retained: the initial run caught one recovered PC
polling timeout, while the repeat observed none. See the change log for the
remaining network-wait warnings and measured runtime heap minima.

## Previous Companion-reliability development build

The preceding installed production-config development image was **2,316,192 bytes**,
leaving **1,878,112 bytes / 44.78% free** in its unchanged 4 MiB app slot.
It is **480 bytes larger** than the preceding font-repair image. SHA-256:

```text
b2c814a7f207e8639955353e2fb7648b2bab83b7cf936ff982b7bdd6a6d1e89f
```

The change is in Companion/background scheduling, clock gating, TLS client
configuration and numeric failure diagnostics, not resource sizes or glyph
cache budgets. The monitor no longer saves TLS sessions on the SDK 6.1
asynchronous path. Its request budget is 8 seconds, while the 500 ms polling
target, short socket waits and manual-action preemption remain.
Admitted widgets are no longer repeatedly cancelled by pending media polls.
An eligible widget queues behind the active request with a bounded 10-second
admission wait instead of racing each subsequent media poll.
No new target heap allocation was added. Configuration, authentication,
logical HomeKit pairing and SD resources were verified preserved by app-only
installation. Automatic render diagnostics remain OFF.

The object-attribution table and font performance/soak figures below describe
the preceding **font-repair** image, not new Companion reliability results.
See the [change log](change-log.md) for connection validation and publication
scope. That image included earlier uncommitted integration and was not
published as a binary reproduced from a scoped source commit. The complete
0.7.3 integration above resolves that earlier source-provenance blocker.

## Previous repaired SD-backed editor and styled-font build

The preceding production-config development image was **2,315,712 bytes**:
**1,878,592 bytes / 1.79 MiB / 44.79% free** in the unchanged 4 MiB slot.
It retains **407,584 bytes / 398.03 KiB** of the SD offloading savings versus
the preceding multilingual build. SHA-256:

```text
13fae5b6ea9593af1728ef94f3a095ba9309116fb829ed0b741a01dcd04ca7ca
```

| Linked category | Image bytes |
| --- | ---: |
| SDK/libraries excluding shared strings and device fonts | 1,711,650 |
| Built-in Montserrat | 187,473 |
| Shared merged string pool | 172,181 |
| Other GooPad objects excluding loaders/notices/pooled strings | 124,321 |
| Essential offline Lato fallbacks | 80,001 |
| Compressed Font Awesome index | 10,913 |
| Styled SD loader | 6,249 |
| Font notices | 5,765 |
| Embedded recovery page | 3,011 |
| Web SD loader | 961 |
| Unattributed linker/image envelope and padding | 13,187 |
| **Actual image** | **2,315,712** |

Initialized object contributions total **2,302,525 bytes**, excluding BSS/no-init.
The corrected styled archive remains **471,647 bytes**; its immutable filename
is `styled-a46365f6bdea.gsp`. The 290,422-byte web archive and existing
multilingual/icon packs are unchanged. Styled metadata remains **43,144 bytes**
and both glyph loaders still share **one 32 KiB bitmap budget**. Space metrics
use existing descriptor padding; no new target descriptor allocation is added.

The preceding styled deployment had conversion errors despite valid hashes
and a clean health soak. Plain glyphs were decoded as compressed data,
compressed RLE state used the wrong bit origin, row-XOR filtering was not
reversed, and kerning was scaled 16 times too strongly. Independent comparisons
against the original LVGL decoder now verify **5,760 glyph pixels/metrics and
36,864 kerning combinations**; converter self-comparison was not sufficient.

The compiled-font boot screen renders at **1,483 ms**, before the font canary
finishes at **13,610 ms**. Repaired diagnostic firmware measured ten page
animations at **403-541 ms** and the menu at **384 ms**, with zero font SD reads
during motion and zero mismatches in ten cached-frame pixel comparisons.
Dirty-region page-cache patches avoid unnecessary full recopying. Configuration
and widget snapshots are deferred during motion. HTTPS/SD work now runs below
LVGL and its renderer: lowering server priority from the SDK default 5 to 2
removed the observed long render stalls; maximum measured cache copy fell from
598,366 to **27,266 microseconds**. This does not certify a universal latency
deadline for every layout or network.

Application-only installation verified the active image, all 68 font variants,
18 tiles/two pages, gzip/identity editor and all nine browser font hashes.
Configuration, authentication and logical HomeKit data were preserved.
Automatic diagnostic gestures/logging are **OFF** in the installed production
build. This is still an integrated development-worktree image, not a firmware
release reproduced from the scoped source commit.

Final **300-second soak: 21 samples, zero errors/reboots, 109 existing
network-admission warnings**. Free PSRAM ranged from **3,343,076 to 3,494,784
bytes**; internal free memory stayed above **38,595 bytes**, with a minimum
largest block of **21,504 bytes**. Styled metadata remained 43,144 bytes and
shared font bitmaps peaked at 6,057 bytes. The PSRAM minimum is 4,188 bytes
below the superseded run, while still 333,716 bytes above the pre-offloading
minimum. Correct glyphs now occupy more of the same bounded bitmap cache;
these independent short-run heap minima are not identical-allocation guarantees.
Sampled media requests averaged **225 ms (139-549 ms)**; a subsequent status
check observed a 1,145 ms media request. Network contention and occasional
long requests remain; physical song-to-screen latency was not measured.

## Superseded first SD-backed editor and styled-font build

**Do not use the following image/pack as the font-repair recovery target.**
Its valid checksums and health soak did not establish visual equivalence.
The preceding section replaces its rendering and delivery claims; the following
historical size and heap observations remain a record of that deployment.

That superseded production-config development image was **2,314,480 bytes**:
**1,879,824 bytes / 1.79 MiB / 44.82% free** in the unchanged 4 MiB slot.
Moving styled fonts, browser fonts and the editor to SD recovers a measured
**408,816 bytes / 399.23 KiB** versus the preceding multilingual build.
SHA-256:

```text
85fe9ee32a7385dfdfd3e8b9c3020fd493669f330443242219758705735fc4c9
```

| Linked category | Image bytes |
| --- | ---: |
| SDK/libraries excluding shared strings and device fonts | 1,711,762 |
| Built-in Montserrat | 187,473 |
| Shared merged string pool | 171,829 |
| Other GooPad objects excluding the new loaders/notices/pooled strings | 123,617 |
| Essential offline Lato fallbacks | 80,001 |
| Compressed Font Awesome index | 10,913 |
| Styled SD loader | 5,966 |
| Font notices | 5,765 |
| Embedded recovery page | 3,011 |
| Web SD loader | 961 |
| Unattributed linker/image envelope and padding | 13,182 |
| **Actual image** | **2,314,480** |

Initialized object contributions total **2,301,298 bytes**; BSS/no-init are
excluded. Styled font payloads, editor HTML and browser WOFF2 payloads are no
longer embedded. Loader attribution excludes pooled strings and is not an
estimate of runtime memory.

The web archive is **290,422 bytes** on SD: gzip and identity editor
representations plus nine WOFF2 fonts. Verification and HTTP streaming each
use a **1 KiB buffer**, without a full editor allocation or on-device
decompression. Missing/corrupt editor data serves an embedded recovery page
with HTTP 503, while retaining authenticated application-only OTA.

The styled archive is **471,647 bytes** on SD, preserving 60 variants,
5,760 glyphs and 4,132 kerning pairs. Its **186,072-byte** glyph metadata stays
on SD. On the device, descriptors, metrics and kerning total **43,144 bytes**
(5,040 + 13,312 + 24,792). Unicode metrics use **26,624 bytes**, an increase
of 2,048 bytes to coordinate the shared cache; the codepoint index remains
206,588 bytes. Both loaders share **one 32 KiB bitmap budget**, not two.
These small metadata costs replace much larger startup PSRAM reservations
from embedded rodata in the production configuration.

Cold styled metrics are read 16 records at a time into the existing bounded
cache, using a 448-byte stack buffer and no additional PSRAM/BSS allocation.
Native testing reduced a 5,700-descriptor sweep from 5,700 file reads/opens
to **360**. The installed full 68-variant ASCII/multilingual canary completed
at **12,376 ms** after boot; all 18 configured tiles and both pages rendered.
The initial unbatched candidate exceeded the deployment helper's 15-second
capture window: it eventually passed, rather than failing or cycling.
The regression was corrected before final delivery.

The identity and gzip editor responses and all nine browser fonts were
verified byte-for-byte on the device. Configuration, authentication and
logical HomeKit records were preserved; immutable resource reuploads reused
existing files. This remains an integrated development-worktree image,
not a source-matched firmware release.

Final 300-second soak: **20 samples, zero errors/reboots, 124 existing
network-admission timeout warnings** (background widgets/PC polling).
Free PSRAM stayed between **3,347,264 and 3,500,796 bytes**, versus the
preceding run's 3,009,360-byte minimum: **337,904 bytes / 329.98 KiB more
minimum headroom** with the same saved configuration. Internal free memory
stayed above **41,707 bytes**; its largest block remained 27,648 bytes.
Styled metadata stayed at 43,144 bytes; shared font bitmaps peaked at
833 bytes. Native churn tests cover the full 32 KiB bound.
These observations show no PSRAM regression in the running UI; they are
short-run evidence, not a guarantee for every configuration.
Sampled media requests averaged 256 ms (122-715 ms), with no physical
song-to-screen measurement. Contention warnings remain and were more frequent
than the prior run's 87; this change does not claim to fix network scheduling.

## Previous SD-backed multilingual build

The preceding production-config development build was **2,723,296 bytes**:
**1,471,008 bytes / 1.40 MiB / 35.07% free** in the unchanged 4 MiB slot.
It recovers **1,438,912 bytes / 1.37 MiB** versus the full-Lato image.
SHA-256:

```text
366b8b64ddeaa8a02536859e214af689dff61ad378f8257fe8cfe2866107530d
```

| Linked category | Image bytes |
| --- | ---: |
| SDK/libraries excluding shared strings and device fonts | 1,711,466 |
| Styled tile fonts | 312,963 |
| Built-in Montserrat | 187,473 |
| Shared merged string pool | 169,825 |
| Other GooPad objects excluding notices/pooled strings | 123,373 |
| Essential offline Lato fallbacks | 80,001 |
| Browser WOFF2 fonts | 55,592 |
| Gzip editor | 52,657 |
| Compressed Font Awesome index | 10,913 |
| Font notices | 5,765 |
| Unattributed linker/image envelope and padding | 13,268 |
| **Actual image** | **2,723,296** |

The former **848,857-byte** linked catalog object is absent. Full Unicode
fallback objects are also absent from linked payload; their historical source
files remain on the host. Compact essential fallbacks replace their previous
686,561-byte contribution. Main code, SDK/bidi processing and shared strings
also changed, so savings are the measured net result, not a sum of projections.

The matching SD pack provides 51,647 glyphs per size across ten sizes in
**66,074,899 bytes / 63.01 MiB**. Its full catalog is **848,853 bytes** on SD;
these files do not enlarge the app or configuration JSON. Glyph metrics used
512 PSRAM entries (24 KiB on that ESP32 build), a **206,588-byte** codepoint
directory (bounded at 512 KiB), and a separate **32 KiB** bitmap limit.
Decoding temporarily uses the existing caller's A8 draw buffer.
The full multilingual font is never loaded into PSRAM.

The size tool attributes **2,710,028 initialized object bytes**, excluding
BSS/no-init. Image envelope, alignment and unattributed linker bytes explain
the remaining 13,268 bytes. The string pool remains merged across modules; do not interpret its
`stdio_vfs.c.obj` ownership as a console-only cost.

That build's five-minute soak recorded 20 samples, no errors or
reboots, and 87 network-contention warnings. Free PSRAM ranged from
**3,009,360 to 3,132,188 bytes**; internal free memory stayed above 39,195 bytes.
Neither free-heap trend declined over the run. This is short-run evidence,
not a long-term leak guarantee. The font bitmap cache stayed at zero because
the live layout did not draw SD-only glyphs; native tests separately exercise
multilingual pixels, cache churn and the 32 KiB limit.

This remains an integrated development-worktree build. A clean source-matched
release is blocked by the earlier application backlog; asset/source commits
alone do not establish binary provenance.

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

## Historical full-Lato app-slot ledger

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
| **Historical image** | **4,162,208** | **4,064.66** | **99.23** |
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
- The current Font Awesome catalog and multilingual glyph pack live under
  `/goopad/resources/`; only the small compressed icon index stays compiled.
  The editor, browser fonts and styled device fonts also live in pinned SD
  archives. [main/CMakeLists.txt](../main/CMakeLists.txt) embeds only recovery,
  font notices and the compressed icon index, and compiles essential fallbacks.
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

The historical size summary reported 1,480,312 bytes of flash code and 2,552,980 bytes of
flash data, roughly 3.85 MiB combined before MMU/page alignment. This is not
an exact measured PSRAM reservation: startup mapping/alignment and section
selection matter. Frame buffers, cached pages, GIFs, TLS and heap allocations
use further memory. The most recent five-minute soak saw approximately
1.83-1.92 million bytes free PSRAM. That is an observed free-heap range, not
the physical capacity or a per-feature allocation measurement.

## Further space-recovery considerations

The first two targets are now implemented in the development build above:

1. The full icon catalog is SD-backed; its small gzip index remains compiled.
2. Essential Latin/punctuation remains in flash; broad multilingual glyphs
   stream from a bounded SD cache with corruption detection.
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
