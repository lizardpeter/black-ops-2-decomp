#!/usr/bin/env python3
"""Use a loopback-only HTTP server; validate the real native test PE imports."""
import http.server
import subprocess
import sys
import threading
from pathlib import Path
import pefile

class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'
    def reply(self, body):
        if self.headers.get('X-XCurl-Request') != 'original-path':
            self.send_error(400, 'Missing reconstructed outgoing header')
            return
        self.send_response(200)
        self.send_header('Content-Length', str(len(body)))
        self.send_header('Content-Type', 'text/plain')
        self.send_header('X-XCurl-Test', 'native')
        self.end_headers()
        self.wfile.write(body)
    def do_GET(self):
        if self.path != '/get':
            self.send_error(404)
            return
        self.reply(b'xcurl-native-get')
    def do_POST(self):
        if self.path != '/echo':
            self.send_error(404)
            return
        self.reply(self.rfile.read(int(self.headers['Content-Length'])))

def main():
    exe = Path(sys.argv[1]).resolve()
    image = pefile.PE(str(exe))
    assert image.FILE_HEADER.Machine == 0x8664
    imports = {e.dll.decode().lower() for e in image.DIRECTORY_ENTRY_IMPORT}
    print('PE x64 imports:', sorted(imports), flush=True)
    assert 'winhttp.dll' in imports
    assert not any('xcurl' in n or 'libcurl' in n or 'bink' in n for n in imports)
    with http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler) as server:
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            subprocess.run([str(exe), str(server.server_port)], check=True, timeout=90)
        finally:
            server.shutdown()
            thread.join()

if __name__ == '__main__':
    main()
