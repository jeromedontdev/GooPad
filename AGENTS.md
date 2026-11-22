# Guidance for future GooPad sessions

## Starting a session

- Read the user's current request and relevant conversation or handoff context.
  Check `git status -sb` and preserve existing changes. Inspect the affected
  code and tests before relying on documentation or earlier conclusions.
- Start with the [architecture map](docs/architecture.md),
  [development and recovery guide](docs/development.md), and
  [API and data contracts](docs/api.md); use the [README](README.md) for
  user-facing behavior. The source and observed behavior are authoritative.
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
- For firmware changes, build and, when feasible, safely flash and test the
  attached device. Test Companion changes with the Companion suite as well.
  Report exactly what was tested and what could not be verified. Documentation
  or comment-only changes do not need an otherwise unnecessary device flash.
- Never assume a serial port, running slot or device address. Preserve NVS,
  HomeKit pairing and SD data; prefer an app-only OTA update on a configured
  device. The config backup does not include all of those assets and keys.
  Follow the recovery guide before any operation that could overwrite them.

## Closing and handoff

- Summarize the result, relevant test outcomes, device/flash status, and any
  remaining limitations or next steps. Separate observed facts from hypotheses
  so the user and a subsequent agent can distinguish them.
- When a commit or publication is requested, inspect the staged diff and
  destination remote; do not include unrelated changes or secrets. The GooPad
  product remote is distinct from the original hardware-test repository.
- If work must continue in another session, leave a concise, non-secret
  handoff in the available session mechanism: current state, evidence, changes,
  exact remaining steps and relevant paths. Durable project knowledge belongs
  in the maintained guides, not only in a chat or temporary session.
