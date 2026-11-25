const assert = require("node:assert/strict");
const {execFileSync} = require("node:child_process");
const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");

const ui = fs.readFileSync(path.join(__dirname, "..", "main", "goopad_ui.c"), "utf8");
const callback = ui.slice(ui.indexOf("static void media_cb("), ui.indexOf("\nstatic void page_navigation_cb("));
assert(callback.startsWith("static void media_cb("));
const dir = fs.mkdtempSync(path.join(os.tmpdir(), "goopad-media-test-"));
const source = path.join(dir, "test.c"), executable = path.join(dir, process.platform === "win32" ? "test.exe" : "test");
try {
 fs.writeFileSync(source, `#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
typedef int esp_err_t;
typedef int lv_event_code_t;
enum { ESP_OK, LV_EVENT_PRESSED, LV_EVENT_LONG_PRESSED, LV_EVENT_CLICKED, LV_EVENT_RELEASED, LV_EVENT_LONG_PRESSED_REPEAT };
typedef struct { int code; uintptr_t index; } lv_event_t;
static bool s_media_long_press[32];
static unsigned toggles, skips, logs, last_index;
static esp_err_t result;
static void *lv_event_get_user_data(lv_event_t *e) { return (void *)e->index; }
static int lv_event_get_code(lv_event_t *e) { return e->code; }
static esp_err_t goopad_trigger(unsigned i) { toggles++; last_index=i; return result; }
static esp_err_t goopad_media_skip(unsigned i) { skips++; last_index=i; return result; }
#define ESP_LOGW(...) (logs++)
${callback}
static void send(int code) { lv_event_t e={code,7}; media_cb(&e); }
int main(void) {
 send(LV_EVENT_PRESSED); send(LV_EVENT_CLICKED);
 assert(toggles==1 && skips==0 && last_index==7);
 send(LV_EVENT_PRESSED); send(LV_EVENT_LONG_PRESSED);
 send(LV_EVENT_LONG_PRESSED_REPEAT); send(LV_EVENT_RELEASED); send(LV_EVENT_CLICKED);
 assert(toggles==1 && skips==1);
 send(LV_EVENT_PRESSED); send(LV_EVENT_RELEASED);
 assert(toggles==1 && skips==1);
 send(LV_EVENT_PRESSED); send(LV_EVENT_CLICKED);
 assert(toggles==2 && skips==1);
 result=99; send(LV_EVENT_PRESSED); send(LV_EVENT_LONG_PRESSED); send(LV_EVENT_CLICKED);
 assert(toggles==2 && skips==2 && logs==1);
 return 0;
}`);
 execFileSync("zig", ["cc", "-Wall", "-Wextra", "-Werror", source, "-o", executable], {stdio:"inherit"});
 execFileSync(executable, [], {stdio:"inherit"});
 console.log("Actual firmware media callback: tap, hold, release, scroll cancellation and error tests passed");
} finally {
 for (const file of [source, executable, path.join(dir, "test.pdb")]) if (fs.existsSync(file)) fs.unlinkSync(file);
 fs.rmdirSync(dir);
}
