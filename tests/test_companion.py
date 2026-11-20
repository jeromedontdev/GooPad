import importlib.util
import json
import sys
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
with patch.object(sys, "path", [str(SOURCE.parent), *sys.path]):
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

    def test_sequence_is_validated_before_any_action(self):
        steps = [{"type": "hotkey", "keys": ["CTRL", "C"]},
                 {"type": "media", "value": "invalid"}]
        with patch.object(COMPANION, "execute_steps") as execute:
            self.assertEqual(self.request("/run", self.token, json.dumps({"steps": steps}).encode()), 400)
            execute.assert_not_called()
        invalid = [
            [{"type": "hotkey", "keys": ["A", "B"]}],
            [{"type": "hotkey", "keys": ["CTRL", "CTRL", "C"]}],
            [{"type": "wait", "value": True}],
            [{"type": "wait", "value": 6000}] * 2,
            [{"type": "text", "value": "x" * 301}],
            [{"type": "open", "value": "file:///C:/Windows/win.ini"}],
            [{"type": "wait", "value": 0}] * 13,
        ]
        for steps in invalid:
            with self.subTest(steps=steps), self.assertRaises(ValueError):
                COMPANION.validate_steps({"steps": steps}, {"actions": {}})

    def test_powershell_requires_local_permission(self):
        body = json.dumps({"steps": [{"type": "powershell", "value": "Get-Date"}]}).encode()
        with patch.object(COMPANION.subprocess, "run") as execute:
            self.assertEqual(self.request("/run", self.token, body), 403)
            execute.assert_not_called()
        config = {"actions": {}, "allow_powershell": True}
        steps = COMPANION.validate_steps(json.loads(body), config)
        with patch.object(COMPANION.subprocess, "run") as execute:
            execute.return_value.returncode = 0
            COMPANION.execute_steps(steps, config)
            self.assertFalse(execute.call_args.kwargs["shell"])
            self.assertEqual(execute.call_args.kwargs["timeout"], 10)
            self.assertIn("-EncodedCommand", execute.call_args.args[0])

    def test_busy_sequence_is_not_queued(self):
        body = b'{"steps":[{"type":"wait","value":0}]}'
        COMPANION.RUN_LOCK.acquire()
        try:
            self.assertEqual(self.request("/run", self.token, body), 429)
        finally:
            COMPANION.RUN_LOCK.release()

    def test_sequence_order_and_failure_release(self):
        config = {"actions": {}}
        steps = [{"type": "hotkey", "keys": ["CTRL", "C"]},
                 {"type": "wait", "value": 250}, {"type": "text", "value": "hello"}]
        with patch.object(COMPANION, "press_keys") as keys, \
             patch.object(COMPANION.time, "sleep") as wait, \
             patch.object(COMPANION, "type_text") as text:
            calls = unittest.mock.Mock()
            calls.attach_mock(keys, "keys")
            calls.attach_mock(wait, "wait")
            calls.attach_mock(text, "text")
            COMPANION.execute_steps(steps, config)
            self.assertEqual(calls.mock_calls, [
                unittest.mock.call.keys(["CTRL", "C"]),
                unittest.mock.call.wait(0.25), unittest.mock.call.text("hello")])
        COMPANION.RUN_LOCK.acquire()
        with patch.object(COMPANION, "execute_steps", side_effect=OSError("Input blocked")):
            COMPANION.run_steps(steps, config)
        self.assertFalse(COMPANION.RUN_LOCK.locked())
        self.assertIn("Input blocked", COMPANION.LAST_RESULT)

    def test_key_modifiers_are_released_after_error(self):
        with patch.object(COMPANION, "send_input", side_effect=[None, OSError("blocked"), None]) as send:
            with self.assertRaises(OSError):
                COMPANION.press_keys(["CTRL", "C"])
            self.assertEqual(send.call_args.kwargs, {"vk": 0x11, "flags": 2})
        with patch.object(COMPANION, "send_input",
                          side_effect=[None, None, None, OSError("blocked"), None, None]) as send:
            with self.assertRaises(OSError):
                COMPANION.press_keys(["CTRL", "SHIFT", "C"])
            self.assertEqual(send.call_count, 6)

    def test_unicode_text_sends_utf16_units(self):
        with patch.object(COMPANION, "send_input") as send:
            COMPANION.type_text("A\U0001f600")
            self.assertEqual([call.kwargs["scan"] for call in send.call_args_list],
                             [65, 65, 0xD83D, 0xD83D, 0xDE00, 0xDE00])

    def test_status_requires_token_and_browser_origin_is_rejected(self):
        with self.assertRaises(HTTPError) as error:
            urlopen(self.base + "/status", timeout=3)
        self.assertEqual(error.exception.code, 401)
        request = Request(self.base + "/status", headers={"X-GooPad-Token": self.token})
        with urlopen(request, timeout=3) as response:
            status = json.load(response)
            self.assertFalse(status["powershell"])
            self.assertNotIn("token", status)
        request = Request(self.base + "/run", data=b"{}", headers={
            "X-GooPad-Token": self.token, "Origin": "https://example.com"})
        with self.assertRaises(HTTPError) as error:
            urlopen(request, timeout=3)
        self.assertEqual(error.exception.code, 403)

