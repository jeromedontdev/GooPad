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

## 2026-11-26 - Rename Wake-on-LAN option

- **Request/changes:** rename the PC widget option to **Wake-on-LAN**, then
  commit and publish without another soak. The README uses the same label.
- **Compatibility:** this changes only presentation; existing `wake_on_lan`
  actions and their saved MAC addresses remain unchanged.
- **Validation:** editor browser regression passed 4,947 checks across 2,520
  layouts; all four web-resource/loader tests passed. The clean-source ESP-IDF
  6.1 production build from `ebd6e39` passed, with 2,318,928 bytes used
  and 1,875,376 bytes free in its app slot. Release checksums, SD ZIP CRC
  and matching web-pack digest, and Companion ZIP and installer-chain
  contents were verified. The final package, ZIP and installer passed Defender
  scans (signatures `1.463.478.0`, engine `1.1.26100.4`).
- **Delivery:** app-only OTA and its matching editor resource were installed
  on the verified COM9 device. Pinned HTTPS, configuration, HomeKit records
  and the previous recovery image were verified; active and recovery images
  were compared using device-side flash MD5 against known SHA-256-verified
  images. The preceding clean-source image passed the soak recorded below;
  the final label-only build was not soaked again at the user's request.
  Source commit `ebd6e39` was pushed to the product repository. Published
  [v0.7.6](https://github.com/jeromedontdev/GooPad/releases/tag/v0.7.6)
  as latest with its tag at that source commit; all eight published assets
  were downloaded and their SHA-256 digests matched the verified local files.
- **Remaining:** alarm playback and Wake-on-LAN actions have not been
  triggered on the live device; the GitHub social-preview image still needs
  an authenticated settings browser. Windows packages are unsigned.

## 2026-11-26 - Tile navigation, Wake-on-LAN and branding

- **Request:** reorganize device appearance and tile editing, group Wake-on-LAN
  under PC, collapse FAQs, align branding with the Companion icon, and deliver
  the changes to the identified device and product repository.
- **Changes:** Tiles now contains device themes and background controls in
  Layout settings; About replaces About & help. The README logo and editor
  logo use the Companion's four-square design. The FAQ starts collapsed.
- **Compatibility:** Wake-on-LAN is presented under Widgets → PC but remains
  a `wake_on_lan` action in saved configurations. Device appearance still
  uses the existing configuration and save path; no migration is required.
- **Validation:** browser regression passed 4,946 checks across 2,520 layouts;
  all four web-resource/loader tests and isolated release-packaging tests
  passed. The documented Companion virtual environment passed 47 tests.
  Running the same suite with the system Python first produced TLS handshake
  failures; the isolated environment passed without those failures. The
  versioned Companion package, ZIP and installer passed Defender scans
  (signatures `1.463.470.0`, engine `1.1.26100.4`). The isolated actual
  frozen legacy updater/installer/restart chain passed 61 samples in 300
  seconds with no errors and preserved its test configuration and identity.
  The packaging script's GNU license download was unreachable, so packaging
  was completed with the previously distributed GNU license notice.
  Earlier in this session the preceding editor/alarm development image was
  installed on COM9 with an app-only OTA and its matching editor resource;
  a 300-second read-only soak had 20 samples, no request errors or reboots,
  and 40 PC polling deferral warnings. PC snapshots were available in all
  20 samples and no widgets were observed deferred.
- **Delivery:** ESP-IDF 6.1 production `reconfigure build` passed for 0.7.6;
  the 2,318,800-byte app leaves 1,875,504 bytes in the 4 MiB slot. App-only
  OTA and matching editor resource were installed on COM9. Pinned HTTPS,
  configuration, previous recovery image, and HomeKit pairing/ownership
  records were verified; HomeKit firmware revision and configuration counter
  advanced as expected. A 300.232-second read-only device soak had 21 samples,
  no errors or reboots, and 48 PC polling deferral warnings (event type 2,
  result 263, detail 4). This event is the PC worker timing out on the shared
  network lock; it is not an internal-memory deferral. Companion monitoring
  was unavailable during the initial boot sample and available in the
  following 20/20 samples; no widget was observed deferred. After startup,
  sampled free PSRAM ranged from 3,610,048 to 3,724,024 bytes.
- **Repeat soak:** after allowing startup to finish, the 300.097-second
  read-only run with `-RequireCompanion -MaxWidgetDeferralSeconds 15` had
  20/20 available PC snapshots, no deferred widgets, errors or reboots, and
  28 bounded PC network-lock warnings. Serial monitoring reported no PC
  request, TLS, watchdog, panic or brownout faults; saved configuration
  remained unchanged. The warning mechanism is documented shared-lock
  contention and was also observed in the validated 0.7.3 release.
- **Release preparation:** the SD bundle was checked against the firmware's
  text, icon, styled-font and web manifests; all 93 firmware notices were
  checked against source, SDK and font notices. Companion frozen bytecode,
  source and portable ZIP were checked against the tested package. These
  prepared artifacts have not been published. An independent build from an
  archive of commit `fbd1606`, with locked managed components and fresh
  defaults, passed with identical resolved SDK configuration and matching
  web pack. Its app is 2,318,928 bytes, leaving 1,875,376 bytes; its image
  digest differs from the preceding installed build. Two preflight attempts
  stopped on corrupt/incomplete serial flash reads before any OTA request.
  A retry using device-side flash MD5 against known, SHA-256-verified images
  installed the clean image by app-only OTA; its prior validated image remains
  the recovery slot. Configuration, HomeKit records and pinned HTTPS were
  preserved. A strict 300.389-second soak of the clean image with
  `-RequireCompanion -MaxWidgetDeferralSeconds 15` had 20/20 available PC
  snapshots, no deferred widgets, errors, reboots or serial faults, and 40
  PC network-lock warnings. Minimum internal free/largest block were
  39,803/19,456 bytes and minimum PSRAM was 3,560,104 bytes.
- **Source publication:** commit `fbd1606` was pushed to the GooPad product
  repository's `main`. Git reported a permission error pruning an unrelated
  worktree administrative directory; the commit and push succeeded.
- **Remaining:** physical action/alarm behavior still needs user testing
  before release; no live configuration or actions were changed for the
  soak. Version 0.7.6 has not been released. GitHub's repository
  social-preview image requires a signed-in settings browser; the available
  browser is signed out.

## 2026-11-25 - Publishing the verified Companion antivirus repair

- **Request/delivery:** completed the antivirus repair as
  [GooPad 0.7.5](https://github.com/jeromedontdev/GooPad/releases/tag/v0.7.5).
  The clean source/tag is `e93032d37e929cfe5ddac6ebc4d832bf3187b410`.
  All ten release asset sizes/SHA-256 values and the remote tag were verified
  before publication; the downloaded installer, portable ZIP, OTA image and
  checksum file match. Latest-stable metadata is 0.7.5. The older 0.7.4
  source updater downloads the exact published installer; the 0.7.5 updater
  reports up to date. The following documentation commit records delivery;
  binaries continue to correspond to the tagged source, not this log update.
- **Antivirus:** final app directory, ZIP and installer, downloaded public
  packages, and installed app/runtime all scan clean (exit 0) with Defender
  intelligence `1.463.463.0`, engine `1.1.26100.4`, protection enabled.
  Executable SHA-256 is
  `db334e0973ef4530986e55e77ec54923a47ef5c5416137c3789deda0644fce65`.
  No post-install Defender detection or application-log error was observed.
  One rejected interrupted/invalid TLS-client connection warning was observed;
  it was not a failed authenticated monitoring request.
- **Tests/distribution:** all 47 Companion tests, Python `py_compile`,
  PowerShell parser, exact packaged source/bytecode/version, directory runtime
  hashes and ZIP CRC/file checks passed. The real frozen legacy caller,
  installer and replacement app passed at the same path after the old
  extraction directory disappeared. The full startup copy then ran without
  the original install directory: 61 samples over 300.03 seconds, zero errors,
  slowest health/monitor/status triplet 78 ms.
- **Installed/live:** the exact new distribution is running from the canonical
  per-user `GooPad\bin` directory, with saved configuration, TLS identity,
  permissions and Windows startup unchanged. Certificate-pinned HTTPS health,
  authenticated status/monitor and unauthenticated-monitor rejection passed.
  A separate read-only 300.02-second live observation had 61 samples, zero
  request errors and a 63 ms maximum triplet; the enabled now-playing sampler
  worked without exposing media metadata or triggering PC actions.
- **Firmware/recovery:** clean production ESP-IDF 6.1 `reconfigure build`
  passed with unchanged resolved 0.7.4 SDK configuration/dependency lock,
  diagnostics OFF and valid image hash/version 0.7.5. Image size is 2,316,320
  bytes, leaving 1,877,984 bytes / 44.77% of the 4 MiB slot. Only the release
  version changed. SD payloads/licenses remain byte-identical to 0.7.4.
  No board was accessed/flashed and no firmware/device soak is claimed.
- **Limits/remaining:** packages remain unsigned; SmartScreen reputation
  warnings are distinct from malware detections. No source-line trigger was
  proven and future definitions/other antivirus products are not certified.
  No exclusions or third-party sample submissions were made. No delivery
  blocker remains; use Microsoft's submission process if a new alert appears.

## 2026-11-25 - Companion antivirus detection and directory packaging

- **Request:** investigate Defender flagging Companion after 0.7.2 and prevent
  the detection without disabling protection.
- **Evidence:** with Defender definitions `1.463.463.0`, the published 0.7.4
  ZIP reports `Trojan:Win32/Bearfoos.B!ml` (scan exit 2); the published 0.7.2
  ZIP does not. Both ZIPs match GitHub's published SHA-256 metadata. The
  installed 0.7.4 executable matches the released executable. Rebuilt
  unchanged 0.7.4 source scans clean with both global and original release
  dependencies, in both one-file and directory layouts (exit 0). Thus no
  particular source line is established as the classifier's trigger.
- **Changes:** 0.7.5 packages a standard PyInstaller directory with `--noupx`,
  rather than a self-extracting application. The installer and optional
  startup copy include `_internal`; packaging rejects stale output directories.
  Sharing, media collection, authentication and user permissions are unchanged.
  The legacy updater reset flag remains for upgrades from older callers.
- **Compatibility:** portable users must extract/retain the entire app folder.
  Configuration, token and paired TLS identity remain outside `bin` and are
  not migrated or erased. Legacy one-file startup copies remain supported.
  The matching firmware version changes only for release metadata; no board
  flash or device soak is required for this Companion-only repair.
- **Validation/delivery:** all 47 Companion unit/integration tests pass,
  including runtime-copy failure preservation and directory-startup copying.
  Python `py_compile`, PowerShell parser and `git diff --check` pass. Pylance
  could not resolve the workspace files; no Pylance result is claimed.
  Final distribution scans, actual legacy upgrade/startup-copy soak, clean
  production build and publication are pending.
- **Limits:** packages remain unsigned. Local clean scans cannot certify all
  antivirus products or future definitions. No exclusions, obfuscation,
  dependency padding or third-party sample uploads were used.

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
  creating thread. All 45 Companion tests and Pylance syntax checks pass.
  The first packaged check
  incorrectly required a non-null initial CPU delta; priming that sample
  corrected the harness, not product code. An intermediate packaged-upgrade
  soak passed before the UI clarification. The final combined-toggle binary
  passed the actual legacy-updater/installer/restart chain with the old
  runtime removed, preserved fixture config/identity, and 61 samples over
  300.06 seconds with zero errors. Slowest local health/monitor/status triplet:
  63 ms. Compiled bytecode confirms the single checkbox and no old controls.
- **Installation:** repaired Companion is running from the stable per-user
  `GooPad\bin` location, with exact release executable bytes, unchanged saved
  config, TLS identity and Windows startup, and trusted HTTPS health plus
  authenticated status/monitor checks. The host installer coordinator first
  waited for the restarted app as well as Setup and reused an earlier
  temporary install directory; waiting only on Setup and explicitly selecting
  the canonical directory corrected that harness, without deleting settings.
- **Release scope:** 0.7.4 firmware changes only its release version
  so latest-stable firmware/Companion asset names remain consistent. No new
  firmware behavior or PSRAM budget changes; all actual SD resource/license
  payloads remain byte-identical to 0.7.3. The clean production firmware build
  is 2,316,320 bytes (1,877,984 bytes / 44.77% slot free), with unchanged resolved
  SDK configuration and diagnostics OFF. The device remains on 0.7.3:
  no unrelated firmware flash or new device soak was performed for this
  Companion-only repair. The five-minute report is a packaged Companion soak.
- **Delivery:** source/tag `v0.7.4` are
  `bda4ca7f1ed28e5a75c6ab44cb8e240c8358e5c0`; the complete
  [stable release](https://github.com/jeromedontdev/GooPad/releases/tag/v0.7.4)
  is published with nine assets. GitHub initially returned HTTP 500 during
  parallel upload; only missing draft assets were resumed and every published
  size/SHA-256 checked. Latest-stable metadata, the old 0.7.3 updater's real
  installer download, the new updater's up-to-date result, and downloaded
  firmware/checksum bytes all pass. Windows packages remain unsigned.
- **Remaining:** no blocker for this request; existing firmware cold-TLS
  latency caveats from 0.7.3 still apply.

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

## 2026-11-25 - Tile editor grouping and display options

- **Request:** simplify tile choices, fix sound guidance and 12-hour time
  formatting, add image-only tiles and Clock alarms.
- **Changes:** grouped existing widgets under Clock, Weather, Market and PC;
  grouped Apple Home and Home Assistant actions/state under Smart home. Renamed
  the action choices and Time heading, removed GitHub from new choices, hid
  action card styles behind filled visuals, clarified sound-only validation,
  and added a no-action image/GIF option. Twelve-hour output omits the leading
  zero; 24-hour output is unchanged.
- **Compatibility:** grouping does not change saved widget identifiers.
  Existing GitHub tiles remain editable. Image-only tiles use the existing
  `simple_link` format with an empty URL and an image; no NVS, SD, Wi-Fi or
  HomeKit migration is required. Rollback retains those records, but older
  editors do not expose the new grouped choices.
- **Validation:** browser editor regression passed 4,929 checks over 2,520
  layouts. `python -m unittest discover -s tests -p test_web_resources.py -v`
  passed all four resource/actual C-loader tests after regenerating the web
  manifest. An intermediate run failed because the generated manifest lagged
  the editor edit; regeneration fixed it. ESP-IDF 6.1 `reconfigure build`
  passed from fresh production defaults with HomeKit, NVS block device, custom
  LVGL allocation and compressed fonts enabled. The development image reports
  0.7.5, is 2,316,256 bytes and leaves 1,878,048 bytes in the 4 MiB app slot.
  The generated web pack matches the final editor source. This build is not a
  new versioned release.
- **Delivery/remaining:** alarms are not implemented: the existing widget
  refresh stops during Wi-Fi loss and UI activity, so it cannot safely serve
  as an alarm scheduler. Implement and test a scheduler independent of UI and
  connectivity, including clock changes, audio contention, restart and power
  loss, before offering alarms. Device identity, pinned credentials and a
  verified recovery slot were not established; no OTA installation, physical
  behavior test or five-minute device soak was performed. No binary release
  is claimed. Source commit `d17014f` was pushed to the GooPad product
  `main` branch and verified remotely. Git reported a permission error while
  trying to prune a separate worktree administrative directory during commit;
  the commit and push succeeded, and that unrelated directory was left alone.

## 2026-11-25 - Daily Clock alarms

- **Request:** complete the Clock alarm option left open by the editor cleanup.
- **Changes:** added daily alarm settings and a dedicated background scheduler,
  independent of UI refresh and Wi-Fi, with stable IDs, same-day in-memory
  deduplication and bounded retries when audio is busy. The tile reports
  disabled, unsynchronized and worker-error states.
- **Compatibility/recovery:** older firmware rejects alarm widgets; remove them
  or restore a compatible backup before downgrading. Alarms require power,
  synchronized time, a correct fixed UTC offset and audible volume; a restart
  during the matching minute may repeat one and missed alarms are not replayed.
  Simultaneous audio, time adjustments or failures may prevent playback. No
  NVS, SD, Wi-Fi or HomeKit migration was performed.
- **Validation:** the Zig host scheduler tests passed, including validation,
  retries, reordering, offsets and midnight boundaries. Browser editor tests
  passed 4,939 checks across 2,520 layouts. All four web resource and actual
  C-loader tests passed. ESP-IDF 6.1 production-config `reconfigure build`
  passed with HomeKit, NVS block device, custom LVGL allocation and compressed
  fonts enabled and hardware AES disabled. The development 0.7.5 app is
  2,318,800 bytes, leaving 1,875,504 bytes in the 4 MiB app slot; its
  generated web archive matches the final manifest.
- **Delivery/remaining:** a generic USB-serial adapter was visible but no
  board identity, pinned HTTPS credentials or valid recovery slot was verified.
  No firmware was installed, physical audio/time behavior tested, or five-minute
  device soak run. Source commit `602671c` was pushed to the GooPad product
  `main` branch. Git again reported a permission error pruning a separate
  worktree administrative directory; the commit and push succeeded and the
  unrelated directory was left alone. No versioned binary release is justified
  until device validation and soak succeed.

## 2026-11-27 - Request results, image cleanup and display widgets

- **Request:** distinguish HTTP/webhook actions, show GET results on tiles,
  delete saved images from About, fix Layered/Modern appearance, offer themes
  on every tile, combine date/time with percent formats, add headlines,
  circular/arched layouts, glass/Material themes and an optional battery reading.
- **Changes:** `http` web requests show bounded response text; `webhook` stays
  send-only. Redirect capture resets and config revisions prevent stale text;
  callback-owning requests are excluded from cached clients. About lists
  recognized images/GIFs/backgrounds and protects saved/draft references.
  Layered surfaces/visuals are centered, Modern has an accent, and all tile
  modes expose presets. Added Date & time, bounded percent formatting with
  owned UI strings and legacy presets, three-title JSON headline aggregation,
  shared non-overlapping ring/arch geometry and glass/Material palettes.
  Battery display defaults off and uses only valid existing board telemetry.
- **Compatibility/recovery:** no storage, pairing or flash-layout migration.
  Existing webhooks and time presets retain their meaning; explicit Custom
  preserves colors. Ring layouts reject enlarged spans. Older firmware does
  not accept new widgets/themes/layout/percent-format values: restore a
  compatible layout before downgrade. Glass is simulated, not blur. Headlines
  support HTTPS JSON Feed/Algolia JSON, not RSS/Atom XML. The Guition has no
  readable battery gauge and reports unavailable rather than inventing a value.
- **Validation:** Zig `-Wall -Wextra -Werror` time and news/layout tests passed,
  including UTF-8/bounds, square-corner circle containment and exact
  non-overlap for 4-16 positions on 240/480-pixel displays. Actual extracted
  HTTP response and image-deletion C handlers passed their Node/Zig harnesses.
  Existing market/layout, alarms, shared Companion config, media gestures and
  GIF normalization tests passed. Browser `runGooPadEditorTests()` passed
  5,081 checks across 2,520 layouts; synthetic visual previews were inspected.
  `python -m unittest discover -s tests -p test_web_resources.py -v` passed
  all four resource/native-loader tests with the final editor manifest.
  A verified public HTTPS request to the default Algolia endpoint returned
  five titled hits within the 64 KiB bound; device polling is not verified.
  Intermediate ring-overlap, widget-centering and harness-cleanup failures
  were fixed and affected tests rerun.
- **Build:** ESP-IDF 6.1 production-defaults `reconfigure build` passed for
  0.7.7, with HomeKit/NVS block device, custom LVGL allocation, compressed
  fonts and TLS performance/session options enabled; hardware AES and
  render/capacity diagnostics off. Initial local SDK config had the custom
  allocator disabled; it was replaced for this build with a separate fresh
  default-seeded config, leaving the user's local config intact. Final app:
  2,326,784 bytes; 1,867,520 bytes free in its 4 MiB slot. Final editor pack
  `web-25b659fcb2c2.gwp` matches source. The actual ESP32-S3 image reports
  0.7.7/IDF 6.1 with a valid checksum and validation hash. Existing FreeRTOS/LVGL deprecation
  warnings remain; no new source warning was introduced. See the
  [storage ledger](storage-layout.md) for the measured image digest.
- **Deployment/soak/publication:** source commit `9e3b3d4` was pushed to
  GooPad product `main` and verified against the remote branch. Board identity,
  certificate-pinned HTTPS
  credentials and a valid live recovery slot were not established; visible
  USB-serial adapters alone are insufficient. No serial reset, firmware/SD
  installation, physical behavior test or five-minute device soak occurred.
  No versioned binary release is justified. Remaining delivery: identify and
  back up the configured device, verify recovery, install matching resources
  and app-only firmware, test actual changed behavior, soak five minutes, then
  prepare a clean-source release. Companion is unchanged; no Companion
  installer build/install is required for this firmware/editor-only change.

## 2026-11-27 - COM9 application-only 0.7.7 installation

- **Request:** flash the new firmware; the user corrected COM4 to COM9.
- **Preflight:** read-only serial diagnostics identified GooPad. ESP32-S3,
  the live 16 MiB layout and validated OTA selectors were verified, then both
  app slots were compared by flash MD5 with known SHA-256-verified 0.7.6
  images. The current image was retained as recovery. The device's stored
  TLS identity and credentials established certificate-pinned HTTPS; private
  pre-update NVS/ownership/config/pairing snapshots stayed local.
- **Installation:** installed the verified 0.7.7 app into the inactive slot
  through application-only OTA, then installed its matching immutable SD web
  archive. No full flash, erase, configuration fixtures or saved action
  triggers were used; old SD resources remain available for recovery.
- **Verification:** pinned HTTPS reports 0.7.7 and the new request/battery
  status fields; the served editor matches source. Config is unchanged.
  Active-image/recovery and logical NVS/pairing comparisons completed before
  the read-only soak, allowing only the expected HomeKit firmware revision
  and one configuration-counter increment (plus reboot advertisement state).
- **Soak:** `tests/Test-GooPadSoak.ps1 -DurationSeconds 300` completed 300.069
  seconds with 21 samples, zero errors/request failures and zero reboots.
  Its 28 warnings were seven early low-memory background deferrals and 21 PC
  network-lock timeout deferrals, not failed foreground requests. These use
  the existing admission/backpressure guards; timeout deferrals were also
  observed before deployment. Internal free memory rose from 25,347 to
  38,147 bytes; the largest block stayed at least 15,360 bytes. After startup,
  PSRAM ranged from 3,457,884 to 3,543,936 bytes and ended at 3,486,576,
  without a sustained downward trend. PC snapshots were available in 20/21
  samples, after the initial startup sample. Config remained unchanged after
  observation. Private flash/NVS snapshots were removed after verification.
- **Delivery/publication:** 0.7.7 is verified active in `ota_0`; the exact
  preceding 0.7.6 image remains in `ota_1` for recovery. Source is published
  on product `main`; the deployment/soak record is included in the scoped
  documentation update. No versioned binary release is claimed: circular
  hardware and all newly configured widgets have not been physically exercised,
  and release packaging requires a separate clean-source build.

## 2026-11-27 - Thumbnail previews, centered Arches, design themes and RSS

- **Request:** show identifiable saved-image thumbnails, replace the two similar
  new-layout choices with Arches, improve Material/Liquid Glass fidelity, and
  provide RSS news with popular presets.
- **Changes:** About and the visual picker share RGB565/RGB565A8/GIF decoding,
  including 480-pixel background thumbnails and explicit load errors. Arches
  stacks bowed rows, centered to within one pixel, with tested square-corner
  circular bounds and separation. Material uses flat Material 3 tonal
  containers, subtle outlines/elevation and pressed state layers, without the
  Modern strip. Glass uses real browser backdrop blur and one shared native
  120x120 RGB565 frosted backdrop; custom backgrounds are downsampled and
  blurred, while the root Glass palette supplies a soft pastel scene.
- **News:** maintained Espressif Expat parses complete RSS 2.0, RSS 1.0/RDF and
  Atom documents, including CDATA/entities/nested title text. DTD/external
  entities, malformed XML, nesting over 32 and responses above 256 KiB are
  rejected. Eight HTTPS presets cover BBC World/Technology, NPR, Guardian,
  NYTimes, Ars Technica, The Verge and Hacker News, plus custom URLs. Legacy
  JSON sources retain their 64 KiB limit. Expat/font notices are embedded and
  included by firmware packaging.
- **Compatibility/recovery:** existing ring/arc and JSON configurations are
  retained. New editor news tiles explicitly default to RSS/BBC World; old
  absent-source news records retain Hacker News JSON. Arches/RSS fields must
  be removed or a compatible configuration restored before downgrading.
  Existing Material/Glass selections intentionally receive corrected styling.
  No live configuration fixtures, saved-action triggers or file deletions were
  used. Root local SDK configuration and partition layout are unchanged.
- **Validation:** the documented Zig native command passed JSON/RSS/Atom,
  exact XML byte/depth limits, malformed tails, DTD/entity rejection, UTF-8
  truncation, centered/non-overlapping circular geometry and actual blur
  regressions. Browser tests passed **7,296 checks across 2,520 layouts**;
  local Material/Arches and Liquid Glass previews were visually inspected.
  All eight live HTTPS responses parsed with three titles, spanning
  **14,506–155,348 bytes** in the host stdin probe. Actual HTTP-response and
  asset-deletion C harnesses passed. All four web-resource tests and
  `tests/Test-ReleasePackaging.ps1` passed. Early host archive/stale-manifest
  checks failed; direct host Expat compilation and resource regeneration
  resolved them before the final passing runs.
- **Build/delivery:** ESP-IDF 6.1 production `reconfigure build` passed for
  **0.7.8**: **2,419,600 bytes**, **1,774,704 bytes / 42.31% free** in the
  4 MiB slot. Matching editor: **322,258 bytes**; exact digests are in the
  storage ledger. Existing FreeRTOS/LVGL warnings remain. COM9 diagnostics,
  validated active 0.7.7 image and partition layout were reverified; an
  app-only recovery copy matched the known SHA-256. Source commit/push and
  pinned application-only installation/soak follow this entry.
- **Remaining/limits:** native Glass approximates frosting, not real-time
  optical refraction. Physical circular hardware and newly configured RSS
  widgets/designs have not been exercised by replacing the user's settings.
  Device installation/soak are pending at this entry. No versioned binary
  release is claimed; separate clean-source packaging/physical validation
  remains required. Companion code, packaging and installation are unaffected.

## 2026-11-27 - 0.7.8 COM9 installation and assessed soak

- **Delivery:** source commit `2f456463da925b95375cb87699e607482747f674`
  is published on product `goopad/main`. Certificate-pinned HTTPS installed
  only the 0.7.8 application and matching SD editor. The verified active image
  is in `ota_1`; the exact preceding 0.7.7 image remains in `ota_0` for
  recovery. Image/editor digests and app-slot headroom match the
  [storage ledger](storage-layout.md).
- **Preservation and device checks:** live partition layout, active/recovery
  images, pinned HTTPS, unchanged saved configuration, primary NVS and TLS
  identity, and preserved HomeKit pairing were verified. HomeKit firmware
  revision changed from 0.7.7 to 0.7.8 with exactly one configuration-counter
  increment. Served editor bytes matched source; three real saved-image/GIF
  responses and the embedded Expat notice were read successfully. No erase,
  live configuration replacement, saved-action trigger or image deletion was
  used.
- **Read-only soak:** **300.183 seconds**, **21 samples**, **zero errors and
  zero reboots**, with valid cached PC snapshots in all 21 samples. Reported
  poll latency was **95-703 ms**. All **25 warnings** were classified: one
  startup internal-memory threshold warning, eight network-admission
  `ESP_ERR_NO_MEM` deferrals (two widget, six PC), and sixteen PC network-lock
  `ESP_ERR_TIMEOUT` deferrals. The latter two are the existing background
  admission/backpressure diagnostic types, not failed foreground requests;
  the soak did not trigger saved actions.
- **Memory assessment:** internal free memory rose from the startup minimum
  **19,535 bytes** to **38,071 bytes** by the second sample and finished at
  **38,071 bytes**. Largest internal block stayed at least **8,192 bytes**:
  lower than the previous soak's 15,360-byte minimum, but above the firmware's
  4,096-byte admission floor. PSRAM fell from **4,506,376** to **3,411,528 bytes**
  during startup; subsequent samples ranged **3,375,844-3,436,772 bytes**,
  ending at **3,408,388 bytes**, without a sustained downward trend.
  This is an observed five-minute bound, not proof of long-term leak freedom
  or sufficient memory for every unexercised configuration.
- **Publication and limits:** installed development firmware and source push
  are verified; no versioned 0.7.8 binary release is published. Clean-source
  release packaging and physical validation of new widgets/designs and
  circular hardware remain outstanding. Native Glass is a frosting
  approximation, not optical refraction. The previous entry's pending status
  records its state before this installation. This documentation-only
  follow-up requires no additional firmware build, flash or soak.

## 2026-11-27 - Circles, consistent surfaces, gradients and typography

- **Request/changes:** retain Circles alongside Arches, repair Glass GIF
  layering and inconsistent Glass/Material application, add two-to-six-stop
  gradient markers/sliders and horizontal/vertical/circle directions, remove
  JSON news selections, normalize dropdown heights, separate headlines, add
  bold/italic, and allow a selectable headline count. The count is 1-5,
  default 3. Choosing Glass/Material or Apply to every tile resets draft
  overrides to Device default; loading saved layouts does not rewrite them.
  Filled GIFs/images remain above Glass frosting and below foreground text.
  Opaque visuals still conceal the surface naturally. Font emphasis is
  synthesized on decoded glyphs, including fallbacks and fitting metrics.
- **Compatibility:** persisted configuration, pairing and SD font formats are
  unchanged. Legacy JSON news sources continue working until explicitly
  replaced with RSS. Legacy two-color gradients remain supported. Older
  firmware ignores new stop/direction/emphasis/count fields; restore compatible
  styling and three-headline behavior before downgrading. Native gradient
  descriptors/font wrappers use PSRAM and outlive their LVGL objects.
  Compile-time guards require at least six configured LVGL stops and complex
  gradient support, preventing an old configuration from overflowing stops.
- **Validation:** browser regression passed **7,320 checks / 2,520 layouts**,
  including draft count preview, Apply to every tile and stable slider input.
  Native RSS/Atom/legacy JSON, count bounds, 635-byte five-title output,
  six/seven-stop boundaries, geometry and blur checks passed. Actual LVGL
  font metrics/pixels/fallback/raw-buffer ownership passed. Extracted native
  GIF layering and multi-stop directional/radial geometry, HTTP responses and
  asset deletion passed. All four web-resource/actual-loader tests and
  isolated release-packaging regressions passed. A wrong HTTP-harness filename
  initially failed; the documented existing harness passed. The initial GCC
  internal compiler crash resolved with unchanged flags, single-job retry and
  a four-job build. An intermediate stale editor manifest failed loader
  validation; final reconfiguration regenerated it and all loader tests passed.
- **Build/delivery:** production ESP-IDF 6.1 reconfigure/build passed for
  **0.7.9**, with the intended SDK configuration and diagnostics off:
  **2,425,328 bytes**, **1,768,976 bytes / 42.18% free**. Matching editor:
  **331,501 bytes**. Exact hashes are in the storage ledger. Source publication,
  COM9 installation/preservation checks and five-minute soak are pending at
  this entry; their subsequent observed results will be appended separately.
- **Remaining:** install/verify/soak the application and matching editor on
  reidentified COM9, then publish delivery documentation. No versioned binary
  release is published; separate clean-source release packaging and physical
  validation of new widgets/designs/circular hardware remain outstanding.
  Synthetic/local tests do not establish physical GIF playback or every
  new live layout; no user configuration fixtures were installed.

## 2026-11-27 - 0.7.9 COM9 installation and assessed soak

- **Delivery:** source commit
  `a831845fddc9593b2d346323852e336e5f552c30` was pushed to the verified
  GooPad product remote. Reidentified COM9 as the CH340-connected ESP32-S3,
  checked the live partition table and validated OTA selector, and verified
  exact active 0.7.8/recovery 0.7.7 images before installation. Only the
  application and matching web resource were updated. Verified 0.7.9 active
  in `ota_0`, with exact preceding 0.7.8 retained in `ota_1`, comparing
  device-side MD5 with SHA-256-verified images. The served editor matches the
  committed source. Pinned HTTPS, saved configuration, primary NVS, TLS
  identity, SD mount, and logical HomeKit pairing/ownership records remained
  intact. HomeKit firmware revision advanced 0.7.8 to 0.7.9 with exactly one
  configuration-counter increment; only expected reboot advertisement state
  is excluded from logical comparisons. Private snapshots stayed in memory.
- **Soak:** the normal preserved configuration passed a **300.139-second**
  certificate-pinned, read-only soak: **24 samples, zero errors, zero reboots,
  34 warnings**. There was one startup internal-memory warning, 15 network
  admission `ESP_ERR_NO_MEM` deferrals (three widget, twelve PC), and eighteen
  PC `ESP_ERR_TIMEOUT` shared-network-lock deferrals. These are the same
  diagnosed warning classes as 0.7.8, not request failures or new error types.
  Cached Companion data was available in all 24 samples; no widget was
  observed deferred. SD Unicode/styled resources remained available and
  their metadata/bitmap caches stayed within the soak's checked budgets.
- **Memory assessment:** internal free rose from **17,379** to **37,915 bytes**
  (a brief later low was 35,527); the largest block rose from **7,680** to
  **15,360 bytes** after startup and stayed there. Initial PSRAM fell from
  **4,254,652** to **3,456,768 bytes**; after 30 seconds the range was
  **3,380,052-3,425,204**, ending at **3,380,052 bytes** without a sustained
  downward trend. This is five-minute evidence, not proof of long-term leak
  freedom. No live fixtures, saved actions or config replacements were used.
- **Publication/remaining:** installed application/editor and source push are
  verified. No versioned 0.7.9 binary release is published; clean-source
  release packaging and physical validation of new widgets/designs/circular
  hardware remain outstanding. Actual GIF playback/new emphasis/gradient
  pixels on a newly configured physical tile were not exercised; native
  composition/geometry/glyph and browser regressions supply the current
  evidence. Native Glass remains a frosting approximation. This subsequent
  documentation-only record needs no additional build, flash or soak.

## 2026-11-28 - Consistent interface capitalization

- **Request/changes:** review grammar and capitalization throughout the
  editor, tile controls, touchscreen, recovery page and Companion. Headings,
  choices and controls use title case; descriptions and messages retain
  sentence case. Updated help and README references to changed labels.
- **Compatibility:** presentation only. Saved action values, existing tile
  names, user text, configuration, pairing and SD resources are not
  rewritten. Firmware 0.7.10 requires its matching editor resource; Companion
  is versioned 0.7.7. Downgrading restores earlier interface wording.
- **Validation:** browser regression passed 7,322 checks across 2,520 layouts,
  including unchanged mixed-case user names and saved action values. Four
  web-resource/loader tests and isolated firmware packaging regressions passed.
  The isolated Companion suite passed 47 tests, including GUI controls; Python
  `py_compile` passed. Pylance syntax checks could not locate workspace files;
  the isolated Python tests and compiler supplied validation instead.
  Production ESP-IDF 6.1 reconfigure/build passed with 2,425,328 bytes used
  and 1,768,976 bytes free. The companion package/installer built successfully;
  the actual frozen older-updater/installer/restart chain passed 61 samples in
  300 seconds, preserving test configuration and identity. Defender custom
  scans of the package, ZIP and installer completed with no recent detections
  (signatures 1.463.504.0, engine 1.1.26100.4).
- **Delivery/remaining:** firmware, matching editor and Companion are built
  but deployment and source publication are pending at this entry. No
  versioned binary release is published. Physical screen fit and the installed
  Companion update remain to be verified; do not overwrite existing user
  settings or pairing. See the storage ledger for build hashes.

## 2026-11-28 - 0.7.10 COM9 installation and assessed soak

- **Delivery:** source commit `443c56cb693bdd56d9cce7d4d309c671008808c3`
  was pushed to the GooPad product remote. Reidentified COM9 and verified its
  chip, live partition table, validated selector and exact 0.7.9 active and
  0.7.8 recovery images. Updated only the app and matching editor resource.
  Verified 0.7.10 in `ota_1` and exact 0.7.9 recovery in `ota_0`. Pinned HTTPS,
  configuration, primary NVS, SD mount and HomeKit pairing/ownership remained
  intact; the HomeKit version and configuration counter advanced as expected.
  Private snapshots were kept in memory.
- **Soak:** the preserved configuration completed a 300.367-second read-only
  certificate-pinned soak: 20 samples, zero errors and zero reboots. Its 44
  warnings comprised one startup internal-memory threshold warning, 17
  low-memory network-admission deferrals (four widget, thirteen PC) and 26 PC
  shared-network-lock timeouts. These are the same diagnostic classes observed
  on 0.7.9, not foreground request failures. Companion data was unavailable
  in the initial boot sample and available in the remaining 19; no widget was
  observed deferred. After the first two startup samples, internal free ranged
  from 35,575 to 41,415 bytes, the largest block stayed at 7,936 bytes and
  free PSRAM ranged from 3,369,380 to 3,421,616 bytes. The sample window does
  not establish long-term memory behavior; monitor if live data lags.
- **Remaining:** the actual physical display text fit was not visually checked.
  The isolated frozen Companion upgrade passed, but the existing running user
  Companion was not replaced or interrupted in the shared environment. No
  0.7.10 binary release was published: a release still needs a clean-checkout
  build from a fresh configuration seeded by the defaults, matching package
  provenance and review of release assets/digests. Preserve user settings and
  pairing when completing the real Companion installation.

## 2026-11-28 - Editor workspace and preview geometry

- **Request/changes:** redesigned the browser editor as a cleaner responsive
  workspace with persistent navigation, a clearer layout and an empty editing
  state. The browser's website themes remain independent of the device theme.
  Corrected preview grid padding, gaps and spanned-cell sizes to match firmware
  integer geometry; corrected radial pill content dimensions and empty-tile
  opacity. Browser fonts, live data, animations and hardware effects still
  make the preview an approximation.
- **Compatibility:** no saved configuration or API format changes. Firmware
  0.7.11 requires its matching editor resource; downgrading restores the older
  editor along with the prior firmware and resource pack.
- **Validation:** local browser suite passed 7,339 checks across 2,520 layouts;
  desktop and mobile layouts were inspected and all three views checked for
  overflow at 320, 375, 768, 1024 and 1440 pixels in six website themes.
  An intermediate web loader test failed because the editor was changed while
  its manifest was being built; regeneration followed by all four web-resource
  and real-loader tests passed. Final ESP-IDF 6.1 production reconfigure/build
  and all four resource/loader tests passed again after geometry refactoring.
  See the storage ledger for final sizes and hashes.
- **Attribution:** GitHub linked existing commits to a different account because
  the repository-local Git email belonged to that account. The local identity
  was corrected to the authenticated product account's ID-based no-reply
  address. Existing published history was not rewritten; changing those
  attributions requires a coordinated history rewrite and force-push.
- **Delivery/remaining:** device installation, read-only soak, source push and
  any binary release are pending at this entry. No live configuration fixtures
  or physical rendering comparisons were used.

## 2026-11-28 - 0.7.11 installation interrupted verification

- **Delivery:** source commit `cc1bcc42183ecf85c8a2676606efda7e01b313a1`
  was pushed to the GooPad product remote; GitHub attributed both its author
  and committer to the authenticated product account. Reidentified COM9 and
  verified its chip, partition table, validated selector and exact 0.7.10
  active and 0.7.9 recovery images. The app-only OTA and matching editor upload
  succeeded. Immediately after OTA, pinned HTTPS reported 0.7.11, the served
  editor matched source, and the saved configuration matched its pre-update
  hash. The subsequent flash inspection verified the new app and preceding
  recovery image.
- **Unresolved preservation check:** after initialization the logical primary
  NVS comparison reported a difference. The helper stopped before comparing
  HomeKit records or starting the soak. It did not persist private snapshots,
  so the pre-update NVS comparison cannot be repeated from those snapshots and
  the differing record and cause are unknown. Do not assume either data loss
  or benign changes; configuration was only verified immediately after OTA.
  Further device writes, release publication and a success claim are blocked
  pending investigation. No five-minute soak or physical preview comparison was
  completed for 0.7.11.
- **Attribution/remaining:** future commits use the corrected repository-local
  identity, as verified on GitHub. Previously published commits remain linked
  to the other account. Changing them requires a coordinated rewrite of shared
  history, including consideration of branches, tags and downstream clones; no
  history was rewritten or force-pushed. No 0.7.11 binary release was published.

## 2026-11-28 - Control redesign and historical attribution

- **Request/changes:** replace the dropdown-heavy tile editor with modern
  controls and correct older GitHub attribution. Actions and visual types use
  descriptive choice cards; common widget, shape, style and request choices
  use compact native radio groups. Longer lists remain dropdowns. The editor
  checkout is pinned to LF to keep the generated resource hash portable.
- **Compatibility:** the controls retain the existing configuration values,
  conditional behavior and legacy actions/widgets; no migration is introduced.
  Version 0.7.12 requires its matching editor resource. Downgrading restores
  the older editor with its own pack and retains the existing downgrade limits.
- **Attribution:** corrected repository-local identity and rewrote the
  product's 90 commits and 23 branch/tag refs. Verified trees, topology,
  messages and dates before the atomic lease-protected push; GitHub links all
  90 main-branch authors and committers to the product account. The 22 release
  records and asset identities, sizes and digests remained unchanged. Existing
  clones must reconcile rewritten history and tags; historical hashes in this
  log still name old objects. The original mirror and commit map remain in the
  session archive; retain a separate backup for long-term recovery.
- **Validation:** local browser suite passed 7,349 checks across 2,520 layouts;
  keyboard Space/arrow navigation and focus after a Smart Home rebuild passed.
  Thirty responsive checks across five widths and six website themes found no
  document or choice-control overflow. Four web-resource/real-loader tests
  passed. ESP-IDF 6.1 production `reconfigure build` passed; image version,
  app-slot headroom and matching web manifest are recorded in the storage ledger.
- **Delivery/remaining:** rewritten history and tags are on the product remote;
  source commit `dffad986e743531bc723292e951efec0cfcf152b` was pushed there.
  The 0.7.12 image is not installed or released. Device writes, HomeKit
  verification and a five-minute device soak
  remain blocked by the unexplained 0.7.11 primary-NVS difference recorded
  above; the missing pre-update snapshot prevents identifying that change.
  Investigate preservation before another deployment or binary publication.

## 2026-11-28 - Hardware-test attribution follow-up

- **Request/changes:** the user still saw the other account as a contributor.
  The separate hardware-test repository had six commits with the same incorrect
  author and committer email; the earlier product-only rewrite had missed it.
  Rewrote its own six-commit history, `main`, and lightweight `v1.0.0` tag,
  without pushing the GooPad history to that repository.
- **Validation:** verified every tree, parent relationship, message and date
  before an atomic, lease-protected push. GitHub's commit API identifies all six
  rewritten authors as the product account; the release record and asset
  identity, size and digest remained unchanged. The contributor API initially
  still listed the old account after the push. GitHub says contributor displays
  can take about 24 hours to refresh after a rewrite.
- **Compatibility/delivery:** hardware-test clones must reconcile the rewritten
  history and tag. Its pre-rewrite mirror and commit map remain in the session
  archive. No firmware or device changes were made in this follow-up.
- **Remaining:** recheck the hardware-test contributor listing after the cache
  refresh and contact GitHub Support if it remains incorrect. The built 0.7.12
  editor is still not installed: the prior unexplained primary-NVS difference
  and missing pre-update snapshot continue to block a safe device update.
