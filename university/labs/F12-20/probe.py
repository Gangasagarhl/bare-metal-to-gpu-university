# probe.py - add a probe instead of guessing: which call of send_error() produced each 404?
# Serves an empty folder on loopback, makes the two requests of the evidence pack, and
# prints, for every send_error() call, the file and line of its caller and the call chain.
import http.client
import http.server
import os
import sys
import tempfile
import threading


class ProbedHandler(http.server.SimpleHTTPRequestHandler):
    def send_error(self, code, message=None, explain=None):
        caller = sys._getframe(1)  # the frame that called send_error()
        chain = []
        frame = caller
        while frame is not None and len(chain) < 4:
            chain.append(frame.f_code.co_name)
            frame = frame.f_back
        where = os.path.basename(caller.f_code.co_filename)
        print("probe: %s -> send_error(%d) called from %s line %d; chain: %s"
              % (self.path, code, where, caller.f_lineno, " <- ".join(chain)), flush=True)
        super().send_error(code, message, explain)

    def log_message(self, format, *args):
        pass  # the evidence pack already has the log lines; keep this output short


with tempfile.TemporaryDirectory() as www:
    handler = lambda *a, **k: ProbedHandler(*a, directory=www, **k)
    server = http.server.HTTPServer(("127.0.0.1", 0), handler)
    port = server.server_address[1]
    threading.Thread(target=server.serve_forever, daemon=True).start()
    for path in ("/nope.txt", "/missing-dir/"):
        conn = http.client.HTTPConnection("127.0.0.1", port, timeout=5)
        conn.request("GET", path)
        print("client: %s -> status %d" % (path, conn.getresponse().status), flush=True)
        conn.close()
    server.shutdown()
    server.server_close()
