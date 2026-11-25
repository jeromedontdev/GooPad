"""Check and download the official Windows Companion installer."""

import hashlib
import json
import os
import re
import ssl
import tempfile
from pathlib import Path
from urllib.parse import urlsplit
from urllib.request import HTTPSHandler, HTTPRedirectHandler, Request, build_opener

import certifi


VERSION = "0.7.3"
RELEASE_API = "https://api.github.com/repos/jeromedontdev/GooPad/releases/latest"
DOWNLOAD_ROOT = "https://github.com/jeromedontdev/GooPad/releases/download"
MAX_METADATA = 2 * 1024 * 1024
MAX_INSTALLER = 100 * 1024 * 1024
VERSION_PATTERN = re.compile(r"(0|[1-9]\d{0,4})\.(0|[1-9]\d{0,4})\.(0|[1-9]\d{0,4})\Z")


def version_parts(version):
    match = VERSION_PATTERN.fullmatch(version) if isinstance(version, str) else None
    if not match:
        raise ValueError("Invalid stable release version.")
    return tuple(map(int, match.groups()))


class ReleaseRedirects(HTTPRedirectHandler):
    def redirect_request(self, request, fp, code, message, headers, new_url):
        target = urlsplit(new_url)
        if (target.scheme != "https" or target.hostname not in
                ("github.com", "release-assets.githubusercontent.com") or
                target.username is not None or target.password is not None or
                target.port not in (None, 443)):
            raise ValueError("The release download redirected to an untrusted address.")
        return super().redirect_request(request, fp, code, message, headers, new_url)


def release_opener():
    context = ssl.create_default_context()
    context.load_verify_locations(cafile=certifi.where())
    return build_opener(ReleaseRedirects(), HTTPSHandler(context=context))


def check_for_update():
    opener = release_opener()
    request = Request(RELEASE_API, headers={"User-Agent": "GooPad-Companion",
                                            "Accept": "application/vnd.github+json"})
    with opener.open(request, timeout=30) as response:
        data = response.read(MAX_METADATA + 1)
    if len(data) > MAX_METADATA:
        raise ValueError("Release information is too large.")
    release = json.loads(data)
    if not isinstance(release, dict):
        raise ValueError("Invalid release information.")
    tag = release.get("tag_name")
    if not isinstance(tag, str):
        raise ValueError("Release has no version.")
    version = tag[1:] if tag.startswith("v") else tag
    newer = version_parts(version) > version_parts(VERSION)
    if not newer:
        return None
    assets = release.get("assets")
    if not isinstance(assets, list):
        raise ValueError("Release has no installer list.")
    name = f"GooPadCompanion-Setup-{version}.exe"
    matches = [asset for asset in assets if isinstance(asset, dict) and asset.get("name") == name]
    if len(matches) != 1:
        raise ValueError("The latest release does not have a Companion installer.")
    asset = matches[0]
    size, digest = asset.get("size"), asset.get("digest")
    if (type(size) is not int or not 0 < size <= MAX_INSTALLER or not isinstance(digest, str)
            or not re.fullmatch(r"sha256:[0-9a-f]{64}", digest)):
        raise ValueError("The installer has invalid size or SHA-256 metadata.")
    return {"version": version, "size": size, "digest": digest[7:],
            "url": f"{DOWNLOAD_ROOT}/{tag}/{name}"}


def download_update(release):
    opener = release_opener()
    request = Request(release["url"], headers={"User-Agent": "GooPad-Companion"})
    descriptor, name = tempfile.mkstemp(prefix="GooPadCompanion-Setup-", suffix=".exe")
    path = Path(name)
    complete = False
    try:
        digest = hashlib.sha256()
        received = 0
        with os.fdopen(descriptor, "wb") as file, opener.open(request, timeout=30) as response:
            while True:
                data = response.read(min(65536, release["size"] - received + 1))
                if not data:
                    break
                received += len(data)
                if received > release["size"]:
                    raise ValueError("The installer is larger than advertised.")
                digest.update(data)
                file.write(data)
        if received != release["size"] or digest.hexdigest() != release["digest"]:
            raise ValueError("The installer did not match the release SHA-256 and size.")
        complete = True
        return path
    finally:
        if not complete:
            path.unlink(missing_ok=True)
