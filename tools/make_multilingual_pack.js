const fs = require("node:fs");
const path = require("node:path");
const crypto = require("node:crypto");
const { createRequire } = require("node:module");
const { spawnSync } = require("node:child_process");

const crcTable = Array.from({ length: 256 }, (_, byte) => {
  let crc = byte;
  for (let i = 0; i < 8; i++) crc = (crc >> 1) ^ ((crc & 1) ? 0xa001 : 0);
  return crc;
});
function crc16(bytes, crc = 0xffff) {
  for (const byte of bytes) crc = (crc >> 8) ^ crcTable[(crc ^ byte) & 255];
  return crc;
}

function encodePixels(pixels) {
  const flat = pixels.flat().map(v => Math.round(v * 3 / 255));
  const raw = Buffer.alloc(Math.ceil(flat.length / 4));
  flat.forEach((v, i) => { raw[i >> 2] |= v << (6 - 2 * (i & 3)); });
  const runs = [];
  for (let i = 0; i < flat.length;) {
    let end = i + 1;
    while (end < flat.length && flat[end] === flat[i] && end - i < 64) end++;
    runs.push(((end - i - 1) << 2) | flat[i]);
    i = end;
  }
  return runs.length < raw.length ? { data: Buffer.from(runs), encoding: 1 } : { data: raw, encoding: 0 };
}

