import datetime
import hashlib
import http.server
import ipaddress
import json
import ssl
import subprocess
import tempfile
import threading
import unittest
from pathlib import Path

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import NameOID


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "tests" / "Test-GooPadSoak.ps1"


class SoakTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.directory = Path(cls.temp.name)
        key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
        name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, "localhost")])
        now = datetime.datetime.now(datetime.timezone.utc)
        cert = (
            x509.CertificateBuilder().subject_name(name).issuer_name(name)
            .public_key(key.public_key()).serial_number(x509.random_serial_number())
            .not_valid_before(now - datetime.timedelta(minutes=1))
            .not_valid_after(now + datetime.timedelta(days=1))
            .add_extension(x509.SubjectAlternativeName([x509.IPAddress(ipaddress.ip_address("127.0.0.1"))]), False)
            .sign(key, hashes.SHA256())
        )
        cls.pin = hashlib.sha256(cert.public_bytes(serialization.Encoding.DER)).hexdigest()
        cls.cert_file = cls.directory / "cert.pem"
        cls.key_file = cls.directory / "key.pem"
        cls.cert_file.write_bytes(cert.public_bytes(serialization.Encoding.PEM))
        cls.key_file.write_bytes(key.private_bytes(
            serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8,
            serialization.NoEncryption()))

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def run_soak(self, mode, pin=None):
        calls = []

        class Handler(http.server.BaseHTTPRequestHandler):
            def log_message(self, *args):
                pass

            def do_GET(self):
                calls.append((self.path, self.headers.get("Authorization")))
                if mode == "unauthorized":
                    self.send_response(401)
                    self.end_headers()
                    return
                if self.path == "/api/status":
                    body = {"connected": True, "sd": True, "ssid": "private-ssid"}
                    if mode.startswith("pc-"):
                        body["pcMonitor"] = {
                            "pc-healthy": "CPU 5%\nRAM 50%",
                            "pc-unavailable": "PC unavailable",
                            "pc-deferred": "Update deferred",
                        }[mode]
                        body["liveWidgets"] = ["private-media-title", "Update deferred"]
                    if mode.startswith("widgets-"):
                        first_sample = sum(path == "/api/status" for path, _ in calls) == 1
                        body["liveWidgets"] = [
                            "Update deferred" if mode == "widgets-stuck" or first_sample else "private-media-title"
                        ]
                    if mode.startswith("font"):
                        body.update(unicode={"available": mode != "font-missing", "bitmapCacheBytes": 1000,
                                             "bitmapCachePeak": 32769 if mode == "font-overflow" else 32768},
                                    mediaRevision=3, mediaPollMs=120, mediaSourceAgeMs=50)
                    if mode.startswith("styled"):
                        body["styledFonts"] = {"available": mode != "styled-missing",
                                               "descriptorBytes": 5040, "metadataCacheBytes": 13312,
                                               "kerningCacheBytes": 131073 if mode == "styled-memory" else 24792,
                                               "sharedBitmapCacheBytes": 32769 if mode == "styled-overflow" else 32768,
                                               "bitmapCachePeak": 32768, "bitmapCacheBudget": 32768}
                elif self.path == "/api/diagnostics":
                    count = sum(path == "/api/diagnostics" for path, _ in calls)
                    if mode == "malformed":
                        self.send_response(200)
                        self.end_headers()
                        self.wfile.write(b"{invalid")
                        return
                    events = [[5, 1, 1, 0]]
                    if mode == "events" and count >= 2:
                        events += [[6, 4, 0, 503], [7, 5, 7, 0]]
                    if mode == "gap" and count >= 2:
                        events = [[10, 4, 0, 503]]
                    uptime = 2 if mode == "reboot" and count >= 2 else 100 + count
                    body = {"format": 1, "uptimeSeconds": uptime,
                            "internalFree": 50000, "internalLargest": 12000,
                            "psramFree": 1000000, "resetReason": 1, "events": events}
                else:
                    self.send_error(404)
                    return
                encoded = json.dumps(body).encode()
                self.send_response(200)
                self.send_header("Content-Length", str(len(encoded)))
                self.end_headers()
                self.wfile.write(encoded)

            def do_POST(self):
                calls.append(("POST", None))
                self.send_error(405)

        class Server(http.server.ThreadingHTTPServer):
            def handle_error(self, request, client_address):
                pass

        server = Server(("127.0.0.1", 0), Handler)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(self.cert_file, self.key_file)
        server.socket = context.wrap_socket(server.socket, server_side=True)
        thread = threading.Thread(target=server.serve_forever)
        thread.start()
        report = self.directory / f"{mode}-{pin or 'valid'}.jsonl"
        command = (
            "$p=ConvertTo-SecureString 'synthetic-password' -AsPlainText -Force;"
            "$c=[pscredential]::new('admin',$p);"
            f"& '{SCRIPT}' -Device 'https://127.0.0.1:{server.server_port}' -Credential $c "
            f"-CertificateSha256 '{pin or self.pin}' -DurationSeconds 4 -IntervalSeconds 1 "
            f"-RequestTimeoutSeconds 5 -ReportPath '{report}'"
            + (" -RequireCompanion" if mode.startswith("pc-") else "")
            + (" -MaxWidgetDeferralSeconds 1" if mode.startswith("widgets-") else "")
        )
        try:
            result = subprocess.run(["pwsh", "-NoProfile", "-Command", command],
                                    text=True, capture_output=True, timeout=30)
        finally:
            server.shutdown()
            server.server_close()
            thread.join()
        records = [json.loads(line) for line in report.read_text(encoding="utf-8").splitlines()]
        self.assertNotIn("synthetic-password", result.stdout + result.stderr + report.read_text())
        self.assertNotIn("private-ssid", result.stdout + result.stderr + report.read_text())
        self.assertNotIn("private-media-title", result.stdout + result.stderr + report.read_text())
        self.assertTrue(all(path in ("/api/status", "/api/diagnostics") for path, _ in calls))
        return result, records, calls

    def test_healthy_read_only_and_numeric_report(self):
        result, records, calls = self.run_soak("healthy")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertGreaterEqual(records[-1]["samples"], 2)
        self.assertEqual(records[-1]["errors"], 0)
        self.assertEqual(records[-1]["firstInternalFree"], 50000)
        self.assertEqual(records[-1]["lastPsramFree"], 1000000)
        self.assertTrue(all("timestampUtc" in record for record in records))
        self.assertTrue(all(auth and auth.startswith("Basic ") for _, auth in calls))
        for sample in (r for r in records if r["type"] == "sample"):
            self.assertGreaterEqual(sample["statusLatencyMs"], 0)
            self.assertGreaterEqual(sample["diagnosticsLatencyMs"], 0)
            self.assertAlmostEqual(sample["latencyMs"], sample["statusLatencyMs"] + sample["diagnosticsLatencyMs"], delta=1)

    def test_new_events_reported_once(self):
        result, records, _ = self.run_soak("events")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(records[-1]["errors"], 1, str(records))
        self.assertEqual(records[-1]["warnings"], 1, str(records))

    def test_required_companion_checks_real_snapshot_without_reporting_media(self):
        for mode, available in (("pc-healthy", True), ("pc-unavailable", False), ("pc-deferred", False)):
            with self.subTest(mode=mode):
                result, records, _ = self.run_soak(mode)
                self.assertEqual(result.returncode == 0, available, result.stdout + result.stderr)
                self.assertTrue(records[0]["requireCompanion"])
                samples = [r for r in records if r["type"] == "sample"]
                self.assertTrue(samples)
                self.assertTrue(all(r["companionAvailable"] == available for r in samples))
                self.assertTrue(all(r["deferredWidgetCount"] == 1 for r in samples))
                if not available:
                    self.assertTrue(any(r.get("code") == "companion-unavailable" for r in records))

    def test_font_health_and_numeric_media_report(self):
        result, records, _ = self.run_soak("font-healthy")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        samples = [record for record in records if record["type"] == "sample"]
        self.assertTrue(all(record["unicodeAvailable"] for record in samples))
        self.assertTrue(all(record["unicodeBitmapCachePeak"] <= 32768 for record in samples))
        self.assertTrue(all(record["mediaPollMs"] >= 0 for record in samples))

    def test_widget_deferral_limit_allows_recovery_but_rejects_starvation(self):
        for mode, passes in (("widgets-recover", True), ("widgets-stuck", False)):
            with self.subTest(mode=mode):
                result, records, _ = self.run_soak(mode)
                self.assertEqual(result.returncode == 0, passes, result.stdout + result.stderr)
                self.assertEqual(records[0]["maxWidgetDeferralSeconds"], 1)
                samples = [r for r in records if r["type"] == "sample"]
                self.assertTrue(samples)
                if passes:
                    self.assertEqual(samples[-1]["deferredWidgetCount"], 0)
                else:
                    self.assertGreater(samples[-1]["longestWidgetDeferralSeconds"], 1)
                    self.assertTrue(any(r.get("code") == "widget-deferred" for r in records))

    def test_missing_font_and_cache_over_budget_fail_soak(self):
        for mode, code in (("font-missing", "unicode-unavailable"), ("font-overflow", "unicode-cache")):
            with self.subTest(mode=mode):
                result, records, _ = self.run_soak(mode)
                self.assertNotEqual(result.returncode, 0)
                self.assertTrue(any(record.get("code") == code for record in records))

    def test_styled_shared_cache_and_metadata_are_reported(self):
        result, records, _ = self.run_soak("styled-healthy")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        samples = [record for record in records if record["type"] == "sample"]
        self.assertTrue(all(record["styledAvailable"] for record in samples))
        self.assertTrue(all(record["styledMetadataBytes"] == 43144 for record in samples))
        self.assertTrue(all(record["sharedFontBitmapCacheBytes"] <= 32768 for record in samples))

    def test_styled_missing_or_excess_memory_fails_soak(self):
        for mode, code in (("styled-missing", "styled-unavailable"),
                           ("styled-overflow", "styled-cache"), ("styled-memory", "styled-memory")):
            with self.subTest(mode=mode):
                result, records, _ = self.run_soak(mode)
                self.assertNotEqual(result.returncode, 0)
                self.assertTrue(any(record.get("code") == code for record in records))

    def test_wrong_pin_sends_no_http_requests(self):
        result, records, calls = self.run_soak("wrong-pin", "00" * 32)
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(calls)
        self.assertEqual(records[-1]["samples"], 0)

    def test_invalid_json_is_reported(self):
        result, records, _ = self.run_soak("malformed")
        self.assertNotEqual(result.returncode, 0)
        self.assertTrue(any(r.get("code") == "invalid-json" for r in records))

    def test_authentication_failure_is_reported(self):
        result, records, _ = self.run_soak("unauthorized")
        self.assertNotEqual(result.returncode, 0)
        self.assertTrue(any(r.get("code") == "http" and "401" in r["detail"] for r in records))

    def test_event_history_gap_is_reported_without_replaying(self):
        result, records, _ = self.run_soak("gap")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(records[-1]["warnings"], 1)
        self.assertTrue(any(r.get("code") == "event-gap" for r in records))

    def test_reboot_is_reported_once(self):
        result, records, _ = self.run_soak("reboot")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(records[-1]["reboots"], 1)
        self.assertEqual(records[-1]["errors"], 1)


if __name__ == "__main__":
    unittest.main()
