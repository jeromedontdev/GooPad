#!/usr/bin/env python3
"""Small, token-authenticated Windows webhook receiver for GooPad."""

import argparse
import base64
import ctypes
import hmac
import ipaddress
import json
import logging
import os
import secrets
import socket
import subprocess
import sys
import threading
import time
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse
import windows_startup


APP_DIR = Path(os.environ.get("LOCALAPPDATA", Path.home())) / "GooPad"
CONFIG_PATH = APP_DIR / "companion.json"
MAX_BODY = 4096
MAX_STEPS = 12
RUN_LOCK = threading.Lock()
LAST_RESULT = "No PC macro has run yet."
MODIFIERS = {"CTRL": 0x11, "SHIFT": 0x10, "ALT": 0x12, "WIN": 0x5B}
KEYS = {
    **{chr(value): value for value in range(ord("A"), ord("Z") + 1)},
    **{str(value): ord(str(value)) for value in range(10)},
    **{f"F{value}": 0x6F + value for value in range(1, 25)},
    "ENTER": 0x0D, "TAB": 0x09, "ESC": 0x1B, "SPACE": 0x20,
    "BACKSPACE": 0x08, "DELETE": 0x2E, "INSERT": 0x2D,
    "HOME": 0x24, "END": 0x23, "PAGEUP": 0x21, "PAGEDOWN": 0x22,
    "LEFT": 0x25, "UP": 0x26, "RIGHT": 0x27, "DOWN": 0x28,
    "PRINTSCREEN": 0x2C,
}
MEDIA_KEYS = {
    "play_pause": 0xB3, "next": 0xB0, "previous": 0xB1, "stop": 0xB2,
    "mute": 0xAD, "volume_up": 0xAF, "volume_down": 0xAE,
}


class MouseInput(ctypes.Structure):
    _fields_ = [("dx", ctypes.c_long), ("dy", ctypes.c_long),
                ("mouseData", ctypes.c_ulong), ("dwFlags", ctypes.c_ulong),
                ("time", ctypes.c_ulong), ("dwExtraInfo", ctypes.c_size_t)]


class KeyboardInput(ctypes.Structure):
    _fields_ = [("wVk", ctypes.c_ushort), ("wScan", ctypes.c_ushort),
                ("dwFlags", ctypes.c_ulong), ("time", ctypes.c_ulong),
                ("dwExtraInfo", ctypes.c_size_t)]


class HardwareInput(ctypes.Structure):
    _fields_ = [("uMsg", ctypes.c_ulong), ("wParamL", ctypes.c_ushort),
                ("wParamH", ctypes.c_ushort)]


class InputUnion(ctypes.Union):
    _fields_ = [("ki", KeyboardInput), ("mi", MouseInput), ("hi", HardwareInput)]


class WindowsInput(ctypes.Structure):
    _fields_ = [("type", ctypes.c_ulong), ("data", InputUnion)]


def load_config():
    APP_DIR.mkdir(parents=True, exist_ok=True)
    if not CONFIG_PATH.exists():
        CONFIG_PATH.write_text(
            json.dumps({"token": secrets.token_urlsafe(32), "actions": {},
                        "allow_powershell": False}, indent=2),
            encoding="utf-8",
        )
    config = json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
    if not isinstance(config.get("token"), str) or len(config["token"]) < 32:
        raise ValueError(f"Set a random token of at least 32 characters in {CONFIG_PATH}")
    if not isinstance(config.get("actions"), dict):
        raise ValueError(f"'actions' must be an object in {CONFIG_PATH}")
    if type(config.get("allow_powershell", False)) is not bool:
        raise ValueError("'allow_powershell' must be true or false")
    return config


def send_input(vk=0, scan=0, flags=0):
    if os.name != "nt":
        raise OSError("Keyboard and media actions require Windows")
    user32 = ctypes.WinDLL("user32", use_last_error=True)
    user32.SendInput.argtypes = [ctypes.c_uint, ctypes.POINTER(WindowsInput), ctypes.c_int]
    user32.SendInput.restype = ctypes.c_uint
    event = WindowsInput(1, InputUnion(ki=KeyboardInput(vk, scan, flags, 0, 0)))
    if user32.SendInput(1, ctypes.byref(event), ctypes.sizeof(event)) != 1:
        raise OSError("Windows rejected input. Focus a normal, non-administrator app.")


