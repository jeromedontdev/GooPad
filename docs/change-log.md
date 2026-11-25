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

## 2026-11-25 - Repairing Companion updates and one-click sharing

- **Request:** fix the Companion updater's missing Python DLL and enable all
  sharing from one control.
- **Cause/evidence:** a frozen same-path restart inherited the old extraction
  directory through the installer. An isolated PyInstaller reproduction
  failed with the same missing `python312.dll` message and exit code -1 after
  that directory was removed; requesting an independent runtime succeeded
  with exit code 0. Both the updater and installer now set
  `PYINSTALLER_RESET_ENVIRONMENT=1`, including support for older callers.
- **UI/security:** following the clarification, one **Enable sharing and PC
  controls** checkbox replaces all three individual controls (not an extra
  accept-all button). It enables now playing, PowerShell and PC power/monitor
  controls together after one explicit warning; switching off disables all.
  Older mixed permissions remain unchanged and show an indeterminate box;
  switch off before enabling the combined set. Existing flags, pairing, token,
  actions and tray/startup preferences retain their formats and defaults;
  installation itself does not grant permissions.
- **Verification:** added cancellation/save-error and settings-preservation
  regressions, plus an isolated frozen legacy-updater/installer/actual-app
  smoke/soak harness. Repeated Tk fixtures exposed worker-thread garbage
  collection of destroyed windows; GUI tests now collect those cycles on the
  creating thread. All 45 Companion tests pass. The first packaged check
  incorrectly required a non-null initial CPU delta; priming that sample
  corrected the harness, not product code. An intermediate packaged-upgrade
  soak passed 61 samples/300 seconds; final combined-toggle packaging,
  installation and publication are pending.
- **Release scope:** preparing 0.7.4; firmware changes only its release version
  so latest-stable firmware/Companion asset names remain consistent. No new
  firmware behavior, SD-resource changes or PSRAM budget changes are intended;
  an unrelated hardware flash is not required for this Companion repair.

## 2026-11-25 - Publishing the complete 0.7.3 release

- **Request:** create the release commit and publish the complete product.
- **Source/build:** release commit and tag `v0.7.3` point to
  `34fd9f80e609505403d5127b659fdcf2674cc70e`, including the accumulated
  integration rather than another narrow repair patch. A separate clean
  checkout, pinned component lockfile and fresh defaults produced the actual
  released image. Its resolved configuration matches the preceding verified
  production configuration. ESP-IDF 6.1 tracked sources were clean at
  `fff9895c82d744c7237be8847347bdd1b07c6643`. Diagnostics remain OFF.
- **Image/storage:** 2,316,320 bytes, leaving 1,877,984 bytes / 44.77% of
  the 4 MiB app slot free. SHA-256:
  `c8a52ecde58835a373dbd6dc392b29ad30662f56917f1b36ca4f79e945168207`.
  The SD pack matches the existing text, icon, corrected styled and web
  archives; no new glyph-cache budget is introduced by this release.
- **Installation/persistence:** COM9/chip, partitions, validated recovery
  bytes and pinned HTTPS were checked before application-only OTA. The
  strict check initially flagged HomeKit's expected firmware-version metadata.
  A controlled recovery/upgrade repeated the comparison: only `fw_rev` and
  its single `config_num` increment changed; all other logical pairing/config
  records, ownership marker and authentication matched. An early serial
  inspection also encountered a still-unvalidated candidate and automatic
  rollback; waiting for initialization before disruptive checks resolved the
  deployment-harness timing issue, without a firmware change or data erase.
  Actual released slot bytes, version, 68 font variants, early loading
  screen, editor gzip/identity, nine browser fonts and SD icon search passed.
