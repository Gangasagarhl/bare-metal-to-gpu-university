#!/usr/bin/env python3
"""F3-45 harness.py: the FS2 acceptance test, the FS1 power-cut harness rerun on a journal.

usage: harness.py <fs2tool> <base.img> <stream> <expectdir> <runs> <first seed> <tmpdir> [commits]

With "commits", every power cut happens right after a commit block was issued (the most
dangerous moment), instead of at a random record; the coin tosses still come from the seed.

Each power-cut image is recovered twice, independently:
  A  by e2fsprogs: "e2fsck -fy" replays the journal (and would repair anything else);
     then "e2fsck -fn" must find nothing;
  B  by our recovery code: "fs2tool recover", then "e2fsck -fn" must find nothing,
     and every file whose fsync had returned must have exactly its bytes (debugfs dump).
"""
import concurrent.futures
import os
import re
import shutil
import subprocess
import sys

QUIET = re.compile(r"^(Pass \d|e2fsck \d|fs2: |\S+: recovering journal|$)")


def findings(text):
    return [l.strip() for l in text.splitlines() if not QUIET.match(l.strip())]


def one_run(args, seed):
    tool, base, stream, expect, tmp, cuts = args
    a = os.path.join(tmp, "a%d.img" % seed)
    b = os.path.join(tmp, "b%d.img" % seed)
    extra = [str(cuts[seed % len(cuts)])] if cuts else []
    cut = subprocess.run([tool, "crash", base, stream, str(seed), a] + extra,
                         capture_output=True, text=True, check=True).stdout
    shutil.copyfile(a, b)
    fix = subprocess.run(["e2fsck", "-fy", a], capture_output=True, text=True)
    replayed_by_e2fsck = "recovering journal" in fix.stdout     # (printed even for an empty log)
    a_fix = findings(fix.stdout + fix.stderr)                 # anything besides the replay
    a_chk = subprocess.run(["e2fsck", "-fn", a], capture_output=True, text=True)
    rec = subprocess.run([tool, "recover", b], capture_output=True, text=True)
    b_chk = subprocess.run(["e2fsck", "-fn", b], capture_output=True, text=True)
    b_find = findings(b_chk.stdout + b_chk.stderr)
    promises = [l.split("\t")[1:] for l in cut.splitlines() if l.startswith("promise\t")]
    lost = []
    if promises:
        cmds = os.path.join(tmp, "cmds%d" % seed)
        with open(cmds, "w") as f:
            for pid, path in promises:
                f.write("dump %s %s\n" % (path, os.path.join(tmp, "got%d_%s" % (seed, pid))))
        subprocess.run(["debugfs", "-f", cmds, b], capture_output=True)
        for pid, path in promises:
            got = os.path.join(tmp, "got%d_%s" % (seed, pid))
            want = open(os.path.join(expect, "c" + pid), "rb").read()
            if not os.path.exists(got) or open(got, "rb").read() != want:
                lost.append(path)
            if os.path.exists(got):
                os.remove(got)
        os.remove(cmds)
    os.remove(a)
    os.remove(b)
    ours = int(re.search(r"(\d+) transaction", rec.stdout).group(1))
    return dict(seed=seed, cut=cut.splitlines()[0], replayed=replayed_by_e2fsck, ours=ours,
                a_fix=a_fix, a_rc=fix.returncode, a_clean=a_chk.returncode == 0,
                b_rec=rec.stdout.strip(), b_find=b_find, b_clean=b_chk.returncode == 0 and not b_find,
                promises=len(promises), lost=lost)


def main():
    tool, base, stream, expect, runs, first, tmp = sys.argv[1:8]
    runs, first = int(runs), int(first)
    cuts = []
    if len(sys.argv) > 8 and sys.argv[8] == "commits":
        cuts = [int(x) for x in subprocess.run([tool, "commits", stream], capture_output=True,
                                               text=True, check=True).stdout.split()]
    args = (tool, base, stream, expect, tmp, cuts)
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        res = sorted(pool.map(lambda s: one_run(args, s), range(first, first + runs)),
                     key=lambda r: r["seed"])
    ours = sum(1 for r in res if r["ours"] > 0)
    a_repairs = [r for r in res if r["a_fix"]]
    a_bad = [r for r in res if not r["a_clean"]]
    b_bad = [r for r in res if not r["b_clean"]]
    lost = [r for r in res if r["lost"]]
    print("power cuts: %d (seeds %d..%d), cut points: %s"
          % (runs, first, first + runs - 1, "right after a commit block" if cuts else "random"))
    print("  A: e2fsck -fy (replay) had to repair anything else  %4d runs" % len(a_repairs))
    print("     e2fsck -fn afterwards not clean                  %4d runs" % len(a_bad))
    print("  B: our recovery replayed a committed transaction in %4d runs" % ours)
    print("     e2fsck -fn afterwards not clean                  %4d runs" % len(b_bad))
    print("  fsynced files checked after B: %d, wrong or missing: %d (in %d runs)"
          % (sum(r["promises"] for r in res), sum(len(r["lost"]) for r in res), len(lost)))
    for r in (a_repairs + b_bad)[:2]:
        print("  example, seed %d (%s)" % (r["seed"], r["cut"]))
        print("      our recovery: " + r["b_rec"])
        for line in (r["a_fix"] or r["b_find"])[:7]:
            print("      " + line)
    for r in lost[:2]:
        print("  fsync violated, seed %d: %s" % (r["seed"], ", ".join(r["lost"][:4])))
    return 0 if not (a_repairs or a_bad or b_bad or lost) else 3


if __name__ == "__main__":
    sys.exit(main())
