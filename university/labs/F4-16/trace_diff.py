#!/usr/bin/env python3
"""trace_diff.py - compare two filtered traces of the same device, access by access.

    trace_diff.py <reference trace> <other trace>

Read-only scans (16 or more reads at offsets 0, 4, 8, ...) and runs of reads of one offset (a poll
loop may run a different number of times) are folded first, so that timing does not hide real
differences.
Writes are compared with their values, reads by offset only.
"""
import difflib
import re
import sys


def load(path):
    acc = []
    for line in open(path, encoding="utf-8"):
        m = re.match(r"#\d+ ([RW]) (0x\w+) (0x\w+)", line)
        if m:
            acc.append((m.group(1), int(m.group(2), 16), int(m.group(3), 16)))
    out, i = [], 0
    while i < len(acc):
        j = i
        while j < len(acc) and acc[j][0] == "R" and acc[j][1] == 4 * (j - i):
            j += 1
        if j - i >= 16:
            out.append("scan 0x00-0x%02x" % acc[j - 1][1]); i = j; continue
        if acc[i][0] == "W":
            out.append("W 0x%02x = 0x%08x" % acc[i][1:]); i += 1; continue
        j = i
        while j < len(acc) and acc[j][0] == "R" and acc[j][1] == acc[i][1]:
            j += 1
        out.append("read(s) of 0x%02x" % acc[i][1]); i = j
    return out


a, b = load(sys.argv[1]), load(sys.argv[2])
print("reference: %d steps, other: %d steps (after folding scans and polls)" % (len(a), len(b)))
diffs = 0
for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(a=a, b=b, autojunk=False).get_opcodes():
    if tag == "equal":
        continue
    diffs += 1
    print("%s at reference step %d:" % (tag, i1 + 1))
    for x in a[i1:i2]:
        print("    reference only: " + x)
    for x in b[j1:j2]:
        print("    other only:     " + x)
print("%d difference(s)" % diffs)
sys.exit(0 if diffs == 0 else 1)
