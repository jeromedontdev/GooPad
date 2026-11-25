# Multilingual text and SD resources

ASCII is the basic 128-code-point character set; Chinese, Japanese, extended
Latin and emoji are Unicode. GooPad checks all 95 printable ASCII characters
and representative multilingual glyphs through its 68 selectable font variants.

The development resource pack provides **51,647 Unicode scalar mappings at
each of ten sizes**: 8, 10, 12, 14, 16, 20, 24, 28, 36 and 40 pixels.
It includes Chinese Han, Japanese kana, Korean Hangul, extended Latin, Greek,
Cyrillic, Arabic, Hebrew, Thai, Devanagari, symbols and outline emoji from
the pinned [Lato/Noto source manifest](../tools/multilingual-font-sources.json).
The CJK font uses Japanese regional glyph forms; this is not a locale-specific
font-selection engine.

This is **not universal Unicode or a complete shaping engine**. Unmapped
characters can display a placeholder. LVGL bidi and Arabic/Persian processing
are enabled. Indic/Thai shaping, combining-mark positioning, variation selectors,
ZWJ emoji sequences and color emoji are not fully supported. Emoji are grayscale.

Styled tile fonts use a separate SD pack, preserving their original pixels,
metrics and kerning. Missing characters use core Montserrat and a same-size
multilingual SD font, then the compact compiled Lato subset.
Without SD resources, printable ASCII, basic extended Latin
U+00A0-U+017F, common punctuation U+2010-U+2026 and euro remain available;
other scripts require the card. This intentionally replaces the previous
large compiled full-Lato fallback. Keep resources installed when upgrading.

## Storage and rendering

The **66,074,899-byte** `GFP2` pack lives only on SD. Glyphs use packed 2-bpp
alpha or byte RLE, whichever is smaller. The loader binary-searches a cached
codepoint directory and lazily reads metadata/decodes bitmaps. It does not load the complete font into
PSRAM. A **206,588-byte** sorted codepoint directory avoids approximately
16 random SD index seeks per uncached glyph; lookups need one record read.
This directory is bounded at 512 KiB. The metrics cache has 512 entries
(24 KiB on ESP32), and an LRU bitmap cache is capped at **32 KiB**, with
pinned-glyph lifetime protection. Per-glyph CRC16, descriptor
bounds and decode limits fail explicitly on corrupt data.

The styled `GSP1` pack contains 60 variants, 5,760 glyphs and 4,132 kerning
pairs in **471,647 bytes**. Its **186,072-byte** indexed glyph metadata remains
on SD; resident descriptors, bounded metrics and kerning avoid whole-font
bitmap loading. Unicode and styled loaders share **one 32 KiB bitmap LRU**,
not separate 32 KiB allocations. Missing/corrupt packs fail independently;
a styled-pack failure does not disable healthy multilingual text.
Styled cold metrics prefetch 16 adjacent records into the existing cache;
the extra 448 bytes are stack-only. The target's resident styled metadata is
43,144 bytes and Unicode metrics are 26,624 bytes. The full canary remains
enabled and verifies every printable ASCII character in all 68 selectors.

The full Font Awesome catalog is also SD-backed; its small compressed search
index remains embedded. Existing user images, configuration and pairing are
not moved or erased. See the [storage ledger](storage-layout.md) for application
and PSRAM measurements; compiled code/read-only data still contributes to
boot-time PSRAM reservation in the selected SDK config.

## Installation and recovery

Generated resources have immutable, content-fingerprinted names:
`text-<12hex>.gfp`, `icons-<12hex>.bin`, `styled-<12hex>.gsp` and
`web-<12hex>.gwp`. The generated
[firmware manifest](../main/goopad_resource_manifest.h) pins their full SHA-256.
Copy matching files to `/goopad/resources/` on the SD card, preserving old
versions for rollback, then reboot.

Alternatively, the authenticated [resource-upload API](api.md) streams a matching
bundle into a staging file, verifies its SHA-256/signature and atomically renames
it. Matching existing content is reused, never duplicated; conflicting immutable
content is rejected. Font uploads require reboot. A resource bundle for another
firmware build is rejected. Resources do not occupy configuration JSON.

`/api/status.unicode` reports availability, glyph count, bitmap-cache bytes/peak,
hits and SD reads. Missing packs use the offline subset with a warning;
invalid packs or glyph data report an error. Never erase live flash partitions
to install font assets.

