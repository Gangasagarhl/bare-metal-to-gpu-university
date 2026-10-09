#!/usr/bin/env python3
"""F3-23..F3-25: boot QEMU, wait until the serial log contains MARKER, then run monitor
commands through QMP and print each command with QEMU's reply.

usage: qmp_cmds.py SERIAL_FILE MARKER "cmd1;cmd2;..." qemu-binary qemu-args...
A command "sleep N" waits N seconds instead of talking to QEMU.
"""
import json
import subprocess
import sys
import time

serial, marker, cmds = sys.argv[1], sys.argv[2], [c.strip() for c in sys.argv[3].split(";") if c.strip()]
qemu = sys.argv[4:] + ["-serial", "file:" + serial, "-qmp", "stdio", "-monitor", "none"]
p = subprocess.Popen(qemu, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)


def send(obj):
    p.stdin.write(json.dumps(obj) + "\n")
    p.stdin.flush()


def reply():
    while True:
        line = p.stdout.readline()
        if not line:
            return None
        try:
            msg = json.loads(line)
        except ValueError:            # a warning QEMU printed itself, not a QMP message
            print("qemu: " + line.rstrip("\n"))
            continue
        if "event" in msg or "QMP" in msg:
            continue
        return msg


while True:                   # wait for the QMP greeting; print any warning QEMU writes first
    first = p.stdout.readline()
    if not first or '"QMP"' in first:
        break
    print("qemu: " + first.rstrip("\n"))
send({"execute": "qmp_capabilities"})
reply()
deadline = time.time() + 60
found = False
while time.time() < deadline and not found:
    time.sleep(0.25)
    with open(serial, errors="replace") as f:
        found = marker in f.read()
status = 0 if found else 1
if not found:
    print("marker not seen within 60 s: %r" % marker)
for cmd in cmds:
    if cmd.startswith("sleep "):
        time.sleep(float(cmd.split()[1]))
        continue
    send({"execute": "human-monitor-command", "arguments": {"command-line": cmd}})
    r = reply()
    print("(qemu) " + cmd)
    if r is None or "error" in r:
        print("ERROR: %s" % (r,))
        status = 1
    else:
        print(r["return"].replace("\r\n", "\n").rstrip("\n"))
send({"execute": "quit"})
p.wait(timeout=20)
sys.exit(status)
