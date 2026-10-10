#!/usr/bin/env python3
"""The host end of COM2 for the paste test: connect to QEMU's socket chardev, send the
64 KiB file, read back everything the kernel echoes, and compare.

usage: paste_feed.py SOCKET_PATH INPUT_FILE SERIAL_LOG
Like a person who waits for the prompt before pasting, it sends nothing until the kernel's
log (COM1) says "paste: waiting": bytes sent before the driver has set up the UART would
be thrown away when the driver clears the FIFOs."""
import socket
import sys
import time

path, data, log = sys.argv[1], open(sys.argv[2], "rb").read(), sys.argv[3]
for _ in range(200):                       # QEMU creates the socket when it starts
    try:
        s = socket.socket(socket.AF_UNIX)
        s.connect(path)
        break
    except OSError:
        time.sleep(0.1)
else:
    sys.exit("could not connect to " + path)
for _ in range(600):
    try:
        if "paste: waiting" in open(log, errors="replace").read():
            break
    except FileNotFoundError:
        pass
    time.sleep(0.1)
else:
    sys.exit("the kernel never said it was ready")
s.sendall(data)
got = b""
s.settimeout(60)
while len(got) < len(data):
    try:
        chunk = s.recv(65536)
    except TimeoutError:
        print("host:  no echo for 60 s after %d of %d bytes" % (len(got), len(data)))
        break
    if not chunk:
        break
    got += chunk
print("host:  sent %d bytes, echoed back %d bytes, %s" %
      (len(data), len(got), "identical" if got == data else "DIFFERENT"))
sys.exit(0 if got == data else 1)
