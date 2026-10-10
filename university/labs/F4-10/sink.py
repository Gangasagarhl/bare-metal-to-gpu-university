#!/usr/bin/env python3
"""sink.py - DR302 F4-10: host-side sink for one guest connection (guestfwd cmd:).
Reads until the guest closes its sending side, then answers with the byte count and the
fnv1a of everything received. Logs to the file named by its argument (stderr would go to
the guest, like stdout)."""
import os, sys

h, n = 2166136261, 0
while True:
    chunk = os.read(0, 65536)
    if not chunk:
        break
    n += len(chunk)
    for b in chunk:
        h = ((h ^ b) * 16777619) & 0xFFFFFFFF
reply = "received %d bytes, fnv1a 0x%08x\n" % (n, h)
os.write(1, reply.encode())
with open(sys.argv[1], "a") as log:
    log.write("host sink: " + reply)
