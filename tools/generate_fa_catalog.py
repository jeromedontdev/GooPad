#!/usr/bin/env python3
"""Generate GooPad's bundled Font Awesome Free icon catalog.

The input directory must contain the unpacked 7.3.1 npm packages:
free-solid-svg-icons, free-regular-svg-icons, and free-brands-svg-icons.
"""

import argparse
import json
import re
from pathlib import Path

PACKAGES = (
    ("free-solid-svg-icons", "solid"),
    ("free-regular-svg-icons", "regular"),
    ("free-brands-svg-icons", "brands"),
)
BLOCK = re.compile(r"^var fa\w+ = \{([\s\S]*?)^\};", re.MULTILINE)
ICON = re.compile(
    r"prefix:\s*'[^']+',\s*iconName:\s*'([^']+)',\s*icon:\s*\[\s*"
    r"(\d+),\s*(\d+),\s*\[[^\]]*\],\s*\"[^\"]*\",\s*"
    r"(\"(?:[^\"\\]|\\.)*\"|\[\s*\"(?:[^\"\\]|\\.)*\""
    r"(?:\s*,\s*\"(?:[^\"\\]|\\.)*\")*\s*\])\s*\]",
    re.DOTALL,
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("packages_dir", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    icons = []
    versions = set()
    for package, style in PACKAGES:
        package_dir = args.packages_dir / package / "package"
        metadata = json.loads((package_dir / "package.json").read_text(encoding="utf-8"))
        versions.add(metadata["version"])
        source = (package_dir / "index.mjs").read_text(encoding="utf-8")
        blocks = BLOCK.findall(source)
        if not blocks:
            raise ValueError(f"No icon definitions found in {package}")
        for block in blocks:
            match = ICON.search(block)
            if not match:
                raise ValueError(f"Unrecognized icon definition in {package}")
            name, width, height, path_data = match.groups()
            paths = json.loads(path_data)
            if isinstance(paths, str):
                paths = [paths]
            icons.append({
                "name": name,
                "style": style,
                "width": int(width),
                "height": int(height),
                "path": paths,
            })

    if len(versions) != 1:
        raise ValueError(f"Font Awesome package versions differ: {sorted(versions)}")
    icons.sort(key=lambda icon: (icon["style"], icon["name"]))
    catalog = {
        "version": next(iter(versions)),
        "attribution": "Font Awesome Free by Fonticons, CC BY 4.0",
        "icons": icons,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(catalog, ensure_ascii=False, separators=(",", ":")) + "\n",
        encoding="utf-8",
    )
    print(f"Wrote {len(icons)} Font Awesome {catalog['version']} icons to {args.output}")


if __name__ == "__main__":
    main()
