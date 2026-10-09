# wave.py: draw a text timing diagram from a VCD file written by a Verilog testbench.
# usage: python3 wave.py <file.vcd> <step>[/<width>] <end> <signal> [<signal> ...]
# One column (<width> characters, default 3) per <step> time units; the value shown is
# the value at the last time unit of the column. 1-bit signals: '_' = 0, '#' = 1, 'x' = unknown; buses: the value
# in decimal, written where it changes.
import sys


def read_vcd(path):
    codes = {}       # vcd id code -> signal name
    widths = {}      # signal name -> width in bits, as declared
    changes = {}     # signal name -> list of (time, value string)
    time = 0
    in_header = True
    with open(path) as f:
        for raw in f:
            line = raw.strip()
            if not line:
                continue
            if in_header:
                if line.startswith("$var"):
                    parts = line.split()
                    code, name = parts[3], parts[4]
                    codes.setdefault(code, name)
                    changes.setdefault(codes[code], [])
                    widths.setdefault(codes[code], int(parts[2]))
                elif line.startswith("$enddefinitions"):
                    in_header = False
                continue
            if line[0] == "#":
                time = int(line[1:])
            elif line[0] in "01xzXZ":
                code = line[1:]
                if code in codes:
                    changes[codes[code]].append((time, line[0].lower()))
            elif line[0] in "bB":
                value, code = line[1:].split()
                if code in codes:
                    changes[codes[code]].append((time, value.lower()))
    return changes, widths


def value_at(chg, t):
    v = "x"
    for when, val in chg:
        if when <= t:
            v = val
        else:
            break
    return v


def show(v, width):
    if any(c in v for c in "xz"):
        return "x"
    return str(int(v, 2))


def main():
    path, end = sys.argv[1], int(sys.argv[3])
    step_text, _, width_text = sys.argv[2].partition("/")
    step = int(step_text)
    cw = int(width_text) if width_text else 3    # characters per column
    names = sys.argv[4:]
    changes, widths = read_vcd(path)
    cols = list(range(0, end, step))
    label_w = max(len(n) for n in names + ["time"]) + 1
    every = -(-6 // cw)                          # a time label every few columns
    ruler = ""
    for i, t in enumerate(cols):
        if i % every == 0:
            ruler += str(t).ljust(cw * every)
    print("time".ljust(label_w) + ruler)
    for name in names:
        chg = changes[name]
        width = widths[name]
        prev = None
        text = []
        for t in cols:
            v = value_at(chg, t + step - 1)
            if width == 1:
                text.append({"0": "_", "1": "#"}.get(v, "x") * cw)
            else:
                v = show(v, width)
                text.append(("|" + v).ljust(cw) if v != prev else " " * cw)
                prev = v
        print(name.ljust(label_w) + "".join(text))
    print("(each column is %d character(s) wide and lasts %d time units; '#' = 1, '_' = 0, 'x' = unknown;"
          " bus values in decimal, written where they change)" % (cw, step))


main()
