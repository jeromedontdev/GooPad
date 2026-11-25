const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const crypto = require("node:crypto");
const { encodePixels, crc16 } = require("../tools/make_multilingual_pack");

function decode(data, encoding, pixels) {
  if (!encoding) {
    assert.equal(data.length, Math.ceil(pixels / 4));
    return Array.from({ length: pixels }, (_, i) => (data[i >> 2] >> (6 - 2 * (i & 3))) & 3);
  }
  const result = [];
  for (const byte of data) {
    const run = (byte >> 2) + 1;
    assert.ok(result.length + run <= pixels, "RLE overrun");
    result.push(...Array(run).fill(byte & 3));
  }
  assert.equal(result.length, pixels, "RLE underrun");
  return result;
}
for (const values of [[0, 85, 170, 255, 85], Array(513).fill(170), []]) {
  const encoded = encodePixels([values]);
  assert.deepEqual(decode(encoded.data, encoded.encoding, values.length), values.map(v => v / 85));
}
assert.throws(() => decode(Buffer.from([255]), 1, 3));
assert.throws(() => decode(Buffer.from([0]), 1, 2));

const output = process.argv[2];
assert.ok(output, "Pass the generated resource directory");
const manifest = JSON.parse(fs.readFileSync(path.join(output, "text-v1.json")));
const pack = fs.readFileSync(path.join(output, manifest.file));
assert.equal(crypto.createHash("sha256").update(pack).digest("hex"), manifest.sha256);
assert.equal(pack.toString("ascii", 0, 4), "GFP2");
assert.equal(pack.readUInt32LE(4), pack.length);
assert.equal(pack.readUInt32LE(8), 10);
assert.equal(pack.readUInt32LE(12), 32);
let previousEnd = 352, records = 0, compressed = 0;
for (let size = 0; size < 10; size++) {
  const dir = 32 + size * 32;
  assert.equal(pack.readUInt16LE(dir), manifest.sizes[size]);
  const count = pack.readUInt32LE(dir + 8), index = pack.readUInt32LE(dir + 12);
  const begin = pack.readUInt32LE(dir + 16), end = pack.readUInt32LE(dir + 20);
  assert.equal(count, manifest.glyphsPerSize);
  assert.equal(index, previousEnd);
  assert.equal(begin, index + count * 24);
  assert.ok(end >= begin && end <= pack.length);
  let last = -1, bitmapEnd = begin;
  const covered = new Set();
  for (let i = 0; i < count; i++) {
    const at = index + i * 24, cp = pack.readUInt32LE(at);
    assert.ok(cp > last && cp <= 0x10ffff && !(cp >= 0xd800 && cp <= 0xdfff));
    last = cp; covered.add(cp);
    const offset = pack.readUInt32LE(at + 4), length = pack.readUInt16LE(at + 8);
    const width = pack.readUInt16LE(at + 12), height = pack.readUInt16LE(at + 14);
    assert.equal(offset, bitmapEnd);
    assert.ok(offset + length <= end && width <= 1024 && height <= 128);
    assert.ok(Math.ceil(width * height / 4) <= 32768);
    assert.equal(pack[at + 21], 2);
    assert.ok(pack[at + 20] <= 1);
    const bytes = pack.subarray(offset, offset + length);
    assert.equal(crc16(bytes, crc16(pack.subarray(at, at + 22))), pack.readUInt16LE(at + 22));
    decode(bytes, pack[at + 20], width * height);
    if (length) {
      const corrupt = Buffer.from(bytes); corrupt[0] ^= 1;
      assert.notEqual(crc16(corrupt, crc16(pack.subarray(at, at + 22))), pack.readUInt16LE(at + 22));
    }
    compressed += pack[at + 20]; records++; bitmapEnd = offset + length;
  }
  assert.equal(bitmapEnd, end);
  for (let cp = 32; cp < 127; cp++) assert.ok(covered.has(cp), "Printable ASCII");
  for (const cp of [0xc6, 0x2014, 0x4e2d, 0x3042, 0x30ab, 0xd55c, 0x627, 0xfe8d, 0x5d0, 0xe01, 0x905, 0x1f600])
    assert.ok(covered.has(cp), `Missing U+${cp.toString(16)}`);
  previousEnd = end;
}
assert.equal(previousEnd, pack.length);
const icons = fs.readFileSync(path.join(output, manifest.icons.file));
assert.equal(crypto.createHash("sha256").update(icons).digest("hex"), manifest.icons.sha256);
assert.equal(icons.toString("ascii", 0, 4), "GFAC");
console.log(`${records} verified multilingual glyph records; ${compressed} RLE bitmaps; pack ${pack.length} bytes`);
