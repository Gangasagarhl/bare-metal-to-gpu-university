#!/usr/bin/env python3
"""toyhipify.py - the university's own teaching translator, NOT AMD's HIPIFY.

Usage: python3 -I toyhipify.py <nvidia_hip_runtime_api.h> <input.cu> <output.hip>

It does what a text-based translator does, in the smallest honest form:
  1. builds a CUDA -> HIP name table by reading HIP's own NVIDIA back-end header
     (where every HIP name is written in terms of a CUDA name, F7-03);
  2. replaces whole identifiers only (never parts of words) and the runtime header;
  3. reports every CUDA-looking identifier it could not translate, and every line
     that looks like a warp-size assumption, with line numbers.
It does not parse C++: it cannot see types, macros or templates (see F7-04, Layer 3).
"""
import re
import sys


def hip_to_cuda(path):
    lines = open(path, encoding="utf-8").read().splitlines()
    found = {}
    func = re.compile(r"^\s*inline\s+static\s+[\w\s\*]+?\b(hip\w+)\s*\(")
    call = re.compile(r"\b(cuda\w+|cu[A-Z]\w+)\s*\(")
    define = re.compile(r"^\s*#define\s+(hip\w+)\s+(cuda\w+)\s*$")
    typedef = re.compile(r"^\s*typedef\s+(?:enum\s+|struct\s+)?(cuda\w+)\s+(hip\w+)\s*;")
    for no, text in enumerate(lines, start=1):
        m = func.match(text)
        if m:
            # the CUDA call is in the body, which may start on a later line
            for k in range(no - 1, min(no + 15, len(lines))):
                if k == no - 1:
                    if "{" not in text:
                        continue
                    body = text.split("{", 1)[1]
                else:
                    body = lines[k]
                c = call.search(body)
                if c and not c.group(1).startswith("cudaErrorTo"):
                    found.setdefault(m.group(1), c.group(1))
                    break
            continue
        m = define.match(text)
        if m:
            found.setdefault(m.group(1), m.group(2))
            continue
        m = typedef.match(text)
        if m:
            found.setdefault(m.group(2), m.group(1))
    return found


def cuda_to_hip(path):
    table = {}
    for hip, cuda in hip_to_cuda(path).items():
        same_suffix = hip[3:] == cuda[4:]
        if cuda not in table or same_suffix:
            table[cuda] = hip
    # names the header does not spell as a CUDA name: HIP's own types and codes
    table.update({"cudaError_t": "hipError_t", "cudaSuccess": "hipSuccess",
                  "cudaDeviceProp": "hipDeviceProp_t"})
    return table


def main():
    if len(sys.argv) != 4:
        print(__doc__)
        return 2
    table = cuda_to_hip(sys.argv[1])
    src = open(sys.argv[2], encoding="utf-8").read().splitlines(keepends=True)
    ident = re.compile(r"\b[A-Za-z_]\w*\b")
    out, changed, left, warp = [], 0, [], []
    for no, line in enumerate(src, start=1):
        new = line.replace("#include <cuda_runtime.h>", "#include <hip/hip_runtime.h>")

        def swap(m):
            return table.get(m.group(0), m.group(0))

        code, sep, comment = new.partition("//")       # leave comments alone
        code = ident.sub(swap, code)
        new = code + sep + comment
        if new != line:
            changed += 1
        for name in ident.findall(code):
            if name.startswith("cuda") or (name.startswith("__") and name.endswith("_sync")):
                left.append((no, name))
        if re.search(r"\b(32|16|31|0xffffffff)\b", code) and \
                re.search(r"warp|lane|shfl|ballot|0xffffffff|threadIdx", code):
            warp.append((no, line.strip()))
        out.append(new)
    open(sys.argv[3], "w", encoding="utf-8").write("".join(out))
    print("toyhipify: %s -> %s" % (sys.argv[2], sys.argv[3]))
    print("  name table: %d CUDA names known (read from the HIP header)" % len(table))
    print("  lines changed: %d of %d" % (changed, len(src)))
    print("  CUDA-looking identifiers NOT translated: %d" % len(left))
    for no, name in left:
        print("    line %d: %s" % (no, name))
    print("  lines that may assume 32 lanes (check by hand): %d" % len(warp))
    for no, text in warp:
        print("    line %d: %s" % (no, text))
    return 0


if __name__ == "__main__":
    sys.exit(main())
