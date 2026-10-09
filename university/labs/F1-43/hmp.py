#!/usr/bin/env python3
"""Run QEMU with a QMP channel on stdin/stdout, send HMP (monitor) commands, print the replies.

usage: hmp.py WAIT_SECONDS "cmd1;cmd2;..." qemu-binary qemu-args...
"""
import json
import subprocess
import sys
import time

wait = float(sys.argv[1])
commands = [c.strip() for c in sys.argv[2].split(";") if c.strip()]
qemu = sys.argv[3:] + ["-qmp", "stdio", "-monitor", "none"]
p = subprocess.Popen(qemu, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                     stderr=subprocess.STDOUT, text=True)


def send(obj):
    p.stdin.write(json.dumps(obj) + "\n")
    p.stdin.flush()


def reply():
    """Return the next QMP reply that is not an asynchronous event."""
    while True:
        line = p.stdout.readline()
        if not line:
            return None
        try:
            msg = json.loads(line)
        except json.JSONDecodeError:
            print("[qemu] " + line.rstrip())
            continue
        if "event" in msg or "QMP" in msg:
            continue
        return msg


p.stdout.readline()                    # QMP greeting
send({"execute": "qmp_capabilities"})
reply()
time.sleep(wait)                       # let the firmware run (assigns PCI addresses)
status = 0
for cmd in commands:
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
