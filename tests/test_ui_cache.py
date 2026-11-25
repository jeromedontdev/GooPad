from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class PageCacheTests(unittest.TestCase):
    def test_actual_frame_capture_patches_dirty_regions_until_final_flush(self):
        source = (ROOT / "main" / "goopad_ui.c").read_text(encoding="utf-8")
        capture = re.search(r"^void goopad_ui_capture_frame\(.*?^\}", source, re.MULTILINE | re.DOTALL)
        self.assertIsNotNone(capture)
        refresh = re.search(r"^static void refresh_cb\(.*?^\}", source, re.MULTILINE | re.DOTALL)
        self.assertIsNotNone(refresh)
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            (directory / "capture.inc").write_text(capture.group(), encoding="utf-8")
            (directory / "refresh.inc").write_text(refresh.group(), encoding="utf-8")
            executable = directory / "ui-cache.exe"
            subprocess.run(["zig", "cc", "-std=c11", "-I", str(directory),
                            str(ROOT / "tests" / "test_ui_cache.c"), "-o", str(executable)],
                           check=True, timeout=120)
            subprocess.run([str(executable)], check=True, timeout=30)


if __name__ == "__main__":
    unittest.main()
