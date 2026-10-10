#!/usr/bin/env python3
"""F3-44 harness.py: the FS1 power-cut harness.

usage: harness.py <fs1tool> <base.img> <stream> <expectdir> <runs> <first seed> <tmpdir>

For each run: fs1tool builds the disk as it could be after a power cut at a random point of
the recorded write stream; e2fsck -fn checks it without changing it; debugfs dumps every file
whose fsync had returned before the cut, and the bytes are compared with what was written.
"""
import concurrent.futures
import os
import re
import subprocess
import sys

# The plan's declared acceptable damage: space or counters that leak. Nothing that loses,
# mixes or exposes data, and nothing that a later write could turn into corruption.
ACCEPTABLE = [
    (r"^Free (blocks|inodes) count wrong", "free-space counter wrong"),
    (r"^Directories count wrong", "directory counter wrong"),
    (r"^(Block|Inode) bitmap differences: +(-\S+ *)+$", "leak: marked in use, used by nobody"),
    (r"^Unattached (zero-length )?inode \d+", "leak: inode with no name (orphan)"),
]
COUNT_HIGH = [  # acceptable only when the stored value is larger than the counted one
    (r"^Inode \d+ ref count is (\d+), should be (\d+)", "link count too high"),
    (r"^Inode \d+, i_blocks is (\d+), should be (\d+)", "i_blocks too high"),
]
NOISE = re.compile(r"^(Pass \d|e2fsck \d|fs1: |Fix\? no|Clear\? no|Connect to /lost\+found\? no|$)")


def classify(text):
    cats, damage = set(), []
    for line in text.splitlines():
        line = line.strip()
        line = re.sub(r"\s+(Fix|Clear)\? no$", "", line)
        if NOISE.match(line):
            continue
        cat = next((c for rx, c in ACCEPTABLE if re.match(rx, line)), None)
        if cat is None:
            for rx, c in COUNT_HIGH:
                m = re.match(rx, line)
                if m and int(m.group(1)) > int(m.group(2)):
                    cat = c
        if cat:
            cats.add(cat)
        else:
            damage.append(line)
    return cats, damage


def one_run(args, seed):
    tool, base, stream, expect, tmp = args
    img = os.path.join(tmp, "crash%d.img" % seed)
    cut = subprocess.run([tool, "crash", base, stream, str(seed), img],
                         capture_output=True, text=True, check=True).stdout
    fsck = subprocess.run(["e2fsck", "-fn", img], capture_output=True, text=True)
    cats, damage = classify(fsck.stdout + fsck.stderr)
    promises = [l.split("\t")[1:] for l in cut.splitlines() if l.startswith("promise\t")]
    lost = []
    if promises:
        cmds = os.path.join(tmp, "cmds%d" % seed)
        with open(cmds, "w") as f:
            for pid, path in promises:
                f.write("dump %s %s\n" % (path, os.path.join(tmp, "got%d_%s" % (seed, pid))))
        subprocess.run(["debugfs", "-f", cmds, img], capture_output=True)
        for pid, path in promises:
            got = os.path.join(tmp, "got%d_%s" % (seed, pid))
            want = open(os.path.join(expect, "c" + pid), "rb").read()
            if not os.path.exists(got) or open(got, "rb").read() != want:
                lost.append(path)
            if os.path.exists(got):
                os.remove(got)
        os.remove(cmds)
    os.remove(img)
    return seed, fsck.returncode, cats, damage, len(promises), lost, cut.splitlines()[0]


def main():
    tool, base, stream, expect, runs, first, tmp = sys.argv[1:8]
    runs, first = int(runs), int(first)
    args = (tool, base, stream, expect, tmp)
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        results = sorted(pool.map(lambda s: one_run(args, s), range(first, first + runs)))
    clean = sum(1 for r in results if r[1] == 0)
    per_cat = {}
    for r in results:
        for c in r[2]:
            per_cat[c] = per_cat.get(c, 0) + 1
    damaged = [r for r in results if r[3]]
    violated = [r for r in results if r[5]]
    checked = sum(r[4] for r in results)
    print("power cuts: %d (seeds %d..%d)" % (runs, first, first + runs - 1))
    print("  e2fsck found nothing at all:          %4d" % clean)
    print("  runs with acceptable findings only:   %4d" % (runs - clean - len(damaged)))
    for c in sorted(per_cat):
        print("      %4d runs: %s" % (per_cat[c], c))
    print("  runs with DAMAGE beyond the plan:     %4d" % len(damaged))
    print("  fsynced files checked after the cut:  %4d, wrong or missing: %d (in %d runs)"
          % (checked, sum(len(r[5]) for r in violated), len(violated)))
    for r in damaged[:2]:
        print("  example, seed %d (%s):" % (r[0], r[6]))
        for line in r[3][:6]:
            print("      " + line)
    for r in violated[:2]:
        print("  fsync violated, seed %d: %s" % (r[0], ", ".join(r[5][:4])))
    return 0 if not damaged and not violated else 3


if __name__ == "__main__":
    sys.exit(main())
