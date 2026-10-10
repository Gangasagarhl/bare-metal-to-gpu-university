#!/usr/bin/env python3
"""dfread.py - F10-32 Listing 3: read a UDF log without knowing its message set in advance.

It knows only the layout of the FMT record (type 128, format "BBnNZ"); every other record
type is decoded from the FMT records found in the file. Damaged or cut files are handled:
bytes that do not start a record are skipped (and counted), and a record that runs past the
end of the file is reported, not decoded.

usage: dfread.py LOG --summary
       dfread.py LOG --type NAME [--from SECONDS] [--to SECONDS]
"""
import struct
import sys

HEAD = b"\xA5\x5A"
FMT_TYPE = 128
LETTERS = {"B": "B", "b": "b", "H": "H", "h": "h", "I": "I", "i": "i", "f": "f", "Q": "Q",
           "n": "4s", "N": "16s", "Z": "64s"}


def text(raw):
    return raw.split(b"\0", 1)[0].decode("ascii", "replace")


class Log:
    def __init__(self, path):
        with open(path, "rb") as f:
            self.data = f.read()
        self.formats = {FMT_TYPE: ("FMT", "BBnNZ", ["Type", "Length", "Name", "Format", "Columns"])}
        self.records = []          # (type name, dict of fields)
        self.skipped = 0
        self.problems = []
        self._parse()

    def _parse(self):
        d, pos = self.data, 0
        while pos + 3 <= len(d):
            if d[pos:pos + 2] != HEAD:
                pos += 1
                self.skipped += 1
                continue
            t = d[pos + 2]
            if t not in self.formats:
                self.problems.append(f"offset {pos}: record type {t} has no FMT: skipped 1 byte")
                pos += 1
                self.skipped += 1
                continue
            name, fmt, labels = self.formats[t]
            layout = "<" + "".join(LETTERS[c] for c in fmt)
            size = 3 + struct.calcsize(layout)
            if pos + size > len(d):
                self.problems.append(f"offset {pos}: {name} record needs {size} bytes, "
                                     f"only {len(d) - pos} left (file cut off): ignored")
                break
            values = struct.unpack(layout, d[pos + 3:pos + size])
            fields = {lab: (text(v) if isinstance(v, bytes) else v)
                      for lab, v in zip(labels, values)}
            if t == FMT_TYPE:
                self.formats[fields["Type"]] = (fields["Name"], fields["Format"],
                                                fields["Columns"].split(","))
            self.records.append((name, fields))
            pos += size

    def of(self, name):
        return [f for n, f in self.records if n == name]


def main(argv):
    if len(argv) < 3:
        sys.exit(__doc__)
    log = Log(argv[1])
    if argv[2] == "--summary":
        print(f"file: {len(log.data)} bytes, {len(log.records)} records, "
              f"{log.skipped} byte(s) skipped")
        times = [f["TimeUS"] for _, f in log.records if "TimeUS" in f]
        if times:
            print(f"time span: {min(times) / 1e6:.2f} s to {max(times) / 1e6:.2f} s")
        print(f"{'type':>4} {'name':5} {'format':8} {'count':>6}  columns")
        for t, (name, fmt, labels) in sorted(log.formats.items()):
            print(f"{t:>4} {name:5} {fmt:8} {len(log.of(name)):>6}  {','.join(labels)}")
        for p in log.problems:
            print("PROBLEM:", p)
    elif argv[2] == "--type":
        name = argv[3]
        lo = float(argv[argv.index("--from") + 1]) if "--from" in argv else -1e9
        hi = float(argv[argv.index("--to") + 1]) if "--to" in argv else 1e9
        rows = [f for f in log.of(name) if lo <= f.get("TimeUS", 0) / 1e6 <= hi]
        if not rows:
            print(f"no {name} records in that range")
            return
        print(",".join(rows[0].keys()))
        for f in rows:
            print(",".join(f"{v:.2f}" if isinstance(v, float) else str(v) for v in f.values()))
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main(sys.argv)
