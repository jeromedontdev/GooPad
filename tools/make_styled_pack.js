const fs = require("node:fs");
const path = require("node:path");
const crypto = require("node:crypto");

const families = [
  "lato", "pacifico", "anton", "archivoblack", "abrilfatface", "lobster",
  "breeserif", "cousine", "sharetechmono", "righteous", "acme", "bangers",
  "patrickhand", "pressstart2p", "spacemono",
];
const sizes = [36, 24, 16, 12];
const HEADER_SIZE = 32;
const DIR_SIZE = 40;
const GLYPH_SIZE = 28;
const KERN_SIZE = 6;

const crcTable = Array.from({ length: 256 }, (_, value) => {
  let crc = value;
  for (let bit = 0; bit < 8; bit++) crc = (crc >>> 1) ^ ((crc & 1) ? 0xa001 : 0);
  return crc;
});
function crc16(buffer, crc = 0xffff) {
  for (const byte of buffer) crc = (crc >>> 8) ^ crcTable[(crc ^ byte) & 255];
  return crc;
}
function values(body) {
  return body.replace(/\/\*[\s\S]*?\*\//g, "").match(/-?0x[\da-f]+|-?\d+/gi)?.map(Number) ?? [];
}
function array(source, name) {
  const escaped = name.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
  const found = source.match(new RegExp(`static [^;]*?\\b${escaped}\\[\\]\\s*=\\s*\\{([\\s\\S]*?)\\};`));
  if (!found) throw new Error(`Missing ${name} array`);
  return values(found[1]);
}
function field(block, name) {
  const found = block.match(new RegExp(`\\.${name}\\s*=\\s*(-?[\\w]+)`));
  return found ? found[1] : null;
}
function arrayName(block, name) {
  const value = field(block, name);
  return value === "NULL" ? null : value;
}
function decodeLvglRle(input, offset, pixels, bpp) {
  let bit = offset * 8, state = 0, prev = 0, count = 0;
  const getBits = length => {
    let result = 0;
    for (let i = 0; i < length; i++, bit++) {
      result = (result << 1) | ((input[bit >>> 3] ?? 0) >> (7 - (bit & 7)) & 1);
    }
    return result;
  };
  const next = () => {
    if (state === 0) {
      const current = getBits(bpp);
      if (bit !== bpp && prev === current) {
        count = 0;
        state = 1;
      }
      prev = current;
      return current;
    }
    if (state === 1) {
      const repeated = getBits(1);
      count++;
      if (repeated) {
        if (count === 11) {
          count = getBits(6);
          if (count) state = 2;
          else {
            prev = getBits(bpp);
            state = 0;
            return prev;
          }
        }
        return prev;
      }
      prev = getBits(bpp);
      state = 0;
      return prev;
    }
    if (--count === 0) {
      state = 0;
      prev = getBits(bpp);
      return prev;
    }
    return prev;
  };
  const decoded = Buffer.alloc(pixels);
  for (let i = 0; i < pixels; i++) decoded[i] = next();
  return decoded;
}
function parseFont(file, family, variant) {
  const source = fs.readFileSync(file, "utf8");
  const bitmap = Buffer.from(array(source, "glyph_bitmap"));
  const glyphBlock = source.match(/static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc\[\]\s*=\s*\{([\s\S]*?)\};/);
  const cmapBlock = source.match(/static const lv_font_fmt_txt_cmap_t cmaps\[\]\s*=\s*\{([\s\S]*?)\};/);
  if (!glyphBlock || !cmapBlock) throw new Error(`Missing glyph/cmap tables in ${file}`);
  const glyphs = [...glyphBlock[1].matchAll(/\{([^{}]+)\}/g)].map(match => {
    const data = {};
    for (const name of ["bitmap_index", "adv_w", "box_w", "box_h", "ofs_x", "ofs_y"])
      data[name] = Number(field(match[1], name));
    return data;
  });
  const cmaps = [...cmapBlock[1].matchAll(/\{([^{}]+)\}/g)].map(match => {
    const block = match[1];
    const cmap = Object.fromEntries(["range_start", "range_length", "glyph_id_start", "list_length"]
      .map(name => [name, Number(field(block, name) ?? 0)]));
    cmap.type = field(block, "type");
    cmap.unicode = arrayName(block, "unicode_list") ? array(source, arrayName(block, "unicode_list")) : null;
    cmap.offsets = arrayName(block, "glyph_id_ofs_list") ? array(source, arrayName(block, "glyph_id_ofs_list")) : null;
    return cmap;
  });
  const cpToGlyph = new Map();
  for (const cmap of cmaps) {
    if (cmap.type.endsWith("FORMAT0_TINY") || cmap.type.endsWith("FORMAT0_FULL")) {
      for (let i = 0; i < cmap.range_length; i++) {
        const offset = cmap.offsets ? cmap.offsets[i] : i;
        if (cmap.type.endsWith("FORMAT0_FULL") && i && offset === 0) continue;
        cpToGlyph.set(cmap.range_start + i, cmap.glyph_id_start + offset);
      }
    } else {
      if (!cmap.unicode) throw new Error(`Missing sparse cmap in ${file}`);
      for (let i = 0; i < cmap.unicode.length; i++) {
        const offset = cmap.offsets ? cmap.offsets[i] : i;
        cpToGlyph.set(cmap.range_start + cmap.unicode[i], cmap.glyph_id_start + offset);
      }
    }
  }
  const bpp = Number(field(source.match(/static const lv_font_fmt_txt_dsc_t font_dsc\s*=\s*\{([\s\S]*?)\};/)?.[1] ?? "", "bpp"));
  const kerningScale = Number(field(source.match(/static const lv_font_fmt_txt_dsc_t font_dsc\s*=\s*\{([\s\S]*?)\};/)?.[1] ?? "", "kern_scale"));
  if (bpp !== 2) throw new Error(`Expected 2-bpp source font in ${file}`);
  const output = [...cpToGlyph].sort((a, b) => a[0] - b[0]).map(([cp, gid]) => {
    const glyph = glyphs[gid];
    if (!glyph) throw new Error(`Invalid glyph id ${gid} for U+${cp.toString(16)} in ${file}`);
    const pixels = glyph.box_w * glyph.box_h;
    const values = pixels ? decodeLvglRle(bitmap, glyph.bitmap_index, pixels, bpp) : Buffer.alloc(0);
    const packed = Buffer.alloc(Math.ceil(pixels / 4));
    for (let i = 0; i < pixels; i++) packed[i >> 2] |= values[i] << (6 - 2 * (i & 3));
    return { cp, gid, glyph, packed, family, variant };
  });
  const kernData = source.match(/static const lv_font_fmt_txt_kern_pair_t kern_pairs\s*=\s*\{([\s\S]*?)\};/)?.[1];
  const kernPairs = [];
  if (kernData) {
    const ids = array(source, arrayName(kernData, "glyph_ids"));
    const kernValues = array(source, arrayName(kernData, "values"));
    if (ids.length !== kernValues.length * 2) throw new Error(`Invalid kerning pair table in ${file}`);
    for (let i = 0; i < kernValues.length; i++)
      kernPairs.push({ left: ids[i * 2], right: ids[i * 2 + 1], delta: kernValues[i] * kerningScale });
  }
  const metrics = source.match(/\.line_height\s*=\s*(\d+)[\s\S]*?\.base_line\s*=\s*(-?\d+)/);
  if (!metrics) throw new Error(`Missing line metrics in ${file}`);
  const underline = source.match(/\.underline_position\s*=\s*(-?\d+)[\s\S]*?\.underline_thickness\s*=\s*(\d+)/);
  if (!underline) throw new Error(`Missing underline metrics in ${file}`);
  return {
    family, variant, size: sizes[variant], lineHeight: Number(metrics[1]), baseline: Number(metrics[2]),
    underlinePosition: Number(underline[1]), underlineThickness: Number(underline[2]),
    glyphs: output, kernPairs,
  };
}
function makePack(fonts) {
  const dirs = Buffer.alloc(fonts.length * DIR_SIZE);
  const chunks = [Buffer.alloc(HEADER_SIZE + dirs.length)];
  let position = chunks[0].length;
  const put = buffer => { chunks.push(buffer); position += buffer.length; };
  for (const [fontIndex, font] of fonts.entries()) {
    const indexOffset = position;
    const index = Buffer.alloc(font.glyphs.length * GLYPH_SIZE);
    put(index);
    const bitmapStart = position;
    const metadata = [];
    for (const [glyphIndex, glyph] of font.glyphs.entries()) {
      const record = Buffer.alloc(GLYPH_SIZE);
      record.writeUInt32LE(glyph.cp, 0);
      record.writeUInt32LE(position, 4);
      record.writeUInt16LE(glyph.packed.length, 8);
      record.writeUInt16LE(glyph.glyph.adv_w, 10);
      record.writeUInt16LE(glyph.glyph.box_w, 12);
      record.writeUInt16LE(glyph.glyph.box_h, 14);
      record.writeInt16LE(glyph.glyph.ofs_x, 16);
      record.writeInt16LE(glyph.glyph.ofs_y, 18);
      record.writeUInt16LE(glyph.gid, 20);
      record[22] = 0;
      record[23] = 2;
      record.writeUInt16LE(crc16(glyph.packed, crc16(record.subarray(0, 26))), 26);
      index.set(record, glyphIndex * GLYPH_SIZE);
      put(glyph.packed);
      metadata.push(glyph);
    }
    const bitmapEnd = position;
    const kernOffset = position;
    const byId = (a, b) => a.left - b.left || a.right - b.right;
    const pairs = font.kernPairs.sort(byId);
    const kern = Buffer.alloc(pairs.length * KERN_SIZE);
    pairs.forEach((pair, i) => {
      const at = i * KERN_SIZE;
      kern.writeUInt16LE(pair.left, at);
      kern.writeUInt16LE(pair.right, at + 2);
      kern.writeInt16LE(pair.delta, at + 4);
    });
    put(kern);
    const dir = dirs.subarray(fontIndex * DIR_SIZE, (fontIndex + 1) * DIR_SIZE);
    dir[0] = font.family;
    dir[1] = font.variant;
    dir.writeUInt16LE(font.size, 2);
    dir.writeUInt16LE(font.lineHeight, 4);
    dir.writeInt16LE(font.baseline, 6);
    dir.writeUInt32LE(font.glyphs.length, 8);
    dir.writeUInt32LE(indexOffset, 12);
    dir.writeUInt32LE(bitmapStart, 16);
    dir.writeUInt32LE(bitmapEnd, 20);
    dir.writeUInt32LE(kernOffset, 24);
    dir.writeUInt32LE(pairs.length, 28);
    dir.writeUInt16LE(GLYPH_SIZE, 32);
    dir.writeUInt16LE(KERN_SIZE, 34);
    dir.writeInt8(font.underlinePosition, 36);
    dir[37] = font.underlineThickness;
  }
  const output = Buffer.concat(chunks);
  const header = output.subarray(0, HEADER_SIZE);
  header.write("GSP1", 0);
  header.writeUInt32LE(output.length, 4);
  header.writeUInt32LE(fonts.length, 8);
  header.writeUInt32LE(DIR_SIZE, 12);
  header.writeUInt32LE(HEADER_SIZE, 16);
  header.writeUInt32LE(GLYPH_SIZE, 20);
  header.writeUInt32LE(KERN_SIZE, 24);
  header.writeUInt32LE(1, 28);
  output.set(dirs, HEADER_SIZE);
  return output;
}
function main() {
  const [fontDirectory, outputDirectory] = process.argv.slice(2);
  if (!fontDirectory || !outputDirectory) throw Error("Pass the compiled font directory and resource output directory.");
  fs.mkdirSync(outputDirectory, { recursive: true });
  const fonts = families.flatMap((name, family) => sizes.map((_, variant) =>
    parseFont(path.join(fontDirectory, `goopad_font_${name}_${sizes[variant]}.c`), family, variant)));
  const pack = makePack(fonts);
  const hash = crypto.createHash("sha256").update(pack).digest("hex");
  const filename = `styled-${hash.slice(0, 12)}.gsp`;
  fs.writeFileSync(path.join(outputDirectory, filename), pack);
  const manifest = {
    format: "GSP1", file: filename, bytes: pack.length, sha256: hash,
    manifestMacros: { GOOPAD_STYLED_FILE: filename, GOOPAD_STYLED_SHA256: hash },
    familyCount: families.length, variantCount: sizes.length, fontCount: fonts.length,
    indexedMetadataBytes: fonts.reduce((n, font) => n + font.glyphs.length * GLYPH_SIZE + font.kernPairs.length * KERN_SIZE, 0),
    bitmapBytes: fonts.reduce((n, font) => n + font.glyphs.reduce((sum, glyph) => sum + glyph.packed.length, 0), 0),
    directoryBytes: HEADER_SIZE + DIR_SIZE * fonts.length,
    sizes, families,
    glyphs: fonts.map(font => ({ family: families[font.family], size: font.size, count: font.glyphs.length,
      kerningPairs: font.kernPairs.length })),
    licenses: families.map(family => `main/fonts/licenses/${family}.txt`),
    embedded: ["Montserrat", "essential"],
  };
  fs.writeFileSync(path.join(outputDirectory, "styled-v1.json"), `${JSON.stringify(manifest, null, 2)}\n`);
  const licenseFiles = families.map(family =>
    `${family}\n${fs.readFileSync(path.join(fontDirectory, "licenses", `${family}.txt`), "utf8").trim()}`).join("\n\n");
  fs.writeFileSync(path.join(outputDirectory, "styled-fonts-NOTICE.txt"), licenseFiles + "\n\nSIL OPEN FONT LICENSE 1.1; see main/fonts/NOTICE.txt.\n");
  console.log(`${filename}: ${pack.length} bytes, SHA-256 ${hash}`);
  console.log(`GSP1 has ${fonts.length} variants; indexed records stay on SD; kerning pairs=${fonts.reduce((n, f) => n + f.kernPairs.length, 0)}.`);
}

if (require.main === module) {
  try { main(); } catch (error) { console.error(error.stack || error.message); process.exitCode = 1; }
}
module.exports = { families, sizes, parseFont, makePack, crc16, decodeLvglRle };
