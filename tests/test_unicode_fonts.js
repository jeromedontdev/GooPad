const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const { createRequire } = require("node:module");

if (!process.argv[2]) throw Error("Pass the installed lv_font_conv.js path (version 1.5.3).");
const opentype = createRequire(path.resolve(process.argv[2]))("opentype.js");
const root = path.join(__dirname, "..");
const font = opentype.loadSync(path.join(root, "main", "fonts", "sources", "Lato-Regular.ttf"));
const expected = Object.keys(font.tables.cmap.glyphIndexMap).map(Number)
 .filter(cp => cp >= 0x80 && cp <= 0xffff && font.tables.cmap.glyphIndexMap[cp] > 0).sort((a, b) => a - b);
for (const size of [8, 10, 12, 14, 16, 20, 24, 28, 36, 40]) {
 const source = fs.readFileSync(path.join(root, "main", "fonts", `goopad_font_unicode_${size}.c`), "utf8");
 const arrays = new Map();
 for (const match of source.matchAll(/static const uint(?:8|16)_t (\w+)\[\] = \{([\s\S]*?)\};/g)) {
  arrays.set(match[1], match[2].replace(/\/\*[\s\S]*?\*\//g, "").match(/0x[\da-f]+|\d+/gi).map(Number));
 }
 const cmap = source.match(/static const lv_font_fmt_txt_cmap_t cmaps\[\]\s*=\s*\{([\s\S]*?)\};/);
 assert(cmap, `Missing character maps at ${size}px`);
 const mapped = new Map();
 for (const entry of cmap[1].matchAll(/\{([^{}]+)\}/g)) {
  const fields = Object.fromEntries([...entry[1].matchAll(/\.(\w+)\s*=\s*(\w+)/g)].map(m => [m[1], m[2]]));
  const start = Number(fields.range_start), base = Number(fields.glyph_id_start), length = Number(fields.range_length);
  const offsets = arrays.get(fields.glyph_id_ofs_list), sparse = arrays.get(fields.unicode_list);
  if (fields.type.endsWith("FORMAT0_TINY") || fields.type.endsWith("FORMAT0_FULL")) {
   for (let i = 0; i < length; i++) {
    // FORMAT0_FULL uses zero for missing characters except at the first position.
    if (offsets && i && !offsets[i]) continue;
    mapped.set(start + i, base + (offsets ? offsets[i] : i));
   }
  } else {
   assert(fields.type.endsWith("SPARSE_TINY") || fields.type.endsWith("SPARSE_FULL"), "Unknown cmap encoding");
   assert.equal(sparse.length, Number(fields.list_length));
   sparse.forEach((offset, i) => mapped.set(start + offset, base + (offsets ? offsets[i] : i)));
  }
 }
 const glyphCount = (source.match(/\.adv_w\s*=/g) || []).length;
 assert.deepEqual([...mapped.keys()].sort((a, b) => a - b), expected, `${size}px lost source Unicode coverage`);
 for (const [cp, id] of mapped) assert(id > 0 && id < glyphCount, `${size}px U+${cp.toString(16)} has an invalid glyph`);
}
console.log(`All ${expected.length} source non-ASCII BMP mappings verified at all 10 fallback sizes.`);
