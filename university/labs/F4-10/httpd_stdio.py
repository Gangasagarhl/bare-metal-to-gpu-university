#!/usr/bin/env python3
"""httpd_stdio.py - DR302 F4-10: a tiny host-side HTTP/1.0 server for one connection.
QEMU's user-mode network runs it for every guest connection to 10.0.2.100:80 (guestfwd
with "cmd:"), with the connection on stdin and stdout. Serves files from the directory
given as the first argument; appends one line per request to the log file named by the
second. Not stderr: QEMU hands the guest connection to the command as stdin, stdout AND
stderr, so anything written to stderr would arrive in the guest as part of the reply."""
import os, sys

root = os.path.abspath(sys.argv[1])
req = b""
while b"\r\n\r\n" not in req:
    chunk = os.read(0, 4096)
    if not chunk:
        break
    req += chunk
line = req.split(b"\r\n", 1)[0].decode(errors="replace")
parts = line.split()
path = os.path.normpath(os.path.join(root, parts[1].lstrip("/"))) if len(parts) == 3 else ""
if parts and parts[0] == "GET" and path.startswith(os.path.abspath(root)) and os.path.isfile(path):
    body = open(path, "rb").read()
    head = "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nContent-Type: %s\r\n\r\n" % (
        len(body), "text/html" if path.endswith(".html") else "application/octet-stream")
else:
    body = b"not found\n"
    head = "HTTP/1.0 404 Not Found\r\nContent-Length: %d\r\n\r\n" % len(body)
out = head.encode() + body
view = memoryview(out)
while view:
    n = os.write(1, view)
    view = view[n:]
h = 2166136261
for b in body:
    h = ((h ^ b) * 16777619) & 0xFFFFFFFF
with open(sys.argv[2], "a") as log:
    log.write("host httpd: %s -> %s, %d body bytes, fnv1a 0x%08x\n" % (line, head.split("\r\n")[0], len(body), h))
