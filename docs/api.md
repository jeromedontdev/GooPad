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
| `GET /` | Pinned SD editor HTML, streamed in bounded chunks. Gzip clients receive stored gzip; identity clients receive stored plain HTML, without whole-editor decompression/allocation. Missing/damaged resources return HTTP 503 with an embedded recovery page. |
| `GET /preview-font?name=lobster` | Public, whitelisted SD WOFF2 preview font. Existing names/24-hour cache policy are preserved. Unknown names return 404; missing/damaged resources return 503. |
| `GET /api/config` | Complete saved JSON, including potentially sensitive configuration. `Cache-Control: no-store`. |
| `POST /api/config` | Complete JSON, max 12,000 bytes. Validate, commit to NVS, update RAM; returns `{"ok":true}` or also `"restarting":true` for changed HomeKit topology. HTTP 500 can mean a config was saved but restart scheduling failed; inspect the response rather than retrying blindly. |
| `GET /api/status` | Runtime connection, installed `firmwareVersion`, render counts, audio, last action, cached PC stats and live-widget strings. `requestResults` contains per-macro cached response text; `batteryAvailable` and nullable `batteryPercent` expose validated board telemetry. `lastAction` is the latest tracked HTTP action, not a universal completion record for every tile type. |
| `GET /font-licenses` | Public bundled-font notices/SIL Open Font License and Expat XML parser MIT notice. |
| `GET /api/diagnostics` | Authenticated, no-store report of numeric reset, memory and bounded event records. No configuration, URLs, credentials, SSIDs, location or widget data are included. Events are `[uptimeSeconds,event,result,detail]`; event types are boot (1), network deferred (2), macro result (3), widget request result (4), Wi-Fi disconnect (5), macro latency in milliseconds from queueing to result (6), and serial PC test latency (7; result is HTTP status or negative ESP error). Uptime resets at each boot; RTC-retained records can span soft resets but not power loss. |
| `POST /api/trigger` | `{"index":0}` uses the **saved**, zero-based macro index and performs the real action. Network and wake actions use a FIFO with one running and up to four waiting; HTTP 429 means the queue is full or unavailable. Acceptance is not action completion. |
| `POST /api/wifi/join` | `{"ssid":"...","password":"..."}` validates input and starts a connection attempt; an HTTP success is not proof the join succeeded. |
| `POST /api/password` | Changes the admin password; inspect `password_post_handler()` for current/initial password rules. |
| `GET /api/backup`, `POST /api/backup` | Read the existing SD config backup, or overwrite it with the current config. Neither operation backs up assets or HomeKit keys. |
| `GET /api/update`, `POST /api/update` | Check the latest stable release (`version`, `available`); POST `{"version":"0.7.0"}` to install that checked version if still newer. GooPad checks GitHub over verified HTTPS, restricts the download redirect, verifies the release SHA-256 and image version, then activates the inactive slot. The request can take time and requires internet access and a correct clock. |
| `POST /api/ota`, `POST /api/ota/sd` | Upload an app-only binary or install `/sdcard/goopad.bin`. Successful activation schedules a reboot; interrupted/invalid transfers are rejected. |
| `GET /api/icons` | With no query, returns the embedded gzip index; `q=` searches the matching SD catalog, or `name=` plus `style=` selects a glyph. Search results are bounded; missing/invalid SD catalog fails explicitly. |
| `POST /api/resources?kind=text\|icons\|web\|styled&sha256=<64-lowercase-hex>` | Authenticated streaming upload of a resource matching the compiled full SHA-256. Requires mounted SD; 96 MiB maximum. Uses staging, integrity/signature checks and atomic rename. Identical content is reused; conflicting immutable content returns 409. Font installation requires reboot. Web requests verify individual asset hashes before serving. |
| `GET /api/saved-icons?offset=0` | Authenticated SD asset list, up to 24 recognized tile images/GIFs, with a nullable next offset. Add `backgrounds=1` to include recognized 480-pixel backgrounds for the About image manager; the default excludes backgrounds. Unrelated files are always excluded. |
| `GET /api/icon-match?format=...&size=...&hash=...&offset=0` | Authenticated SHA-256 lookup for normalized 160-pixel RGB565A8/GIF icons or 480-pixel RGB565 backgrounds. Returns a matching `key` or a nullable `next` offset; scans at most 16 matching files per page. |
| `POST`, `GET`, `DELETE /api/icon?key=...` | Upload, read or delete an SD asset. Upload supports RGB565/alpha, GPR1 RLE and GIF; GET selects format/size via query. Saved macro icons and the root `backgroundImage` cannot be deleted (409). Deletion removes recognized variants only, including 480-pixel RGB565. |
| `POST`, `DELETE /api/sound` | Upload/delete SD WAV assets. Deletion checks references. |
| `DELETE /api/background` | Remove the configured background reference/asset via the dedicated handler. |
| `POST /api/time` | Set a bounded Unix epoch supplied by the editor. Network time is also started on connection. |
| `POST /api/controls` | Change and persist brightness/volume; physical output remains capped by the board constants. |
| `POST /api/companion` | Check an HTTPS Companion endpoint with a provided token and public certificate. Legacy direct `/monitor` proxy requests are rejected; reload the editor and use cached status. |
| `GET`, `POST /api/homekit` | If compiled in: bridge status (`active`, `starting`, `error`), or request a temporary pairing code. Pairing is unavailable when already paired. Treat the response as a secret. |

