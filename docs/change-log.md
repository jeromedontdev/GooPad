# GooPad agent change log

Read this alongside [AGENTS.md](../AGENTS.md) and the
[development guide](development.md) before continuing work. The source and
observed behavior remain authoritative. This is a concise delivery record,
not a conversation transcript or a substitute for Git history.

Append entries using this structure:

- **Date and request:** what was requested.
- **Changes:** affected components and user-visible behavior.
- **Compatibility:** persistence, migration, security and recovery implications.
- **Validation:** exact checks, measured results and known gaps.
- **Delivery:** build, installation, device soak, commit/push and release status.
- **Remaining:** actionable next steps or explicit blockers.

Never include credentials, private keys, pairing codes, raw user configuration,
serial captures or flash dumps. Do not claim historical work was committed or
released merely because it was installed.

## 2026-11-24 - Delivery workflow and persistent agent log

- **Request:** make documentation, verification, flashing, commits,
  publication and device soak testing part of the normal code-change workflow;
  preserve a readable history for future agents.
- **Changes:** expanded [AGENTS.md](../AGENTS.md) with a delivery checklist,
  one-hour default device soak, safe deployment, source/binary provenance,
  release verification, explicit blockers and documentation-only exceptions.
  Added this append-only log and made it part of session orientation.
- **Validation:** checked the existing development/recovery guide, soak
  interface, release packaging and separate product/hardware-test remotes.
  This change modifies documentation only.
- **Delivery:** no firmware build, flash, binary release or device soak is
  applicable. The documentation-only commit is scoped to `AGENTS.md` and this
  log; consult Git history and the product remote for its commit/push record.

## Prior work baseline - recorded 2026-11-24

These entries summarize observed results from the preceding session. They are
not a complete historical changelog.

### Configuration storage and SD visual reuse

- Configuration was migrated from the constrained 24 KiB NVS partition into
  the existing 64 KiB configuration/HomeKit store. Atomic saves and the retained
  legacy configuration were covered by native migration tests.
- The editor stages visuals until Save and searches existing SD assets by
  normalized SHA-256 before uploading new copies. GIF normalization was checked
  against the compiled firmware routine. Existing user assets already live on
  SD; essential configuration and identities remain in flash.
- The bundled icon catalog uses independently compressed records. All 2,163
  records were compared with source; real-device searches and SD lookup passed.
- Firmware was installed and HTTPS/configuration preservation verified. An
  initial HTTP handler-capacity failure was repaired; capacity now derives from
  the route table. Long-duration device soak was not performed.

### Shared Companion configuration

- PC macro, PC-open, PC-monitor and now-playing tiles can store `sharedPc`
  references with short endpoint paths rather than repeated addresses/tokens.
  Existing configurations remain unchanged until saved in the updated editor;
  older per-tile pairings remain supported.
- Firmware without `sharedPc` support cannot execute compacted PC tiles.
  Preserve a compatible backup before downgrading.
- Native resolution tests, configuration migration tests and 4,886 editor
  checks across 2,520 layouts passed. Production build and embedded editor were
  verified; the app fit its 4 MiB slot with 17% free.
- App-only OTA, active-image verification and configuration preservation passed.
  Live PC actions were not triggered; device soak remains outstanding.

### Read-only soak harness

- Added [Test-GooPadSoak.ps1](../tests/Test-GooPadSoak.ps1) and
  [test_soak.py](../tests/test_soak.py). The runner uses authenticated,
  certificate-pinned HTTPS GETs for status/diagnostics, records memory trends,
  resets and error categories, and optionally writes flushed JSONL reports.
- Seven synthetic HTTPS integration tests passed, including healthy operation,
  wrong certificate pin, authentication rejection, malformed JSON, reboot
  detection, diagnostic-event overlap and history gaps.
- No device soak has been run. The 48-event ring lacks sequence IDs, so the
  runner cannot guarantee capture of every event. No raw serial recording or
  automatic action triggering is included.

### Outstanding delivery/provenance at this baseline

- The worktree contains substantial uncommitted changes from earlier requests.
  These feature summaries do not establish a matching published release or
  committed source revision. Do not bulk-stage that work without scope review.
- Next delivery work should reconcile source/binary provenance, run the device
  soak using verified identity and credentials, and publish a versioned release
  only after the relevant changes and validation are accounted for.

