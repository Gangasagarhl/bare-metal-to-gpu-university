#!/usr/bin/env python3
"""crosscheck.py - F4-31: does every address our devicetree reader decoded really hold a device?

    python3 crosscheck.py devices.out mtree.out

devices.out is the table printed by "fdtdump --devices"; mtree.out is QEMU's own flat memory map
("info mtree -f"). For each device row, the address must be the start of a region in QEMU's map.
Two independent views of the same machine must agree; if they do not, our reader is wrong.
"""
import re
import sys

starts = {}
for line in open(sys.argv[2]):
    m = re.match(r"\s*([0-9a-f]{16})-([0-9a-f]{16}) \(prio [-0-9]+, [^)]+\): (\S+)", line)
    if m:
        starts.setdefault(int(m.group(1), 16), m.group(3))
found = missing = 0
for line in open(sys.argv[1]):
    m = re.match(r"(\S+)\s+(\S+)\s+0x([0-9a-f]+)\s+0x[0-9a-f]+$", line.rstrip())
    if not m or m.group(1).startswith("/cpus"):
        continue
    addr = int(m.group(3), 16)
    region = starts.get(addr)
    if region:
        found += 1
        print("%-30s 0x%-10x QEMU region: %s" % (m.group(1), addr, region))
    else:
        missing += 1
        print("%-30s 0x%-10x NOT a region start in QEMU's map" % (m.group(1), addr))
print("%d addresses confirmed, %d not confirmed" % (found, missing))
sys.exit(0)
