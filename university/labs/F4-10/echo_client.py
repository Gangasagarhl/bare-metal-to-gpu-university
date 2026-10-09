#!/usr/bin/env python3
"""echo_client.py - DR302 F4-10: host TCP client for the kernel's echo server.
Waits until the kernel says it listens, connects to the forwarded port, sends N random
bytes (fixed seed) while reading the echo, then compares. Usage: echo_client.py port N log"""
import hashlib, random, socket, sys, threading, time

port, total, log = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3]
end = time.time() + 60
while time.time() < end:
    try:
        if "echo: listening" in open(log, errors="replace").read():
            break
    except FileNotFoundError:
        pass
    time.sleep(0.1)
data = random.Random(302).randbytes(total)
s = socket.create_connection(("127.0.0.1", port), timeout=60)
got = bytearray()
def reader():
    while len(got) < total:
        chunk = s.recv(65536)
        if not chunk:
            break
        got.extend(chunk)
t = threading.Thread(target=reader)
t.start()
start = time.time()
s.sendall(data)
t.join(120)
elapsed = time.time() - start
s.shutdown(socket.SHUT_WR)
s.close()
print("host echo client: sent %d bytes, received %d bytes back in %.1f s (wall clock), identical: %s, sha256 %s"
      % (total, len(got), elapsed, bytes(got) == data, hashlib.sha256(data).hexdigest()[:16]))