async function main() {
  const [converter, cache, output, python = "python"] = process.argv.slice(2);
  if (!converter || !cache || !output) throw Error("Pass converter, downloaded-font cache and output directories.");
  const requireFont = createRequire(path.resolve(converter));
  const opentype = requireFont("opentype.js");
  const ft = require(path.join(path.dirname(converter), "lib", "freetype"));
  const manifest = JSON.parse(fs.readFileSync(path.join(__dirname, "multilingual-font-sources.json")));
  const lato = path.join(__dirname, "..", "main", "fonts", "sources", "Lato-Regular.ttf");
  const sources = [lato, ...Object.values(manifest).map(s => path.join(cache, s.file))].map(file => {
    const bytes = fs.readFileSync(file);
    const font = opentype.parse(bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength));
    return { file, bytes, map: font.tables.cmap.glyphIndexMap };
  });
  const mapping = new Map();
  sources.forEach((source, index) => {
    for (const [key, glyph] of Object.entries(source.map)) {
      const cp = Number(key);
      if (glyph && cp >= 0x20 && cp <= 0x10ffff && (cp < 0xd800 || cp > 0xdfff) && !mapping.has(cp))
        mapping.set(cp, index);
    }
  });
  const codes = [...mapping.keys()].sort((a, b) => a - b);
  if (codes.length > 131072) throw Error("Codepoint directory exceeds the bounded 512 KiB lookup budget");
  const sizes = [8, 10, 12, 14, 16, 20, 24, 28, 36, 40];
  const outputPath = path.join(output, "text-v1.gfp");
  const fd = fs.openSync(outputPath, "w");
  const directory = Buffer.alloc(32 * sizes.length);
  fs.writeSync(fd, Buffer.alloc(32 + directory.length));
  let position = 32 + directory.length;
  await ft.init();
  try {
    for (const [sizeIndex, size] of sizes.entries()) {
      const faces = sources.map(s => ft.fontface_create(s.bytes, size));
      try {
        const indexOffset = position;
        const index = Buffer.alloc(codes.length * 24);
        position += index.length;
        fs.writeSync(fd, index, 0, index.length, indexOffset);
        const dataStart = position;
        let minY = 0, maxY = 0;
        for (const [i, cp] of codes.entries()) {
          const face = faces[mapping.get(cp)];
          if (!ft.glyph_exists(face, cp)) throw Error(`Mapped glyph U+${cp.toString(16)} is unavailable`);
          const glyph = ft.glyph_render(face, cp);
          if (glyph.width > 1024 || glyph.height > 128 || Math.ceil(glyph.width * glyph.height / 4) > 32768)
            throw Error(`U+${cp.toString(16)} at ${size}px exceeds the bounded decoder (${glyph.width}x${glyph.height})`);
          const encoded = encodePixels(glyph.pixels);
          const offsetY = glyph.y - glyph.height;
          minY = Math.min(minY, offsetY);
          maxY = Math.max(maxY, glyph.y);
          const at = i * 24;
          index.writeUInt32LE(cp, at);
          index.writeUInt32LE(position, at + 4);
          index.writeUInt16LE(encoded.data.length, at + 8);
          index.writeUInt16LE(Math.round(glyph.advance_x * 16), at + 10);
          index.writeUInt16LE(glyph.width, at + 12);
          index.writeUInt16LE(glyph.height, at + 14);
          index.writeInt16LE(glyph.x, at + 16);
          index.writeInt16LE(offsetY, at + 18);
          index[at + 20] = encoded.encoding;
          index[at + 21] = 2;
          index.writeUInt16LE(crc16(encoded.data, crc16(index.subarray(at, at + 22))), at + 22);
          fs.writeSync(fd, encoded.data, 0, encoded.data.length, position);
          position += encoded.data.length;
        }
        fs.writeSync(fd, index, 0, index.length, indexOffset);
        const at = sizeIndex * 32;
        directory.writeUInt16LE(size, at);
        directory.writeUInt16LE(maxY - minY, at + 2);
        directory.writeInt16LE(-minY, at + 4);
        directory.writeUInt32LE(codes.length, at + 8);
        directory.writeUInt32LE(indexOffset, at + 12);
        directory.writeUInt32LE(dataStart, at + 16);
        directory.writeUInt32LE(position, at + 20);
        console.log(`${size}px: ${codes.length} glyphs; ${position - indexOffset} bytes`);
      } finally { faces.forEach(face => ft.fontface_destroy(face)); }
    }
    const header = Buffer.alloc(32);
    header.write("GFP2");
    header.writeUInt32LE(position, 4);
    header.writeUInt32LE(sizes.length, 8);
    header.writeUInt32LE(32, 12);
    fs.writeSync(fd, header, 0, header.length, 0);
    fs.writeSync(fd, directory, 0, directory.length, 32);
  } finally {
    ft.destroy();
    fs.closeSync(fd);
  }
  const hash = crypto.createHash("sha256").update(fs.readFileSync(outputPath)).digest("hex");
  const textFile = `text-${hash.slice(0, 12)}.gfp`;
  fs.renameSync(outputPath, path.join(output, textFile));
  const catalogPath = path.join(output, "fa-catalog-v1.bin");
  const catalog = spawnSync(python, [path.join(__dirname, "make_fa_index.py"),
    path.join(__dirname, "..", "main", "fa-icons.json"),
    path.join(__dirname, "..", "main", "fa-index.json.gz"), catalogPath], { stdio: "inherit" });
  if (catalog.status !== 0) throw Error("Icon catalog generation failed");
  const iconHash = crypto.createHash("sha256").update(fs.readFileSync(catalogPath)).digest("hex");
  const iconFile = `icons-${iconHash.slice(0, 12)}.bin`;
  fs.renameSync(catalogPath, path.join(output, iconFile));
  fs.writeFileSync(path.join(__dirname, "..", "main", "goopad_resource_manifest.h"),
    `#pragma once\n\n#define GOOPAD_TEXT_FILE "${textFile}"\n#define GOOPAD_TEXT_SHA256 "${hash}"\n` +
    `#define GOOPAD_ICONS_FILE "${iconFile}"\n#define GOOPAD_ICONS_SHA256 "${iconHash}"\n`);
  fs.writeFileSync(path.join(output, "text-v1.json"), JSON.stringify({
    format: "GFP2", file: textFile, bytes: position, sha256: hash, sizes, glyphsPerSize: codes.length,
    icons: { file: iconFile, sha256: iconHash },
    sources: sources.map(s => ({ file: path.basename(s.file), sha256: crypto.createHash("sha256").update(s.bytes).digest("hex") }))
  }, null, 2) + "\n");
}

if (require.main === module) main().catch(error => { console.error(error.message); process.exitCode = 1; });
module.exports = { encodePixels, crc16 };
