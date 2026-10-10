"""report.py - MP7 SITL test report generator (milestone 1 evidence for gate R2).

Reads sitl_results.csv (written by sitl_feature.cpp) and prints a test report that names
exactly which sources were tested (short SHA-256 of each file), the verdicts, the margins
to each limit, and the open items. Usage: python3 report.py <lab folder>
"""
import csv
import hashlib
import os
import re
import sys

lab = sys.argv[1] if len(sys.argv) > 1 else "."
files = ["est_guard.h", "mp7_sitl.h", "sitl_feature.cpp", "../F10-33/dronesim.hpp"]


def short_hash(path):
    with open(os.path.join(lab, path), "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()[:12]


rows = list(csv.DictReader(open(os.path.join(lab, "sitl_results.csv"), newline="")))

print("MP7 SITL TEST REPORT - milestone 1 (estimator guard)")
print("configuration under test (short SHA-256 of each source):")
for f in files:
    print("  %-26s %s" % (f, short_hash(f)))
print("simulator: DN401 course simulator, lockstep, 10 ms step; NOT PX4, NOT ArduPilot")
print()

kinds = {}
for r in rows:
    kinds.setdefault(r["kind"], []).append(r["verdict"])
for k in ("TEST", "CONTROL", "GAP"):
    v = kinds.get(k, [])
    print("%-8s %2d rows: %s" % (k, len(v), ", ".join("%s x%d" % (x, v.count(x)) for x in sorted(set(v)))))
print()

print("margins (limit written before the run minus what was measured):")
for r in rows:
    lat = float(r["latency_s"])
    m = re.search(r"<= ([0-9.]+) s", r["expect"])
    if m and lat >= 0:
        print("  %-3s detection %.2f s, limit %s s, margin %.2f s" % (r["id"], lat, m.group(1), float(m.group(1)) - lat))
    m = re.search(r"drift <= ([0-9.]+) m", r["expect"])
    if m:
        print("  %-3s drift %.1f m, limit %s m, margin %.1f m" % (r["id"], float(r["drift_m"]), m.group(1),
                                                            float(m.group(1)) - float(r["drift_m"])))
nominal = [float(r["max_innov_m"]) for r in rows if r["id"].startswith("N")]
print("  N*  largest innovation in nominal flight %.2f m against the 2.0 m gate (ratio %.2f)"
      % (max(nominal), max(nominal) / 2.0))
print()

failed = [r for r in rows if r["verdict"] in ("FAIL", "GAP-CHANGED")]
gaps = [r for r in rows if r["kind"] == "GAP"]
print("open items for the R2 review:")
for r in gaps:
    print("  %s: %s -> fault %s, landed %s, t_end %s s" % (
        r["id"], r["what"], r["faults"], "yes" if r["landed"] == "1" else "NO", r["t_end_s"]))
for r in failed:
    print("  %s: FAILED - %s" % (r["id"], r["expect"]))
print()
ok = not failed and all(r["verdict"] == "PASS" for r in rows if r["kind"] in ("TEST", "CONTROL"))
print("R2 entry evidence (SITL part): %s" % ("COMPLETE - every test and control passed; gaps listed above"
                                              if ok else "INCOMPLETE - see failures above"))
sys.exit(0 if ok else 1)
