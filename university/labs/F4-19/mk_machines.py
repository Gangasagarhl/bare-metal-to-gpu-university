#!/usr/bin/env python3
"""mk_machines.py - collect the device records of F4-14's three machines into one list.

    mk_machines.py <F4-14 lab folder>

Reads inventory.out (the build VM, from Linux sysfs), devreport.out (QEMU q35, from the lab
kernel) and fdt_virt.out (QEMU AArch64 virt, from its devicetree) and prints one record per
line:  machine|kind|id|class or compatible strings|where
"""
import os
import re
import sys

d = sys.argv[1]
for line in open(os.path.join(d, "inventory.out"), encoding="utf-8"):
    m = re.match(r"(\w\w:\w\w\.\w)  (\w{4}:\w{4}) sub \S+ rev \w\w class (\w{6})", line)
    if m:
        print("build-vm|pci|%s|%s|%s" % (m.group(2), m.group(3), m.group(1)))
    m = re.match(r"(\w+)\s+(\\\S*)\s", line)
    if m and not line.startswith("pci.ids"):
        print("build-vm|acpi|%s|-|%s" % (m.group(1), m.group(2)))
for line in open(os.path.join(d, "devreport.out"), encoding="utf-8"):
    m = re.match(r"pci (\S+) id (\w{4}:\w{4}) sub \S+ rev \w\w class (\w\w) (\w\w) (\w\w)", line)
    if m:
        print("q35|pci|%s|%s%s%s|%s" % (m.group(2), m.group(3), m.group(4), m.group(5), m.group(1)))
for line in open(os.path.join(d, "fdt_virt.out"), encoding="utf-8"):
    m = re.match(r"(/\S*): ((?:\"[^\"]*\" ?)+)", line)
    if m:
        compat = " ".join(re.findall(r"\"([^\"]*)\"", m.group(2)))
        extra = re.search(r"\(and (\d+) more", line)
        where = m.group(1) + (" (+%s alike)" % extra.group(1) if extra else "")
        print("virt|dt|%s|%s|%s" % (compat.split()[0], compat, where))