Asset keys are `i` plus 16 lowercase hex digits. The editor stages newly selected
images in browser memory, then searches SD for matching normalized data on
Save and uploads only new referenced assets before saving their keys in the
layout. A failed configuration save can leave unreferenced
assets on SD; they can be reused. RGB565A8 assets contain a little-endian
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

`screenLayout` is `grid` (default), `arches`, or legacy `ring`/`arc`.
Circles selects `ring`; Arches uses stacked bowed rows, maintaining column/row
positions on partial pages. Ring/arc rendering is unchanged; only `arc` requires
opening an existing layout to select it. Non-grid layouts preserve macro indices/paging,
keep square corners inside a circular viewport, and reject spans larger than 1x1.
`showBattery` is boolean and defaults false; unsupported boards report
`batteryAvailable:false` and `batteryPercent:null`, never an estimated value.
Root and tile themes additionally accept `liquid_glass` and `material`.
`widgetTheme:"device"` or absence inherits those new root palettes; explicit
`custom` retains custom colors. Older root themes retain historical behavior.
Choosing Material/Glass in the editor or using Apply to every tile explicitly
sets all draft `widgetTheme` values to `device`; loading a saved configuration
does not silently rewrite its overrides.

`gradientStops` optionally contains two to six `{color:"RRGGBB", position:integer}`
records, strictly increasing from 0 to 100. `gradientDirection` is `vertical`
(default), `horizontal`, or `circle` (center outward to the farthest corner).
Custom tiles use these stops for any action/widget; presets retain priority.
Without stops, legacy `tileGradient`/`widgetGradient` and default shading remain
unchanged. Endpoints cannot move; interior positions use whole percentages.

For `actionMode:"advanced"`, `action:"http"` displays the bounded response
after a GET or POST; `action:"webhook"` is send-only. Capture is limited to
191 bytes, control characters are sanitized, UTF-8 is validated, and
truncation is marked `...`. Non-2xx, transport and empty-body results replace
stale success text. Results are volatile; a successful config save clears them
and prevents queued results from populating a remapped layout.

Clock/date retain `clockFormat`/`dateFormat` and their legacy preset keys.
`widget:"datetime"` uses `datetimeFormat`, default `%Y-%m-%d%n%H:%M`.
Percent formats are at most 96 UTF-8 bytes and support
`%%`, `%a`, `%A`, `%b`, `%B`, `%d`, `%e`, `%H`, `%I`, `%m`, `%M`, `%S`,
`%p`, `%P`, `%j`, `%u`, `%w`, `%Y`, `%y`, `%F`, `%T`, `%R`, `%D`, `%r`,
`%n`, `%t`. Unpadded numeric directives use `%-d`, `%-H`, `%-I`, `%-m`,
`%-M`, `%-S`, `%-j`, `%-y`. Names are English; the fixed `timeOffsetMinutes`
applies, not daylight-saving timezone rules. Unsupported directives such as
`%z`, control characters other than newline/tab, invalid UTF-8 and overlong
formats are rejected.

New editor news tiles use `widget:"news", newsSource:"rss"` and
`newsFeed:"bbc_world"` (default). Preset IDs are `bbc_world`, `bbc_tech`,
`npr`, `guardian`, `nytimes`, `ars`, `verge` and `hackernews`.
`newsFeed:"custom"` requires an HTTPS `newsUrl` shorter than 180 characters
without URL credentials. RSS 2.0, RSS 1.0/RDF and Atom are supported through
Expat, with a 256 KiB response limit and nesting limit of 32. Channel/feed
titles are ignored; CDATA, XML entities and nested Atom title text are decoded.
DTD/external-entity processing is rejected and the complete XML document must
be valid, even after the requested titles have been collected. `newsCount` is an
integer from 1 to 5, default 3. Duplicate XML headlines
are omitted; each title is capped at 120 UTF-8 bytes plus an ellipsis.
Empty feeds show `No headlines`; entries without usable titles fail explicitly.
Headlines are separated by `\n---\n`; the PSRAM live-text cache is 640 bytes per
position, enough for five 123-byte truncated titles and four separators.
The browser/device reading paths retain the full cache. Small tiles may need
fewer headlines or larger spans to display all text legibly.

