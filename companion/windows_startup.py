"""Opt-in, per-user Windows desktop startup."""

import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


STARTUP_NAME = "GooPadCompanion"
STARTUP_KEY = r"Software\Microsoft\Windows\CurrentVersion\Run"


def startup_enabled():
    import winreg

    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, STARTUP_KEY) as key:
            winreg.QueryValueEx(key, STARTUP_NAME)
        return True
    except FileNotFoundError:
        return False


def copy_executable(source, destination):
    if source.resolve() == destination.resolve():
        return
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=destination.parent, prefix="GooPad-", delete=False) as temporary:
        temporary_path = Path(temporary.name)
    try:
        shutil.copy2(source, temporary_path)
        os.replace(temporary_path, destination)
    finally:
        temporary_path.unlink(missing_ok=True)


def set_startup(enabled, app_directory, script, port):
    import winreg

    if enabled:
        if getattr(sys, "frozen", False):
            destination = app_directory / "bin" / "GooPadCompanion.exe"
            copy_executable(Path(sys.executable), destination)
            command = [str(destination)]
        else:
            pythonw = Path(sys.executable).with_name("pythonw.exe")
            if not pythonw.exists():
                raise OSError("Windows startup requires pythonw.exe or the packaged Companion app.")
            command = [str(pythonw), str(Path(script).resolve())]
        command += ["--gui", "--minimized", "--port", str(port)]
        with winreg.CreateKey(winreg.HKEY_CURRENT_USER, STARTUP_KEY) as key:
            winreg.SetValueEx(key, STARTUP_NAME, 0, winreg.REG_SZ,
                              subprocess.list2cmdline(command))
    else:
        try:
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, STARTUP_KEY,
                                0, winreg.KEY_SET_VALUE) as key:
                winreg.DeleteValue(key, STARTUP_NAME)
        except FileNotFoundError:
            pass
