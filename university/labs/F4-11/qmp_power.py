#!/usr/bin/env python3
"""qmp_power.py - DR302 F4-11: presses the virtual power button through the QEMU monitor.
Waits until the kernel's serial log says it is ready, sends system_powerdown, then prints
every QMP event until QEMU closes the socket (or 10 s pass).
Usage: qmp_power.py <qmp socket> <serial log>
QMP commands used: qmp_capabilities, system_powerdown (QEMU QMP reference, tier 2, written
from memory; they worked with QEMU 8.2.2 in this build)."""
import json, socket, sys, time

def wait_for(path, marker, timeout=60):
    end = time.time() + timeout
    while time.time() < end:
        try:
            with open(path, errors="replace") as f:
                if marker in f.read():
                    return True
        except FileNotFoundError:
            pass
        time.sleep(0.1)
    return False

def main():
    sock_path, log = sys.argv[1], sys.argv[2]
    for _ in range(100):
        try:
            s = socket.socket(socket.AF_UNIX); s.connect(sock_path); break
        except OSError:
            time.sleep(0.1)
    else:
        print("qmp: no monitor socket"); return
    f = s.makefile("rw")
    json.loads(f.readline())                     # the greeting
    f.write(json.dumps({"execute": "qmp_capabilities"}) + "\n"); f.flush()
    json.loads(f.readline())
    if not wait_for(log, "acpi: press the power button"):
        print("qmp: the kernel never asked for the power button"); return
    time.sleep(0.2)
    t0 = time.time()
    f.write(json.dumps({"execute": "system_powerdown"}) + "\n"); f.flush()
    s.settimeout(10)
    try:
        for line in f:
            r = json.loads(line)
            if "return" in r or "error" in r:
                print("qmp: system_powerdown ->", r)
            elif "event" in r:
                print("qmp: +%.3f s event %s %s" % (time.time() - t0, r["event"], json.dumps(r.get("data", {}))))
    except (socket.timeout, OSError):
        print("qmp: no more events within 10 s")
    print("qmp: monitor connection closed after %.3f s" % (time.time() - t0))

main()
