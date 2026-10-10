#!/usr/bin/env python3
"""qmp_usb.py - DR302 F4-09: plays the user at the QEMU monitor (QMP) during the USB lab.
Watches the kernel's serial log; when the kernel asks, types a line on the USB keyboard,
hot-plugs a USB mouse, then removes it. Usage: qmp_usb.py <qmp socket> <serial log> <text>
QMP commands used: qmp_capabilities, input-send-event, device_add, device_del (QEMU QMP
reference, tier 2, written from memory; they worked with QEMU 8.2.2 in this build)."""
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
    sock_path, log, text = sys.argv[1], sys.argv[2], sys.argv[3]
    for _ in range(100):
        try:
            s = socket.socket(socket.AF_UNIX); s.connect(sock_path); break
        except OSError:
            time.sleep(0.1)
    f = s.makefile("rw")
    def cmd(name, **args):
        f.write(json.dumps({"execute": name, "arguments": args}) + "\n"); f.flush()
        while True:
            r = json.loads(f.readline())
            if "return" in r or "error" in r:
                return r
    json.loads(f.readline())                     # the greeting
    cmd("qmp_capabilities")
    if wait_for(log, "hid: ready"):
        keys = {" ": "spc", "\n": "ret", "-": "minus", ".": "dot"}
        for ch in text + "\n":
            q = keys.get(ch, ch)
            for down in (True, False):
                r = cmd("input-send-event",
                        events=[{"type": "key", "data": {"down": down, "key": {"type": "qcode", "data": q}}}])
                if "error" in r:
                    print("qmp: input-send-event ->", r)
                time.sleep(0.03)
        print("qmp: typed %r on the keyboard (input-send-event, qcodes)" % text)
    if wait_for(log, "hotplug: waiting"):
        time.sleep(0.5)
        print("qmp: device_add usb-mouse ->", cmd("device_add", driver="usb-mouse", id="mouse0", bus="xhci.0"))
    if wait_for(log, "hotplug: remove it now"):
        time.sleep(0.5)
        print("qmp: device_del mouse0 ->", cmd("device_del", id="mouse0"))
    wait_for(log, "F4-09 ", 30)

main()
