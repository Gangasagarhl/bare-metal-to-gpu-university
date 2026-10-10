# compare.py - BR-05 Listing 12: one table from the SUMMARY lines of every world's run.
# Each world is compared with its OWN period (emulated clocks on the microcontroller, real
# nanoseconds on Linux), because the two units cannot be compared directly.
import sys


def summary(path):
    for line in open(path, encoding="utf-8"):
        if line.startswith("SUMMARY "):
            return dict(kv.split("=", 1) for kv in line.split()[1:])
    raise SystemExit("no SUMMARY line in " + path)


rows = [summary(p) for p in sys.argv[1:]]
print("%-28s %-6s %8s %8s %8s %9s %9s  %s" % (
    "world", "unit", "min", "max", "jitter", "jitter/T", "late>=T", "checksum"))
for r in rows:
    period = int(r["period"])
    jitter = int(r["jitter"])
    print("%-28s %-6s %8s %8s %8s %8.2f%% %9s  %s" % (
        r["world"], r["unit"], r["min"], r["max"], r["jitter"], 100.0 * jitter / period,
        r["late_by_a_period_or_more"], r["checksum"]))
same = len({r["checksum"] for r in rows}) == 1
print("same control output in every world:", "yes" if same else "NO")
sys.exit(0 if same else 1)
