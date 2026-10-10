#!/usr/bin/env python3
"""mp1ci.py - MP1 starter: the regression harness (milestone 1, scaled down).

  mp1ci.py run      <matrix> <build dir> <results dir> [--only REGEX] [--set NAME=VALUE ...]
  mp1ci.py compare  <baseline.tsv> <new.tsv> [--only REGEX]
  mp1ci.py timeline <a.serial> <b.serial> [--labels A,B] [--budget-ms N]

run      boots every matrix row in QEMU with a time limit, stamps each serial line with the
         host's monotonic clock on arrival (milliseconds since QEMU was started), applies the
         row's pass rule (exit status AND a required line), writes <id>.serial per row and
         results.tsv, and exits 1 if any row did not pass.
compare  compares two results.tsv files row by row; a row that passed in the baseline and does
         not pass now (or is absent now) is a REGRESSION (exit 1). New, fixed and still-failing
         rows are listed. --only limits both files to the row ids that match.
timeline extracts the 'MP1 STAGE' lines of two stamped serial logs and prints each stage's
         duration side by side, by the host stamps. With --budget-ms, exits 1 if the second
         log's last stage arrives later than the budget.

Matrix rows (one per line, '#' starts a comment), fields separated by ' | ':
  id | arch | timeout s | pass exit codes | required regex | command
The command is split with shlex after replacing {B} with the build directory and each
{NAME} given with --set. Exit status 124 is reported as TIMEOUT. A row whose id starts with
"~" is quarantined: it runs and is reported, but it does not decide the verdict.
"""
import os
import re
import shlex
import subprocess
import sys
import threading
import time


def parse_matrix(path, build, sets):
    rows = []
    with open(path, encoding="utf-8") as f:
        for n, line in enumerate(f, 1):
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            parts = [p.strip() for p in line.split(" | ")]
            if len(parts) != 6:
                sys.exit("%s:%d: expected 6 fields, found %d" % (path, n, len(parts)))
            rid, arch, tmo, codes, regex, cmd = parts
            cmd = cmd.replace("{B}", build)
            for k, v in sets.items():
                cmd = cmd.replace("{%s}" % k, v)
            if re.search(r"\{[A-Z_]+\}", cmd):
                sys.exit("%s:%d: unreplaced placeholder in: %s" % (path, n, cmd))
            rows.append({"id": rid, "arch": arch, "timeout": float(tmo),
                         "codes": {int(c) for c in codes.split(",")}, "regex": regex, "cmd": cmd})
    return rows


def run_row(row, outdir):
    """Boot one row; returns (verdict, exit code, seconds, reason)."""
    t0 = time.monotonic()
    proc = subprocess.Popen(shlex.split(row["cmd"]), stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)
    lines = []

    def reader():
        for raw in proc.stdout:
            stamp = (time.monotonic() - t0) * 1000.0
            lines.append((stamp, raw.decode("utf-8", "replace").rstrip("\r\n")))

    th = threading.Thread(target=reader)
    th.start()
    try:
        rc = proc.wait(timeout=row["timeout"])
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()
        rc = 124
    th.join()
    secs = time.monotonic() - t0
    with open(os.path.join(outdir, row["id"] + ".serial"), "w", encoding="utf-8") as f:
        for stamp, text in lines:
            f.write("%9.1f | %s\n" % (stamp, text))
    seen = any(re.search(row["regex"], text) for _, text in lines)
    if rc == 124:
        return "TIMEOUT", rc, secs, "no exit within %g s" % row["timeout"]
    if rc not in row["codes"]:
        want = ",".join(map(str, sorted(row["codes"])))
        return "FAIL", rc, secs, "exit status %d, expected %s" % (rc, want)
    if not seen:
        return "FAIL", rc, secs, "required line /%s/ not printed" % row["regex"]
    return "PASS", rc, secs, "exit %d and /%s/" % (rc, row["regex"])


def cmd_run(args):
    matrix, build, outdir = args[0], os.path.abspath(args[1]), args[2]
    only, sets = None, {}
    rest = args[3:]
    while rest:
        if rest[0] == "--only":
            only, rest = re.compile(rest[1]), rest[2:]
        elif rest[0] == "--set":
            k, v = rest[1].split("=", 1)
            sets[k], rest = v, rest[2:]
        else:
            sys.exit("unknown option " + rest[0])
    os.makedirs(outdir, exist_ok=True)
    rows = [r for r in parse_matrix(matrix, build, sets) if only is None or only.search(r["id"])]
    results = []
    print("%-16s %-8s %-8s %5s %7s  %s" % ("row", "arch", "verdict", "exit", "seconds", "reason"))
    for row in rows:
        verdict, rc, secs, reason = run_row(row, outdir)
        results.append((row["id"], row["arch"], verdict, rc, secs, reason))
        print("%-16s %-8s %-8s %5d %7.1f  %s" % (row["id"], row["arch"], verdict, rc, secs, reason),
              flush=True)
    with open(os.path.join(outdir, "results.tsv"), "w", encoding="utf-8") as f:
        f.write("id\tarch\tverdict\texit\tseconds\treason\n")
        for r in results:
            f.write("%s\t%s\t%s\t%d\t%.1f\t%s\n" % r)
    counted = [r for r in results if not r[0].startswith("~")]
    npass = sum(1 for r in counted if r[2] == "PASS")
    quarantined = ["%s %s" % (r[0], r[2]) for r in results if r[0].startswith("~")]
    print("matrix: %d rows counted, %d passed, %d did not pass; quarantined (not counted): %s" % (
        len(counted), npass, len(counted) - npass, ", ".join(quarantined) or "none"))
    return 0 if npass == len(counted) else 1


