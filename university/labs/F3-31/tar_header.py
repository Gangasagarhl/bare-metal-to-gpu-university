# tar_header.py - F3-31: print the raw fields of one USTAR header block of an archive.
#   python3 tar_header.py <archive.tar> <member name>
import sys
import tarfile

FIELDS = [("name", 0, 100), ("mode", 100, 8), ("uid", 108, 8), ("gid", 116, 8), ("size", 124, 12),
          ("mtime", 136, 12), ("chksum", 148, 8), ("typeflag", 156, 1), ("magic", 257, 6),
          ("version", 263, 2), ("prefix", 345, 155)]
with tarfile.open(sys.argv[1]) as t:
    m = t.getmember(sys.argv[2])
    with open(sys.argv[1], "rb") as f:
        f.seek(m.offset)
        blk = f.read(512)
print("header block at byte offset %d (block %d) of the archive" % (m.offset, m.offset // 512))
for name, off, n in FIELDS:
    raw = blk[off:off + n].rstrip(b"\0")
    print("%-8s offset %3d length %3d : %r" % (name, off, n, raw.decode("ascii")))
s = sum(b if not 148 <= i < 156 else 32 for i, b in enumerate(blk))
print("checksum recomputed (chksum field counted as 8 spaces) = %d decimal = %o octal" % (s, s))