Legacy `newsSource:"hackernews"` (also the absent-field default for old saved
configurations) and `"jsonfeed"` retain their behavior.
The editor no longer offers JSON source selections. A saved legacy tile shows
an unselected RSS-feed control and a preservation hint until the user explicitly
chooses a replacement feed; saving other edits does not change its source.
JSON Feed requires an HTTPS `newsUrl` shorter than 180 characters and without
URL credentials. The default source uses Algolia's front-page Hacker News
endpoint. The background worker polls every ten minutes, with a 64 KiB response
limit and verified TLS; redirects are not followed. It reads the requested number of
`items[].title` or `hits[].title` strings, validates UTF-8, flattens control
whitespace and limits each title to 120 bytes plus an ellipsis. Empty arrays
show `No headlines`; invalid feeds and failed requests show an error, and old
results become stale. The same refresh/TLS/error rules apply to RSS/Atom;
clickable article navigation is absent.
New widget/theme/layout/percent-format values are not downgrade-compatible:
remove them or restore a compatible configuration before installing older firmware.

Common macro fields include `name`, `icon`, `actionMode`, `action`, `method`,
`url`, `body`, `secret`, `sound`, colors and shape. Mode-specific fields cover
widgets, PC steps, Home Assistant and HomeKit. Browser-only `imagePreview`,
`pcSteps` and legacy `tileName` are stripped when saving; a PC sequence is
serialized into the HTTP request body. `homekitId` is a stable `s` plus 14
lowercase hex digits; `homekitName` is 1–24 UTF-8 bytes. `iconColor` is an
optional six-digit hex color for built-in symbols and Font Awesome; absence
preserves the prior appearance.

Optional tile text fields are `font` (`montserrat`, `lato`, `cousine`,
`breeserif`, `abrilfatface`, `anton`, `archivoblack`, `lobster`, `pacifico`,
`sharetechmono`, `righteous`, `acme`, `bangers`, `patrickhand`,
`pressstart2p`, or `spacemono`),
`textSize` (12, 14, 16, 20, 28, or 40), `textAlign` (`left`, `center`,
`right`), `textDecoration` (`none`, `underline`, `strike`), and `textSpacing`
(integer 0–4 pixels). Widget readings use `widgetTextSize` independently.
`fontBold` and `fontItalic` are independent optional booleans, default false.
The device synthesizes one-pixel bold dilation and a one-in-four-pixel slant on
decoded A8 glyphs, including fallback fonts; expanded metrics participate in
fitting. This does not add fonts to the SD pack. Older firmware ignores these
fields, multi-stop gradients/directions and `newsCount`.
The optional `widgetLayout` field applies to every tile: `"classic"`,
`"panel"` (Modern), or `"layered"`; absence defaults to Modern for compatibility,
while newly created tiles explicitly use `"classic"`. Small or
curved tiles simplify the layout, and full-bleed GIFs hide surface styling.
Firmware predating Layered rejects `"layered"` configurations, so change those
tiles to Classic or Modern before downgrading.
Optional integer `spanCols` and `spanRows` specify a tile's width and height in
grid cells (default 1). The macro's existing array index is its top-left
anchor; spans must stay within the page and the first 32 positions and cannot
cover another configured tile or span. Covered empty records are preserved.
Enlarged tiles cannot be combined with `hideUnconfigured: true`. Firmware
predating spans does not render them and must not be used with such layouts.
Font sizes fit down to available space. `widgetGraphic: "custom_icon"` combines
any widget with its selected icon or image, replacing weather's automatic
condition graphic when explicitly selected; the existing `"icon"`
badge remains unchanged. `visualFit` accepts `"contain"` or `"cover"` for
the selected visual. When absent, action GIFs retain their legacy `gifFit`
behavior; other visuals default to contain. Pomodoro `focusMinutes` and `breakMinutes` accept
whole minutes from 1 through 180 and default to 25 and 5 when absent.
The `alarm` widget requires a unique `alarmId` (`a` plus 14 lowercase hex
digits), `alarmTime` (`HH:MM` in 24-hour time), boolean `alarmEnabled`, and
`alarmSound` (`alert`, `chime`, or `beep`). A background task checks saved
alarms against synchronized time and the fixed `timeOffsetMinutes`; it retries
busy audio every five seconds within the matching minute. Same-day deduplication
is in RAM, not persistent: restarting during that minute can repeat an alarm,
and missed alarms are not replayed. Power loss, unsynchronized or adjusted
time, volume and audio contention can prevent playback. Firmware predating
alarms rejects `widget: "alarm"`; remove those tiles or restore a compatible
backup before downgrading.
The `exchange_rate` widget requires different uppercase three-letter
`currencyBase` and `currencyQuote` codes. Frankfurter is polled every six hours;
the displayed date belongs to the provider's reference rate.
The `weather_alerts` widget uses `weatherLatitude` and `weatherLongitude` for
US NWS active alerts. Each tile refreshes at most once per minute, with at
least 30 seconds between NWS requests across tiles. Responses are
bounded to 64 KiB, and overflow or errors report unavailable rather than no
alerts. Results older than three minutes are marked stale. It is not a
reliable sole emergency-warning system.

