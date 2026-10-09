#!/usr/bin/env python3
"""Print, as hex, the data bytes of one frame of a QEMU USB pcap file.

QEMU writes USB traffic in the pcap link type "USB packets with Linux header
and padding" (tshark reports it as encapsulation 115): every record starts with
a 64-byte header, and the transferred data bytes follow it.

With a third argument "setup", print instead the 8 SETUP bytes of a control
request, which that header carries at offsets 40-47 (checked against tshark's
own decoding in run.sh).

usage: pcap_extract.py FILE FRAME_NUMBER [setup]
"""
import struct
import sys

path, want = sys.argv[1], int(sys.argv[2])
setup = len(sys.argv) > 3 and sys.argv[3] == "setup"
with open(path, "rb") as f:
    data = f.read()
magic = struct.unpack_from("<I", data, 0)[0]
end = "<" if magic == 0xA1B2C3D4 else ">"
pos, frame = 24, 0                         # skip the 24-byte pcap file header
while pos + 16 <= len(data):
    _, _, incl, _ = struct.unpack_from(end + "IIII", data, pos)
    pos += 16                              # record header
    frame += 1
    if frame == want:
        payload = data[pos + 40:pos + 48] if setup else data[pos + 64:pos + incl]
        print(" ".join("%02x" % b for b in payload))
        sys.exit(0)
    pos += incl
sys.exit("frame %d not found" % want)
