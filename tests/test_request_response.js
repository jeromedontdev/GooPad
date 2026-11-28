const assert = require("node:assert/strict");
const {execFileSync} = require("node:child_process");
const fs = require("node:fs");
const http = require("node:http");
const os = require("node:os");
const path = require("node:path");

const firmware = fs.readFileSync(path.join(__dirname, "..", "main", "goopad.c"), "utf8").replace(/\r\n/g, "\n");
const capture = firmware.slice(firmware.indexOf("static esp_err_t macro_response_event("), firmware.indexOf("\ntypedef struct {\n    esp_http_client_handle_t client;"));
const result = firmware.slice(firmware.indexOf("static void action_result("), firmware.indexOf("\nstatic void action_finished("));
const requestText = firmware.slice(firmware.indexOf("void goopad_request_text("), firmware.indexOf("\nstatic esp_err_t macro_response_event("));
const ui = fs.readFileSync(path.join(__dirname, "..", "main", "goopad_ui.c"), "utf8").replace(/\r\n/g, "\n");
const updateWidgets = ui.slice(ui.indexOf("static void update_widgets(void)"), ui.indexOf("\nstatic lv_color_t color(", ui.indexOf("static void update_widgets(void)")));
assert(capture.startsWith("static esp_err_t macro_response_event(") && result.startsWith("static void action_result(") && requestText.startsWith("void goopad_request_text(") && updateWidgets.startsWith("static void update_widgets(void)"));
const dir = fs.mkdtempSync(path.join(os.tmpdir(), "goopad-request-test-"));
const source = path.join(dir, "test.c"), executable = path.join(dir, process.platform === "win32" ? "test.exe" : "test");
async function main() {
 let server;
try {
 fs.writeFileSync(source, `#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "goopad_utf8.h"
#ifdef _WIN32
static struct tm *gmtime_r(const time_t *value, struct tm *out) { *out=*gmtime(value);return out; }
#endif
#define MAX_MACROS 32
typedef int esp_err_t;
enum { ESP_OK, HTTP_EVENT_HEADERS_SENT, HTTP_EVENT_ON_DATA };
typedef struct { int event_id; void *user_data; char *data; int data_len; } esp_http_client_event_t;
typedef struct {
 bool diagnostic, display_response, response_truncated, wake_on_lan;
 unsigned config_revision, index, serial;
 int64_t queued_at;
 char response[192];
 size_t response_size;
} macro_action_t;
static char s_request_text[32][192], s_last_action_message[160];
static unsigned s_config_revision=1, s_action_serial=1;
static int s_last_action_status, s_last_action_tls_error;
static bool s_last_action_running, s_last_action_success;
static int64_t esp_timer_get_time(void) { return 1000000; }
static const char *esp_err_to_name(int err) { (void)err; return "TEST_ERROR"; }
static size_t strlcpy(char *out, const char *in, size_t size) {
 size_t length=strlen(in);
 if(size) { size_t n=length<size-1?length:size-1; memcpy(out,in,n); out[n]=0; }
 return length;
}
#define portENTER_CRITICAL(...) ((void)0)
#define portEXIT_CRITICAL(...) ((void)0)
#define goopad_diag_record(event,result,detail) ((void)(detail))
${capture}
${result}
${requestText}
typedef struct { char text[768]; } lv_obj_t;
static lv_obj_t *s_pages, *s_info_overlay, *s_battery_label, *s_gif_objects[32], *s_widget_labels[32];
static int s_gif_pages[32], s_page_width, s_time_offset, s_widget_types[32], s_widget_stats[32];
static bool s_gif_paused[32], s_page_scrolling, s_menu_animating, s_stopwatch_running[32], s_pomodoro_break[32], s_alarm_enabled[32], s_widget_fahrenheit[32];
static int64_t s_countdown_targets[32], s_stopwatch_elapsed[32], s_stopwatch_started[32];
static int s_break_minutes[32], s_focus_minutes[32];
static const char *s_alarm_times[32], *s_widget_formats[32];
static struct { struct { bool present; int level_pct; } power; } g_app;
static int lv_obj_get_scroll_x(lv_obj_t *o) { (void)o; return 0; }
static void lv_gif_pause(lv_obj_t *o) { (void)o; }
static void lv_gif_resume(lv_obj_t *o) { (void)o; }
static const char *lv_label_get_text(lv_obj_t *o) { return o->text; }
static void lv_label_set_text(lv_obj_t *o, const char *text) { strlcpy(o->text,text,sizeof(o->text)); }
static void invalidate_macro_page(unsigned i) { (void)i; }
static void fit_widget_text(unsigned i,const char *text) { (void)i;(void)text; }
static void update_widget_graphic(unsigned i,const struct tm *now,bool synced) { (void)i;(void)now;(void)synced; }
static void update_weather_icon(unsigned i) { (void)i; }
static esp_err_t hw_audio_play_sound(const char *sound,void *arg) { (void)sound;(void)arg;return ESP_OK; }
static bool goopad_alarm_ready(void) { return true; }
static bool goopad_time_text(char *out,size_t size,const char *format,const struct tm *now) { (void)out;(void)size;(void)format;(void)now;return false; }
static void goopad_live_widget_text(unsigned i,char *out,size_t size) { (void)i;strlcpy(out,"Live",size); }
static void goopad_pc_stat_text(int stat,bool f,char *out,size_t size) { (void)stat;(void)f;strlcpy(out,"PC",size); }
#define ESP_LOGW(...) ((void)0)
${updateWidgets}
static void data(macro_action_t *action, char *bytes, int size) {
 esp_http_client_event_t e={HTTP_EVENT_ON_DATA,action,bytes,size};
 assert(macro_response_event(&e)==ESP_OK);
}
static void connected(macro_action_t *action) {
 esp_http_client_event_t e={HTTP_EVENT_HEADERS_SENT,action,NULL,0};
 macro_response_event(&e);
}
int main(void) {
 macro_action_t action={.display_response=true,.config_revision=1,.index=7,.serial=1};
 char get_response[128]={0}; size_t received=fread(get_response,1,sizeof(get_response)-1,stdin);
 assert(received==15 && !strcmp(get_response,"temperature: 42"));
 connected(&action); data(&action,get_response,13); data(&action,get_response+13,(int)received-13);
 action_result(&action,200,ESP_OK,0);
 assert(!strcmp(s_request_text[7],"temperature: 42") && !s_request_text[0][0]);
 lv_obj_t tile={0}; s_widget_labels[7]=&tile; s_widget_types[7]=18;
 update_widgets(); assert(!strcmp(tile.text,"temperature: 42"));
 connected(&action); data(&action,"{\\"value\\":42}",12); action_result(&action,200,ESP_OK,0);
 assert(!strcmp(s_request_text[7],"{\\"value\\":42}"));
 action_result(&action,500,ESP_OK,0); assert(!strcmp(s_request_text[7],"HTTP 500\\nRequest rejected"));
 action_result(&action,0,99,0); assert(strstr(s_request_text[7],"Request failed"));
 connected(&action); action_result(&action,204,ESP_OK,0); assert(!strcmp(s_request_text[7],"HTTP 204\\nNo response body"));
 char big[300]; memset(big,'x',sizeof(big));
 data(&action,big,sizeof(big)); action_result(&action,200,ESP_OK,0);
 assert(action.response_truncated && strlen(s_request_text[7])<192 && strstr(s_request_text[7],"..."));
 connected(&action); data(&action,"next",4); action_result(&action,200,ESP_OK,0);
 assert(!strcmp(s_request_text[7],"next"));
 connected(&action); memset(big,'x',sizeof(big)); big[186]=(char)0xe2; big[187]=(char)0x82; big[188]=(char)0xac;
 data(&action,big,sizeof(big)); action_result(&action,200,ESP_OK,0);
 assert(strlen(s_request_text[7])==189 && goopad_utf8_valid(s_request_text[7],189));
 connected(&action); char unicode[]={ (char)0xe2,(char)0x82,(char)0xac };
 data(&action,unicode,1); data(&action,unicode+1,2); action_result(&action,200,ESP_OK,0);
 assert(goopad_utf8_valid(s_request_text[7],3));
 connected(&action); char binary[]={(char)0xff}; data(&action,binary,1);
 action_result(&action,200,ESP_OK,0); assert(!strcmp(s_request_text[7],"Response is not UTF-8 text"));
 connected(&action); char controls[]={'a',0,'b','\\n'}; data(&action,controls,4);
 action_result(&action,200,ESP_OK,0); assert(!strcmp(s_request_text[7],"a b\\n"));
 s_config_revision++; strcpy(s_request_text[7],"new layout");
 action_result(&action,200,ESP_OK,0); assert(!strcmp(s_request_text[7],"new layout"));
 action.config_revision=s_config_revision; action.display_response=false;
 action_result(&action,200,ESP_OK,0); assert(!strcmp(s_request_text[7],"new layout"));
 return 0;
}`);
 execFileSync("zig", ["cc", "-Wall", "-Wextra", "-Werror", "-I", path.join(__dirname, "..", "main"), source, "-o", executable], {stdio:"inherit"});
 server = http.createServer((request, response) => {
  if (request.method !== "GET" || request.url !== "/reading") { response.writeHead(404).end(); return }
  response.writeHead(200, {"Content-Type":"text/plain"}).end("temperature: 42");
 });
 await new Promise((resolve,reject)=>{server.once("error",reject);server.listen(0,"127.0.0.1",resolve)});
 const response=await fetch(`http://127.0.0.1:${server.address().port}/reading`);
 assert.equal(response.status,200);
 execFileSync(executable, [], {input:await response.text(), stdio:["pipe","inherit","inherit"]});
 console.log("Loopback GET fed actual firmware response capture and LVGL widget update; errors, bounds, UTF-8 and stale-layout tests passed");
} finally {
 if (server) await new Promise((resolve,reject)=>server.close(error=>error?reject(error):resolve()));
 for (const file of [source, executable, path.join(dir, "test.pdb")]) if (fs.existsSync(file)) fs.unlinkSync(file);
 fs.rmdirSync(dir);
}
}
main().catch(error=>{console.error(error);process.exitCode=1});
