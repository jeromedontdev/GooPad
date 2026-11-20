#!/usr/bin/env python3
"""Small, token-authenticated Windows webhook receiver for GooPad."""

import argparse
import hmac
import ipaddress
import json
import os
import secrets
import socket
import subprocess
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse


APP_DIR = Path(os.environ.get("LOCALAPPDATA", Path.home())) / "GooPad"
CONFIG_PATH = APP_DIR / "companion.json"
MAX_BODY = 4096


def load_config():
    APP_DIR.mkdir(parents=True, exist_ok=True)
    if not CONFIG_PATH.exists():
        CONFIG_PATH.write_text(
            json.dumps({"token": secrets.token_urlsafe(32), "actions": {}}, indent=2),
            encoding="utf-8",
        )
    config = json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
    if not isinstance(config.get("token"), str) or len(config["token"]) < 32:
        raise ValueError(f"Set a random token of at least 32 characters in {CONFIG_PATH}")
    if not isinstance(config.get("actions"), dict):
        raise ValueError(f"'actions' must be an object in {CONFIG_PATH}")
    return config


def lan_address():
    probe = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        probe.connect(("192.0.2.1", 1))
        return probe.getsockname()[0]
    except OSError:
        return "127.0.0.1"
    finally:
        probe.close()


class Handler(BaseHTTPRequestHandler):
    server_version = "GooPadCompanion"
    sys_version = ""

    def log_message(self, fmt, *args):
        print(f"{self.client_address[0]} - {fmt % args}")

    def respond(self, status, message):
        payload = json.dumps({"ok": status < 400, "message": message}).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(payload)

    def do_GET(self):
        if self.path != "/health":
            return self.respond(404, "Not found")
        return self.respond(200, "GooPad companion is running")

    def do_POST(self):
        try:
            peer = ipaddress.ip_address(self.client_address[0])
        except ValueError:
            return self.respond(403, "Invalid peer address")
        if not (peer.is_private or peer.is_loopback):
            return self.respond(403, "LAN clients only")

        config = load_config()
        supplied = self.headers.get("X-GooPad-Token", "")
        if not hmac.compare_digest(supplied, config["token"]):
            return self.respond(401, "Invalid GooPad token")

        action_id = urlparse(self.path).path.removeprefix("/hook/").strip("/")
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
    args = parser.parse_args()
    if not 1024 <= args.port <= 65535:
        parser.error("port must be between 1024 and 65535")
    config = load_config()
    address = lan_address()
    print(f"GooPad companion listening on http://0.0.0.0:{args.port}")
    print(f"Health check: http://{address}:{args.port}/health")
    print(f"Shared token: {config['token']}")
    print(f"Add actions in: {CONFIG_PATH}")
    print("Keep the token private and allow this port only on trusted/private networks.")
    try:
        ThreadingHTTPServer(("0.0.0.0", args.port), Handler).serve_forever()
    except KeyboardInterrupt:
        print("\nGooPad companion stopped.")


if __name__ == "__main__":
    main()
