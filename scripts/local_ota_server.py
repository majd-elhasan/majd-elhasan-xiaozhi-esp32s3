#!/usr/bin/env python3
"""
Local OTA server for Xiaozhi firmware testing.

Endpoints:
- GET/POST /xiaozhi/ota/         -> OTA check JSON (firmware version/url)
- POST     /xiaozhi/ota/activate -> activation success JSON
- GET      /xiaozhi.bin          -> firmware binary
"""

from __future__ import annotations

import argparse
import json
import os
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Tuple


def utc_millis() -> int:
    return int(datetime.now(timezone.utc).timestamp() * 1000)


class OtaHandler(BaseHTTPRequestHandler):
    server_version = "XiaozhiLocalOTA/1.0"

    def _send_json(self, status: int, data: dict) -> None:
        body = json.dumps(data, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _send_file(self, file_path: str, content_type: str) -> None:
        try:
            file_size = os.path.getsize(file_path)
            self.send_response(200)
            self.send_header("Content-Type", content_type)
            self.send_header("Content-Length", str(file_size))
            self.end_headers()
            with open(file_path, "rb") as f:
                while True:
                    chunk = f.read(64 * 1024)
                    if not chunk:
                        break
                    self.wfile.write(chunk)
        except FileNotFoundError:
            self._send_json(404, {"error": "File not found"})

    def _ota_payload(self) -> dict:
        cfg = self.server.cfg  # type: ignore[attr-defined]
        firmware_url = f"http://{cfg['public_host']}:{cfg['port']}/{cfg['bin_name']}"
        return {
            "server_time": {
                "timestamp": utc_millis(),
                "timezone_offset": 0,
            },
            "firmware": {
                "version": cfg["version"],
                "url": firmware_url,
                "force": 1,
            },
        }

    def _read_body(self) -> bytes:
        length = int(self.headers.get("Content-Length", "0"))
        if length <= 0:
            return b""
        return self.rfile.read(length)

    def log_message(self, fmt: str, *args) -> None:
        # Keep logs concise and timestamped.
        now = datetime.now().strftime("%H:%M:%S")
        print(f"[{now}] {self.client_address[0]} - {fmt % args}")

    def do_GET(self) -> None:
        cfg = self.server.cfg  # type: ignore[attr-defined]
        if self.path in ("/", "/health"):
            self._send_json(
                200,
                {
                    "ok": True,
                    "ota_check_url": f"http://{cfg['public_host']}:{cfg['port']}/xiaozhi/ota/",
                    "firmware_url": f"http://{cfg['public_host']}:{cfg['port']}/{cfg['bin_name']}",
                },
            )
            return
        if self.path == "/xiaozhi/ota/":
            self._send_json(200, self._ota_payload())
            return
        if self.path == f"/{cfg['bin_name']}":
            self._send_file(cfg["bin_path"], "application/octet-stream")
            return
        self._send_json(404, {"error": "Not found", "path": self.path})

    def do_POST(self) -> None:
        if self.path == "/xiaozhi/ota/":
            # Device posts system info here. We don't need to parse it for local testing.
            _ = self._read_body()
            self._send_json(200, self._ota_payload())
            return
        if self.path == "/xiaozhi/ota/activate":
            _ = self._read_body()
            self._send_json(200, {"ok": True, "message": "activated"})
            return
        self._send_json(404, {"error": "Not found", "path": self.path})


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Run a local Xiaozhi OTA test server.")
    p.add_argument(
        "--bin",
        default=os.path.join("build", "xiaozhi.bin"),
        help="Path to firmware .bin (default: build/xiaozhi.bin)",
    )
    p.add_argument(
        "--version",
        default="9.9.9-local",
        help="Version string reported by OTA API (default: 9.9.9-local)",
    )
    p.add_argument("--host", default="0.0.0.0", help="Bind host (default: 0.0.0.0)")
    p.add_argument("--port", type=int, default=8000, help="Bind port (default: 8000)")
    p.add_argument(
        "--public-host",
        default=None,
        help="LAN IP/hostname used in generated firmware URL. Default: bind host or 127.0.0.1",
    )
    return p.parse_args()


def resolve_config(args: argparse.Namespace) -> Tuple[str, dict]:
    bin_path = os.path.abspath(args.bin)
    if not os.path.exists(bin_path):
        raise FileNotFoundError(f"Firmware binary not found: {bin_path}")
    bin_name = os.path.basename(bin_path)

    if args.public_host:
        public_host = args.public_host
    elif args.host in ("0.0.0.0", "::"):
        public_host = "127.0.0.1"
    else:
        public_host = args.host

    cfg = {
        "bin_path": bin_path,
        "bin_name": bin_name,
        "version": args.version,
        "host": args.host,
        "port": args.port,
        "public_host": public_host,
    }
    return bin_path, cfg


def main() -> None:
    args = parse_args()
    _, cfg = resolve_config(args)

    server = ThreadingHTTPServer((cfg["host"], cfg["port"]), OtaHandler)
    server.cfg = cfg  # type: ignore[attr-defined]

    print("Local OTA server started")
    print(f"- OTA check URL : http://{cfg['public_host']}:{cfg['port']}/xiaozhi/ota/")
    print(f"- Firmware URL  : http://{cfg['public_host']}:{cfg['port']}/{cfg['bin_name']}")
    print(f"- Serving file  : {cfg['bin_path']}")
    print("Press Ctrl+C to stop.")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()