def press_keys(keys):
    pressed = []
    try:
        for key in keys:
            vk = MODIFIERS[key] if key in MODIFIERS else KEYS[key]
            extended = 1 if key in {"WIN", "DELETE", "INSERT", "HOME", "END",
                                   "PAGEUP", "PAGEDOWN", "LEFT", "RIGHT", "UP", "DOWN",
                                   "PRINTSCREEN"} else 0
            send_input(vk=vk, flags=extended)
            pressed.append((vk, extended))
        time.sleep(0.03)
    finally:
        release_error = None
        for vk, extended in reversed(pressed):
            try:
                send_input(vk=vk, flags=extended | 2)
            except OSError as error:
                release_error = error
                logging.error("Windows rejected a key release: %s", error)
        if release_error is not None:
            raise release_error


def type_text(text):
    encoded = text.encode("utf-16-le")
    for index in range(0, len(encoded), 2):
        scan = int.from_bytes(encoded[index:index + 2], "little")
        send_input(scan=scan, flags=4)
        send_input(scan=scan, flags=6)


def validate_steps(payload, config):
    steps = payload.get("steps") if isinstance(payload, dict) else None
    if not isinstance(steps, list) or not 1 <= len(steps) <= MAX_STEPS:
        raise ValueError(f"Choose between 1 and {MAX_STEPS} steps")
    delay = 0
    for step in steps:
        if not isinstance(step, dict):
            raise ValueError("Each step must be an action")
        kind = step.get("type")
        value = step.get("value")
        if kind == "hotkey":
            keys = step.get("keys")
            if (not isinstance(keys, list) or not 1 <= len(keys) <= 4
                    or any(not isinstance(key, str) for key in keys)
                    or len(set(keys)) != len(keys) or keys[-1] not in KEYS
                    or any(key not in MODIFIERS for key in keys[:-1])):
                raise ValueError("Choose modifiers and one supported key")
        elif kind == "media":
            if not isinstance(value, str) or value not in MEDIA_KEYS:
                raise ValueError("Choose a supported media key")
        elif kind == "text":
            if not isinstance(value, str) or not 1 <= len(value) <= 300:
                raise ValueError("Text must contain 1-300 characters")
        elif kind == "wait":
            if type(value) is not int or not 0 <= value <= 10000:
                raise ValueError("Wait must be 0-10000 milliseconds")
            delay += value
        elif kind == "open":
            if not valid_web_target(value):
                raise ValueError("Choose a valid HTTP(S) website")
        elif kind == "powershell":
            if not config.get("allow_powershell", False):
                raise PermissionError("Enable PowerShell locally in the Companion app first")
            if not isinstance(value, str) or not value.strip() or len(value) > 512:
                raise ValueError("PowerShell commands must contain 1-512 characters")
        elif kind == "action":
            if not isinstance(value, str) or value not in config["actions"]:
                raise ValueError("Choose an action configured on this PC")
            configured_command(config["actions"][value])
        else:
            raise ValueError("Unsupported PC action")
    if delay > 10000:
        raise ValueError("Total waiting time cannot exceed 10 seconds")
    return steps


def configured_command(action):
    if not isinstance(action, dict):
        raise ValueError("Invalid configured app")
    program, arguments = action.get("program"), action.get("arguments", [])
    directory = action.get("working_directory") or None
    if (not isinstance(program, str) or not program or not isinstance(arguments, list)
            or any(not isinstance(value, str) for value in arguments)
            or (directory is not None and not isinstance(directory, str))):
        raise ValueError("Configured apps need a program and fixed string arguments")
    return [program, *arguments], directory


