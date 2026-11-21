import importlib.util
import json
import socket
import ssl
import sys
import tempfile
import threading
import unittest
from pathlib import Path
from urllib.error import HTTPError, URLError
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
        self.cert_path, self.key_path = COMPANION.tls_identity(Path(self.temp.name), "127.0.0.1")
        self.tls_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self.tls_context.load_verify_locations(cafile=self.cert_path)
        self.server = COMPANION.secure_server(("127.0.0.1", 0), COMPANION.Handler,
                                            self.cert_path, self.key_path)
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.base = f"https://127.0.0.1:{self.server.server_port}"

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
            with urlopen(request, timeout=3, context=self.tls_context) as response:
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
        with urlopen(self.base + "/health", timeout=3, context=self.tls_context) as response:
            self.assertEqual(response.status, 200)

    @unittest.skipUnless(sys.platform == "win32", "Windows desktop UI")
    def test_desktop_offers_only_secure_pairing_and_controls_fit(self):
        import tkinter as tk
        from tkinter import ttk
        from unittest.mock import Mock

        def inspect_panel(root):
            try:
                root.update_idletasks()
                def descendants(widget):
                    for child in widget.winfo_children():
                        yield child
                        yield from descendants(child)
                widgets = list(descendants(root))
                buttons = [widget for widget in widgets if isinstance(widget, ttk.Button)]
                self.assertEqual([button.cget("text") for button in buttons
                                  if button.cget("text").startswith("Copy")], ["Copy secure pairing"])
                for button in buttons:
                    self.assertLessEqual(button.winfo_rooty() + button.winfo_height(),
                                         root.winfo_rooty() + root.winfo_height())
            finally:
                root.destroy()

        with patch.object(tk.Tk, "mainloop", inspect_panel), patch("pystray.Icon"), \
                patch.object(COMPANION.windows_startup, "startup_enabled", return_value=False):
            COMPANION.desktop_panel(Mock(), {"token":self.token,"actions":{}},
                                    "127.0.0.1", 8787)

    def test_https_rejects_untrusted_identity_and_plain_http(self):
        untrusted = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        with self.assertRaises(URLError):
            urlopen(self.base + "/health", timeout=3, context=untrusted)
        with self.assertRaises((URLError, ConnectionError)):
            urlopen(self.base.replace("https:", "http:") + "/health", timeout=3)

    def test_partial_tls_handshake_does_not_block_other_clients(self):
        with socket.create_connection(("127.0.0.1", self.server.server_port), timeout=3):
            with urlopen(self.base + "/health", timeout=3, context=self.tls_context) as response:
                self.assertEqual(response.status, 200)

    def test_persistent_identity_and_private_pairing_bundle(self):
        original = self.cert_path.read_bytes()
        cert, key = COMPANION.tls_identity(Path(self.temp.name), "192.0.2.20")
        self.assertEqual(cert.read_bytes(), original)
        bundle = json.loads(COMPANION.pairing_bundle("192.0.2.20", 8787, self.token, cert))
        self.assertEqual(bundle["address"], "https://192.0.2.20:8787")
        self.assertEqual(bundle["token"], self.token)
        self.assertNotIn("PRIVATE KEY", bundle["certificate"])
        with self.tls_context.wrap_socket(
                socket.create_connection(("127.0.0.1", self.server.server_port), timeout=3),
                server_hostname=COMPANION.TLS_NAME) as client:
            client.sendall(b"GET /health HTTP/1.1\r\nHost: companion.goopad\r\nConnection: close\r\n\r\n")
            self.assertIn(b"200", client.recv(1024).split(b"\r\n", 1)[0])

    def test_incomplete_identity_fails_without_rotation(self):
        original_key = self.key_path.read_bytes()
        self.cert_path.unlink()
        with self.assertRaises(ValueError):
            COMPANION.tls_identity(Path(self.temp.name), "127.0.0.1")
        self.assertEqual(self.key_path.read_bytes(), original_key)

    def test_invalid_signature_is_reported_without_rotation(self):
        original_key = self.key_path.read_bytes()
        certificate = COMPANION.x509.load_pem_x509_certificate(self.cert_path.read_bytes())
        damaged = bytearray(certificate.public_bytes(COMPANION.serialization.Encoding.DER))
        damaged[-1] ^= 1
        invalid = COMPANION.x509.load_der_x509_certificate(bytes(damaged))
        self.cert_path.write_bytes(invalid.public_bytes(COMPANION.serialization.Encoding.PEM))
        with self.assertRaisesRegex(ValueError, "certificate is invalid"):
            COMPANION.tls_identity(Path(self.temp.name), "127.0.0.1")
        self.assertEqual(self.key_path.read_bytes(), original_key)

    def test_mismatched_identity_is_reported_without_rotation(self):
        original = self.cert_path.read_bytes()
        _, other_key = COMPANION.tls_identity(Path(self.temp.name) / "other", "127.0.0.1")
        self.key_path.write_bytes(other_key.read_bytes())
        with self.assertRaisesRegex(ValueError, "do not match"):
            COMPANION.tls_identity(Path(self.temp.name), "127.0.0.1")
        self.assertEqual(self.cert_path.read_bytes(), original)

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

    def test_system_controls_require_local_permission_and_final_position(self):
        config = {"actions": {}}
        step = {"type": "system", "value": "shutdown"}
        with self.assertRaises(PermissionError):
            COMPANION.validate_steps({"steps": [step]}, config)
        config["allow_system_controls"] = True
        self.assertEqual(COMPANION.validate_steps({"steps": [step]}, config), [step])
        for steps in ([step, {"type": "wait", "value": 0}],
                      [{"type": "system", "value": "arbitrary"}]):
            with self.assertRaises(ValueError):
                COMPANION.validate_steps({"steps": steps}, config)
        with patch.object(COMPANION, "system_control") as control:
            COMPANION.execute_steps([step], config)
            control.assert_called_once_with("shutdown")

    @unittest.skipUnless(sys.platform == "win32", "Windows system controls")
    def test_shutdown_and_cancel_use_fixed_arguments_without_shell(self):
        result = COMPANION.subprocess.CompletedProcess([], 0)
        with patch.object(COMPANION.subprocess, "run", return_value=result) as run:
            COMPANION.system_control("restart")
            args = run.call_args.args[0]
            self.assertEqual(args[1:4], ["/r", "/t", "60"])
            self.assertFalse(run.call_args.kwargs["shell"])
            COMPANION.system_control("cancel_shutdown")
            self.assertEqual(run.call_args.args[0][1:], ["/a"])
        with patch.object(COMPANION.subprocess, "run", return_value=COMPANION.subprocess.CompletedProcess([], 1)):
            with self.assertRaises(OSError):
                COMPANION.system_control("cancel_shutdown")

    def test_unicode_text_sends_utf16_units(self):
        with patch.object(COMPANION, "send_input") as send:
            COMPANION.type_text("A\U0001f600")
            self.assertEqual([call.kwargs["scan"] for call in send.call_args_list],
                             [65, 65, 0xD83D, 0xD83D, 0xDE00, 0xDE00])

    def test_status_requires_token_and_browser_origin_is_rejected(self):
        with self.assertRaises(HTTPError) as error:
            urlopen(self.base + "/status", timeout=3, context=self.tls_context)
        self.assertEqual(error.exception.code, 401)
        request = Request(self.base + "/status", headers={"X-GooPad-Token": self.token})
        with urlopen(request, timeout=3, context=self.tls_context) as response:
            status = json.load(response)
            self.assertFalse(status["powershell"])
            self.assertNotIn("token", status)
        request = Request(self.base + "/run", data=b"{}", headers={
            "X-GooPad-Token": self.token, "Origin": "https://example.com"})
        with self.assertRaises(HTTPError) as error:
            urlopen(request, timeout=3, context=self.tls_context)
        self.assertEqual(error.exception.code, 403)

    def test_monitor_requires_token_and_reports_only_usage_metrics(self):
        with patch.object(COMPANION, "monitor_snapshot", return_value={
                "cpuPercent": 37.5, "memoryPercent": 50,
                "memoryUsedBytes": 8000, "memoryTotalBytes": 16000,
                "gpuPercent": 25, "gpuMemoryUsedBytes": 4000, "cpuTemperatureC": None,
                "gpuTemperatureC": 65, "hardwareStatus": "Some sensors unavailable"}) as snapshot:
            with self.assertRaises(HTTPError) as error:
                urlopen(self.base + "/monitor", timeout=3, context=self.tls_context)
            self.assertEqual(error.exception.code, 401)
            snapshot.assert_not_called()
            request = Request(self.base + "/monitor", headers={"X-GooPad-Token": self.token})
            with urlopen(request, timeout=3, context=self.tls_context) as response:
                payload = json.load(response)
            self.assertEqual(payload["cpuPercent"], 37.5)
            self.assertEqual(payload["memoryPercent"], 50)
            self.assertEqual(payload["gpuPercent"], 25)
            self.assertEqual(payload["gpuTemperatureC"], 65)
            self.assertIsNone(payload["cpuTemperatureC"])
            self.assertNotIn("token", payload)

    def test_monitor_failure_is_reported(self):
        with patch.object(COMPANION, "monitor_snapshot", side_effect=OSError("unavailable")):
            request = Request(self.base + "/monitor", headers={"X-GooPad-Token": self.token})
            with self.assertRaises(HTTPError) as error:
                urlopen(request, timeout=3, context=self.tls_context)
            self.assertEqual(error.exception.code, 503)

    def test_tray_preferences_reject_invalid_settings(self):
        for setting in ({"close_behavior": "service"}, {"minimize_behavior": "invalid"},
                        {"hide_on_startup": "yes"}):
            with self.subTest(setting=setting):
                self.write_config({"token": self.token, "actions": {}, **setting})
                with self.assertRaises(ValueError):
                    COMPANION.load_config()

