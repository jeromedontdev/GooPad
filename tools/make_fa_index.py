"""Build the compressed Font Awesome name index and independently compressed glyphs."""

import gzip
import json
import struct
import sys
import zlib
from pathlib import Path


catalog = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
index = {**catalog, "icons": [{"name": icon["name"], "style": icon["style"]}
                            for icon in catalog["icons"]]}
payload = json.dumps(index, separators=(",", ":")).encode("utf-8")
Path(sys.argv[2]).write_bytes(gzip.compress(payload, mtime=0))

packed = bytearray(b"GFAC\x01")
packed.extend(struct.pack("<H", len(catalog["icons"])))
for icon in catalog["icons"]:
    name = icon["name"].encode("ascii")
    style = icon["style"].encode("ascii")
    record = json.dumps(icon, separators=(",", ":")).encode("utf-8")
    compressed = zlib.compress(record, 9)
    if len(name) > 95 or len(style) > 15 or len(record) > 128 * 1024:
        raise ValueError("Font Awesome icon exceeds firmware limits")
    packed.extend(struct.pack("<BBII", len(name), len(style), len(compressed), len(record)))
    packed.extend(name)
    packed.extend(style)
    packed.extend(compressed)
Path(sys.argv[3]).write_bytes(packed)
