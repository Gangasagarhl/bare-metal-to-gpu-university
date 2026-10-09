#!/usr/bin/env python3
"""timeline.py - compare the stage times of two bring-up logs.
Reads the "end <stage> ticks <N>" lines of each log (time-stamp counter ticks, not converted
to seconds), prints one row per stage and names the stage that grew the most. Exit code 1 when a
stage grew more than 10 times AND by more than 100,000,000 ticks (a regression worth a look);
0 otherwise. Usage: timeline.py before.log after.log"""
import re
import sys

def stages(path):
    out = {}
    for line in open(path, encoding="utf-8", errors="replace"):
        m = re.search(r"\] end (\S+) ticks (\d+)", line)
        if m:
            out[m.group(1)] = int(m.group(2))
    return out

a, b = stages(sys.argv[1]), stages(sys.argv[2])
print("%-10s %14s %14s %10s" % ("stage", "before", "after", "after/before"))
worst, worst_growth = None, 0
for name in a:
    if name not in b:
        print("%-10s %14d %14s %10s" % (name, a[name], "MISSING", "-"))
        worst, worst_growth = name, float("inf")
        continue
    ratio = b[name] / a[name] if a[name] else float("inf")
    print("%-10s %14d %14d %10.1f" % (name, a[name], b[name], ratio))
    growth = b[name] - a[name]
    if ratio > 10 and growth > 100_000_000 and growth > worst_growth:
        worst, worst_growth = name, growth
print("total      %14d %14d" % (sum(a.values()), sum(b.values())))
if worst:
    print("REGRESSION: stage '%s' grew by %s ticks; read that stage's lines in the after log"
          % (worst, worst_growth))
    sys.exit(1)
print("no stage grew more than 10 times by more than 100000000 ticks")