class SensorTests(unittest.TestCase):
    def setUp(self):
        for name in ("HARDWARE_CACHE", "HARDWARE_BUSY", "HARDWARE_LAST"):
            original = getattr(COMPANION, name)
            self.addCleanup(setattr, COMPANION, name, original)

    def test_sensor_values_preserve_missing_readings_and_reject_invalid_numbers(self):
        reading = {"gpuPercent": 25, "gpuMemoryUsedBytes": 4000,
                   "cpuTemperatureC": None, "gpuTemperatureC": 65,
                   "downloadBytesPerSec": 1000, "uploadBytesPerSec": 500, "fanRpm": None}
        self.assertEqual(COMPANION.hardware_metrics(reading), reading)
        for field, value in (("gpuPercent", True), ("gpuPercent", 101),
                             ("cpuTemperatureC", float("nan")), ("gpuMemoryUsedBytes", -1),
                             ("fanRpm", -1), ("downloadBytesPerSec", float("inf"))):
            with self.subTest(field=field, value=value), self.assertRaises(ValueError):
                COMPANION.hardware_metrics({**reading, field: value})
        with self.assertRaises(ValueError):
            COMPANION.hardware_metrics({"cpuTemperatureC": 50})

    def test_cached_requests_start_only_one_sensor_query(self):
        COMPANION.HARDWARE_BUSY = False
        COMPANION.HARDWARE_LAST = 0
        COMPANION.HARDWARE_CACHE = {"gpuPercent": 25}
        with patch.object(COMPANION.time, "monotonic", return_value=100), \
                patch.object(COMPANION.threading, "Thread") as thread:
            self.assertEqual(COMPANION.hardware_snapshot(), {"gpuPercent": 25})
            self.assertEqual(COMPANION.hardware_snapshot(), {"gpuPercent": 25})
            thread.return_value.start.assert_called_once()

    def test_invalid_nvidia_temperature_does_not_poison_other_gpu_readings(self):
        reading = {"gpuPercent": 25, "gpuMemoryUsedBytes": 4000,
                   "cpuTemperatureC": None, "gpuTemperatureC": None,
                   "downloadBytesPerSec": None, "uploadBytesPerSec": None, "fanRpm": None}
        replies = [
            COMPANION.subprocess.CompletedProcess([], 0, stdout=json.dumps(reading)),
            COMPANION.subprocess.CompletedProcess([], 0, stdout="nan\n"),
        ]
        with patch.object(COMPANION.subprocess, "run", side_effect=replies), \
                patch.object(Path, "is_file", return_value=True), self.assertLogs(level="WARNING"):
            result = COMPANION.query_hardware()
        self.assertEqual(result, reading)

    def test_failed_sensor_query_clears_stale_readings_and_reports_failure(self):
        COMPANION.HARDWARE_CACHE = {"gpuPercent": 25}
        COMPANION.HARDWARE_BUSY = True
        with patch.object(COMPANION, "query_hardware", side_effect=OSError("blocked")), \
                self.assertLogs(level="WARNING"):
            COMPANION.refresh_hardware()
        self.assertIsNone(COMPANION.HARDWARE_CACHE["gpuPercent"])
        self.assertIn("failed", COMPANION.HARDWARE_CACHE["hardwareStatus"])
        self.assertFalse(COMPANION.HARDWARE_BUSY)

    def test_sensor_collector_updates_cache_without_turning_missing_temp_into_zero(self):
        reading = {"gpuPercent": 25, "gpuMemoryUsedBytes": 4000,
                   "cpuTemperatureC": None, "gpuTemperatureC": 65}
        with patch.object(COMPANION, "query_hardware", return_value=reading):
            COMPANION.refresh_hardware()
        self.assertEqual(COMPANION.HARDWARE_CACHE["gpuTemperatureC"], 65)
        self.assertIsNone(COMPANION.HARDWARE_CACHE["cpuTemperatureC"])
        self.assertIn("unavailable", COMPANION.HARDWARE_CACHE["hardwareStatus"])

    @unittest.skipUnless(sys.platform == "win32", "Windows sensor script")
    def test_sensor_script_aggregates_engines_and_prefers_package_and_core_temperatures(self):
        fixture = r"""
        function Get-CimInstance {
            param([Parameter(Position=0)]$ClassName, $Namespace, $Filter)
            switch ($ClassName) {
                'Win32_PerfFormattedData_GPUPerformanceCounters_GPUEngine' {
                    [pscustomobject]@{Name='pid_1_luid_1_phys_0_eng_0_engtype_3D';UtilizationPercentage=20}
                    [pscustomobject]@{Name='pid_2_luid_1_phys_0_eng_0_engtype_3D';UtilizationPercentage=15}
                    [pscustomobject]@{Name='pid_3_luid_1_phys_0_eng_1_engtype_Copy';UtilizationPercentage=5}
                }
                'Win32_PerfFormattedData_GPUPerformanceCounters_GPUAdapterMemory' {
                    [pscustomobject]@{DedicatedUsage=6000}
                    [pscustomobject]@{DedicatedUsage=4000}
                }
                'Win32_PerfFormattedData_Tcpip_NetworkInterface' {
                    [pscustomobject]@{Name='Ethernet';BytesReceivedPersec=1000;BytesSentPersec=200}
                    [pscustomobject]@{Name='Loopback';BytesReceivedPersec=9000;BytesSentPersec=9000}
                }
                'Sensor' {
                    if ($Filter -eq "SensorType='Fan'") {
                        [pscustomobject]@{Name='Fan';Value=1200}
                        [pscustomobject]@{Name='Fan 2';Value=1800}
                        break
                    }
                    [pscustomobject]@{Parent='/intelcpu/0';Name='CPU Package';Value=70}
                    [pscustomobject]@{Parent='/intelcpu/0';Name='CPU Core';Value=60}
                    [pscustomobject]@{Parent='/nvidiagpu/0';Name='GPU Core';Value=65}
                    [pscustomobject]@{Parent='/nvidiagpu/0';Name='GPU Hot Spot';Value=80}
                }
            }
        }
        """
        powershell = Path(COMPANION.os.environ.get("SystemRoot", r"C:\Windows")) / "System32" / "WindowsPowerShell" / "v1.0" / "powershell.exe"
        completed = COMPANION.subprocess.run(
            [str(powershell), "-NoProfile", "-NonInteractive", "-Command", fixture + COMPANION.HARDWARE_SCRIPT],
            capture_output=True, text=True, timeout=10, check=True)
        values = COMPANION.hardware_metrics(json.loads(completed.stdout))
        self.assertEqual(values, {"gpuPercent": 35, "gpuMemoryUsedBytes": 10000,
                                  "cpuTemperatureC": 70, "gpuTemperatureC": 65,
                                  "downloadBytesPerSec": 1000, "uploadBytesPerSec": 200, "fanRpm": 1800})


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
                    runtime.set_startup(True, app, SOURCE, 18787, hidden=True)
                    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, key_path) as key:
                        hidden_command, kind = winreg.QueryValueEx(key, runtime.STARTUP_NAME)
                    self.assertIn("--gui --hidden --port 18787", hidden_command)
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
