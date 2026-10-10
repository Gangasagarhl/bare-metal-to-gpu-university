#!/usr/bin/env python3
"""a3_test.py - acceptance test for milestone A3, run against a live QEMU.

    a3_test.py <kernel.elf> <monitor socket> -- qemu-system-x86_64 ... -serial stdio -monitor unix:<sock>,server,nowait

1. Reads the serial port until the kernel says it is halting (time limit 90 s) and prints it.
2. Through the QEMU monitor: walks the page tables (CR3) for every page of every PT_LOAD
   segment of kernel.elf and compares the writable and no-execute bits with the segment flags.
3. Compares what the kernel reported with QEMU's own view: the VGA device's memory BAR
   ("info pci") and the screen size and pixels of a screendump.
"""
import os
import re
import select
import socket
import struct
import subprocess
import sys
import time

CSI = re.compile(rb"\x1b\[[0-9;?=]*[A-Za-z]")
results = []


def check(ok, text):
    results.append(ok)
    print("  [%s] %s" % (" ok " if ok else "FAIL", text))


def ask(sock, command):
    sock.sendall(command.encode() + b"\n")
    data = b""
    while not data.rstrip().endswith(b"(qemu)"):
        data += sock.recv(65536)
    text = CSI.sub(b"", data).decode(errors="replace").replace("\r", "")
    return text[text.rfind(command) + len(command):]


def segments(path):
    data = open(path, "rb").read()
    phoff, = struct.unpack_from("<Q", data, 32)
    phnum, = struct.unpack_from("<H", data, 56)
    out = []
    for i in range(phnum):
        ptype, flags, _, vaddr, _, _, memsz, _ = struct.unpack_from("<IIQQQQQQ", data, phoff + 56 * i)
        if ptype == 1:
            out.append((vaddr, memsz, flags))
    return out


def pte(sock, cr3, virt):
    table = cr3 & 0x000ffffffffff000
    for shift in (39, 30, 21, 12):
        addr = table + ((virt >> shift) & 0x1ff) * 8
        entry = int(re.search(r":\s*(0x[0-9a-f]+)", ask(sock, "xp /1gx 0x%x" % addr)).group(1), 16)
        if not entry & 1:
            return None
        if shift in (30, 21) and entry >> 7 & 1:
            return entry
        table = entry & 0x000ffffffffff000
    return entry


def main():
    elf_path, sock_path = sys.argv[1], sys.argv[2]
    cmd = sys.argv[4:]
    qemu = subprocess.Popen(cmd, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    raw, serial, t0 = b"", "", time.monotonic()
    while "kernel: halting" not in serial and time.monotonic() - t0 < 90:
        ready, _, _ = select.select([qemu.stdout], [], [], 0.2)
        if ready:
            chunk = os.read(qemu.stdout.fileno(), 65536)
            if not chunk:
                break
            raw += chunk
            serial = CSI.sub(b"\n", raw).replace(b"\r", b"").decode(errors="replace")
    time.sleep(0.5)
    lines = [l.rstrip() for l in serial.split("\n") if l.strip()]
    print("== serial port ==")
    for l in lines:
        print(l)
    print("== acceptance checks ==")
    check("kernel entered" in serial, "the kernel printed 'kernel entered'")
    check("boot info version 1" in serial, "the kernel printed the boot-info version (1)")
    if "kernel entered" not in serial:
        qemu.kill()
        print("A3 acceptance: FAIL (the kernel never started)")
        sys.exit(1)
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.connect(sock_path)
    data = b""
    while not data.rstrip().endswith(b"(qemu)"):
        data += sock.recv(4096)
    ask(sock, "stop")

    m = re.search(r"signature '(.*?)', revision (\d+)", serial)
    check(m is not None and m.group(1) == "RSD PTR " and m.group(2) == "2",
          "RSDP signature and revision as the firmware's tables say ('RSD PTR ', 2; see F3-12)")
    m = re.search(r"memory map: (\d+) entries", serial)
    check(m is not None and int(m.group(1)) > 0, "memory-map entry count reported (%s)" % (m.group(1) if m else "none"))

    # page permissions of every kernel page
    regs = ask(sock, "info registers")
    cr3 = int(re.search(r"CR3=([0-9a-f]+)", regs).group(1), 16)
    for vaddr, memsz, flags in segments(elf_path):
        want_w, want_x = bool(flags & 2), bool(flags & 1)
        page_ok = True
        for virt in range(vaddr, vaddr + memsz, 4096):
            e = pte(sock, cr3, virt)
            page_ok = page_ok and e is not None and bool(e >> 1 & 1) == want_w and (not (e >> 63 & 1)) == want_x
        check(page_ok, "segment at 0x%x (%s%s%s): every page has W=%d, NX=%d"
              % (vaddr, "R", "W" if want_w else "-", "X" if want_x else "-", want_w, not want_x))

    # framebuffer against QEMU's own view of the display device
    pci = ask(sock, "info pci")
    vga = re.search(r"VGA controller:.*?BAR0: 32 bit prefetchable memory at (0x[0-9a-f]+)", pci, re.S)
    fb = re.search(r"at physical (0x[0-9a-f]+), size", serial)
    check(vga is not None and fb is not None and int(vga.group(1), 16) == int(fb.group(1), 16),
          "framebuffer base equals the VGA device's BAR0 in 'info pci' (%s)" % (vga.group(1) if vga else "?"))
    dump = sock_path + ".ppm"
    ask(sock, "screendump " + dump)
    for _ in range(50):
        if os.path.exists(dump) and os.path.getsize(dump) > 0:
            break
        time.sleep(0.1)
    ppm = open(dump, "rb").read()
    head = re.match(rb"P6\s+(\d+)\s+(\d+)\s+(\d+)\s", ppm)
    width, height = int(head.group(1)), int(head.group(2))
    geom = re.search(r"framebuffer: (\d+) x (\d+)", serial)
    check(geom is not None and (int(geom.group(1)), int(geom.group(2))) == (width, height),
          "reported size equals the screendump size (%d x %d)" % (width, height))
    pixel = lambda x, y: tuple(ppm[head.end() + 3 * (y * width + x): head.end() + 3 * (y * width + x) + 3])
    check(pixel(10, 10) == (0x33, 0x66, 0x99) and pixel(width - 1, 63) == (0x33, 0x66, 0x99),
          "screendump pixels (10,10) and (%d,63) are 0x336699, the kernel's pattern" % (width - 1))
    sock.sendall(b"quit\n")
    qemu.wait(timeout=10)
    print("A3 acceptance: %s (%d of %d checks passed)" % ("PASS" if all(results) else "FAIL", sum(results), len(results)))
    sys.exit(0 if all(results) else 1)


if __name__ == "__main__":
    main()
