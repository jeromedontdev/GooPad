const assert = require("node:assert/strict");
const crypto = require("node:crypto");
const fs = require("node:fs");
const path = require("node:path");
const { families, sizes, parseFont } = require("../tools/make_styled_pack.js");

const [fontDirectory, resourceDirectory] = process.argv.slice(2);
if (!fontDirectory || !resourceDirectory) throw Error("Pass current compiled-font and generated-resource directories.");
const manifest = JSON.parse(fs.readFileSync(path.join(resourceDirectory, "styled-v1.json"), "utf8"));
const packPath = path.join(resourceDirectory, manifest.file);
const pack = fs.readFileSync(packPath);
assert.equal(pack.subarray(0, 4).toString(), "GSP1");
assert.equal(pack.readUInt32LE(4), pack.length);
assert.equal(pack.readUInt32LE(8), families.length * sizes.length);
assert.equal(pack.length, manifest.bytes);
assert.equal(crypto.createHash("sha256").update(pack).digest("hex"), manifest.sha256);
assert.equal(manifest.file, `styled-${manifest.sha256.slice(0, 12)}.gsp`);
assert.equal(manifest.manifestMacros.GOOPAD_STYLED_FILE, manifest.file);
assert.equal(manifest.manifestMacros.GOOPAD_STYLED_SHA256, manifest.sha256);
assert.equal(pack.readUInt32LE(12), 40);
assert.equal(pack.readUInt32LE(20), 28);
assert.equal(pack.readUInt32LE(24), 6);
assert.equal(manifest.indexedMetadataBytes, 186072);
assert.equal(manifest.bitmapBytes + manifest.indexedMetadataBytes + manifest.directoryBytes, pack.length);

let glyphsChecked = 0, pairsChecked = 0;
const expectedCodepoints = [
  ...Array.from({ length: 95 }, (_, i) => i + 32),
  0x00b0,
];
for (let family = 0; family < families.length; family++) {
  for (let variant = 0; variant < sizes.length; variant++) {
    const font = parseFont(path.join(fontDirectory, `goopad_font_${families[family]}_${sizes[variant]}.c`), family, variant);
    assert.deepEqual(font.glyphs.map(glyph => glyph.cp), expectedCodepoints);
    const dirAt = 32 + (family * sizes.length + variant) * 40;
    assert.equal(pack[dirAt], family);
    assert.equal(pack[dirAt + 1], variant);
    assert.equal(pack.readUInt16LE(dirAt + 2), sizes[variant]);
    assert.equal(pack.readUInt16LE(dirAt + 4), font.lineHeight);
    assert.equal(pack.readInt16LE(dirAt + 6), font.baseline);
    assert.equal(pack.readInt8(dirAt + 36), font.underlinePosition);
    assert.equal(pack[dirAt + 37], font.underlineThickness);
    assert.equal(pack.readUInt32LE(dirAt + 8), font.glyphs.length);
    const index = pack.readUInt32LE(dirAt + 12);
    const bitmapStart = pack.readUInt32LE(dirAt + 16);
    const bitmapEnd = pack.readUInt32LE(dirAt + 20);
    const kernOffset = pack.readUInt32LE(dirAt + 24);
    const kernCount = pack.readUInt32LE(dirAt + 28);
    assert.equal(kernCount, font.kernPairs.length);
    assert.equal(index + font.glyphs.length * 28, bitmapStart);
    assert.ok(bitmapStart <= bitmapEnd && bitmapEnd <= kernOffset);
    for (let i = 0; i < font.glyphs.length; i++) {
      const expected = font.glyphs[i], at = index + i * 28;
      assert.equal(pack.readUInt32LE(at), expected.cp);
      assert.equal(pack.readUInt16LE(at + 8), expected.packed.length);
      assert.equal(pack.readUInt16LE(at + 10), expected.glyph.adv_w);
      assert.equal(pack.readUInt16LE(at + 12), expected.glyph.box_w);
      assert.equal(pack.readUInt16LE(at + 14), expected.glyph.box_h);
      assert.equal(pack.readInt16LE(at + 16), expected.glyph.ofs_x);
      assert.equal(pack.readInt16LE(at + 18), expected.glyph.ofs_y);
      assert.equal(pack.readUInt16LE(at + 20), expected.gid);
      assert.equal(pack[at + 22], 0);
      assert.equal(pack[at + 23], 2);
      const bitmapAt = pack.readUInt32LE(at + 4);
      assert.deepEqual(pack.subarray(bitmapAt, bitmapAt + expected.packed.length), expected.packed);
      glyphsChecked++;
    }
    const pairs = [...font.kernPairs].sort((a, b) => a.left - b.left || a.right - b.right);
    for (let i = 0; i < pairs.length; i++) {
      const at = kernOffset + i * 6;
      assert.equal(pack.readUInt16LE(at), pairs[i].left);
      assert.equal(pack.readUInt16LE(at + 2), pairs[i].right);
      assert.equal(pack.readInt16LE(at + 4), pairs[i].delta);
      pairsChecked++;
    }
  }
}
assert.ok(glyphsChecked > 5000);
assert.ok(pairsChecked > 4000);
console.log(`GSP1 source parity passed: ${glyphsChecked} glyphs/bitmaps/metrics and ${pairsChecked} kerning pairs across 60 variants.`);
