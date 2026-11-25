#!/usr/bin/env python3
"""Opt-in genuine GDK acceptance, with only local loopback traffic.

Requires the independently linked Windows x64 native_xcurl_consumer.exe
and a licensed, usable Gaming Services runtime. Does not install software,
change trust policy, modify game binaries, or substitute any GDK functions.
"""
import argparse
import http.server
import subprocess
import threading
from pathlib import Path

class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    def do_GET(self):
        if self.path != "/get" or self.headers.get("X-XCurl-Consumer") != "native":
            self.send_error(400)
            return
        data = b"xcurl-live-gdk-loopback"
        self.send_response(200)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    args = parser.parse_args()
    exe = args.executable.resolve(strict=True)
    with http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler) as server:
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            subprocess.run([str(exe), "--loopback-port", str(server.server_port)], check=True, timeout=75)
        finally:
            server.shutdown()
            thread.join()

if __name__ == "__main__":
    main()
