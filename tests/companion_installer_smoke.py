"""Exercise an old frozen updater -> installer -> replacement app, in isolation."""

import argparse
import ast
import hashlib
import http.client
import importlib
import json
import os
from pathlib import Path
import secrets
import shutil
import socket
import ssl
import subprocess
import sys
import tempfile
import time
import uuid


BOOTSTRAP = """
import json, os, subprocess, sys
from pathlib import Path
installer, directory, port, report = sys.argv[1:]
process = subprocess.Popen([installer, "/VERYSILENT", "/SUPPRESSMSGBOXES",
    "/NORESTART", "/CLOSEAPPLICATIONS", "/DIR=" + directory,
    "/GOOPADPORT=" + port, "/GOOPADPID=" + str(os.getpid()),
    "/LOG=" + str(Path(report).with_suffix(".install.log"))],
    shell=False, close_fds=True)
Path(report).write_text(json.dumps({"runtime": sys._MEIPASS,
    "installer_pid": process.pid,
    "reset_requested": os.environ.get("PYINSTALLER_RESET_ENVIRONMENT") == "1"}))
"""


def run_logged(arguments, log):
    with log.open("w", encoding="utf-8") as output:
        subprocess.run(arguments, stdout=output, stderr=subprocess.STDOUT, check=True)


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise ValueError(f"Installer fixture no longer matches: {old}")
    return text.replace(old, new, 1)


def hashes(directory):
    return {name: hashlib.sha256((directory / name).read_bytes()).hexdigest()
            for name in ("companion.json", "tls-cert.pem", "tls-key.pem")}


def close_fixture_app(executable):
    literal = str(executable).replace("'", "''")
    command = (
        f"$owned=@(Get-CimInstance Win32_Process | Where-Object ExecutablePath -eq '{literal}'); "
        "foreach($p in $owned) { $null=(Get-Process -Id $p.ProcessId).CloseMainWindow() }; "
        "Start-Sleep -Seconds 2; "
        f"$remaining=@(Get-CimInstance Win32_Process | Where-Object ExecutablePath -eq '{literal}'); "
        "foreach($p in $remaining) { Stop-Process -Id $p.ProcessId -Force }; "
        "if($remaining.Count) { exit 1 }"
    )
    subprocess.run(["powershell.exe", "-NoProfile", "-Command", command], check=True)


