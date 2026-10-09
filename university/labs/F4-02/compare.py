#!/usr/bin/env python3
"""Acceptance test C1: compare the kernel's PCI list with QEMU's own (query-pci).

usage: compare.py KERNEL_SERIAL_LOG QUERY_PCI_JSON
Compared per function: bus/device/function, vendor and device ID, class and subclass,
and every BAR 0-5 (I/O or memory, address, size). The expansion ROM (QEMU's bar 6) is
not sized by the kernel and is left out."""
import json
import re
import sys

kernel = {}
cur = None
for line in open(sys.argv[1]):
    m = re.match(r"PCI (\w\w):(\w\w)\.(\w) (\w{4}):(\w{4}) class (\w\w)\.(\w\w)\.(\w\w)", line)
    if m:
        bdf = (int(m[1], 16), int(m[2], 16), int(m[3], 16))
        cur = kernel[bdf] = {"id": (int(m[4], 16), int(m[5], 16)),
                             "class": int(m[6] + m[7], 16), "bars": {}}
        continue
    m = re.match(r"\s+BAR(\d) (io|mem32|mem64)\s+0x(\w+) size 0x(\w+)", line)
    if m and cur is not None:
        cur["bars"][int(m[1])] = ("io" if m[2] == "io" else "memory", int(m[3], 16), int(m[4], 16))

qemu = {}


def walk(devices):
    for d in devices:
        bdf = (d["bus"], d["slot"], d["function"])
        bars = {}
        for r in d.get("regions", []):
            if r["bar"] <= 5:
                bars[r["bar"]] = (r["type"], r["address"], r["size"])
        qemu[bdf] = {"id": (d["id"]["vendor"], d["id"]["device"]),
                     "class": d["class_info"]["class"], "bars": bars}
        if "pci_bridge" in d and "devices" in d["pci_bridge"]:
            walk(d["pci_bridge"]["devices"])


for bus in json.load(open(sys.argv[2])):
    walk(bus["devices"])

problems = 0
for bdf in sorted(set(kernel) | set(qemu)):
    name = "%02x:%02x.%x" % bdf
    k, q = kernel.get(bdf), qemu.get(bdf)
    if k is None or q is None:
        print("%s only in %s" % (name, "QEMU" if k is None else "kernel"))
        problems += 1
        continue
    diffs = [f for f in ("id", "class", "bars") if k[f] != q[f]]
    if diffs:
        problems += 1
        print("%s DIFFERENT in %s" % (name, ", ".join(diffs)))
        for f in diffs:
            print("    kernel: %s\n    qemu:   %s" % (k[f], q[f]))
    else:
        print("%s %04x:%04x class %04x, %d BAR(s): same" % (name, *k["id"], k["class"], len(k["bars"])))
print("functions: kernel %d, QEMU %d; %s" % (len(kernel), len(qemu),
      "MATCH" if problems == 0 else "%d DIFFERENCE(S)" % problems))
sys.exit(0 if problems == 0 else 1)
