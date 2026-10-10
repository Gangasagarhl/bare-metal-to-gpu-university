#!/usr/bin/env python3
"""walk.py - milestone P3, second acceptance test: find the page-table root of a running guest
and walk one translation by hand, using only the QEMU monitor.

    walk.py <seconds to let the firmware run> -- qemu-system-x86_64 ... -monitor unix:<sock>,server,nowait

The script starts QEMU, waits, asks the monitor for the registers, takes CR3 (the root of the
4-level page tables, Intel SDM Vol. 3 "Paging", pending verification) and the instruction
pointer RIP, then reads one 8-byte entry per level with "xp" and prints what each entry says.
At the end it asks QEMU's own translator (gva2gpa) and compares.
"""
import re
import socket
import subprocess
import sys
import time


def monitor(sock, command):
    sock.sendall(command.encode() + b"\n")
    data = b""
    while not data.rstrip().endswith(b"(qemu)"):
        data += sock.recv(65536)
    text = re.sub(r"\x1b\[[0-9;]*[A-Za-z]", "", data.decode(errors="replace")).replace("\r", "")
    # the monitor echoes the command as it is typed: keep only what follows the full echo
    text = text[text.rfind(command) + len(command):]
    return [l for l in text.split("\n") if l.strip() and not l.startswith("(qemu)")]


def read_qword(sock, addr):
    out = monitor(sock, "xp /1gx 0x%x" % addr)
    return int(out[-1].split(":")[1].strip(), 16)


def flags(entry):
    names = [(0, "P"), (1, "RW"), (2, "US"), (5, "A"), (6, "D"), (7, "PS"), (63, "NX")]
    return " ".join(n for bit, n in names if entry >> bit & 1)


def main():
    wait = float(sys.argv[1])
    cmd = sys.argv[3:]
    path = re.search(r"unix:([^,]+)", " ".join(cmd)).group(1)
    qemu = subprocess.Popen(cmd, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)
    time.sleep(wait)
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.connect(path)
    data = b""
    while not data.rstrip().endswith(b"(qemu)"):
        data += sock.recv(4096)
    monitor(sock, "stop")
    regs = "\n".join(monitor(sock, "info registers"))
    cr0 = int(re.search(r"CR0=([0-9a-f]+)", regs).group(1), 16)
    cr3 = int(re.search(r"CR3=([0-9a-f]+)", regs).group(1), 16)
    cr4 = int(re.search(r"CR4=([0-9a-f]+)", regs).group(1), 16)
    efer = int(re.search(r"EFER=([0-9a-f]+)", regs).group(1), 16)
    rip = int(re.search(r"RIP=([0-9a-f]+)", regs).group(1), 16)
    print("after %.0f s of firmware execution (virtual machine stopped):" % wait)
    print("  CR0  = 0x%08x  (PE bit 0 = %d, PG bit 31 = %d)" % (cr0, cr0 & 1, cr0 >> 31 & 1))
    print("  CR4  = 0x%08x  (PAE bit 5 = %d, LA57 bit 12 = %d)" % (cr4, cr4 >> 5 & 1, cr4 >> 12 & 1))
    print("  EFER = 0x%x  (LME bit 8 = %d, LMA bit 10 = %d, NXE bit 11 = %d)"
          % (efer, efer >> 8 & 1, efer >> 10 & 1, efer >> 11 & 1))
    print("  CR3  = 0x%x  -> page-table root (PML4) at physical 0x%x" % (cr3, cr3 & ~0xfff))
    print("  RIP  = 0x%x  (the virtual address we translate)" % rip)
    idx = [(rip >> s) & 0x1ff for s in (39, 30, 21, 12)]
    print("index split of RIP: PML4 %d, PDPT %d, PD %d, PT %d, offset 0x%x"
          % (idx[0], idx[1], idx[2], idx[3], rip & 0xfff))
    table = cr3 & 0x000ffffffffff000
    names = ["PML4", "PDPT", "PD", "PT"]
    phys = None
    for level in range(4):
        addr = table + idx[level] * 8
        entry = read_qword(sock, addr)
        print("  %-4s entry at physical 0x%x = 0x%016x  [%s]" % (names[level], addr, entry, flags(entry)))
        if not entry & 1:
            print("  not present: the walk stops here")
            break
        base = entry & 0x000ffffffffff000
        if level in (1, 2) and entry >> 7 & 1:   # PS bit: 1 GiB page (PDPT) or 2 MiB page (PD)
            size_bits = 30 if level == 1 else 21
            phys = (entry & 0x000fffffffffffff & ~((1 << size_bits) - 1)) | (rip & ((1 << size_bits) - 1))
            print("  PS=1: a %s page; physical = page base + low %d bits of RIP"
                  % ("1 GiB" if level == 1 else "2 MiB", size_bits))
            break
        if level == 3:
            phys = base | (rip & 0xfff)
        table = base
    if phys is not None:
        print("hand walk result:  virtual 0x%x -> physical 0x%x" % (rip, phys))
    qemu_says = " ".join(monitor(sock, "gva2gpa 0x%x" % rip))
    print("QEMU gva2gpa says: %s" % qemu_says)
    match = phys is not None and ("0x%x" % phys) in qemu_says
    print("match: %s" % ("yes" if match else "NO"))
    sock.sendall(b"quit\n")
    qemu.wait(timeout=10)
    sys.exit(0 if match else 1)


if __name__ == "__main__":
    main()
