#!/usr/bin/env python3
"""trace_filter.py - keep only one device's accesses from a QEMU memory trace.

    trace_filter.py <QEMU trace file> <region name> <BAR base, hex>

QEMU's memory_region_ops_read / memory_region_ops_write trace events print every access to
every emulated region (firmware, serial port, PCI configuration ports ...). This keeps the
accesses to one region and prints them numbered, as offsets from the BAR base:
    #<n> R|W <offset> <value> (<size> bytes)
QEMU prints values 64 bits wide; values are cut to the access size here.
"""
import re
import sys

path, region, base = sys.argv[1], sys.argv[2], int(sys.argv[3], 16)
n = 0
pat = re.compile(r"memory_region_ops_(read|write) .*addr (0x[0-9a-f]+) value (0x[0-9a-f]+) size (\d+) name '([^']+)'")
for line in open(path, encoding="utf-8", errors="replace"):
    m = pat.search(line)
    if not m or m.group(5) != region:
        continue
    n += 1
    size = int(m.group(4))
    value = int(m.group(3), 16) & ((1 << (8 * size)) - 1)
    print("#%d %s 0x%02x 0x%0*x (%d bytes)" % (n, "R" if m.group(1) == "read" else "W",
                                              int(m.group(2), 16) - base, 2 * size, value, size))
