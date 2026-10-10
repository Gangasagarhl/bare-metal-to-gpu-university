#!/usr/bin/env python3
"""Extract the HIP -> CUDA name map from HIP's NVIDIA back-end header.

Usage: python3 -I hipmap.py <path to nvidia_hip_runtime_api.h> [name ...]

The NVIDIA back end of HIP is a header in which every HIP runtime function,
type and constant is written in terms of its CUDA counterpart. Reading that
header is therefore a way to see the map the installed HIP version really uses.
Prints one line per HIP name: kind, HIP name, CUDA name, header line number.
With names given, prints only those (in the order given) plus a summary.
"""
import re
import sys


def extract(path):
    lines = open(path, encoding="utf-8").read().splitlines()
    found = {}
    func = re.compile(r"^\s*inline\s+static\s+[\w\s\*]+?\b(hip\w+)\s*\(")
    call = re.compile(r"\b(cuda\w+|cu[A-Z]\w+)\s*\(")
    define = re.compile(r"^\s*#define\s+(hip\w+)\s+(cuda\w+|CU\w+|cu\w+)\s*$")
    typedef = re.compile(r"^\s*typedef\s+(?:enum\s+|struct\s+)?(cuda\w+|CU\w+)\s+(hip\w+)\s*;")
    for no, text in enumerate(lines, start=1):
        m = func.match(text)
        if m and m.group(1) not in found:
            # the CUDA call is in the body: search this line and the next 15
            for k in range(no - 1, min(no + 15, len(lines))):
                body = lines[k].split("{", 1)[1] if k == no - 1 and "{" in lines[k] else lines[k]
                if k == no - 1 and "{" not in lines[k]:
                    continue
                c = call.search(body)
                if c and not c.group(1).startswith("cudaErrorTo"):
                    found[m.group(1)] = ("function", c.group(1), no)
                    break
            continue
        m = define.match(text)
        if m and m.group(1) not in found:
            found[m.group(1)] = ("macro", m.group(2), no)
            continue
        m = typedef.match(text)
        if m and m.group(2) not in found:
            found[m.group(2)] = ("type", m.group(1), no)
    return found


def main():
    if len(sys.argv) < 2:
        print("usage: hipmap.py <nvidia_hip_runtime_api.h> [name ...]")
        return 2
    found = extract(sys.argv[1])
    wanted = sys.argv[2:] or sorted(found)
    same = 0
    for name in wanted:
        if name in found:
            kind, cuda, no = found[name]
            note = ""
            if cuda.replace("cuda", "", 1) != name.replace("hip", "", 1):
                note = "   <- name differs"
            else:
                same += 1
            print("%-8s %-26s %-28s line %4d%s" % (kind, name, cuda, no, note))
        else:
            print("%-8s %-26s %-28s" % ("-", name, "(not written as a CUDA name here)"))
    shown = [n for n in wanted if n in found]
    print()
    print("HIP names mapped in this header: %d (functions %d, macros %d, types %d)" % (
        len(found),
        sum(1 for v in found.values() if v[0] == "function"),
        sum(1 for v in found.values() if v[0] == "macro"),
        sum(1 for v in found.values() if v[0] == "type")))
    print("of the %d names shown, %d differ only by the prefix" % (len(shown), same))
    return 0


if __name__ == "__main__":
    sys.exit(main())
