#!/usr/bin/env python3
"""press.py - F9-69: press the e-stop of the QEMU robot from outside.

    python3 press.py MONITOR_SOCKET DELAY_SECONDS

Connects to the QEMU monitor socket, waits DELAY_SECONDS, then types the monitor command
"system_powerdown". On QEMU's Arm virt machine with ACPI off, that command raises the
"power button" input, which QEMU's generated devicetree wires to line 3 of the PL061 GPIO
(gpio-keys node, read in the F9-67 lab); the robot image's devicetree names that line as
its e-stop. On a real robot the e-stop is a physical, hard-wired button (F9-69 Layer 2).
"""
import socket
import sys
import time

path, delay = sys.argv[1], float(sys.argv[2])
sock = None
for _ in range(200):
    try:
        sock = socket.socket(socket.AF_UNIX)
        sock.connect(path)
        break
    except OSError:
        sock = None
        time.sleep(0.05)
if sock is None:
    sys.exit("press.py: no monitor socket at " + path)
time.sleep(delay)
sock.sendall(b"system_powerdown\n")
time.sleep(0.2)
sock.close()
print("press.py: typed system_powerdown into the QEMU monitor %.1f s after connecting" % delay)
