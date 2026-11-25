"""Build a pinned SD archive without embedding editor or browser font payloads."""

import argparse
import gzip
import hashlib
import json
from pathlib import Path

FONTS = ("lobster", "pacifico", "sharetechmono", "righteous", "acme", "bangers",
         "patrickhand", "pressstart2p", "spacemono")


def generate(root: Path, output: Path, manifest: Path) -> dict:
    editor = (root / "index.html").read_bytes()
    compressed = bytearray(gzip.compress(editor, compresslevel=9, mtime=0))
    # Python/platform versions vary this OS byte; immutable packs must not.
    compressed[9] = 255
    assets = [("index.html.gz", bytes(compressed)),
              ("index.html", editor)]
    assets.extend((f"{name}.woff2", (root / "fonts" / "web" / f"{name}.woff2").read_bytes())
                  for name in FONTS)
    payload = bytearray(b"GWP1\0\0\0\0")
    records = []
    for name, data in assets:
        if not data or len(data) > 512 * 1024:
            raise ValueError(f"Web asset size invalid: {name}")
        records.append({"name": name, "offset": len(payload), "bytes": len(data),
                        "sha256": hashlib.sha256(data).hexdigest()})
        payload.extend(data)
    payload[4:8] = len(payload).to_bytes(4, "little")
    digest = hashlib.sha256(payload).hexdigest()
    filename = f"web-{digest[:12]}.gwp"
    output.mkdir(parents=True, exist_ok=True)
    (output / filename).write_bytes(payload)
    metadata = {"file": filename, "sha256": digest, "bytes": len(payload), "assets": records}
    (output / "web-v1.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    (output / "fonts-NOTICE.txt").write_bytes((root / "fonts" / "NOTICE.txt").read_bytes())
    lines = ["#pragma once", f'#define GOOPAD_WEB_FILE "{filename}"',
             f'#define GOOPAD_WEB_SHA256 "{digest}"', f"#define GOOPAD_WEB_BYTES {len(payload)}",
             "#define GOOPAD_WEB_ASSETS(X) \\"]
    for i, record in enumerate(records):
        suffix = " \\" if i + 1 < len(records) else ""
        lines.append(f'    X("{record["name"]}", {record["offset"]}, {record["bytes"]}, '
                     f'"{record["sha256"]}"){suffix}')
    text = "\n".join(lines) + "\n"
    if not manifest.exists() or manifest.read_text(encoding="utf-8") != text:
        manifest.write_text(text, encoding="utf-8")
    return metadata


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("manifest", type=Path)
    args = parser.parse_args()
    result = generate(args.root, args.output, args.manifest)
    print(f'{result["file"]}: {result["bytes"]} bytes, SHA-256 {result["sha256"]}')
