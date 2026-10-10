# MP4 Listing 11: the portability report generator (milestone 4), started in milestone 1.
#   python3 -I report.py <targets.json> <bench.csv or -> 
# One row per (kernel, size); one column group per GPU: % of that vendor's library, the target
# frozen at R1, and a verdict. It refuses to hide problems:
#   - a target that is null      -> "no target (R1 incomplete)"
#   - targets edited after R1    -> a loud warning (curriculum 13.4: never lowered without the owner)
#   - a GPU with no data         -> "not measured" (AH-23), never a guess
import csv
import hashlib
import json
import sys

FIELDS = ["platform", "device", "key", "kernel", "M", "N", "K", "median_ms", "ref_name", "ref_median_ms", "stack"]


def digest(targets):
    return hashlib.sha256(json.dumps(targets, sort_keys=True).encode()).hexdigest()[:16]


def main(tpath, bpath):
    spec = json.load(open(tpath))
    targets = spec["targets_percent"]
    print("targets file: %s (frozen at: %s)" % (tpath, spec["frozen_at"]))
    if spec.get("digest_at_R1") is None:
        print("targets not frozen yet: R1 is open (digest to record at R1: %s)" % digest(targets))
    elif spec["digest_at_R1"] != digest(targets):
        print("WARNING: targets differ from the digest recorded at R1 (%s now %s): owner approval and a log entry required"
              % (spec.get("digest_at_R1"), digest(targets)))
    rows = []
    if bpath != "-":
        with open(bpath) as f:
            rows = [r for r in csv.DictReader(f) if not r["platform"].startswith("#")]
    gpus = spec["gpus"]
    by = {(r["key"], r["kernel"], int(r["M"])): r for r in rows}
    print("%-12s %5s | " % ("kernel", "size") + " | ".join("%-34s" % g for g in gpus))
    for name in sorted(targets):
        kernel, size = name.rsplit("_", 1)
        cells = []
        for g in gpus:
            t = targets[name].get(g)
            r = by.get((g, kernel, int(size)))
            if r is None:
                cells.append("%-34s" % "not measured")
                continue
            pct = 100.0 * float(r["ref_median_ms"]) / float(r["median_ms"])
            if t is None:
                verdict = "no target (R1 incomplete)"
            else:
                verdict = "meets %d %%" % t if pct >= t else "BELOW %d %%" % t
            cells.append("%-34s" % ("%5.1f %% of %s, %s" % (pct, r["ref_name"], verdict)))
        print("%-12s %5s | " % (kernel, size) + " | ".join(cells))
    stacks = sorted({(r["key"], r["device"], r["stack"]) for r in rows})
    for key, dev, stack in stacks:
        print("measured on %s (%s): %s" % (key, dev, stack))
    if not rows:
        print("no benchmark records: every cell is 'not measured'")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
