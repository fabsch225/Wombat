#!/usr/bin/env python3
# Local test server with the cross-origin isolation headers the threaded Wasm build needs.
# Usage: python3 web/serve.py [dir] [port]   (default: build-wasm/site, 8000)
import http.server, sys, functools

class Handler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        super().end_headers()

d = sys.argv[1] if len(sys.argv) > 1 else 'build-wasm/site'
port = int(sys.argv[2]) if len(sys.argv) > 2 else 8000
http.server.ThreadingHTTPServer(('', port), functools.partial(Handler, directory=d)).serve_forever()
