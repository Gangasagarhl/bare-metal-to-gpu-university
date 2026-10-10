#!/usr/bin/env python3
"""make_dtb.py - F4-27: writes pi3.dtb, a small flattened devicetree shaped like the one a
Raspberry Pi's firmware hands the kernel, for QEMU's raspi3b model (QEMU 8.2 ships no Pi
devicetree and the build container has no dtc).

What is copied from a real Pi devicetree is the SHAPE, recalled from memory and unverified
(chapter F4-27, unverified box): the console named through an alias ("serial0"), peripheral
addresses written as VideoCore bus addresses (0x7e......) under /soc and translated to ARM
physical addresses by /soc's ranges, and CPUs started through a spin table.
The ADDRESSES are those of QEMU's raspi3b model, which the lab run checks.

Format: "Devicetree Specification" (title only, pending verification), the same layout
F4-23's fdt.h reads and fdt_test.cpp writes.
"""
import struct
import sys

FDT_BEGIN_NODE, FDT_END_NODE, FDT_PROP, FDT_END = 1, 2, 3, 9


def u32(*v):
    return b"".join(struct.pack(">I", x) for x in v)


def s(text):
    return text.encode() + b"\0"


class Writer:
    def __init__(self):
        self.struct = b""
        self.strings = b""
        self.offsets = {}

    def name_off(self, name):
        if name not in self.offsets:
            self.offsets[name] = len(self.strings)
            self.strings += s(name)
        return self.offsets[name]

    def pad(self):
        while len(self.struct) % 4:
            self.struct += b"\0"

    def begin(self, name):
        self.struct += u32(FDT_BEGIN_NODE) + s(name)
        self.pad()

    def end(self):
        self.struct += u32(FDT_END_NODE)

    def prop(self, name, value):
        self.struct += u32(FDT_PROP, len(value), self.name_off(name)) + value
        self.pad()

    def blob(self):
        self.struct += u32(FDT_END)
        rsvmap = b"\0" * 16                      # one empty reservation entry ends the list
        off_rsv = 40
        off_struct = off_rsv + len(rsvmap)
        off_strings = off_struct + len(self.struct)
        total = off_strings + len(self.strings)
        header = u32(0xD00DFEED, total, off_struct, off_strings, off_rsv, 17, 16, 0,
                     len(self.strings), len(self.struct))
        return header + rsvmap + self.struct + self.strings


def build():
    w = Writer()
    w.begin("")
    w.prop("#address-cells", u32(1))
    w.prop("#size-cells", u32(1))
    w.prop("compatible", s("raspberrypi,3-model-b") + s("brcm,bcm2837"))
    w.prop("model", s("Raspberry Pi 3 Model B (QEMU raspi3b; devicetree written by make_dtb.py)"))

    w.begin("aliases")
    w.prop("serial0", s("/soc/serial@7e201000"))
    w.end()

    w.begin("chosen")
    w.prop("stdout-path", s("serial0:115200n8"))
    w.end()

    w.begin("memory@0")
    w.prop("device_type", s("memory"))
    w.prop("reg", u32(0x0, 0x3C000000))       # below the peripherals at 0x3f000000
    w.end()

    w.begin("cpus")
    w.prop("#address-cells", u32(1))
    w.prop("#size-cells", u32(0))
    for n in range(4):
        w.begin("cpu@%d" % n)
        w.prop("device_type", s("cpu"))
        w.prop("compatible", s("arm,cortex-a53"))
        w.prop("reg", u32(n))
        w.prop("enable-method", s("spin-table"))
        w.prop("cpu-release-addr", u32(0x0, 0xD8 + 8 * n))
        w.end()
    w.end()

    w.begin("soc")
    w.prop("compatible", s("simple-bus"))
    w.prop("#address-cells", u32(1))
    w.prop("#size-cells", u32(1))
    w.prop("ranges", u32(0x7E000000, 0x3F000000, 0x01000000))   # bus address -> ARM physical
    w.begin("serial@7e201000")
    w.prop("compatible", s("arm,pl011") + s("arm,primecell"))
    w.prop("reg", u32(0x7E201000, 0x200))
    w.prop("clock-frequency", u32(48000000))
    w.end()
    w.begin("watchdog@7e100000")
    w.prop("compatible", s("brcm,bcm2835-pm-wdt"))
    w.prop("reg", u32(0x7E100000, 0x28))
    w.end()
    w.end()

    w.end()
    return w.blob()


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "pi3.dtb"
    data = build()
    with open(out, "wb") as f:
        f.write(data)
    print("wrote %s: %d bytes" % (out, len(data)))