- **Soak:** the initial 300-second run kept PC monitoring available in 56/56
  samples, with zero functional errors/reboots, but serial monitoring caught
  one recovered 8,094 ms PC polling timeout. Its extra zero-serial-fault check
  failed; that report is retained, not replaced. A second 300-second run with
  the same criteria passed: 56/56 available snapshots, zero observed deferred
  widgets, errors, reboots or serial polling faults. There were 28 bounded PC
  slot-wait warnings, none widget admission warnings. Minimum sampled
  internal free/largest block were 39,599/17,408 bytes; minimum PSRAM free
  was 3,349,780 bytes. The three currently configured network providers had
  valid output shapes; GET-only PC diagnostics returned HTTP 200 in 4,607 ms.
  Saved configuration was unchanged. These are observations, not guarantees
  of timeout-free networks or subsecond media updates.
- **Packages/publication:** the matching Companion installer and ZIP were
  built from the same clean source. Embedded executable and installer versions,
  exact bundled source/executable bytes, ZIP integrity and dependency notices
  were checked without replacing the user's running Companion. The stable
  [0.7.3 release](https://github.com/jeromedontdev/GooPad/releases/tag/v0.7.3)
  contains nine assets: OTA, installer, portable ZIP, SD resources, firmware
  notices, both sanitized soak reports, build inputs and SHA256SUMS. Every
  uploaded size/digest and the peeled remote tag target were verified before
  publication. Latest-stable metadata exposes both updater-required filenames;
  downloaded OTA/checksum files match the verified local bytes.
- **Remaining:** cold Companion TLS can still take 2-4 seconds and one initial
  polling deadline expired as recorded above. Only Guition hardware was
  verified; Windows packages are not code-signed. This follow-up documentation
  records delivery; the released firmware's source identity remains the tagged
  integration commit above.

## 2026-11-25 - Preparing the complete 0.7.3 release

- **Request:** commit and publish a complete release, not another patch-only
  prerelease.
- **Changes:** reconcile the accumulated editor, configuration storage/dedup,
  shared Companion settings, board portability, diagnostics, widgets, SD
  resources, multilingual fonts and rendering integration. Set firmware and
  Companion versions to 0.7.3 and retain the resolved dependency lockfile.
  Packaging uses the selected build's resolved flash settings and verifies its
  project/version instead of silently reading a different local configuration.
- **Compatibility:** configured devices must receive only the application OTA
  image. Retain configuration, TLS identity, HomeKit pairing and immutable SD
  resources. Fresh-build validation and a new 300-second functional soak are
  required before publication.
- **Delivery:** source integration and clean-build inputs are being committed;
  final artifact hashes, hardware results and publication verification will
  be recorded separately after validation. Historical patch-only release
  warnings below remain accurate for those earlier tags.
- **Pre-build validation:** 62 Python/native-helper tests, native configuration,
  shared Companion, network and market/layout suites, GIF/media gesture checks,
  516,470 multilingual records, actual LVGL/font-loader parity and 4,891 editor
  checks across 2,520 layouts passed. The editor runner isolates periodic
  preview intervals from mocked requests; real polling remains a hardware
  soak requirement. The packager regression covers external build settings,
  byte-identical OTA copying and rejection of missing/mismatched metadata.

## 2026-11-25 - Repairing PC availability and deferred widgets

- **Request:** fix "PC unavailable" with the Companion running and widgets
  repeatedly showing "Update deferred".
- **Diagnosis:** pinned/authenticated Companion probes were healthy, but the
  device was unavailable in 19/24 baseline samples. ESP-IDF 6.1 reapplies a
  saved session on successive asynchronous handshake calls; Mbed TLS rejects
  that with `-0x7080`. Media also cancelled admitted widget handshakes and
  immediate-retry admission could starve widgets. The preceding font soak's
  109 network warnings were not evidence of harmless functional behavior.
- **Changes:** disable saved sessions only for the asynchronous PC monitor,
  retaining its owned certificate-bound client and unchanged synchronous
  action sessions. Give verified requests an 8-second total budget rather
  than cancelling cold 2-4-second handshakes after 1.5 seconds. Gate PC/media
  TLS on a valid clock with explicit "Waiting for time" text. Eligible widgets
  queue behind the active request for up to 10 seconds and retain their turn;
  pending media cannot cancel their admitted request. Foreground/manual
  priority checks remain before service and during streaming. Failure logs
  include numeric TLS flags and elapsed time, not credentials or media text.
- **Regression:** the soak harness adds opt-in `-RequireCompanion` and
  `-MaxWidgetDeferralSeconds 15`, plus availability, count and duration fields.
  New reports also record enabled criteria. Missing PC snapshots and
  sustained widget deferral are functional failures, not just HTTP health.
  Reports never include widget/media text. Thirteen harness tests pass,
  including healthy/unavailable/deferred PC and recovering/starved widgets;
  focused tests also verify criteria metadata and privacy. Actual native
  streaming-helper tests pass manual preemption, deadlines and bounds.
  The published hardware report retains its original start header, emitted
  before criteria metadata was added; availability/deferral sample fields
  are present. Current header output was separately regression-tested, not
  retroactively inserted into the hardware report.
- **Validation:** initial session/cancellation fixes alone still failed 49/57
  PC samples. Extending the handshake deadline restored availability, but an
  intermediate soak still had 89 admission warnings. Final fair-queue build:
  **300 seconds, 55/55 PC snapshots available, zero observed deferred widgets,
  zero errors and zero reboots**. Remaining 22 warnings were PC lock waits
  while another admitted request completed; none were widget admission waits.
  No monitored PC request failure, saved-session error, watchdog or panic was
  captured during that run. Separate pinned checks verified actual weather,
  stock, crypto and GitHub output formats, not merely HTTP status or absence
  of an error label. A GET-only PC diagnostic completed with HTTP 200 in
  5,248 ms, verified through diagnostic event type 7, not `lastAction`.
- **Resources/performance:** no persistent heap allocations or font-cache
  budgets were added. Soak minima: internal 39,711 bytes, largest block 24,576,
  PSRAM 3,317,296. Sampled media requests took 1,927-4,339 ms (2,508 ms mean);
  source heartbeat age stayed at or below 266 ms. Fresh TLS is reliable but
  the 500 ms polling target is not a guaranteed update deadline. No physical
  song-to-screen or tap-to-action deadline is claimed.
- **Delivery:** production reconfigure/build and identity-verified app-only
  installation passed with render diagnostics OFF. Image **2,316,192 bytes**,
  **44.78% app slot free**; SHA is in the [storage ledger](storage-layout.md).
  All 68 font variants, 18 tiles/two pages, editor and browser-font hashes
  were verified. Configuration, authentication, logical HomeKit pairing and
  SD resources were preserved. The Companion required no reinstall/re-pair.
- **Publication scope:** the changed monitor/admission integration is part
  of the earlier uncommitted application backlog and is absent from the
  committed foundation. Do not blanket-stage it or publish the installed
  binary as matching the scoped test/documentation commit. A narrow
  `GooPad-PC-reliability.patch` reverse-checks cleanly against the installed
  source, preserving the repair independently of unrelated work. Scoped
  regression/documentation commit
  `bc5508ff9ea84d5310eeee7557890feffecd9e9d` is pushed. The repair patch and
  sanitized final soak are published in
  [v0.7.3-pcfix1](https://github.com/jeromedontdev/GooPad/releases/tag/v0.7.3-pcfix1),
  an integration-patch/report-only prerelease. Both asset SHA-256 digests and
  the tag target were verified; stable `v0.7.2` remains unchanged.
  There is no firmware binary in that release, and its tagged source alone
  does not reproduce the installed image. A reproducible firmware release
  remains blocked on review/integration of the earlier application backlog.
- **Remaining:** reconcile the application backlog before a complete firmware
  release, and investigate SDK-safe session reuse before promising subsecond
  media updates. The previous font-repair resource prerelease remains valid.

## 2026-11-25 - Repairing garbled fonts, boot loading and swipe stalls

- **Request:** fix garbled text, the missing loading screen and slow page/menu
  swipes following the styled-font SD offload.
- **Correction to prior entry:** the previous glyph parity check reused the
  converter and did not prove visual equivalence. A valid resource hash and
  a clean health soak did not catch the wrong pixels/kerning. The previous
  `styled-55567412e0ca.gsp` pack and resource ZIP are superseded.
- **Changes:** correctly copy plain 2bpp glyphs, decode compressed glyphs with
  glyph-relative RLE state, undo the format-1 row-XOR filter and apply LVGL's
  fixed-point kerning scale. The corrected immutable archive is
  `styled-a46365f6bdea.gsp`. Metrics eviction/prefetch preserve resident glyph
  bitmaps; validated space metrics use existing descriptor padding.
  Selected text is prewarmed, including offscreen captions. Same-page cache
  invalidations patch dirty rectangles and become valid on the final flush.
  Configuration/widget reads are deferred during page/menu motion.
  HTTPS runs at priority 2, below LVGL (4) and its software renderer (3).
  A compiled-font loading screen appears before SD/service initialization;
  the exhaustive canary yields the LVGL lock between font variants.
- **Compatibility:** application-only OTA preserved configuration,
  authentication and logical HomeKit data. Corrected styled reuploads reuse
  existing content; web/multilingual/icon resources and old immutable files
  were not erased. Styled metadata remains 43,144 bytes and the shared glyph
  bitmap budget remains 32 KiB.
- **Validation:** reproduced original Lato36 `!` pixel 4 as 2 instead of 1.
  Independent tests compile all 60 original fonts and the actual LVGL
  plain/RLE decoder: 5,760 glyph pixel/metric comparisons and 36,864 adjacent
  kerning combinations pass. Warm text remains no-I/O after exhaustive
  other-font measurement; mixed caches stay within 32 KiB. All 16
  UI-capture/refresh, web-resource and soak-harness tests pass.
  Hardware diagnostics verify synthetic touch swipe/click, menu swipe,
  page topology, fallback and ten cache/frame checks with zero mismatches.
  Ten page animations measured 403-541 ms; menu animation 384 ms.
  Earlier 1,216 ms page stalls persisted with zero font reads during motion;
  protecting rendering from higher-priority HTTPS work removed the observed
  stalls. Maximum observed cache copy dropped from 598,366 to 27,266 us.
- **Delivery:** production reconfigure/build and app-only installation passed,
  with automatic render diagnostics OFF. The loading screen renders at
  1,483 ms; the 68-variant canary passes at 13,610 ms, followed by all 18
  tiles/two pages. Identity/gzip editor and all nine WOFF2 hashes match.
  Image size 2,315,712 bytes, 44.79% of its app slot free; see the reconciled
  [storage ledger](storage-layout.md) for SHA and exact attribution.
  Final 300-second soak: 21 samples, zero errors/reboots, 109 existing
  network-admission warnings (previously 124). Minimum PSRAM 3,343,076 bytes,
  internal minimum 38,595 bytes, largest-block minimum 21,504 bytes.
  Styled metadata remained 43,144 bytes; shared font bitmap peak 6,057 bytes.
  Sampled media requests averaged 225 ms (139-549 ms); a subsequent request
  took 1,145 ms. No physical song-to-screen deadline is claimed.
- **Publication scope:** the scoped source foundation includes corrected
  generators/loaders, independent font tests, documentation, isolated early-boot
  UI and HTTPS scheduling changes. Page-cache/prewarming/motion integration and its
  actual capture regression still depend on the earlier dirty application
  backlog. Do not blanket-stage that backlog or publish the installed binary
  as if it were reproduced from the scoped commit.
  Foundation commit `3bbe4c3512d33b1d7bedf570092eec8bd60f1fdb` is pushed.
  Corrected licensed resources and sanitized final soak evidence are published
  in [v0.7.3-assets1](https://github.com/jeromedontdev/GooPad/releases/tag/v0.7.3-assets1),
  a resources/source-only prerelease with no firmware binary. The stable
  `v0.7.2` release remains unchanged.
- **Remaining:** reconcile the earlier application integration before a
  source-matched firmware release. The five-minute observation is not a
  universal UI/network latency or long-term leak certification.

## 2026-11-25 - Editor, browser fonts and styled fonts offloaded to SD

- **Request:** implement the recommended SD storage savings without reducing
  PSRAM headroom or changing the saved layout/font appearance.
- **Changes:** pinned GWP1 editor/browser-font archive, GSP1 styled-font pack,
  bounded streaming/hash checks, embedded recovery and four-kind resource
  uploads. Original styled glyph pixels, metrics and kerning are retained.
  Unicode and styled fonts share one 32 KiB bitmap cache. Styled cold metrics
  prefetch 16 records into the existing cache; no additional PSRAM/BSS.
- **Compatibility:** verified application-only OTA, unchanged configuration,
  authentication and logical HomeKit records. Immutable web/styled uploads
  reused existing content. Existing multilingual/icon packs and rollback
  resources remain untouched; recovery works without the full SD editor.
- **Validation:** 4 web-resource and 11 soak-harness tests passed; Pylance
  reported no generator syntax errors. Actual native HTTP tests covered
  hashes, gzip/identity negotiation, 1 KiB chunks, abort cleanup and recovery.
  Styled source parity checked 5,760 glyphs and 4,132 kerning pairs; native
  loaders checked 5,700 non-space renders, corruption/fallback, mixed-cache
  bounds and real LVGL selector semantics for all 68 variants.
  Cold descriptor sweep dropped from 5,700 opens/reads to 360.
  Fresh local editor regression: 4,891 checks across 2,520 layouts; mocked
  recovery browser checks passed without touching live configuration.
- **Delivery:** production reconfigure/build and app-only installation passed.
  The installed image is 2,314,480 bytes, with 44.82% of its app slot free:
  408,816 bytes smaller than the preceding build. Identity/gzip editor and all
  nine WOFF2 responses matched source hashes. The full boot canary passed at
  12,376 ms; 18 tiles/two pages rendered. See the exact SHA and reconciled
  [storage ledger](storage-layout.md).
  Final 300-second soak: 20 samples, zero errors/reboots, 124 network-admission
  warnings; minimum PSRAM 3,347,264 bytes, 337,904 bytes above the previous
  minimum. Internal minimum 41,707 bytes. Styled metadata 43,144 bytes and
  shared bitmap peak 833 bytes. Sampled media requests averaged 256 ms.
- **Resolved delivery issues:** ESP-IDF script-mode requirements expansion
  rejects GLOB `CONFIGURE_DEPENDS`; directory input dependencies remain.
  The first SD-font build's exhaustive boot canary exceeded the original
  15-second capture window, leaving UI initialization incomplete at inspection.
  Longer observation proved correct resolution; bounded metrics batching
  removed excessive per-glyph file operations without weakening the canary.
- **Publication:** independently usable loaders/generators, font inputs and
  licenses, font/soak tests and scoped documentation are committed/pushed as
  a foundation. The earlier dirty application backlog is not blanket-staged.
  CMake/HTTP/upload/status integration and its worktree HTTP regression still
  depend on that backlog; no source-matched firmware release is claimed.
  Matching licensed SD resource bundle and sanitized soak report are retained
  as local deliverables.
- **Remaining:** reconcile the earlier application/editor integration before
  publishing a firmware release; do not publish the installed image as if it
  were reproduced from this scoped commit. Network contention remains
  observable (124 warnings versus 87 previously); physical media latency and
  long-term leaks are not certified by this five-minute observation.

## 2026-11-25 - Multilingual SD fonts, headroom and faster now playing

- **Request:** Chinese/Japanese and varied artist text, meaningful app/PSRAM
  headroom, and less song-change delay.
- **Changes:** checksummed SD glyph pack with 51,647 mappings at ten sizes;
  compact offline fallbacks, SD Font Awesome catalog, bounded indexed
  metrics/bitmap caches, bidi/Arabic processing, and full UTF-8 metadata.
  Persistent Windows sampling at 250 ms and device polling at a 500 ms target;
  certificate-bound TLS session resumption releases the socket after each poll.
  See [Unicode coverage and limitations](unicode-fonts.md) and the measured
  [storage ledger](storage-layout.md).
- **Compatibility:** application-only OTA preserved configuration, device
  credentials and logical HomeKit records. SD resources remain immutable,
  hash-pinned and reusable; keep old versions for rollback. The rebooting
  HomeKit `hap_main/state_num` advertisement counter legitimately changes;
  every other protected record was compared privately. Broad coverage is not
  universal Unicode, complete Indic/Thai shaping or color emoji.
- **Validation:** 44 Companion and 9 soak-harness tests passed; all 516,470
  pack records verified. Actual-loader native tests verified decoded pixels,
  one-record SD reads, missing/corrupt packs, pinned LRU lifetime and limits.
  Editor/media-gesture regressions passed. Production build and installed
  68-variant printable-ASCII/multilingual canary passed.
- **Delivery:** Companion executable/ZIP/installer built and installed with
  pairing/settings preserved. Fixed an installer wait that included its
  launched GUI, and replaced overstrict raw NVS equality with logical record
  comparison excluding only the documented reboot counter. Firmware and
  matching resources installed and verified on the attached board.
  Final 300-second soak: 20 samples, zero errors/resets, 87 contention
  warnings; PSRAM minimum 3,009,360 bytes. Sampled media requests averaged
  225 ms (111-293 ms), versus the prior observed 1,230 ms. Live revision
  changes were observed without issuing playback commands.
- **Publication:** this scoped foundation commit contains independently usable
  font/resources, Companion sampling, tests and documentation.
  The installed binary includes earlier accumulated uncommitted application
  integration; no source-matched binary release is claimed or published.
- **Remaining:** reconcile that earlier application backlog before releasing
  firmware; contention warnings remain observable. Physical song-to-glass
  timing and scripts needing full shaping require further validation.

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

## 2026-11-24 - Measured storage layout and application ledger

- **Request:** explain the nearly full app slot and document the storage layout
  and its consumers.
- **Changes:** added [storage-layout.md](storage-layout.md) with the complete
  16 MiB partition map, shared configuration/HomeKit tail, measured app ledger,
  per-size Unicode costs, SDK attribution caveats, SD/PSRAM distinctions,
  reproduction commands and safe space-recovery candidates. Added an agent
  orientation link and corrected the Unicode guide/runtime memory note.
- **Findings:** actual app length is 4,162,208 bytes; 32,096 remain in one
  4 MiB slot. The Unicode update's net growth was 683,216 bytes. Unicode fonts
  contribute 686,561 bytes; all device fonts plus the catalog/index contribute
  2,046,767 bytes (about 49% of the slot). The unassigned storage prefix is
  8,187,904 bytes but must remain erased under current ownership checks.
- **Correction:** the previous Unicode entry's claim that complete font tables
  are not loaded into PSRAM is incorrect for the selected SDK config.
  `SPIRAM_FETCH_INSTRUCTIONS` and `SPIRAM_RODATA` copy flash code/read-only
  data into PSRAM at boot. Fonts/catalog consume flash and boot-time PSRAM;
  absence of per-font heap loading does not mean zero PSRAM cost.
- **Validation:** used the installed ESP-IDF size tool on the deployed build's
  map for section, object and archive attribution; checked binary length/hash,
  flash offsets, partition table, ownership offsets and actual SDK flags.
  Object attribution plus explicit linker/image residual reconciles exactly
  to the binary. No user configuration, flash dumps or private serial data
  were inspected or published for this report.
- **Delivery:** documentation-only change; no build, flash, device soak or
  binary release required. Scoped commit/push includes the new ledger, agent
  link, Unicode-guide correction and this log; the broader pre-existing
  runtime memory document remains in the accumulated worktree.
