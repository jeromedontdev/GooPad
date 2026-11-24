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