def exercise(package, compiler, seconds, work):
    sys.path.insert(0, str(package))
    companion = importlib.import_module("GooPadCompanion")
    versions = [node.value.value for node in ast.parse(
        (package / "companion_update.py").read_text(encoding="utf-8")).body
        if isinstance(node, ast.Assign) and any(
            isinstance(target, ast.Name) and target.id == "VERSION" for target in node.targets)]
    if len(versions) != 1 or not isinstance(versions[0], str):
        raise ValueError("Package has no unambiguous VERSION")
    version = versions[0]
    profile, install = work / "profile", work / "installed"
    app = profile / "GooPad"
    app.mkdir(parents=True)
    install.mkdir()
    token = secrets.token_urlsafe(32)
    (app / "companion.json").write_text(json.dumps({
        "token": token, "actions": {}, "allow_powershell": False,
        "allow_system_controls": False, "allow_now_playing": False,
        "close_behavior": "quit", "minimize_behavior": "taskbar",
    }), encoding="utf-8")
    cert, _ = companion.tls_identity(app, "127.0.0.1")
    original = hashes(app)
    bootstrap = work / "bootstrap.py"
    bootstrap.write_text(BOOTSTRAP, encoding="utf-8")
    run_logged([sys.executable, "-m", "PyInstaller", "--noconfirm", "--onefile", "--console",
                "--name", "GooPadCompanion", "--distpath", str(work / "bootstrap-dist"),
                "--workpath", str(work / "bootstrap-build"), "--specpath", str(work), str(bootstrap)],
               work / "bootstrap-build.log")
    executable = install / "GooPadCompanion.exe"
    shutil.copyfile(work / "bootstrap-dist" / executable.name, executable)
    source = (Path(__file__).parents[1] / "tools" / "companion_installer.iss").read_text(encoding="utf-8")
    source = replace_once(source, "AppId={{B1A919C1-1D39-44C7-B3EF-592E55A9EF5D}",
                          f"AppId=GooPadInstallerTest-{uuid.uuid4()}")
    source = replace_once(source, "RestartApplications=no",
                          "RestartApplications=no\nUninstallable=no")
    source = replace_once(source, 'Name: "{group}\\GooPad Companion";',
                          'Name: "{app}\\GooPad Companion";')
    script = work / "installer.iss"
    script.write_text(source, encoding="utf-8")
    run_logged([str(compiler), f"/DPackageDir={package}", f"/DOutputDir={work}",
                f"/DAppVersion={version}", str(script)], work / "installer-build.log")
    installer = work / f"GooPadCompanion-Setup-{version}.exe"
    with socket.socket() as reservation:
        reservation.bind(("127.0.0.1", 0))
        port = reservation.getsockname()[1]
    environment = {**os.environ, "LOCALAPPDATA": str(profile),
                   "PYINSTALLER_RESET_ENVIRONMENT": "1"}
    report = work / "bootstrap.json"
    context = ssl.create_default_context(cafile=str(cert))

    def request(path, authenticated=True):
        connection = http.client.HTTPSConnection("companion.goopad", port, context=context, timeout=5)
        try:
            connection.sock = context.wrap_socket(
                socket.create_connection(("127.0.0.1", port), timeout=5),
                server_hostname="companion.goopad")
            headers = {"X-GooPad-Token": token} if authenticated else {}
            connection.request("GET", path, headers=headers)
            response = connection.getresponse()
            return response.status, json.loads(response.read())
        finally:
            connection.close()

    try:
        with (work / "bootstrap-launch.log").open("w") as output:
            parent = subprocess.Popen([str(executable), str(installer), str(install), str(port), str(report)],
                                      env=environment, stdout=output, stderr=subprocess.STDOUT)
            parent.wait(timeout=45)
            if parent.returncode != 0:
                raise RuntimeError(f"Frozen updater exited {parent.returncode}")
        details = json.loads(report.read_text())
        if details["reset_requested"]:
            raise AssertionError("Bootstrap must model an old updater without the reset flag")
        deadline = time.monotonic() + 60
        last_error = None
        while time.monotonic() < deadline:
            try:
                if request("/health", False)[0] == 200:
                    break
            except (OSError, http.client.HTTPException) as error:
                last_error = error
            time.sleep(.25)
        else:
            raise RuntimeError("Installed Companion did not start") from last_error
        if Path(details["runtime"]).exists():
            raise AssertionError("Original frozen runtime was not removed")
        if executable.read_bytes() != (package / executable.name).read_bytes():
            raise AssertionError("Installer did not replace the bootstrap with the actual packaged app")
        if request("/monitor", False)[0] != 401:
            raise AssertionError("Installed app accepted unauthenticated monitoring")
        start, samples, worst_ms = time.monotonic(), 0, 0.0
        while True:
            sample_start = time.monotonic()
            status, health = request("/health", False)
            if status != 200 or not health.get("ok"):
                raise AssertionError("Packaged app health failed")
            status, monitor = request("/monitor")
            if (status != 200 or not monitor.get("ok") or
                    monitor.get("mediaStatus") != "Media disabled" or
                    not isinstance(monitor.get("cpuPercent"), (int, float)) or
                    not 0 <= monitor["cpuPercent"] <= 100 or
                    not 0 <= monitor.get("memoryPercent", -1) <= 100):
                raise AssertionError("Packaged monitoring or sharing consent changed")
            status, state = request("/status")
            if status != 200 or state.get("powershell") is not False or state.get("actions") != []:
                raise AssertionError("Packaged permissions or actions changed")
            worst_ms = max(worst_ms, (time.monotonic() - sample_start) * 1000)
            samples += 1
            remaining = seconds - (time.monotonic() - start)
            if remaining <= 0:
                break
            time.sleep(min(5, remaining))
        if hashes(app) != original:
            raise AssertionError("Update changed existing configuration or paired TLS identity")
        return {"version": version, "seconds": round(time.monotonic() - start, 2),
                "samples": samples, "errors": 0, "max_three_request_ms": round(worst_ms, 2),
                "legacy_updater_restart": "passed", "old_runtime_removed": True,
                "configuration_and_identity_preserved": True,
                "actual_package_installed": True}
    finally:
        close_fixture_app(executable)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package-directory", type=Path, required=True)
    parser.add_argument("--inno-compiler", type=Path, required=True)
    parser.add_argument("--seconds", type=int, default=300)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    if sys.platform != "win32" or args.seconds < 1:
        parser.error("Requires Windows and a positive observation duration")
    with tempfile.TemporaryDirectory(prefix="GooPadCompanionInstallerTest-") as work:
        result = exercise(args.package_directory.resolve(), args.inno_compiler.resolve(),
                          args.seconds, Path(work))
    args.report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result))


if __name__ == "__main__":
    main()
