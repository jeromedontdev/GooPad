# Guidance for future GooPad sessions

## Starting a session

- Read the user's current request and relevant conversation or handoff context.
  Check `git status -sb` and preserve existing changes. Inspect the affected
  code and tests before relying on documentation or earlier conclusions.
- Start with the [architecture map](docs/architecture.md),
  [development and recovery guide](docs/development.md), and
  [API and data contracts](docs/api.md). Read the latest relevant entries in
  the [agent change log](docs/change-log.md); use the [README](README.md) for
  user-facing behavior. The source and observed behavior are authoritative.
- For flash/app-slot or PSRAM capacity questions, read the measured
  [storage layout and application ledger](docs/storage-layout.md); distinguish
  app slots, persistent configuration, SD files and runtime memory.
- Use tokens and the user's time carefully: focus on the requested work, reuse
  existing patterns, and give concise updates. Explain problems in plain
  language, including what is broken, its impact, and the next step.

## Keeping documentation useful

- In the same change, update the relevant guide and user-facing README when
  behavior, setup, APIs, configuration, architecture, build, testing,
  security, or recovery changes. Update code comments when an invariant or
  ownership rule changes; explain why rather than narrating obvious code.
- Check examples, links, commands, limitations and cross-references against
  the implementation. Preserve distinctions between verified behavior and
  assumptions, especially for hardware, Apple Home and older ESP-IDF versions.
  Do not claim a test or feature works without evidence.
- Keep guides navigable and current rather than appending a session transcript
  or duplicating entire specifications. Do not put credentials, pairing
  codes, private keys, personal configurations or flash dumps in documentation,
  commits, logs shared with others, or chat summaries.

## Implementation and verification

- Make focused changes, retain saved-configuration compatibility, and test
  the behavior changed. Follow the build, editor and Companion test
  instructions in the development guide; run relevant tests rather than
  treating a successful build as proof of device behavior.
- Treat code changes as an end-to-end delivery, not just an edit. Complete
  the delivery checklist below; explicitly report blocked or inapplicable
  steps rather than silently skipping them or claiming success.
- Never assume a serial port, running slot or device address. Preserve NVS,
  HomeKit pairing and SD data; prefer an app-only OTA update on a configured
  device. The config backup does not include all of those assets and keys.
  Follow the recovery guide before any operation that could overwrite them.

## Closing and handoff

- Maintain [docs/change-log.md](docs/change-log.md) in every change. Append a
  dated, concise entry with the request, affected components, behavior changes,
  compatibility/recovery implications, exact validation and results, deployment,
  soak and publication status, and remaining work. Do not rewrite earlier
  entries to conceal failures. Distinguish implemented, built, installed,
  committed and released; these are not interchangeable.
- Summarize the result, relevant test outcomes, device/flash status, and any
  remaining limitations or next steps. Separate observed facts from hypotheses
  so the user and a subsequent agent can distinguish them.
- Commit and publish completed changes by default. Inspect the staged diff,
  branch and destination remote; do not include unrelated changes or secrets.
  The GooPad product remote is distinct from the original hardware-test
  repository. Never use a blanket `git add .` on a dirty worktree. If a build
  includes pre-existing uncommitted changes that cannot safely be included in
  the release, report that provenance blocker rather than publishing binaries
  as if they matched the committed source.
- If work must continue in another session, leave a concise, non-secret
  handoff in the available session mechanism: current state, evidence, changes,
  exact remaining steps and relevant paths. Durable project knowledge belongs
  in the maintained guides, not only in a chat or temporary session.

## Delivery checklist for code changes

1. Inspect current state, identify the exact requirement and reproduce defects
   where possible. Preserve concurrent user edits, credentials and pairing.
2. Implement focused changes and regressions. Update directly related
   documentation and the change log, including migration and downgrade limits.
3. Run relevant native, editor and Companion tests, type/syntax checks and
   available lint. Review the diff for correctness, sensitive data and scope.
4. For firmware or embedded-editor changes, run the production
   `reconfigure build` with the intended SDK config. Verify the embedded editor,
   app-slot fit/headroom and firmware version. Companion-only changes require
   its tests and packaging checks, not an unrelated firmware flash.
5. For device changes, identify the attached board and live partition layout,
   verify certificate-pinned HTTPS and a valid recovery slot, then safely
   install the application-only image. Never erase NVS, SD, HomeKit storage or
   OTA metadata to make deployment work. Verify the active image, HTTPS,
   unchanged configuration, and the actual changed behavior after reboot.
6. Run the [read-only soak test](tests/Test-GooPadSoak.ps1) against the updated
   device for five minutes by default after relevant code changes, not once
   per chat session. Use a longer soak when the user asks or when resets,
   memory trends, timing/concurrency changes or unresolved failures warrant it;
   explain the chosen duration. Review its report for resets,
   request failures, memory trends and new diagnostic errors; investigate
   findings and repeat relevant validation after fixes. Use normal user
   operation during observation; do not trigger actions or replace the live
   configuration with fixtures without specific permission. Synthetic harness
   tests and short smoke tests do not count as a completed device soak.
7. For Companion changes, build and verify the distributable and installer as
   applicable; preserve existing pairing and settings during installation.
8. Commit the intended source, tests, documentation and log; push to the
   verified GooPad product remote. For deployable changes, prepare and publish
   a new versioned GitHub release only after validation and soak pass. Keep
   version fields, tags, OTA assets, applicable Companion packages, notices
   and release digests consistent with the committed source. Do not overwrite
   an existing release's binaries as a shortcut.
9. Verify published assets and update metadata, and record the outcome in the
   change log. If validation, device access, credentials, publishing rights or
   build provenance prevents completion, record the exact blocker and remaining
   steps. Do not bypass identity checks or manufacture test results.

Documentation/instruction-only changes still need documentation and a log
entry, review, commit and push, but do not require a firmware build, flash,
binary release or device soak. Honor explicit user requests to pause, avoid
commits/publication, or limit the scope.
