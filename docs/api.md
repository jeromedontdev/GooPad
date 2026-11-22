# API and data contracts

The source is authoritative: see `start_server()` and each handler in
[`main/goopad.c`](../main/goopad.c), the save/asset functions in
[`main/index.html`](../main/index.html), and the Companion `Handler` in
[`companion/GooPadCompanion.py`](../companion/GooPadCompanion.py). This
document is a navigation aid, not a versioned external API guarantee.

## Trust boundaries

The device serves the editor over HTTPS on port 443. `/` serves the editor;
the listed `/api/` handlers call `auth_required()` and require HTTP Basic
authentication as `admin`. A supplied `Origin` must match the request's HTTPS
host. The device certificate is self-signed and contains `goopad.local` and
the IP used at creation; protect and verify its identity on trusted networks.
Do not print, commit, or share real credentials, configuration exports, tokens,
certificates with private keys, or flash dumps.

The Companion defaults to HTTPS on port 8787. `/health` is unauthenticated;
other listed routes require `X-GooPad-Token`, a private/loopback peer, and no
browser `Origin`. GooPad verifies the paired public certificate with the fixed
identity `companion.goopad`; public HTTPS actions use the certificate bundle.
PowerShell and system controls require separate local opt-ins. HomeKit uses
encrypted HAP on TCP port 80, **not** a plaintext editor.

## Device routes

| Method and path | Purpose and important behavior |
| --- | --- |
| `GET /` | Embedded editor HTML. |
| `GET /api/config` | Complete saved JSON, including potentially sensitive configuration. `Cache-Control: no-store`. |
| `POST /api/config` | Complete JSON, max 12,000 bytes. Validate, commit to NVS, update RAM; returns `{"ok":true}` or also `"restarting":true` for changed HomeKit topology. HTTP 500 can mean a config was saved but restart scheduling failed; inspect the response rather than retrying blindly. |
| `GET /api/status` | Runtime connection, render counts, audio, last action, cached PC stats and live-widget strings. `lastAction` is the latest tracked HTTP action, not a universal completion record for every tile type. |
| `POST /api/trigger` | `{"index":0}` uses the **saved**, zero-based macro index and performs the real action. HTTP 429 indicates a busy path; acceptance is not necessarily action completion. |
| `POST /api/wifi/join` | `{"ssid":"...","password":"..."}` validates input and starts a connection attempt; an HTTP success is not proof the join succeeded. |
| `POST /api/password` | Changes the admin password; inspect `password_post_handler()` for current/initial password rules. |
| `GET /api/backup`, `POST /api/backup` | Read the existing SD config backup, or overwrite it with the current config. Neither operation backs up assets or HomeKit keys. |
| `POST /api/ota`, `POST /api/ota/sd` | Upload an app-only binary or install `/sdcard/goopad.bin`. Successful activation schedules a reboot; interrupted/invalid transfers are rejected. |
| `GET /api/icons` | With no query, returns the gzip index; `q=` searches, or `name=` plus `style=` selects a glyph. Search results are bounded. |
| `POST`, `GET`, `DELETE /api/icon?key=...` | Upload, read or delete an SD asset. Upload supports RGB565/alpha, GPR1 RLE and GIF; GET selects format/size via query. Referenced macro icons cannot be deleted. |
| `POST`, `DELETE /api/sound` | Upload/delete SD WAV assets. Deletion checks references. |
| `DELETE /api/background` | Remove the configured background reference/asset via the dedicated handler. |
| `POST /api/time` | Set a bounded Unix epoch supplied by the editor. Network time is also started on connection. |
| `POST /api/controls` | Change and persist brightness/volume; physical output remains capped by the board constants. |
| `POST /api/companion` | Check an HTTPS Companion endpoint with a provided token and public certificate. Legacy direct `/monitor` proxy requests are rejected; reload the editor and use cached status. |
| `GET`, `POST /api/homekit` | If compiled in: bridge status (`active`, `starting`, `error`), or request a temporary pairing code. Pairing is unavailable when already paired. Treat the response as a secret. |

Asset keys are `i` plus 16 lowercase hex digits. The editor uploads an image
before saving its key in the layout. RGB565A8 assets contain a little-endian
RGB565 plane followed by an 8-bit alpha plane. GPR1 encodes runs of a
little-endian 16-bit count, two RGB565 bytes and one alpha byte after its
header. See `packIconRle()` and `decode_icon_rle()` together before changing
the format. GIFs are validated, capped to 160x160/128 KiB/240 frames and
normalized to at least 100 ms per frame.

## Configuration model

The root contains grid dimensions (`cols`, `rows`), appearance, optional page
and visibility settings, optional Companion pairing fields, time offset, and a
`macros` array. The implementation accepts up to 32 positions and 2–4 rows
and columns. The exact accepted fields, defaults, combinations and bounds
are in `validate_config()`. Empty positions are valid.

Common macro fields include `name`, `icon`, `actionMode`, `action`, `method`,
`url`, `body`, `secret`, `sound`, colors and shape. Mode-specific fields cover
widgets, PC steps, Home Assistant and HomeKit. Browser-only `imagePreview`,
`pcSteps` and legacy `tileName` are stripped when saving; a PC sequence is
serialized into the HTTP request body. `homekitId` is a stable `s` plus 14
lowercase hex digits; `homekitName` is 1–24 UTF-8 bytes. `iconColor` is an
optional six-digit hex color for built-in symbols and Font Awesome; absence
preserves the prior appearance.

Compatibility matters: older saved modes such as `simple_link`,
`pc_open_url`, and `page_navigation` may still be read even though not all are
available for new tiles. Add optional fields with sensible old-config
behavior, keep the frontend and validator aligned, and test upgrades and
round trips. Do not add a required property to existing records without a
migration strategy.

## Companion routes

| Method and path | Behavior |
| --- | --- |
| `GET /health` | Unauthenticated liveness. |
| `GET /status` | Authenticated last result, busy state, PowerShell permission and configured action IDs. |
| `GET /monitor` | Authenticated PC stats; sensor values can be unavailable or cached. |
| `POST /run` | JSON `{"steps":[...]}`; validates every step and permissions before execution. Returns 202 when queued or 429 if a sequence is already running. Inspect `/status` for the eventual result. |
| `POST /open` | Opens a validated HTTP(S) URL on the PC; retained for existing layouts. |
| `POST /hook/<id>` | Launches a locally configured program with fixed arguments and `shell=False`. |

`validate_steps()` is authoritative for allowed step types, keys, sizes,
ordering, and local opt-ins. A successful `/run` response means *accepted*,
not that every step succeeded. Do not add a step only to the frontend: validate
and execute it in Companion, review its security implications, and test
permission denial, malformed inputs and failure reporting.
