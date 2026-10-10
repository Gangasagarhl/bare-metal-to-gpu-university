#!/usr/bin/env python3
"""qemu_devices.py - the emulator as a catalog: QEMU's own list of the devices it can emulate
(qemu-system-x86_64 -device help, read on standard input), counted per QEMU category, with the
names in the categories this course touches."""
import re
import sys

cats, cur = {}, None
for line in (l.rstrip("\n") for l in sys.stdin):
    if line.strip().endswith("devices:"):
        cur = line.strip()[:-1]
        cats[cur] = []
    m = re.match(r'name "([^"]+)"(?:, bus ([^,]+))?(?:, alias "[^"]+")?(?:, desc "([^"]*)")?', line)
    if m and cur:
        cats[cur].append((m.group(1), m.group(2) or "-", m.group(3) or ""))
print("%d device types in %d categories" % (sum(len(v) for v in cats.values()), len(cats)))
for c, devs in cats.items():
    print("%-30s %3d" % (c, len(devs)))
for c in ("Storage devices", "Input devices", "Watchdog devices", "Sound devices"):
    print("== %s ==" % c)
    for name, bus, desc in cats.get(c, []):
        print("  %-24s bus %-14s %s" % (name, bus, desc))
