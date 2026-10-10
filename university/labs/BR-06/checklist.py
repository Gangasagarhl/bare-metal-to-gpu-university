#!/usr/bin/env python3
"""checklist.py - BR-06: fills the nine-row platform checklist (curriculum 9.1) for three
QEMU machines from what the machines themselves printed in this lab:
  rows 1-2 from the entry probes (probe_<m>.out),
  rows 3-9 from the QEMU monitor (info mtree -f, info cpus, info pci), saved by run.sh.
Every cell is data found in those files; nothing is typed in from memory. Where the data
cannot answer a row, the cell says so and names the document to read instead.
usage: checklist.py <dir with monitor_<m>.txt and probe_<m>.out>"""
import re
import sys

MACHINES = [("x86", "x86-64 q35"), ("aarch64", "aarch64 virt"), ("riscv64", "riscv64 virt")]


def flatview(text, as_name):
    """Regions (start, end, kind, name) of the FlatView that serves address space as_name."""
    for block in re.split(r"\nFlatView #\d+\n", text):
        if re.search(r'^ AS "%s"' % re.escape(as_name), block, re.M):
            return [(int(a, 16), int(b, 16), k, n.split(" @")[0])
                    for a, b, k, n in re.findall(r"^\s+([0-9a-f]+)-([0-9a-f]+) \(prio \d+, ([a-z/]+)\): (.+)$", block, re.M)]
    return []


def fmt(regs, limit=4):
    out = ["%s 0x%x" % (n, a) for a, b, k, n in regs[:limit]]
    if len(regs) > limit:
        out.append("... (%d in all)" % len(regs))
    return ", ".join(out) if out else "none in the map"


def probe(path):
    try:
        text = open(path, encoding="utf-8").read()
    except OSError:
        return {}
    d = {"privilege": re.search(r"privilege at entry: (\S+(?: \S+)?)  \(", text)}
    d = {k: (v.group(1) if v else "?") for k, v in d.items()}
    d["regs"] = re.findall(r"^  (\w+) \(([^)]*)\)\s*= 0x0*([0-9a-f]+)$", text, re.M)
    d["dtb"] = "devicetree" in text and "hardware description: devicetree" in text
    return d


def main():
    base = sys.argv[1] if len(sys.argv) > 1 else "."
    rows = [[] for _ in range(9)]
    for key, title in MACHINES:
        mon = open("%s/monitor_%s.txt" % (base, key), encoding="utf-8").read()
        mem = flatview(mon, "memory")
        io = flatview(mon, "I/O")
        p = probe("%s/probe_%s.out" % (base, key))
        def pick(regex, regions=mem):
            return [r for r in regions if re.search(regex, r[3])]
        rows[0].append("entered at %s" % p.get("privilege", "?"))
        regs = ["%s=%s" % (r[0], ("0x" + r[2])) for r in p.get("regs", []) if r[1] != "reserved, 0"]
        rows[1].append(", ".join(regs[:2]) + ("; DTB" if p.get("dtb") else "; no DTB"))
        ram = pick(r"ram$|\.ram$")
        rows[2].append(", ".join("0x%x-0x%x" % (a, b) for a, b, k, n in ram if k == "ram"))
        rows[3].append(fmt(pick(r"apic|gic|plic|aclint\.swi|imsic|aplic")))
        rows[4].append(fmt(pick(r"hpet|mtimer|timer") + pick(r"^pit$", io)) or "none in the map")
        con = pick(r"serial|pl011|uart")
        con_io = pick(r"serial", io)
        rows[5].append(fmt(con) if con else ("port I/O " + ", ".join("%s 0x%x" % (n, a) for a, b, k, n in con_io)))
        virtio = pick(r"virtio-mmio")
        pcie = pick(r"pcie-mmcfg")
        sata = re.findall(r"(SATA controller|IDE controller|NVMe|SCSI)[^\n]*", mon)
        st = []
        if virtio:
            st.append("%d virtio-mmio slots from 0x%x" % (len(virtio), virtio[0][0]))
        if pcie:
            st.append("PCIe config space at 0x%x" % pcie[0][0])
        if sata:
            st.append("PCI: " + sata[0])
        rows[6].append("; ".join(st) or "none found")
        ncpu = len(re.findall(r"^\*? *CPU #\d+", mon, re.M))
        rows[7].append("%d CPUs present; start-up method: not in the map (documents)" % ncpu)
        known = r"ram|rom|flash|apic|gic|plic|aclint|imsic|aplic|hpet|timer|serial|pl011|virtio|pcie|gpex|fwcfg|its|control|translation|redist|vga|mch|smram|tseg|pam|lpc|isa"
        other = sorted({r[3] for r in mem if r[2] not in ("ram", "rom", "romd") and not re.search(known, r[3])})
        rows[8].append(", ".join(other) or "-")
    names = ["1 CPU: privilege at entry", "2 firmware handoff", "3 memory map (RAM)", "4 interrupt controller",
             "5 timer devices in the map", "6 console", "7 storage", "8 SMP", "9 other devices in the map"]
    print("Platform checklist (curriculum 9.1), filled from this lab's runs")
    for i, n in enumerate(names):
        print("row %s" % n)
        for (key, title), cell in zip(MACHINES, rows[i]):
            print("    %-13s %s" % (title, cell))
    return 0


if __name__ == "__main__":
    sys.exit(main())
