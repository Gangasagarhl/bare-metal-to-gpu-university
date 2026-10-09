# hmp.py - OS305 lab helper (not a chapter listing).
# usage: hmp.py <socket> <command>...  -- send HMP monitor commands to a QEMU started with
# -monitor unix:<socket>,server=on,wait=off and print QEMU's replies (escape codes removed)
import socket, sys, time, re
s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
for _ in range(50):
    try:
        s.connect(sys.argv[1]); break
    except OSError:
        time.sleep(0.1)
s.settimeout(0.5)
def drain():
    out = b""
    try:
        while True:
            d = s.recv(4096)
            if not d: break
            out += d
    except OSError:
        pass
    return out
drain()
for cmd in sys.argv[2:]:
    s.sendall(cmd.encode() + b"\n")
    time.sleep(0.3)
    text = re.sub(r"\x1b\[[0-9;]*[A-Za-z]", "", drain().decode(errors="replace")).replace("\r", "")
    print("(qemu) " + cmd)
    for line in text.splitlines():
        if line.strip() and not line.startswith("(qemu)") and cmd not in line:
            print(line)
