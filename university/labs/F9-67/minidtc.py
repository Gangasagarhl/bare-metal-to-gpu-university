#!/usr/bin/env python3
# Copied from the DR403 lab folder F4-31 (only this line added); RB403 reuses it for the robot image.
"""minidtc.py - DR403: compile a small subset of devicetree source (DTS) into a DTB.

    python3 minidtc.py input.dts output.dtb

The build container has no dtc (the Devicetree Compiler), so the course writes the part it
needs. Supported: one root node "/ { ... };", child nodes "name@unit { ... };", labels
"label: name { ... };", properties with no value ("flag;"), strings and string lists
("a", "b"), cell lists <0x1 2 &label> (a &label becomes that node's phandle), and // comments.
Not supported (use the real dtc): /include/, /memreserve/, byte strings [..], expressions,
overlays, /delete-node/.

Output layout after the Devicetree Specification, chapter "Flattened Devicetree (DTB) Format"
(title only, pending verification): a 40-byte header, an empty memory reservation block, the
structure block (tokens 1 begin node, 2 end node, 3 property, 9 end) and the strings block.
The F4-31 lab reads every blob it writes with fdt.h and with QEMU, which is the check.
"""
import re
import struct
import sys

TOKEN = re.compile(r'\s*(//[^\n]*|"(?:[^"\\]|\\.)*"|<[^>]*>|[A-Za-z0-9_,.+#@/-]+:?|[{};=,])', re.S)


def tokens(text):
    pos, out = 0, []
    while pos < len(text):
        if text[pos:].strip() == "":
            break
        m = TOKEN.match(text, pos)
        if not m:
            sys.exit("minidtc: cannot read source near: " + text[pos:pos + 40])
        pos = m.end()
        if not m.group(1).startswith("//"):
            out.append(m.group(1))
    return out


class Node:
    def __init__(self, name):
        self.name, self.props, self.children, self.label = name, [], [], None


def parse(toks):
    i = 0

    def node_body(node):
        nonlocal i
        while toks[i] != "}":
            label = None
            if toks[i].endswith(":"):
                label = toks[i][:-1]
                i += 1
            name = toks[i]
            i += 1
            if toks[i] == "{":
                i += 1
                child = Node(name)
                child.label = label
                node_body(child)
                node.children.append(child)
            elif toks[i] == ";":
                node.props.append((name, []))
            elif toks[i] == "=":
                i += 1
                parts = []
                while True:
                    parts.append(toks[i])
                    i += 1
                    if toks[i] == ",":
                        i += 1
                        continue
                    break
                node.props.append((name, parts))
            if toks[i] != ";":
                sys.exit("minidtc: expected ';' after " + name)
            i += 1
        i += 1                       # the closing brace

    if toks[:2] != ["/", "{"]:
        sys.exit("minidtc: the source must start with '/ {'")
    i = 2
    root = Node("")
    node_body(root)
    return root


def assign_phandles(root):
    labels, next_handle = {}, [1]

    def walk(n):
        if n.label:
            labels[n.label] = (n, next_handle[0])
            next_handle[0] += 1
        for c in n.children:
            walk(c)
    walk(root)
    for node, handle in labels.values():
        node.props.append(("phandle", ["<%d>" % handle]))
    return {name: handle for name, (node, handle) in labels.items()}


def value_bytes(parts, labels):
    out = b""
    for p in parts:
        if p.startswith('"'):
            out += bytes(p[1:-1], "utf-8").decode("unicode_escape").encode("latin-1") + b"\0"
        elif p.startswith("<"):
            for cell in p[1:-1].split():
                if cell.startswith("&"):
                    out += struct.pack(">I", labels[cell[1:]])
                else:
                    out += struct.pack(">I", int(cell, 0) & 0xFFFFFFFF)
        else:
            sys.exit("minidtc: unsupported value " + p)
    return out


def build(root, labels):
    structure, strings, offsets = bytearray(), bytearray(), {}

    def pad():
        while len(structure) % 4:
            structure.append(0)

    def emit(n):
        structure.extend(struct.pack(">I", 1) + n.name.encode() + b"\0")
        pad()
        for name, parts in n.props:
            data = value_bytes(parts, labels)
            if name not in offsets:
                offsets[name] = len(strings)
                strings.extend(name.encode() + b"\0")
            structure.extend(struct.pack(">III", 3, len(data), offsets[name]) + data)
            pad()
        for c in n.children:
            emit(c)
        structure.extend(struct.pack(">I", 2))

    emit(root)
    structure.extend(struct.pack(">I", 9))
    rsv = bytes(16)                       # one (0, 0) entry: no reserved memory
    off_rsv = 40
    off_struct = off_rsv + len(rsv)
    off_strings = off_struct + len(structure)
    total = off_strings + len(strings)
    header = struct.pack(">10I", 0xd00dfeed, total, off_struct, off_strings, off_rsv,
                         17, 16, 0, len(strings), len(structure))
    return header + rsv + bytes(structure) + bytes(strings)


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: minidtc.py input.dts output.dtb")
    with open(sys.argv[1]) as f:
        root = parse(tokens(f.read()))
    labels = assign_phandles(root)
    blob = build(root, labels)
    with open(sys.argv[2], "wb") as f:
        f.write(blob)
    print("minidtc: %s -> %s, %d bytes, %d labels" % (sys.argv[1], sys.argv[2], len(blob), len(labels)))


if __name__ == "__main__":
    main()