def read_tsv(path):
    with open(path, encoding="utf-8") as f:
        f.readline()
        out = {}
        for line in f:
            rid, arch, verdict, rc, secs, reason = line.rstrip("\n").split("\t")
            out[rid] = (arch, verdict, reason)
    return out


def cmd_compare(args):
    only = None
    if "--only" in args:
        i = args.index("--only")
        only = re.compile(args[i + 1])
        args = args[:i] + args[i + 2:]
    base, new = read_tsv(args[0]), read_tsv(args[1])
    if only is not None:
        base = {k: v for k, v in base.items() if only.search(k)}
        new = {k: v for k, v in new.items() if only.search(k)}
    regressions = 0
    print("%-16s %-8s %-9s %-9s %s" % ("row", "arch", "baseline", "now", "change"))
    for rid in list(base) + [r for r in new if r not in base]:
        b = base.get(rid, ("-", "absent", ""))
        n = new.get(rid, (b[0], "absent", ""))
        if rid.startswith("~"):
            change = "quarantined, not counted"
        elif b[1] == "PASS" and n[1] != "PASS":
            change, regressions = "REGRESSION: " + n[2], regressions + 1
        elif b[1] != "PASS" and n[1] == "PASS":
            change = "fixed" if b[1] != "absent" else "new row, passes"
        elif b[1] != "PASS":
            change = "still not passing" if b[1] != "absent" else "new row, does not pass"
        else:
            change = "-"
        print("%-16s %-8s %-9s %-9s %s" % (rid, n[0], b[1], n[1], change))
    print("compare: %d regression(s)" % regressions)
    return 1 if regressions else 0


STAGE = re.compile(r"^\s*([0-9.]+) \| MP1 STAGE (\S+)")
FIRST = re.compile(r"^\s*([0-9.]+) \| ")


def stages(path):
    first, out = None, []
    with open(path, encoding="utf-8") as f:
        for line in f:
            m = FIRST.match(line)
            if m and first is None:
                first = float(m.group(1))
            m = STAGE.match(line)
            if m:
                out.append((m.group(2), float(m.group(1))))
    return first, out


def cmd_timeline(args):
    budget, labels = None, ["first", "second"]
    if "--budget-ms" in args:
        i = args.index("--budget-ms")
        budget = float(args[i + 1])
        args = args[:i] + args[i + 2:]
    if "--labels" in args:
        i = args.index("--labels")
        labels = args[i + 1].split(",")
        args = args[:i] + args[i + 2:]
    (fa, sa), (fb, sb) = stages(args[0]), stages(args[1])
    print("host-stamped stage durations in ms (first serial line = 0)")
    print("%-16s %10s %10s %10s" % ("stage", labels[0][:10], labels[1][:10], "change"))
    prev_a, prev_b = fa, fb
    names_b = dict(sb)
    for name, ta in sa:
        tb = names_b.get(name)
        da = ta - prev_a
        prev_a = ta
        if tb is None:
            print("%-16s %10.1f %10s" % (name, da, "missing"))
            continue
        db = tb - prev_b
        prev_b = tb
        flag = "  <-- grew by %.0f ms" % (db - da) if db - da > 500 and db > 2 * da else ""
        print("%-16s %10.1f %10.1f %+10.1f%s" % (name, da, db, db - da, flag))
    end_a = sa[-1][1] - fa if sa else 0.0
    end_b = sb[-1][1] - fb if sb else 0.0
    print("%-16s %10.1f %10.1f %+10.1f" % ("total", end_a, end_b, end_b - end_a))
    if budget is not None:
        ok = end_b <= budget
        print("budget: last stage of '%s' at %.1f ms, budget %.0f ms: %s" % (
            labels[1], end_b, budget, "within" if ok else "EXCEEDED"))
        return 0 if ok else 1
    return 0


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in ("run", "compare", "timeline"):
        sys.exit(__doc__)
    fn = {"run": cmd_run, "compare": cmd_compare, "timeline": cmd_timeline}[sys.argv[1]]
    sys.exit(fn(sys.argv[2:]))


if __name__ == "__main__":
    main()
