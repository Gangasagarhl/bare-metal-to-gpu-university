# kshell_ref.py - F3-35: the reference transcript for kshell_session.txt, computed on the host from
# the same initial RAM disk with Python's tarfile module (an independent reader of the archive).
#   python3 kshell_ref.py <initrd.tar> <session file>
import sys
import tarfile

t = tarfile.open(sys.argv[1], format=tarfile.USTAR_FORMAT)
members = t.getmembers()
paths = {"/" + m.name.rstrip("/"): m for m in members}
ino = {"/" + m.name.rstrip("/"): None for m in members}
# tarfs numbers its nodes in creation order: the root is 1, then every new path component.
order = ["/"]
for m in members:
    parts = m.name.rstrip("/").split("/")
    for k in range(1, len(parts) + 1):
        p = "/" + "/".join(parts[:k])
        if p not in order:
            order.append(p)
number = {p: i + 1 for i, p in enumerate(order)}
MOUNTS = "tarfs on /\ninfofs on /sys\n"     # the mount configuration the kernel sets up

def children(d):
    pre = d.rstrip("/") + "/"
    return [m for m in members if ("/" + m.name.rstrip("/")).startswith(pre)
            and "/" not in ("/" + m.name.rstrip("/"))[len(pre):]]

out = ["OS304 kernel shell. Type help.\n"]
for line in open(sys.argv[2]).read().splitlines():
    out.append("os304> " + line + "\n")
    w = line.split()
    if not w:
        continue
    c = w[0]
    if c == "help":
        out.append("commands: help echo ls cat stat mounts exit\n")
    elif c == "echo":
        out.append(" ".join(w[1:]) + "\n")
    elif c == "ls":
        for m in children(w[1]):
            out.append("%s %04o %6d %s\n" % ("d" if m.isdir() else "-", m.mode, 0 if m.isdir() else m.size,
                                             m.name.rstrip("/").split("/")[-1]))
    elif c == "cat":
        p = w[1]
        if p in paths:
            out.append(t.extractfile(paths[p]).read().decode())
        else:
            out.append("cat: %s: error -2\n" % p)
    elif c == "stat":
        m = paths[w[1]]
        out.append("%s: %s, mode %04o, size %d, inode %d, file system 0\n"
                   % (w[1], "directory" if m.isdir() else "file", m.mode, 0 if m.isdir() else m.size, number[w[1]]))
    elif c == "mounts":
        out.append(MOUNTS)
    elif c == "exit":
        out.append("bye\n")
        break
    else:
        out.append("%s: unknown command\n" % c)
sys.stdout.write("".join(out))
