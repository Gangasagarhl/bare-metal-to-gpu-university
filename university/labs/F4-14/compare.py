#!/usr/bin/env python3
"""compare.py - check the lab kernel's PCI device report against QEMU's own list.

    compare.py <kernel report file> <QEMU device options...>

Starts the same QEMU machine paused (-S), asks it for its PCI devices over QMP
("query-pci"), and compares, function by function: vendor and device ID, subsystem IDs,
class (base and subclass) and the size of every BAR. Exit status 0 = identical, 1 = not.
"""
import json
import re
import subprocess
import sys

report, qemu_args = sys.argv[1], sys.argv[2:]
kern = {}
cur = None
for line in open(report, encoding="utf-8"):
    m = re.match(r"pci (\w\w):(\w\w)\.(\w) id (\w{4}):(\w{4}) sub (\w{4}):(\w{4}) rev \w\w class (\w\w) (\w\w) \w\w", line)
    if m:
        b, d, f = (int(x, 16) for x in m.group(1, 2, 3))
        cur = (b, d, f)
        kern[cur] = {"id": (int(m.group(4), 16), int(m.group(5), 16)),
                     "sub": (int(m.group(6), 16), int(m.group(7), 16)),
                     "class": int(m.group(8), 16) << 8 | int(m.group(9), 16), "bars": {}}
        continue
    m = re.match(r"\s+bar(\d) \w+\s+base 0x\w+ size 0x(\w+)", line)
    if m and cur:
        kern[cur]["bars"][int(m.group(1))] = int(m.group(2), 16)

cmds = '{"execute":"qmp_capabilities"}\n{"execute":"query-pci"}\n{"execute":"quit"}\n'
out = subprocess.run(["qemu-system-x86_64", "-machine", "q35", "-m", "128M", "-nodefaults", "-display", "none",
                      "-S", "-qmp", "stdio"] + qemu_args, input=cmds, capture_output=True, text=True, timeout=60).stdout
qemu = {}
for line in out.splitlines():
    try:
        j = json.loads(line)
    except ValueError:
        continue
    if isinstance(j.get("return"), list):
        for bus in j["return"]:
            for d in bus["devices"]:
                key = (d["bus"], d["slot"], d["function"])
                qemu[key] = {"id": (d["id"]["vendor"], d["id"]["device"]),
                             "sub": (d["id"].get("subsystem-vendor", 0), d["id"].get("subsystem", 0)),
                             "class": d["class_info"]["class"],
                             "bars": {r["bar"]: r["size"] for r in d.get("regions", []) if r["bar"] < 6}}

diffs = 0
for key in sorted(set(kern) | set(qemu)):
    name = "%02x:%02x.%x" % key
    if key not in kern:
        print("%s  MISSING from the kernel's report (QEMU has %04x:%04x class %04x)" % ((name,) + qemu[key]["id"] + (qemu[key]["class"],)))
        diffs += 1
    elif key not in qemu:
        print("%s  in the kernel's report but not in QEMU's list" % name)
        diffs += 1
    else:
        bad = [k for k in ("id", "sub", "class", "bars") if kern[key][k] != qemu[key][k]]
        print("%s  %04x:%04x  %s" % ((name,) + kern[key]["id"] + ("match (IDs, subsystem, class, BAR sizes)" if not bad
                                                               else "DIFFERS in " + ", ".join(bad),)))
        diffs += bool(bad)
print("%d functions in QEMU's list, %d in the kernel's report: %s" %
      (len(qemu), len(kern), "identical" if diffs == 0 else "%d difference(s)" % diffs))
sys.exit(0 if diffs == 0 else 1)