def execute_steps(steps, config):
    for step in steps:
        kind, value = step["type"], step.get("value")
        if kind == "hotkey":
            press_keys(step["keys"])
        elif kind == "media":
            vk = MEDIA_KEYS[value]
            send_input(vk=vk, flags=1)
            send_input(vk=vk, flags=3)
        elif kind == "text":
            type_text(value)
        elif kind == "wait":
            time.sleep(value / 1000)
        elif kind == "open":
            if not webbrowser.open(value, new=2):
                raise OSError("Could not open the default web browser")
        elif kind == "action":
            command, directory = configured_command(config["actions"][value])
            subprocess.Popen(command, cwd=directory, shell=False, close_fds=True)
        elif kind == "powershell":
            script = "$ErrorActionPreference='Stop'\n" + value
            script += "\nif ($null -ne $LASTEXITCODE -and $LASTEXITCODE -ne 0) { exit $LASTEXITCODE }"
            encoded = base64.b64encode(script.encode("utf-16-le")).decode("ascii")
            executable = Path(os.environ.get("SystemRoot", r"C:\Windows")) / "System32" / "WindowsPowerShell" / "v1.0" / "powershell.exe"
            try:
                result = subprocess.run(
                    [str(executable), "-NoLogo", "-NoProfile", "-NonInteractive",
                     "-EncodedCommand", encoded], shell=False, timeout=10,
                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                    creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0),
                )
            except subprocess.TimeoutExpired:
                raise TimeoutError("PowerShell exceeded the 10-second limit") from None
            if result.returncode:
                raise OSError(f"PowerShell failed (exit {result.returncode})")


def run_steps(steps, config):
    global LAST_RESULT
    LAST_RESULT = "Running PC macro..."
    try:
        execute_steps(steps, config)
        LAST_RESULT = "PC macro completed."
        logging.info(LAST_RESULT)
    except (OSError, ValueError, TimeoutError, webbrowser.Error) as error:
        LAST_RESULT = f"PC macro failed: {error}"
        logging.error(LAST_RESULT)
    finally:
        RUN_LOCK.release()


def lan_address():
    probe = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        probe.connect(("192.0.2.1", 1))
        return probe.getsockname()[0]
    except OSError:
        return "127.0.0.1"
    finally:
        probe.close()


def valid_web_target(value):
    if not isinstance(value, str) or not value or len(value) > 179:
        return False
    if any(character.isspace() or ord(character) < 32 or ord(character) == 127
           for character in value):
        return False
    try:
        parsed = urlparse(value)
        hostname = parsed.hostname
        parsed.port
    except ValueError:
        return False
    return (
        parsed.scheme in ("http", "https")
        and hostname is not None
        and not parsed.username
        and not parsed.password
    )


