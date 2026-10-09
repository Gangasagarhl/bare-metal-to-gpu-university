# serial_session.py - F3-35: drive a QEMU serial console like a person at a terminal: wait for the
# prompt, type one command line, wait for the next prompt, and so on. Prints the whole transcript.
#   python3 serial_session.py <session file> <prompt> <timeout s> -- <qemu command ...>
import os
import subprocess
import sys
import time

session, prompt, limit = sys.argv[1], sys.argv[2].encode(), float(sys.argv[3])
cmd = sys.argv[sys.argv.index("--") + 1:]
lines = open(session).read().splitlines()
p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
buf = b""
deadline = time.monotonic() + limit

def wait_for(token):
    """Read the console until it ends with token; False on end of output or time limit."""
    global buf
    while not buf.endswith(token):
        if time.monotonic() > deadline:
            return False
        chunk = os.read(p.stdout.fileno(), 4096)
        if not chunk:
            return False
        buf += chunk.replace(b"\r", b"")
    return True

ok = True
for line in lines:
    if not wait_for(prompt):
        ok = False
        break
    p.stdin.write(line.encode() + b"\n")
    p.stdin.flush()
while True:                                      # collect the rest until QEMU exits
    if time.monotonic() > deadline:
        p.kill()
        ok = False
        break
    chunk = os.read(p.stdout.fileno(), 4096)
    if not chunk:
        break
    buf += chunk.replace(b"\r", b"")
rc = p.wait()
sys.stdout.write(buf.decode(errors="replace"))
sys.exit(rc if ok else 124)
