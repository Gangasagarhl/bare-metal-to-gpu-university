#!/usr/bin/env python3
"""mk_evidence.py - build the forensic lab's exhibits from the real boot log.

    mk_evidence.py hwids <boot log> <bdf>   Windows-style hardware and compatible IDs for one
                                            unbound function, formatted from its real IDs
    mk_evidence.py inf                      the INF excerpt exhibit (constructed, see below)
    mk_evidence.py qtree <qtree text> <vendor:device>   the qtree block of that device

The ID strings follow the form the curriculum gives (PCI\\VEN_xxxx&DEV_xxxx&SUBSYS_xxxxxxxx&REV_xx,
PCI\\CC_xxxxxx); the full list and its order are this lab's assumption (see the chapter's
unverified box). The INF excerpt is CONSTRUCTED for the lab: no real vendor's file was used.
"""
import re
import sys

mode = sys.argv[1]
if mode == "hwids":
    for line in open(sys.argv[2], encoding="utf-8"):
        m = re.search(r"NO DRIVER (\S+) (\w{4}):(\w{4}) class (\w\w) (\w\w) (\w\w) sub (\w{4}):(\w{4}) rev (\w\w)", line)
        if not m or m.group(1) != sys.argv[3]:
            continue
        v, d, b, s, p, sv, sd, r = (x.upper() for x in m.group(2, 3, 4, 5, 6, 7, 8, 9))
        dev = "PCI\\VEN_%s&DEV_%s" % (v, d)
        print("Device instance at %s (from the boot log line: %s)" % (m.group(1), line.split("] ", 1)[1].strip()))
        print("Hardware Ids:")
        for x in ("%s&SUBSYS_%s%s&REV_%s" % (dev, sd, sv, r), "%s&SUBSYS_%s%s" % (dev, sd, sv),
                  "%s&CC_%s%s%s" % (dev, b, s, p), "%s&CC_%s%s" % (dev, b, s)):
            print("    " + x)
        print("Compatible Ids:")
        for x in ("%s&REV_%s" % (dev, r), dev, "PCI\\VEN_%s&CC_%s%s%s" % (v, b, s, p), "PCI\\VEN_%s&CC_%s%s" % (v, b, s),
                  "PCI\\VEN_%s" % v, "PCI\\CC_%s%s%s" % (b, s, p), "PCI\\CC_%s%s" % (b, s)):
            print("    " + x)
elif mode == "inf":
    print("""; exhibit_edu.inf - CONSTRUCTED for the DR401 forensic lab (not a real vendor's file)
[Version]
Signature   = "$Windows NT$"
Class       = System
Provider    = %Provider%
DriverVer   = 01/01/2026,1.0.0.0

[Manufacturer]
%Mfg% = Models, NTamd64

[Models.NTamd64]
%EduDesc% = Edu_Install, PCI\\VEN_1234&DEV_11E8&SUBSYS_11001AF4
%EduDesc% = Edu_Install, PCI\\VEN_1234&DEV_11E8

[Edu_Install.NT.Services]
AddService = edulab, 0x00000002, Edu_Service

[Edu_Service]
ServiceType   = 1
StartType     = 3
ServiceBinary = %12%\\edulab.sys

[Strings]
Provider = "Unnamed driver vendor"
Mfg      = "Unnamed hardware maker"
EduDesc  = "Educational PCI device\"""")
elif mode == "qtree":
    want = "pci id " + sys.argv[3]
    block, keep = [], False
    for line in open(sys.argv[2], encoding="utf-8"):
        if re.match(r"\s+dev: ", line):
            if keep:
                break
            block = [line.rstrip()]
            continue
        block.append(line.rstrip())
        if want in line:
            keep = True
    print("\n".join(block) if keep else "device %s not found" % sys.argv[3])
