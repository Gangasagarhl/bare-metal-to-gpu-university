#!/usr/bin/env python3
"""Compare two snapshots of /proc/interrupts and print the lines that changed.

usage: irq_delta.py BEFORE_FILE AFTER_FILE SECONDS
"""
import sys


def load(path):
    rows = {}
    with open(path) as f:
        header = f.readline().split()
        ncpu = len(header)
        for line in f:
            parts = line.split()
            if not parts:
                continue
            name = parts[0].rstrip(":")
            counts = [int(x) for x in parts[1:1 + ncpu] if x.isdigit()]
            rest = " ".join(parts[1 + len(counts):])
            rows[name] = (sum(counts), rest)
    return rows


a, b, dt = load(sys.argv[1]), load(sys.argv[2]), float(sys.argv[3])
print("%-5s %8s %8s  %s" % ("line", "count", "per s", "controller / type / device"))
for name, (total, rest) in b.items():
    if name in a and total != a[name][0]:
        d = total - a[name][0]
        print("%-5s %8d %8.0f  %s" % (name, d, d / dt, rest))