class Handler(BaseHTTPRequestHandler):
    server_version = "GooPadCompanion"
    sys_version = ""

    def log_message(self, fmt, *args):
        logging.info("%s - %s", self.client_address[0], fmt % args)

    def respond(self, status, message, **details):
        payload = json.dumps({"ok": status < 400, "message": message, **details}).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(payload)

    def do_GET(self):
        if self.path == "/status":
            config = self.authenticated_config()
            if config is not None:
                self.respond(200, LAST_RESULT, busy=RUN_LOCK.locked(),
                             powershell=config.get("allow_powershell", False),
                             actions=list(config["actions"]))
            return
        if self.path != "/health":
            return self.respond(404, "Not found")
        return self.respond(200, "GooPad companion is running")

    def authenticated_config(self):
        if self.headers.get("Origin"):
            self.respond(403, "Send PC actions through GooPad, not directly from a browser")
            return None
        try:
            peer = ipaddress.ip_address(self.client_address[0])
        except ValueError:
            self.respond(403, "Invalid peer address")
            return None
        if not (peer.is_private or peer.is_loopback):
            self.respond(403, "LAN clients only")
            return None
        try:
            config = load_config()
        except (OSError, ValueError) as error:
            logging.error("Companion configuration error: %s", error)
            self.respond(500, "Could not read Companion configuration")
            return None
        supplied = self.headers.get("X-GooPad-Token", "")
        if not supplied.isascii() or not hmac.compare_digest(supplied, config["token"]):
            self.respond(401, "Invalid GooPad token")
            return None
        return config

    def do_POST(self):
        config = self.authenticated_config()
        if config is None:
            return
        self.connection.settimeout(5)

        path = urlparse(self.path).path
        if path == "/run":
            try:
                size = int(self.headers.get("Content-Length", "0"))
                if not 0 < size <= MAX_BODY:
                    return self.respond(413, "PC macro request is too large")
                body = self.rfile.read(size)
                if len(body) != size:
                    return self.respond(400, "PC macro request was interrupted")
                steps = validate_steps(json.loads(body), config)
            except PermissionError as error:
                return self.respond(403, str(error))
            except (UnicodeDecodeError, ValueError, OSError) as error:
                return self.respond(400, f"Invalid PC macro: {error}")
            if not RUN_LOCK.acquire(blocking=False):
                return self.respond(429, "A PC macro is already running")
            worker = threading.Thread(target=run_steps, args=(steps, config), daemon=True)
            try:
                worker.start()
            except RuntimeError:
                RUN_LOCK.release()
                logging.error("Could not start PC macro worker")
                return self.respond(500, "Could not start PC macro")
            return self.respond(202, "PC macro accepted. See Companion for its result.")
        if path == "/open":
            try:
                size = int(self.headers.get("Content-Length", "0"))
            except ValueError:
                return self.respond(400, "Invalid content length")
            if size <= 0 or size > MAX_BODY:
                return self.respond(413, "Request body must contain a short website link")
            try:
                payload = json.loads(self.rfile.read(size))
            except (UnicodeDecodeError, json.JSONDecodeError):
                return self.respond(400, "Invalid website request")
            target = payload.get("url") if isinstance(payload, dict) else None
            if not valid_web_target(target):
                return self.respond(400, "Only valid http:// or https:// websites can be opened")
            try:
                opened = webbrowser.open(target, new=2)
            except (OSError, webbrowser.Error) as error:
                return self.respond(500, f"Could not open website: {error}")
            if not opened:
                return self.respond(500, "Could not open the default web browser")
            return self.respond(202, "Opened website on this PC")

        if not path.startswith("/hook/"):
            return self.respond(404, "Unknown action")
        action_id = path.removeprefix("/hook/").strip("/")
        if not action_id or "/" in action_id or len(action_id) > 48:
            return self.respond(404, "Unknown action")
        action = config["actions"].get(action_id)
        if not isinstance(action, dict):
            return self.respond(404, "Action is not configured")
        program = action.get("program")
        arguments = action.get("arguments", [])
        if not isinstance(program, str) or not program or not isinstance(arguments, list):
            return self.respond(500, "Invalid action configuration")
        if any(not isinstance(value, str) for value in arguments):
            return self.respond(500, "Action arguments must be strings")

        try:
            size = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            return self.respond(400, "Invalid content length")
        if size < 0 or size > MAX_BODY:
            return self.respond(413, "Request body too large")
        if size:
            self.rfile.read(size)
        working_directory = action.get("working_directory") or None
        try:
            subprocess.Popen(
                [program, *arguments],
                cwd=working_directory,
                shell=False,
                close_fds=True,
            )
        except (OSError, ValueError) as error:
            return self.respond(500, f"Could not launch configured app: {error}")
        return self.respond(202, f"Started action '{action_id}'")


