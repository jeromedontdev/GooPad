const assert = require("node:assert/strict");
const {execFileSync, spawnSync} = require("node:child_process");
const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");
const vm = require("node:vm");

const root = path.join(__dirname, "..");
const firmware = fs.readFileSync(path.join(root, "main", "goopad.c"), "utf8");
const editor = fs.readFileSync(path.join(root, "main", "index.html"), "utf8");
const cFunction = firmware.slice(firmware.indexOf("bool goopad_prepare_gif("), firmware.indexOf("\nstatic bool read_string("));
const jsFunction = editor.slice(editor.indexOf("function canonicalGif("), editor.indexOf("\nfunction prunePendingIconAssets("));
assert(cFunction.startsWith("bool goopad_prepare_gif(") && jsFunction.startsWith("function canonicalGif("));
const canonicalGif = vm.runInNewContext(`(${jsFunction})`);
const dir = fs.mkdtempSync(path.join(os.tmpdir(), "goopad-gif-test-"));
const source = path.join(dir, "test.c"), executable = path.join(dir, process.platform === "win32" ? "test.exe" : "test");
try {
 fs.writeFileSync(source, `#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
#define GIF_MAX_BYTES (128 * 1024)
${cFunction}
int main(void) {
#ifdef _WIN32
 _setmode(_fileno(stdin), _O_BINARY); _setmode(_fileno(stdout), _O_BINARY);
#endif
 uint8_t data[GIF_MAX_BYTES + 240 * 8];
 size_t size = fread(data, 1, GIF_MAX_BYTES, stdin);
 if (!goopad_prepare_gif(data, &size, sizeof(data))) return 1;
 return fwrite(data, 1, size, stdout) == size ? 0 : 2;
}`);
 execFileSync("zig", ["cc", "-Wall", "-Wextra", "-Werror", source, "-o", executable], {stdio:"inherit"});
 const base = Buffer.from("R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7", "base64");
 const withoutControl = Buffer.concat([base.subarray(0, 19), base.subarray(27)]);
 const longDelay = Buffer.from(base); longDelay[23] = 50;
 const twoFrames = Buffer.concat([withoutControl.subarray(0, -1), withoutControl.subarray(19)]);
 for (const [name, input] of [["short delay", base], ["missing control", withoutControl], ["long delay", longDelay], ["two frames", twoFrames]]) {
  const native = spawnSync(executable, {input});
  assert.equal(native.status, 0, `${name}: firmware rejected`);
  assert.deepEqual(Buffer.from(canonicalGif(input)), native.stdout, `${name}: browser differs from firmware`);
 }
 for (const input of [base.subarray(0, -1), Buffer.from(base).fill(0, 6, 8)]) {
  assert.notEqual(spawnSync(executable, {input}).status, 0);
  assert.throws(() => {
   const width = input[6] | input[7] << 8, height = input[8] | input[9] << 8;
   if (!width || !height) throw Error("Invalid dimensions");
   canonicalGif(input);
  });
 }
 console.log("GIF browser/firmware normalization tests passed");
} finally {
 for (const file of [source, executable, path.join(dir, "test.pdb")]) if (fs.existsSync(file)) fs.unlinkSync(file);
 fs.rmdirSync(dir);
}
