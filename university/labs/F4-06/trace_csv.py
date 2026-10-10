#!/usr/bin/env python3
"""Turn the BOOTTRACE lines of two boots (healthy, slow) into a boot-trace summary CSV in
the style of a Windows boot-trace tool's driver table, and print it sorted by the slow
boot's init time. The numbers are the lab kernel's own measurements in QEMU (emulated
milliseconds from its 1 kHz PIT tick), not a Windows ETL trace.

usage: trace_csv.py HEALTHY_LOG SLOW_LOG OUT_CSV"""
import csv
import sys


def load(path):
    rows = {}
    for line in open(path, errors="replace"):
        p = line.split()
        if len(p) == 5 and p[0] == "BOOTTRACE":
            rows[p[1]] = (p[2], int(p[3]), int(p[4]))
    return rows


healthy, slow = load(sys.argv[1]), load(sys.argv[2])
out = []
for name in healthy:
    phase, start_h, init_h = healthy[name]
    _, start_s, init_s = slow[name]
    out.append([name, phase, start_s, init_h, init_s, init_s - init_h])
out.sort(key=lambda r: -r[4])
with open(sys.argv[3], "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["driver", "phase", "start_ms_slow", "init_ms_healthy", "init_ms_slow", "delta_ms"])
    w.writerows(out)
print("%-11s %-9s %13s %16s %13s %9s" % ("driver", "phase", "start (slow)", "init healthy", "init slow", "delta"))
for r in out:
    print("%-11s %-9s %10d ms %13d ms %10d ms %+7d ms" % tuple(r))
total_h = sum(r[3] for r in out)
total_s = sum(r[4] for r in out)
print("sum of driver init times: healthy %d ms, slow %d ms" % (total_h, total_s))
