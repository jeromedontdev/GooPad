import importlib.util
import json
import tempfile
import threading
import unittest
from pathlib import Path
from urllib.error import HTTPError
from urllib.request import Request, urlopen
from unittest.mock import patch


SOURCE = Path(__file__).parents[1] / "companion" / "GooPadCompanion.py"
SPEC = importlib.util.spec_from_file_location("goopad_companion", SOURCE)
COMPANION = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(COMPANION)


class CompanionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.original_path = COMPANION.CONFIG_PATH
        COMPANION.CONFIG_PATH = Path(self.temp.name) / "companion.json"
        self.token = "T" * 40
        self.write_config({"token": self.token, "actions": {
            "open": {"program": "example.exe", "arguments": ["--safe"]}
        }})
        self.server = COMPANION.ThreadingHTTPServer(("127.0.0.1", 0), COMPANION.Handler)
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.base = f"http://127.0.0.1:{self.server.server_port}"

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join(timeout=2)
        COMPANION.CONFIG_PATH = self.original_path
        self.temp.cleanup()

    def write_config(self, value):
        COMPANION.CONFIG_PATH.write_text(json.dumps(value), encoding="utf-8")

    def request(self, path, token=None, body=b"{}"):
        headers = {"Content-Length": str(len(body))}
        if token is not None:
            headers["X-GooPad-Token"] = token
        request = Request(self.base + path, data=body, headers=headers, method="POST")
        try:
            with urlopen(request, timeout=3) as response:
                return response.status
        except HTTPError as error:
            return error.code

    def test_requires_shared_token(self):
        self.assertEqual(self.request("/hook/open"), 401)
        self.assertEqual(self.request("/hook/open", "wrong"), 401)
        self.assertEqual(self.request("/open"), 401)

    def test_unknown_action_does_not_launch(self):
        with patch.object(COMPANION.subprocess, "Popen") as launch:
            self.assertEqual(self.request("/hook/missing", self.token), 404)
            launch.assert_not_called()

    def test_valid_action_uses_shell_false_and_fixed_arguments(self):
        with patch.object(COMPANION.subprocess, "Popen") as launch:
            self.assertEqual(self.request("/hook/open", self.token), 202)
            launch.assert_called_once_with(
                ["example.exe", "--safe"], cwd=None, shell=False, close_fds=True
            )

    def test_open_url_uses_default_browser(self):
        body = json.dumps({"url": "https://example.com/dashboard"}).encode()
        with patch.object(COMPANION.webbrowser, "open", return_value=True) as open_browser:
            self.assertEqual(self.request("/open", self.token, body), 202)
            open_browser.assert_called_once_with("https://example.com/dashboard", new=2)

    def test_open_url_rejects_non_web_schemes(self):
        for target in ("javascript:alert(1)", "file:///C:/Windows/win.ini",
                       "https://user:password@example.com", "http://[",
                       "https://bad host/"):
            body = json.dumps({"url": target}).encode()
            with self.subTest(target=target), patch.object(COMPANION.webbrowser, "open") as open_browser:
                self.assertEqual(self.request("/open", self.token, body), 400)
                open_browser.assert_not_called()

    def test_health_endpoint(self):
        with urlopen(self.base + "/health", timeout=3) as response:
            self.assertEqual(response.status, 200)


if __name__ == "__main__":
    unittest.main()