@unittest.skipUnless(sys.platform == "win32", "Windows startup integration")
class WindowsStartupTests(unittest.TestCase):
    def test_registration_persists_and_can_be_disabled_in_isolated_registry_key(self):
        import winreg
        runtime = COMPANION.windows_startup
        with tempfile.TemporaryDirectory() as directory:
            app = Path(directory)
            source = app / "download.exe"
            source.write_bytes(b"packaged app")
            key_path = rf"Software\GooPadStartupTest-{app.name}"
            try:
                with patch.object(runtime, "STARTUP_KEY", key_path), \
                        patch.object(sys, "frozen", True, create=True), \
                        patch.object(sys, "executable", str(source)):
                    self.assertFalse(runtime.startup_enabled())
                    runtime.set_startup(True, app, SOURCE, 18787)
                    self.assertTrue(runtime.startup_enabled())
                    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, key_path) as key:
                        command, kind = winreg.QueryValueEx(key, runtime.STARTUP_NAME)
                    self.assertEqual(kind, winreg.REG_SZ)
                    self.assertIn("--gui --minimized --port 18787", command)
                    runtime.set_startup(False, app, SOURCE, 18787)
                    self.assertFalse(runtime.startup_enabled())
            finally:
                try:
                    winreg.DeleteKey(winreg.HKEY_CURRENT_USER, key_path)
                except FileNotFoundError:
                    pass

    def test_startup_copies_packaged_executable_and_quotes_stable_path(self):
        import winreg
        runtime = COMPANION.windows_startup
        with tempfile.TemporaryDirectory(prefix="GooPad startup ") as directory:
            source = Path(directory) / "download.exe"
            source.write_bytes(b"packaged app")
            app = Path(directory) / "User App Data"
            with patch.object(sys, "frozen", True, create=True), patch.object(sys, "executable", str(source)), \
                    patch.object(winreg, "CreateKey") as create, patch.object(winreg, "SetValueEx") as save:
                runtime.set_startup(True, app, SOURCE, 18787)
            executable = app / "bin" / "GooPadCompanion.exe"
            self.assertEqual(executable.read_bytes(), source.read_bytes())
            self.assertEqual(save.call_args.args[4],
                             f'"{executable}" --gui --minimized --port 18787')
            self.assertEqual(create.call_args.args, (winreg.HKEY_CURRENT_USER, runtime.STARTUP_KEY))
            with patch.object(winreg, "OpenKey"), patch.object(winreg, "DeleteValue") as delete:
                runtime.set_startup(False, app, SOURCE, 18787)
            self.assertEqual(delete.call_args.args[1], runtime.STARTUP_NAME)
            self.assertTrue(executable.exists())

    def test_failed_copy_preserves_existing_startup_executable_and_registration(self):
        import winreg
        runtime = COMPANION.windows_startup
        with tempfile.TemporaryDirectory() as directory:
            app = Path(directory)
            old = app / "bin" / "GooPadCompanion.exe"
            old.parent.mkdir()
            old.write_bytes(b"old working app")
            with patch.object(sys, "frozen", True, create=True), \
                    patch.object(runtime.shutil, "copy2", side_effect=OSError("disk full")), \
                    patch.object(winreg, "SetValueEx") as save:
                with self.assertRaises(OSError):
                    runtime.set_startup(True, app, SOURCE, 8787)
            self.assertEqual(old.read_bytes(), b"old working app")
            self.assertEqual(list(old.parent.iterdir()), [old])
            save.assert_not_called()

if __name__ == "__main__":
    unittest.main()
