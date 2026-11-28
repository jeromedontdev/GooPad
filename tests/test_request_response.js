const assert = require("node:assert/strict");
const {execFileSync} = require("node:child_process");
const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");

const firmware = fs.readFileSync(path.join(__dirname, "..", "main", "goopad.c"), "utf8").replace(/\r\n/g, "\n");
const capture = firmware.slice(firmware.indexOf("static esp_err_t macro_response_event("), firmware.indexOf("\ntypedef struct {\n    esp_http_client_handle_t client;"));
const result = firmware.slice(firmware.indexOf("static void action_result("), firmware.indexOf("\nstatic void action_finished("));
assert(capture.startsWith("static esp_err_t macro_response_event(") && result.startsWith("static void action_result("));
const dir = fs.mkdtempSync(path.join(os.tmpdir(), "goopad-request-test-"));
const source = path.join(dir, "test.c"), executable = path.join(dir, process.platform === "win32" ? "test.exe" : "test");
try {
 fs.writeFileSync(source, `#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "goopad_utf8.h"
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
 connected(&action); data(&action,"temperature: ",13); data(&action,"42",2);
 action_result(&action,200,ESP_OK,0);
 assert(!strcmp(s_request_text[7],"temperature: 42") && !s_request_text[0][0]);
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
 execFileSync(executable, [], {stdio:"inherit"});
 console.log("Actual HTTP response capture/result: chunks, errors, redirects, bounds, UTF-8 and stale-layout tests passed");
} finally {
 for (const file of [source, executable, path.join(dir, "test.pdb")]) if (fs.existsSync(file)) fs.unlinkSync(file);
 fs.rmdirSync(dir);
}
