#!/usr/bin/env python3
"""Boot the F4-02 kernel with 'hold', wait for "F4-02 done" on its serial log, then ask
QEMU's monitor (QMP) for its own PCI listing.

usage: qmp_pci.py SERIAL_FILE OUT_JSON OUT_INFO qemu-binary qemu-args...
Writes query-pci (JSON) to OUT_JSON and the human-readable 'info pci' to OUT_INFO."""
import json
import subprocess
import sys
import time

serial, out_json, out_info = sys.argv[1:4]
qemu = sys.argv[4:] + ["-qmp", "stdio"]
p = subprocess.Popen(qemu, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                     stderr=subprocess.STDOUT, text=True)


def send(obj):
    p.stdin.write(json.dumps(obj) + "\n")
    p.stdin.flush()


def reply():
    while True:
        line = p.stdout.readline()
        if not line:
            return None
        msg = json.loads(line)
        if "event" in msg or "QMP" in msg:
            continue
        return msg


p.stdout.readline()                                   # greeting
send({"execute": "qmp_capabilities"})
reply()
deadline = time.time() + 60
while time.time() < deadline:
    try:
        if "F4-02 done" in open(serial, errors="replace").read():
            break
    except FileNotFoundError:
        pass
    time.sleep(0.2)
else:
    print("timeout waiting for the kernel")
    sys.exit(1)
send({"execute": "query-pci"})
r = reply()
json.dump(r["return"], open(out_json, "w"), indent=1)
send({"execute": "human-monitor-command", "arguments": {"command-line": "info pci"}})
r = reply()
open(out_info, "w").write(r["return"].replace("\r\n", "\n"))
send({"execute": "quit"})
p.wait(timeout=20)