## 2026-11-24 - Completed real-device one-hour soak

- **Request:** execute the read-only soak and summarize the findings.
- **Setup:** verified the CH340-attached ESP32-S3 and live partition table.
  Retrieved certificate/authentication only in memory over USB, then verified
  authenticated HTTPS against that device certificate. USB bootstrap rebooted
  the board once before the baseline; no flash, configuration write or action
  trigger was performed.
- **Validation:** ran `tests\Test-GooPadSoak.ps1` for 3,600 seconds with
  15-second intervals and 10-second request timeouts, from 08:31:56 to 09:31:56
  at UTC-05:00. Collected 238 valid samples; exit code 0, zero errors and zero
  observed reboots. Wi-Fi connected and SD mounted in every sample.
- **Warnings:** 165 observed network-deferral events, all type 2/result 263.
  `ESP_ERR_TIMEOUT` is `0x107` (263); `network_begin()` records this event when
  the shared network semaphore cannot be acquired in time. These were not
  low-memory admission failures or failed soak HTTP requests. The ring does
  not identify the affected tile/request; deferral frequency warrants follow-up
  if live widgets lag. Event counts may undercount because the ring lacks IDs.
- **Memory:** internal free ranged from 43,287 to 52,611 bytes (first 43,287,
  last 52,115); largest internal block never fell below 28,672 bytes. PSRAM
  free ranged from 1,905,896 to 2,607,776 bytes (first 2,372,532, last 2,220,444).
  Ten-minute PSRAM means fluctuated around 2.20-2.25 million bytes after the
  first window, rather than steadily decreasing. No sustained downward trend
  was observed; this is not proof against slower or workload-specific leaks.
- **Latency:** the combined status/diagnostics poll averaged 230 ms, with
  69 ms minimum and 1,128 ms maximum.
- **Delivery:** saved a sanitized JSONL health report in the session workspace.
  This is a results-only log update; no new firmware or binary release is
  applicable. Commit/push provenance is available in Git history.
- **Remaining:** earlier uncommitted source/release provenance remains
  outstanding. This run monitored the installed firmware, not a freshly
  released build. Touch/rendering and macro execution were not exercised.

## 2026-11-24 - Media gestures, punctuation, board boundary and soak policy

- **Request:** investigate deferrals/latency, display em dashes, add now-playing
  tap/hold controls, prepare other ESP32 displays, and shorten routine soaks.
- **Changes:** network-deferral details identify macro/manual-diagnostic/widget/
  PC-monitor callers without hiding contention. Soak samples separate status
  and diagnostics request latency. The previous hour's slowest poll was its
  first (1,128 ms); later polls peaked at 456 ms. This alone does not establish
  the cause or measure physical action latency.
- **Text/input:** small compressed Lato punctuation fallbacks supplement all
  68 font variants with em/en dashes, smart quotes and ellipsis. Now-playing
  taps queue play/pause; holds queue next track and suppress the release click.
  Requests reuse Companion pairing, TLS checks and the bounded action queue;
  configuration still stores the monitor endpoint. Other widgets are unchanged.
- **Portability:** `GOOPAD_BOARD_DIR` replaces the six hardware implementation
  files and pin-map header explicitly. Status, editor preview metrics and
  save validation use display dimensions and minimum tile size. Removed fixed
  page/GIF/overlay/keyboard geometry; retained compatible SD image formats.
  See [boards.md](boards.md) for backend contracts and remaining limits.
  No Waveshare-specific driver or pin map has been guessed or certified.
- **Policy:** default read-only soak is now five minutes after relevant changes,
  not every session. Longer runs remain appropriate on request or evidence.
- **Validation:** Guition production build passed with a `0x3515d0` application,
  17% app-slot headroom. Native media request/layout/network suites, 41 Companion
  tests, seven synthetic soak tests, 4,891 browser checks across 2,520 layouts,
  and compiled actual media-callback event tests passed.
- **Hardware:** app-only OTA preserved configuration and the embedded editor
  matched source. The first update's active application was byte-verified;
  final refinements were validated by OTA integrity checks and matching the
  active descriptor/ELF digest against the final build after reboot. Serial
  startup verified an actual em-dash glyph in all 68 font variants.
