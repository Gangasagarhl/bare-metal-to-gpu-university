# host_view.py - F3-31: the host's view of the same archive, in the kernel's listing format.
#   python3 host_view.py <archive.tar>     (Python's own tarfile module, independent of our parser)
import sys
import tarfile

with tarfile.open(sys.argv[1], format=tarfile.USTAR_FORMAT) as t:
    for m in t.getmembers():
        kind = "d" if m.isdir() else "-"
        size = 0 if m.isdir() else m.size
        print("%s %04o %6d /%s" % (kind, m.mode, size, m.name.rstrip("/")))
    data = t.extractfile("docs/numbers.txt").read()
h = 2166136261
for b in data:
    h = ((h ^ b) * 16777619) & 0xFFFFFFFF
print("fnv1a /docs/numbers.txt = %08x" % h)
