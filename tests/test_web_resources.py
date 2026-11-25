import gzip
import hashlib
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from tools.make_web_resources import FONTS, generate

ROOT = Path(__file__).resolve().parents[1]


class WebResourceTests(unittest.TestCase):
    def test_platform_gzip_os_byte_does_not_duplicate_archive(self):
        compress = gzip.compress
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary)
            fingerprints = []
            for os_byte in (3, 10, 255):
                def platform_gzip(*args, **kwargs):
                    result = bytearray(compress(*args, **kwargs))
                    result[9] = os_byte
                    return bytes(result)
                with patch("tools.make_web_resources.gzip.compress", side_effect=platform_gzip):
                    fingerprints.append(generate(ROOT / "main", output, output / "manifest.h")["sha256"])
            self.assertEqual(len(set(fingerprints)), 1)

    def test_real_pack_is_reproducible_and_preserves_editor_and_fonts(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary)
            first = generate(ROOT / "main", output, output / "manifest.h")
            archive = (output / first["file"]).read_bytes()
            self.assertEqual(first, generate(ROOT / "main", output, output / "manifest.h"))
            self.assertEqual(hashlib.sha256(archive).hexdigest(), first["sha256"])
            self.assertEqual(int.from_bytes(archive[4:8], "little"), len(archive))
            for asset in first["assets"]:
                data = archive[asset["offset"]:asset["offset"] + asset["bytes"]]
                self.assertEqual(hashlib.sha256(data).hexdigest(), asset["sha256"])
                if asset["name"] == "index.html.gz":
                    self.assertEqual(gzip.decompress(data), (ROOT / "main" / "index.html").read_bytes())
                else:
                    source = ROOT / "main" / ("index.html" if asset["name"] == "index.html"
                                             else f'fonts/web/{asset["name"]}')
                    self.assertEqual(data, source.read_bytes())

    def test_empty_editor_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "fonts" / "web").mkdir(parents=True)
            (root / "index.html").write_bytes(b"")
            for name in FONTS:
                (root / "fonts" / "web" / f"{name}.woff2").write_bytes(b"wOF2")
            with self.assertRaisesRegex(ValueError, "size invalid"):
                generate(root, root / "output", root / "manifest.h")

    def test_actual_c_loader_and_http_handlers(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            pack = generate(ROOT / "main", directory, directory / "manifest.h")
            destination = directory / "goopad" / "resources"
            destination.mkdir(parents=True)
            shutil.copyfile(directory / pack["file"], destination / pack["file"])
            source = (ROOT / "main" / "goopad.c").read_text(encoding="utf-8")
            functions = []
            for name in ("send_web_asset", "index_handler", "preview_font_handler"):
                match = re.search(rf"^static esp_err_t {name}\(.*?^\}}", source, re.MULTILINE | re.DOTALL)
                if match is None:
                    self.fail(f"Missing actual handler: {name}")
                functions.append(match.group())
            (directory / "web_handlers.inc").write_text("\n".join(functions), encoding="utf-8")
            executable = directory / "web-test.exe"
            subprocess.run(["zig", "cc", "-std=c11", "-I", str(directory),
                            "-I", str(ROOT / "tests" / "web_stubs"),
                            "-I", str(ROOT / "tests" / "unicode_stubs"),
                            str(ROOT / "tests" / "test_web_resources.c"),
                            "-lbcrypt", "-o", str(executable)], check=True, timeout=120)
            subprocess.run([str(executable)], cwd=directory, check=True, timeout=30)


if __name__ == "__main__":
    unittest.main()
