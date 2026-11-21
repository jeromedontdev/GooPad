"""Build the small, compressed Font Awesome name index from the bundled catalog."""

import gzip
import json
import sys
from pathlib import Path


catalog = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
index = {**catalog, "icons": [{"name": icon["name"], "style": icon["style"]}
                            for icon in catalog["icons"]]}
payload = json.dumps(index, separators=(",", ":")).encode("utf-8")
Path(sys.argv[2]).write_bytes(gzip.compress(payload, mtime=0))