def main():
    parser = argparse.ArgumentParser(description="GooPad Windows webhook companion")
    parser.add_argument("--port", type=int, default=8787)
    parser.add_argument("--gui", action="store_true", help="Show the desktop control panel")
    parser.add_argument("--no-gui", action="store_true", help="Run in console mode")
    parser.add_argument("--minimized", action="store_true", help="Start the desktop panel minimized")
    args = parser.parse_args()
    if not 1024 <= args.port <= 65535:
        parser.error("port must be between 1024 and 65535")
    config = load_config()
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s",
                        handlers=[logging.FileHandler(APP_DIR / "companion.log", encoding="utf-8")])
    address = lan_address()
    print(f"GooPad companion listening on http://0.0.0.0:{args.port}")
    print(f"Companion address: http://{address}:{args.port}")
    print(f"Health check: http://{address}:{args.port}/health")
    print(f"Shared token: {config['token']}")
    print(f"Add actions in: {CONFIG_PATH}")
    print("Keep the token private and allow this port only on trusted/private networks.")
    try:
        server = ThreadingHTTPServer(("0.0.0.0", args.port), Handler)
    except OSError as error:
        logging.error("Could not start Companion listener: %s", error)
        if (args.gui or getattr(sys, "frozen", False)) and not args.no_gui:
            from tkinter import messagebox
            messagebox.showerror("GooPad Companion", f"Could not listen on port {args.port}.\n"
                                "Check whether Companion is already running.")
            return
        parser.error(f"Could not listen on port {args.port}: {error}")
    try:
        if (args.gui or getattr(sys, "frozen", False)) and not args.no_gui:
            desktop_panel(server, config, address, args.port, args.minimized)
        else:
            server.serve_forever()
    except KeyboardInterrupt:
        print("\nGooPad companion stopped.")
    finally:
        server.server_close()


def desktop_panel(server, config, address, port, minimized=False):
    import tkinter as tk
    from tkinter import messagebox, ttk

    root = tk.Tk()
    root.title("GooPad Companion")
    root.geometry("540x380")
    root.resizable(False, False)
    frame = ttk.Frame(root, padding=20)
    frame.pack(fill="both", expand=True)
    ttk.Label(frame, text="GooPad Companion", font=("Segoe UI", 18, "bold")).pack(anchor="w")
    ttk.Label(frame, text="Keep this app running in your logged-in Windows session.").pack(anchor="w", pady=8)
    ttk.Button(frame, text=f"Copy PC address: {address}:{port}",
               command=lambda: copy_text(f"{address}:{port}")).pack(fill="x", pady=4)
    ttk.Button(frame, text="Copy private pairing token",
               command=lambda: copy_text(config["token"])).pack(fill="x", pady=4)
    allowed = tk.BooleanVar(value=config.get("allow_powershell", False))

    def copy_text(value):
        root.clipboard_clear()
        root.clipboard_append(value)

    def change_permission():
        enabled = allowed.get()
        if enabled and not messagebox.askyesno(
            "Allow PowerShell?",
            "Anyone with your pairing token on this LAN can run PowerShell as your Windows user.\n"
            "Only enable this on a trusted private network. Never share the token.",
        ):
            allowed.set(False)
            return
        try:
            latest = load_config()
            latest["allow_powershell"] = enabled
            CONFIG_PATH.write_text(json.dumps(latest, indent=2), encoding="utf-8")
            config["allow_powershell"] = enabled
        except (OSError, ValueError) as error:
            allowed.set(config.get("allow_powershell", False))
            messagebox.showerror("Could not save permission", str(error))

    ttk.Checkbutton(frame, text="Allow PowerShell commands (off by default)",
                    variable=allowed, command=change_permission).pack(anchor="w", pady=10)
    startup = tk.BooleanVar(value=windows_startup.startup_enabled())

    def change_startup():
        try:
            windows_startup.set_startup(startup.get(), APP_DIR, __file__, port)
        except OSError as error:
            startup.set(windows_startup.startup_enabled())
            messagebox.showerror("Could not change Windows startup", str(error))

    ttk.Checkbutton(frame, text="Start with Windows (at sign-in, minimized)",
                    variable=startup, command=change_startup).pack(anchor="w", pady=4)
    state = tk.StringVar(value=LAST_RESULT)
    ttk.Label(frame, textvariable=state, wraplength=490).pack(anchor="w", pady=6)
    ttk.Label(frame, text="In GooPad: Settings > Windows companion > paste address and token.\n"
              "Allow port 8787 only on a Private network. Minimize; closing stops the app.",
              wraplength=490).pack(anchor="w")

    def refresh():
        state.set(LAST_RESULT)
        root.after(500, refresh)

    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    refresh()
    if minimized:
        root.iconify()
    try:
        root.mainloop()
    finally:
        server.shutdown()
        thread.join(timeout=3)


if __name__ == "__main__":
    main()