The `GWP1` archive stores gzip/plain editor HTML and nine browser WOFF2 fonts.
Compiled asset offsets, sizes and SHA-256 pins are verified before each response.
Both verification and serving use 1 KiB buffers, with no whole-editor PSRAM
allocation or decompression. Missing/damaged editor resources produce HTTP 503
with an embedded recovery page; preview-font failures return 503 rather than
serving unverified data. Core UI fonts, recovery, notices and the icon index
stay in firmware. Old resource filenames remain safe for rollback.

## Reproduction and checks

Use Node and an installed `lv_font_conv` **1.5.3**:

```powershell
.\tools\make_multilingual_resources.ps1 `
  -ConverterPath C:\font-tools\node_modules\lv_font_conv\lv_font_conv.js `
  -CacheDir C:\font-cache -OutputDir C:\goopad-resources
node tests\test_multilingual_pack.js C:\goopad-resources
zig cc -std=c11 -I tests\unicode_stubs tests\test_unicode_cache.c -o C:\font-tests\unicode-test.exe
```

The generator downloads pinned upstream revisions, records source hashes and
copies their OFL licenses into the resource directory. It verifies source
mappings against FreeType before rendering. The pack regression validates all
**516,470 records**, scalar/order/bounds, CRCs, raw/RLE decode lengths and
representative multilingual/supplementary mappings.

For the native loader test, create a disposable
`C:\font-tests\goopad\resources\` directory with the generated text pack, run
the executable there with `valid`, then `corrupt`. The latter intentionally
damages **only that disposable copy** and verifies fail-closed decoding.
Run `missing` with no pack or a truncated fixture to test the offline fallback.
It also verifies cache hits, nonempty glyph pixels, LRU limits and pin lifetime.

The old [full-Lato generator](../tools/make_unicode_fonts.ps1) and
[regression](../tests/test_unicode_fonts.js) remain historical asset tooling;
those full Unicode objects are no longer linked into this firmware.
Preserve all source copyrights/OFL licenses when redistributing the generated
subsets or resource bundle.

Generate the styled archive directly from the checked-in 60 generated font C
inputs, retaining their original metrics, kerning and pixels:

```powershell
node tools\make_styled_pack.js main\fonts C:\goopad-resources
.\tests\run_styled_native_tests.ps1 -ResourceDirectory C:\font-tests `
  -UnicodePack C:\goopad-resources\text-371771469d5d.gfp
python tools\make_web_resources.py main C:\goopad-resources main\goopad_web_manifest.h
python -m unittest tests.test_web_resources tests.test_soak
```

The native styled runner checks source parity, glyph decoding, 360 cold reads
for a 5,700-descriptor sweep, warm no-I/O lookup, mixed-cache bounds, corruption
and independent pack failure. It also compiles the real LVGL descriptor
lookup with the actual font selector to check all fallback chains.
The web handler test requires the integrated SD HTTP handlers; this application
integration remains part of the development worktree until the source backlog
is reconciled. Generated packs must ship with the accompanying OFL notices.

## Now-playing text and latency

The Companion keeps one Windows media sampler alive, querying every 250 ms
instead of launching PowerShell behind a five-second cache. The device polls
now-playing every 500 ms and gives it bounded priority over background widgets;
manual diagnostics and macros still take priority. Windows/provider publication
delay remains outside GooPad's control.

Title and artist each retain up to 80 Unicode characters. The device accepts
up to 320 UTF-8 bytes per field without the former 26-byte truncation.
`mediaRevision`, `mediaPollMs` and `mediaSourceAgeMs` distinguish updates,
request duration and source heartbeat age. They are not physical glass latency.

The monitor retains a certificate-bound HTTP client and TLS session ticket,
but closes its socket/TLS connection after each request, releasing shared
network resources. URL, token or certificate changes discard that client;
credentials/certificates are owned in PSRAM rather than borrowed from deleted
configuration JSON. SDKs without session-ticket support keep the same safe
connection lifecycle without resumption.

After installation, 17 nonzero samples in the five-minute soak measured
**111-293 ms requests (225 ms mean)**, compared with the previous observed
1,230 ms request. Source heartbeat age was 15-250 ms (132 ms mean).
An additional post-soak request took 516 ms: 500 ms is a target cadence,
not a guaranteed deadline. Provider publication and physical song-to-screen
latency were not measured; no playback actions were triggered by the test.

## Delivery status

The font assets, source, generator and coverage regression can be committed
independently of the earlier accumulated application worktree. The font
selection/startup integration and a matching binary release still require that
earlier source backlog to be reconciled; an asset commit alone is not a released
firmware build.
