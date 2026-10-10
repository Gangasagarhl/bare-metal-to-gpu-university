#!/usr/bin/env python3
"""teach_board.py - BR-07: writes teach.dtb, the devicetree of an IMAGINARY teaching board.

No real board is described here. The tree only has the SHAPE that SoC board trees have and that
QEMU's machine trees mostly lack: a clock controller, a reset controller, a power controller,
a pin controller with a pin group, a fixed regulator, consumers that reference all of them, a
disabled node and a dma-coherent node. The addresses and clock/reset numbers match the model
in clock_gate.cpp (UART: clock 5, reset 5). The build container has no dtc, so the blob is
written directly in the "Flattened Devicetree (DTB) Format" of the Devicetree Specification
(title only, pending verification); F4-23's fdt.h reads it back in the lab run.
"""
import struct
import sys


def u32s(*v):
    return b"".join(struct.pack(">I", x) for x in v)


def strs(*v):
    return b"".join(x.encode() + b"\0" for x in v)


class Dtb:
    def __init__(self):
        self.body, self.names = b"", {}
        self.strings = b""

    def _pad(self):
        self.body += b"\0" * (-len(self.body) % 4)

    def node(self, name, props, children=()):
        self.body += u32s(1) + name.encode() + b"\0"           # FDT_BEGIN_NODE
        self._pad()
        for key, val in props:
            if key not in self.names:
                self.names[key] = len(self.strings)
                self.strings += key.encode() + b"\0"
            self.body += u32s(3, len(val), self.names[key]) + val  # FDT_PROP
            self._pad()
        for child in children:
            self.node(*child)
        self.body += u32s(2)                                    # FDT_END_NODE

    def blob(self):
        struct_block = self.body + u32s(9)                      # FDT_END
        rsv = b"\0" * 16                                        # empty reservation map
        off_rsv = 40
        off_struct = off_rsv + len(rsv)
        off_str = off_struct + len(struct_block)
        total = off_str + len(self.strings)
        hdr = u32s(0xd00dfeed, total, off_struct, off_str, off_rsv, 17, 16, 0,
                   len(self.strings), len(struct_block))
        return hdr + rsv + struct_block + self.strings


OSC, VCC, CCU, RCU, PWR, PINS, INTC = 1, 2, 3, 4, 5, 7, 8

tree = ("", [("#address-cells", u32s(1)), ("#size-cells", u32s(1)),
             ("model", strs("BR-07 teaching board (imaginary)")),
             ("compatible", strs("univ,br07-board")), ("interrupt-parent", u32s(INTC))], [
    ("chosen", [("stdout-path", strs("serial0:115200n8"))]),
    ("aliases", [("serial0", strs("/soc/serial@4500000"))]),
    ("memory@40000000", [("device_type", strs("memory")), ("reg", u32s(0x40000000, 0x10000000))]),
    ("osc24m", [("compatible", strs("fixed-clock")), ("#clock-cells", u32s(0)),
                ("clock-frequency", u32s(24000000)), ("phandle", u32s(OSC))]),
    ("vcc-3v3", [("compatible", strs("regulator-fixed")), ("regulator-name", strs("vcc-3v3")),
                 ("phandle", u32s(VCC))]),
    ("soc", [("compatible", strs("simple-bus")), ("#address-cells", u32s(1)),
             ("#size-cells", u32s(1)), ("ranges", b"")], [
        ("interrupt-controller@3020000", [("compatible", strs("univ,br07-intc")),
                                          ("reg", u32s(0x3020000, 0x1000)),
                                          ("interrupt-controller", b""),
                                          ("#interrupt-cells", u32s(1)), ("phandle", u32s(INTC))]),
        ("clock-controller@3000000", [("compatible", strs("univ,br07-ccu")),
                                      ("reg", u32s(0x3000000, 0x1000)), ("clocks", u32s(OSC)),
                                      ("#clock-cells", u32s(1)), ("phandle", u32s(CCU))]),
        ("reset-controller@3001000", [("compatible", strs("univ,br07-rcu")),
                                      ("reg", u32s(0x3001000, 0x1000)),
                                      ("#reset-cells", u32s(1)), ("phandle", u32s(RCU))]),
        ("power-controller@3002000", [("compatible", strs("univ,br07-power")),
                                      ("reg", u32s(0x3002000, 0x1000)),
                                      ("#power-domain-cells", u32s(1)), ("phandle", u32s(PWR))]),
        ("pinctrl@3003000", [("compatible", strs("univ,br07-pinctrl")),
                             ("reg", u32s(0x3003000, 0x1000)), ("clocks", u32s(CCU, 1))], [
            ("uart0-pins", [("pins", strs("PB8", "PB9")), ("function", strs("uart0")),
                            ("phandle", u32s(PINS))]),
        ]),
        ("serial@4500000", [("compatible", strs("univ,br07-uart")),
                            ("reg", u32s(0x4500000, 0x100)), ("interrupts", u32s(32)),
                            ("clocks", u32s(CCU, 5)), ("resets", u32s(RCU, 5)),
                            ("pinctrl-names", strs("default")), ("pinctrl-0", u32s(PINS)),
                            ("status", strs("okay"))]),
        ("mmc@4020000", [("compatible", strs("univ,br07-mmc")), ("reg", u32s(0x4020000, 0x1000)),
                         ("interrupts", u32s(39)), ("clocks", u32s(CCU, 7, CCU, 8)),
                         ("resets", u32s(RCU, 7)), ("vmmc-supply", u32s(VCC)),
                         ("power-domains", u32s(PWR, 1)), ("status", strs("okay"))]),
        ("ethernet@4800000", [("compatible", strs("univ,br07-mac")),
                              ("reg", u32s(0x4800000, 0x10000)), ("interrupts", u32s(50)),
                              ("clocks", u32s(CCU, 12)), ("resets", u32s(RCU, 12)),
                              ("power-domains", u32s(PWR, 2)), ("dma-coherent", b""),
                              ("status", strs("disabled"))]),
    ]),
])

if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "teach.dtb"
    d = Dtb()
    d.node(*tree)
    data = d.blob()
    with open(out, "wb") as f:
        f.write(data)
    print("wrote %s: %d bytes (imaginary teaching board)" % (out, len(data)))