- **Soak:** final 300-second run completed at 10:03:53 UTC-05:00 with 21 valid
  samples, exit code 0, no errors/resets and Wi-Fi/SD present throughout.
  Internal free went from 33,239 to 49,447 bytes; largest block stayed at
  least 24,576 bytes. PSRAM free went from 2,504,572 to 2,603,204 bytes, with
  a 2,483,676-byte minimum. The initial preliminary soak was stopped to install
  the final font-table/preview refinements; it is not counted as a complete run.
- **Findings:** 31 lock-contention warnings remain: 27 PC-monitor/now-playing
  requests and four other live-widget requests. These are deferred background
  attempts, not failed monitoring HTTP calls. Combined poll latency averaged
  281 ms (248 ms excluding the first), ranging 103-932 ms. Status averaged
  121 ms; diagnostics averaged 159 ms. The first status call took 870 ms.
  These endpoint/client timings do not prove physical action latency or a
  performance improvement over the different one-hour workload.
- **Delivery:** policy, porting guidance and these results are scoped for a
  documentation commit/push. Feature source remains integrated with earlier
  uncommitted work; no source-matched feature commit or binary release is
  claimed. Reconcile that backlog before tagging or publishing binaries.
- **Limits:** gesture regression tests mock the OS/event transport, not physical
  touch or Windows media playback. The existing accumulated worktree predates
  this request and still prevents a source-matched binary release until its
  changes are reconciled; do not silently include unrelated work in a release.

## 2026-11-24 - Extended Unicode fallback coverage

- **Request:** fix missing Æ and expand extended Unicode text support.
- **Changes:** replaced the punctuation-only fallbacks with all 2,068 available
  non-ASCII BMP Lato mappings at all ten required sizes. Existing selected-font
  glyphs retain their styling; new fallback glyphs are compressed 1-bpp
  monochrome. Added the licensed source, reproducible generator and full-source
  coverage regression. Startup checks ten representative extended characters
  through all 68 font variants. See [unicode-fonts.md](unicode-fonts.md).
- **Limits:** this supplies extended Latin, Greek, Cyrillic and the symbols
  actually present in Lato, not universal Unicode. CJK, emoji, complex-script
  shaping and combining-mark positioning are not guaranteed. Availability
  checks do not replace physical visual inspection.
- **Build:** the first 2-bpp expansion exceeded the unchanged 4 MiB slots by
  `0xb010`; it was not flashed. The 1-bpp production build passed at `0x3f82a0`,
  leaving `0x7d60` (32,096 bytes, about 31 KiB) of app-slot headroom. The fonts
  reside in flash rather than loading the expanded bitmap tables into PSRAM.
  Do not repartition the configured device to force future additions through.
- **Compatibility/hardware:** verified the actual board, live partition table
  and valid recovery image, then installed only the app via authenticated,
  certificate-pinned OTA. Configuration remained byte-identical in memory;
  the embedded editor matched source. The active image descriptor/ELF digest
  matched the build, and serial startup confirmed extended glyph resolution
  in all 68 font variants. NVS, HomeKit and SD data were not erased.
- **Validation:** all 20,680 source-to-generated character mappings and their
  glyph indices passed across ten font sizes. The 300-second read-only device
  soak ended at 20:06:55 UTC-05:00 with 20 valid samples, exit code 0, zero
  errors/resets and Wi-Fi/SD available throughout. Internal free was
  40,275 -> 49,479 bytes (minimum 40,275); largest block stayed at 28,672.
  PSRAM free was 1,869,572 -> 1,867,308 bytes (minimum 1,831,660).
- **Findings:** 40 existing shared-network-lock admission warnings remain:
  28 PC-monitor/now-playing and 12 other live-widget attempts. No low-memory
  admission errors were observed. Combined polling averaged 239 ms (214 ms
  excluding the first), range 106-712 ms; status averaged 114 ms and diagnostics
  124 ms. This short run does not certify long-term leak freedom or physical
  rendering/action latency.
- **Delivery:** scoped publication contains the Unicode assets, source/license,
  generator, coverage test, guide and this log. Font selection/startup
  integration is mixed with the earlier accumulated source worktree, so no
  source-matched firmware release is claimed. Reconcile that backlog before
  publishing the installed development image as a versioned release.
