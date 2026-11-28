const assert = require("node:assert/strict");
const {execFileSync} = require("node:child_process");
const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");

const ui = fs.readFileSync(path.join(__dirname, "..", "main", "goopad_ui.c"), "utf8").replace(/\r\n/g, "\n");
const start = ui.indexOf("lv_obj_t *gif = lv_gif_create(tile);");
const end = ui.indexOf("} else {\n                                lv_obj_delete(gif);", start);
assert(start >= 0 && end > start);
const gif = ui.slice(start, end) + "}";
const gradientStart = ui.indexOf("static void custom_tile_gradient(");
const gradientEnd = ui.indexOf("\nstatic lv_obj_t *graphic_box(", gradientStart);
assert(gradientStart >= 0 && gradientEnd > gradientStart);
const gradient = ui.slice(gradientStart, gradientEnd);
const repo = path.join(__dirname, "..");
const dir = fs.mkdtempSync(path.join(os.tmpdir(), "goopad-visual-test-"));
try {
 const source = path.join(dir, "test.c"), executable = path.join(dir, "test.exe");
 fs.writeFileSync(source, `#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "goopad_gradient.h"
typedef struct {
 struct {uint32_t color; unsigned opa,frac;} stops[6];
 unsigned stops_count;
 int direction,x,y,edge_x,edge_y;
} lv_grad_dsc_t;
typedef struct { int order; bool paused; const lv_grad_dsc_t *gradient; } lv_obj_t;
#define MAX_MACROS 32
#define MALLOC_CAP_SPIRAM 1
#define MALLOC_CAP_8BIT 2
#define LV_OPA_COVER 255
#define LV_GRAD_EXTEND_PAD 0
#define ESP_LOGE(...) ((void)0)
static lv_grad_dsc_t *s_tile_gradients;
static void *heap_caps_calloc(size_t n,size_t size,unsigned caps) {(void)caps;return calloc(n,size);}
static uint32_t color(uint32_t rgb) {return rgb;}
static void lv_grad_radial_init(lv_grad_dsc_t *g,int x,int y,int edge_x,int edge_y,int extend) {
 assert(extend==LV_GRAD_EXTEND_PAD);g->direction=2;g->x=x;g->y=y;g->edge_x=edge_x;g->edge_y=edge_y;
}
static void lv_grad_horizontal_init(lv_grad_dsc_t *g) {g->direction=1;}
static void lv_grad_vertical_init(lv_grad_dsc_t *g) {g->direction=0;}
static void lv_obj_set_style_bg_grad(lv_obj_t *o,const lv_grad_dsc_t *g,int part) {(void)part;o->gradient=g;}
${gradient}
enum { LV_ALIGN_CENTER, LV_ALIGN_TOP_MID, LV_ALIGN_TOP_LEFT, LV_EVENT_READY };
static lv_obj_t object;
static lv_obj_t *lv_gif_create(lv_obj_t *tile) { (void)tile; object=(lv_obj_t){.order=5}; return &object; }
static void lv_obj_set_clickable(lv_obj_t *o,bool b) { (void)o;(void)b; }
static void lv_gif_set_src(lv_obj_t *o,void *s) { (void)o;(void)s; }
static bool lv_gif_is_loaded(lv_obj_t *o) { (void)o;return true; }
static void gif_loop_cb(void) {}
static void lv_obj_add_event_cb(lv_obj_t *o,void (*cb)(void),int e,void *d) { (void)o;(void)cb;(void)e;(void)d; }
static void lv_obj_set_style_border_post(lv_obj_t *o,bool b,int s) { (void)o;(void)b;(void)s; }
static void lv_image_set_scale(lv_obj_t *o,int scale) { (void)o;assert(scale>0); }
static void lv_obj_align(lv_obj_t *o,int a,int x,int y) { (void)o;(void)a;(void)x;(void)y; }
static int goopad_widget_image_top(int y,int size,int height) { return y+(size-height)/2; }
static void lv_obj_move_to_index(lv_obj_t *o,unsigned index) { o->order=index; }
static void lv_obj_move_background(lv_obj_t *o) { o->order=0; }
static void lv_gif_pause(lv_obj_t *o) { o->paused=true; }
static lv_obj_t *s_gif_objects[32];
static int s_gif_images[32],s_gif_pages[32];
static struct {unsigned gif_count;} g_app;
static void render(bool cover,unsigned surface_layers,bool widget_icon) {
 lv_obj_t parent,*tile=&parent;
 bool image_drawn=false,action_card=false;
 int index=0,page=1,icon_size=80,w=64,h=64,width=160,height=120,widget_icon_y=8,icon_y=0;
 ${gif}
 assert(image_drawn && object.paused && s_gif_objects[0]==&object && s_gif_pages[0]==1);
 assert(object.order==(cover?(int)surface_layers:5));
}
int main(void) {
 render(true,2,false);render(true,2,true);render(true,0,false);render(false,2,false);
 assert(g_app.gif_count==4);
 cJSON *macro=cJSON_Parse("{\\"gradientStops\\":[{\\"color\\":\\"FF0000\\",\\"position\\":0},{\\"color\\":\\"00FF00\\",\\"position\\":40},{\\"color\\":\\"0000FF\\",\\"position\\":100}]}");
 lv_obj_t tile={0};
 custom_tile_gradient(&tile,macro,0,160,120);
 assert(tile.gradient->direction==0 && tile.gradient->stops_count==3);
 assert(tile.gradient->stops[0].frac==0 && tile.gradient->stops[1].frac==102 && tile.gradient->stops[2].frac==255);
 assert(tile.gradient->stops[1].color==0x00ff00 && tile.gradient->stops[1].opa==255);
 cJSON_AddStringToObject(macro,"gradientDirection","horizontal");
 custom_tile_gradient(&tile,macro,0,160,120);assert(tile.gradient->direction==1);
 cJSON_ReplaceItemInObjectCaseSensitive(macro,"gradientDirection",cJSON_CreateString("circle"));
 custom_tile_gradient(&tile,macro,0,160,120);
 assert(tile.gradient->direction==2 && tile.gradient->x==80 && tile.gradient->y==60 &&
        tile.gradient->edge_x==160 && tile.gradient->edge_y==120);
 cJSON_Delete(macro);free(s_tile_gradients);
 puts("Actual GIF layering and native multi-stop horizontal/vertical/radial gradient geometry passed");
}`);
 const json = path.join(repo,"managed_components","espressif__cjson","cJSON");
 execFileSync("zig", ["cc","-I",path.join(repo,"main"),"-I",json,source,path.join(json,"cJSON.c"),"-o",executable], {stdio:"inherit"});
 execFileSync(executable, [], {stdio:"inherit"});
} finally {
 fs.rmSync(dir, {recursive:true,force:true});
}
