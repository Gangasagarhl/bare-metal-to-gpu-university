# BR-01 helper for run.sh: turns `nvcc --dryrun` output into one short line per tool step.
# For each step it prints the tool, the source file it reads, the file it writes (temporary-name
# prefix shortened to *), whether it only preprocesses (-E), and whether its command line
# defines __CUDA_ARCH__.
import re
import shlex
import sys

n = 0
for line in sys.stdin:
    if not line.startswith("#$ ") or re.match(r"#\$ (\w+=|rm )", line):
        continue
    words = shlex.split(line[3:])
    out = ""
    for k, w in enumerate(words):
        if w == "-o" and k + 1 < len(words):
            out = words[k + 1]
        elif w.startswith("--embedded-fatbin="):
            out = w.split("=", 1)[1]
        elif w == "--gen_c_file_name" and not out:
            out = words[k + 1]
    src = ""
    for k in range(1, len(words)):                # last input-like word not owned by a --option
        w = words[k]
        if (not w.startswith("-") and re.search(r"\.(cu|ii|cpp|ptx)$", w) and w != out
                and not words[k - 1].startswith("--")):
            src = w
    short = lambda p: re.sub(r"tmpxft_[0-9a-f]+_[0-9a-f]+-\d+_", "*", p.rsplit("/", 1)[-1])
    out = short(out)
    src = short(src) if src else "-"
    notes = []
    if "-E" in words:
        notes.append("preprocess only")
    if any(w.startswith("-D__CUDA_ARCH__=") for w in words):
        notes.append("__CUDA_ARCH__ defined")
    n += 1
    print("%2d  %-10s reads %-24s writes %-31s %s" % (n, words[0], src, out, "; ".join(notes)))
