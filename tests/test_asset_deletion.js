const assert = require("node:assert/strict");
const {execFileSync} = require("node:child_process");
const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");

const root = path.join(__dirname, "..");
const firmware = fs.readFileSync(path.join(root, "main", "goopad.c"), "utf8").replace(/\r\n/g, "\n");
const keyStart = firmware.indexOf("static bool valid_icon_key(const char *key)\n{");
const key = firmware.slice(keyStart, firmware.indexOf("\nstatic bool valid_hex_color(", keyStart));
const handler = firmware.slice(firmware.indexOf("static esp_err_t icon_delete_handler("), firmware.indexOf("\n#ifdef CONFIG_GOOPAD_HOMEKIT\nstatic esp_err_t homekit_handler("));
assert(keyStart >= 0 && handler.endsWith("}\n"));
const dir = fs.mkdtempSync(path.join(os.tmpdir(), "goopad-assets-test-"));
const source = path.join(dir, "test.c"), executable = path.join(dir, process.platform === "win32" ? "test.exe" : "test");
try {
 fs.writeFileSync(source, `#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
typedef int esp_err_t;
typedef struct { const char *query; int status; } httpd_req_t;
enum { ESP_OK, HTTPD_400_BAD_REQUEST=400, HTTPD_404_NOT_FOUND=404, HTTPD_500_INTERNAL_SERVER_ERROR=500 };
static const char *s_config;
static unsigned deletes;
static int remove_error;
static char last_path[128];
#define BSP_SD_MOUNT_POINT "fixture"
#define portMAX_DELAY 0
#define xSemaphoreTake(...) ((void)0)
#define xSemaphoreGive(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
static int auth_required(httpd_req_t *req) { (void)req; return ESP_OK; }
static int httpd_req_get_url_query_str(httpd_req_t *req,char *out,size_t size) { snprintf(out,size,"%s",req->query); return ESP_OK; }
static int httpd_query_key_value(const char *query,const char *name,char *out,size_t size) {
 if(strcmp(name,"key")||strncmp(query,"key=",4))return -1;
 snprintf(out,size,"%s",query+4);return ESP_OK;
}
static int httpd_resp_send_err(httpd_req_t *req,int code,const char *text) { (void)text; req->status=code; return code; }
static int httpd_resp_set_status(httpd_req_t *req,const char *text) { (void)text; req->status=409; return 0; }
static int httpd_resp_set_type(httpd_req_t *req,const char *text) { (void)req;(void)text;return 0; }
static int httpd_resp_sendstr(httpd_req_t *req,const char *text) { (void)req;(void)text;return 0; }
static int mock_remove(const char *file) {
 deletes++;snprintf(last_path,sizeof(last_path),"%s",file);
 if(remove_error){errno=remove_error;return -1;}return 0;
}
#define remove mock_remove
${key}
${handler}
int main(void) {
 httpd_req_t req={"key=i0000000000000001",0};
 s_config="{\\"backgroundImage\\":\\"i0000000000000001\\",\\"macros\\":[]}";
 icon_delete_handler(&req);assert(req.status==409&&deletes==0);
 s_config="{\\"macros\\":[{\\"iconImage\\":\\"i0000000000000001\\"}]}";
 req.status=0;icon_delete_handler(&req);assert(req.status==409&&deletes==0);
 s_config="{\\"macros\\":[]}";
 req.status=0;assert(icon_delete_handler(&req)==ESP_OK&&deletes==5);
 assert(!strcmp(last_path,"fixture/goopad/icon-i0000000000000001.480.rgb565"));
 deletes=0;req.query="key=../../private";icon_delete_handler(&req);assert(req.status==400&&deletes==0);
 req.query="key=i0000000000000001";remove_error=EIO;icon_delete_handler(&req);assert(req.status==500&&deletes==1);
 s_config="invalid";deletes=0;icon_delete_handler(&req);assert(req.status==500&&deletes==0);
 return 0;
}`);
 const cjson = path.join(root, "managed_components", "espressif__cjson", "cJSON");
 execFileSync("zig", ["cc", "-Wall", "-Wextra", "-Werror", "-I", cjson, source, path.join(cjson, "cJSON.c"), "-o", executable], {stdio:"inherit"});
 execFileSync(executable, [], {stdio:"inherit"});
 console.log("Actual asset deletion handler: saved image/background protection, key bounds and errors passed");
} finally {
 for (const file of [source, executable, path.join(dir, "test.pdb"), path.join(dir, "test.lib")]) if (fs.existsSync(file)) fs.unlinkSync(file);
 fs.rmdirSync(dir);
}