The `home_state` widget uses `homeAddress`, `homeEntity`, `homeToken`, and
the HTTPS `url` for the entity state endpoint. The token is sent in a Bearer
header over verified HTTPS without redirects and is included in backups.
The `now_playing` widget uses the paired Companion `/monitor` endpoint; the
Companion shares media metadata only when enabled locally.

PC macros, legacy PC-open actions and PC monitor/now-playing widgets may use
`sharedPc: true`. Their `url` is the mode-specific path (`/run`, `/open` or
`/monitor`) and `secret` is empty; firmware resolves both from the root
`companionAddress` and `companionToken` and requires the paired certificate.
Older per-tile URLs and tokens remain supported. Older firmware cannot resolve
`sharedPc`; export a compatible backup before downgrading, and re-save the
pairing and affected tiles on older firmware.

Market tiles use `widget: "stock"` or `"crypto"` and `marketSymbol`. Stocks
accept `stockProvider: "finnhub"` with `finnhubKey`, or `"alphavantage"` with
`stockKey`; a missing provider means Alpha Vantage for compatibility. Keys are
separate and both can remain in the configuration when switching. Finnhub sends
its key in `X-Finnhub-Token` over verified HTTPS without redirects and polls once
per minute per tile; Alpha Vantage polls every six hours. Both keys are sensitive
and included in configuration exports and backups. Crypto uses Coinbase.

`wake_on_lan` requires `wakeMac`: six hexadecimal octets separated consistently
by colons or hyphens, representing a nonzero unicast MAC address. The editor
stores uppercase colon-separated octets. On trigger the device broadcasts a
standard 102-byte magic packet on UDP port 9 to its connected Wi-Fi IPv4 subnet.
`lastAction` reports packet transmission or failure, not whether the target woke.
Delivery across subnets and through client isolation is not guaranteed.
Older firmware does not recognize this action; remove Wake-on-LAN tiles before
downgrading and keep a compatible configuration backup.

Compatibility matters: older saved modes such as `simple_link`,
`pc_open_url`, and `page_navigation` may still be read even though not all are
available for new tiles. Add optional fields with sensible old-config
behavior, keep the frontend and validator aligned, and test upgrades and
round trips. Do not add a required property to existing records without a
migration strategy.

The editor's Clock, Weather, Market, PC and Smart home groups are presentation
only: they retain the existing mode and widget values above. Wake-on-LAN is
shown under Widgets → PC but remains an actionable `wake_on_lan` record, not
a live widget. Image-only tiles
save as `simple_link` with an empty URL and an image asset, and do not execute
an action. New GitHub profile widgets are no longer offered, but saved
`github` widgets remain supported. Twelve-hour time formatting omits a leading
zero; the stored `clockFormat` keys are unchanged.

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

## Display capabilities and now-playing input

`GET /api/status` additionally reports `board`, `screenWidth`, `screenHeight`,
`maxCols` and `maxRows`. The editor uses these to size previews and limit grid
density; older firmware without these fields retains the 480x480/4x4 baseline.
These describe the compiled backend, not automatic controller detection.

A normal trigger of a saved `now_playing` widget sends a pinned, authenticated
Companion `/run` request with one `media`/`play_pause` step. A device long press
sends `media`/`next`; it does not send a second play/pause on release. Other
display-only widgets retain their prior behavior. Widget configuration remains
the `/monitor` endpoint; input builds a separate request and never rewrites it.

Network-deferral diagnostics use detail 1 (macro), 2 (manual PC diagnostic),
3 (live widget) or 4 (PC monitor/now-playing). The original detail 0 remains
ambiguous. Result 263 is lock contention, not proof the outbound HTTP request
timed out; result 257 is insufficient internal-memory headroom.
