#!/usr/bin/env python3
"""name_report.py - put names on a device report: every PCI function of the lab kernel's
report is looked up in the local PCI ID database (pci.ids, the file lspci uses).

    name_report.py <kernel report file> [pci.ids path]
"""
import re
import sys

report = sys.argv[1]
ids_path = sys.argv[2] if len(sys.argv) > 2 else "/usr/share/misc/pci.ids"
vendors, devices, classes = {}, {}, {}
version = "?"
vendor = base = sub = None
in_classes = False
for raw in open(ids_path, encoding="utf-8", errors="replace"):
    line = raw.rstrip("\n")
    if line.startswith("#\tVersion:"):
        version = line.split(":", 1)[1].strip()
    if not line or line.startswith("#"):
        continue
    if line.startswith("C "):
        in_classes, base = True, int(line[2:4], 16)
        classes[(base,)] = line[6:]
    elif in_classes and line.startswith("\t\t"):
        classes[(base, sub, int(line[2:4], 16))] = line[6:]
    elif in_classes and line.startswith("\t"):
        sub = int(line[1:3], 16)
        classes[(base, sub)] = line[5:]
    elif not in_classes and not line.startswith("\t"):
        vendor = int(line[:4], 16)
        vendors[vendor] = line[6:]
    elif not in_classes and not line.startswith("\t\t"):
        devices[(vendor, int(line[1:5], 16))] = line[7:]

print("pci.ids version %s: %d vendors, %d devices" % (version, len(vendors), len(devices)))
unnamed = 0
for line in open(report, encoding="utf-8"):
    m = re.match(r"pci (\S+) id (\w{4}):(\w{4}) sub \S+ rev \w\w class (\w\w) (\w\w) (\w\w)", line)
    if not m:
        continue
    v, d = int(m.group(2), 16), int(m.group(3), 16)
    b, s, p = (int(x, 16) for x in m.group(4, 5, 6))
    vn = vendors.get(v, "(vendor %04x not in pci.ids)" % v)
    dn = devices.get((v, d), "(device not in pci.ids)")
    cn = " / ".join(classes[k] for k in ((b,), (b, s), (b, s, p)) if k in classes) or "(class not in pci.ids)"
    unnamed += (v, d) not in devices
    print("%s  %04x:%04x  %s : %s\n           class %02x %02x %02x = %s" % (m.group(1), v, d, vn, dn, b, s, p, cn))
print("%d function(s) without a device name in pci.ids" % unnamed)
