#!/usr/bin/env python3
"""F3-21: boot the B4 kernel, wait until it prints its layout, then ask QEMU's monitor
(through QMP's human-monitor-command) for CR3, the mapping view and one page-table walk.

usage: qmp_walk.py SERIAL_FILE qemu-binary qemu-args...
Prints every monitor command and QEMU's reply unchanged. The walk's entry addresses are
computed from the previous reply, exactly as a person would do it by hand.
"""
import json
import re
import subprocess
import sys
import time

serial = sys.argv[1]
qemu = sys.argv[2:] + ["-serial", "file:" + serial, "-qmp", "stdio", "-monitor", "none"]
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


def hmp(cmd, show=True):
    send({"execute": "human-monitor-command", "arguments": {"command-line": cmd}})
    r = reply()
    text = r["return"].replace("\r\n", "\n").rstrip("\n") if r and "return" in r else "ERROR %s" % r
    if show:
        print("(qemu) " + cmd)
        print(text)
    return text


p.stdout.readline()
send({"execute": "qmp_capabilities"})
reply()
deadline = time.time() + 60
log = ""
while time.time() < deadline and "waiting for the QEMU monitor" not in log:
    time.sleep(0.5)
    with open(serial, errors="replace") as f:
        log = f.read()
target = int(re.search(r"walk target: virt ([0-9a-f]+)", log).group(1), 16)
regs = hmp("info registers", show=False)
cr3 = int(re.search(r"CR3=([0-9a-f]+)", regs).group(1), 16)
print("(qemu) info registers   [only the control-register line is shown]")
print([l for l in regs.splitlines() if l.startswith("CR0=")][0])
hmp("info mem")
print("== walk of virtual address %016x by hand ==" % target)
table = cr3 & 0x000FFFFFFFFFF000
for level, shift in (("PML4", 39), ("PDPT", 30), ("PD", 21), ("PT", 12)):
    index = (target >> shift) & 511
    addr = table + 8 * index
    print("-- %s index = (virt >> %d) & 511 = %d; entry at 0x%x + 8 * %d = 0x%x" % (level, shift, index, table, index, addr))
    out = hmp("xp /1gx 0x%x" % addr)
    entry = int(out.split(":")[1].strip(), 16)
    table = entry & 0x000FFFFFFFFFF000
phys = table | (target & 0xFFF)
print("-- frame 0x%x + offset 0x%x = physical 0x%x" % (table, target & 0xFFF, phys))
hmp("xp /1gx 0x%x" % phys)
hmp("gva2gpa 0x%x" % target)
send({"execute": "quit"})
p.wait(timeout=20)
