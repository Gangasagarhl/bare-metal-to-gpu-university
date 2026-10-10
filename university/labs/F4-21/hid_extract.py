#!/usr/bin/env python3
"""hid_extract.py - find HID report descriptors inside a program file, by their shape.
A report descriptor that describes a pointer or keyboard starts with Usage Page (Generic Desktop)
= 05 01, then a Usage (09 xx), then Collection (Application) = a1 01. From each place that
pattern occurs, the items are walked by their size field (low two bits of the prefix: 0, 1, 2 or
4 data bytes) until the collection depth returns to 0. Output: one line per descriptor,
"offset usage length hex...". Usage: hid_extract.py <file>"""
import re
import sys

data = open(sys.argv[1], "rb").read()
SIZES = (0, 1, 2, 4)
found = 0
for m in re.finditer(rb"\x05\x01\x09[\x00-\xff]\xa1\x01", data):
    start, p, depth = m.start(), m.start(), 0
    while p < len(data) and p - start < 1024:
        prefix = data[p]
        if prefix == 0xFE:                     # long item: [1] = data size, [2] = tag
            p += 3 + data[p + 1]
            continue
        size = SIZES[prefix & 3]
        if prefix & 0xFC == 0xA0:              # Collection
            depth += 1
        p += 1 + size
        if prefix & 0xFC == 0xC0:              # End Collection
            depth -= 1
            if depth == 0:
                break
    if depth != 0:
        print("# %d: pattern found but its collections do not close within 1024 bytes "
              "(incomplete or not a descriptor)" % start)
        continue
    usage = data[start + 3]
    print("%d usage-0x%02x %d %s" % (start, usage, p - start, data[start:p].hex()))
    found += 1
print("# %d descriptors found" % found, file=sys.stderr)
