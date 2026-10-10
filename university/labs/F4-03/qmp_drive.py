#!/usr/bin/env python3
"""Drive a QEMU run through QMP while the guest writes its serial log to a file.

usage: qmp_drive.py SERIAL_FILE STEP... -- qemu-binary qemu-args...
steps:  wait:TEXT        wait (up to 60 s) until TEXT appears in the serial log; fail if not
        hmp:COMMAND      run one human-monitor command (for example "sendkey h")
        sleep:SECONDS
        exit             wait for QEMU to exit on its own; our exit status is QEMU's
        quit             ask QEMU to quit; our exit status is 0
Each hmp step is echoed to stdout, so the run record shows exactly what was injected."""
import json
import subprocess
import sys
import time

serial = sys.argv[1]
sep = sys.argv.index("--")
steps, qemu = sys.argv[2:sep], sys.argv[sep + 1:] + ["-qmp", "stdio"]
p = subprocess.Popen(qemu, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                     stderr=subprocess.STDOUT, text=True)


def send(obj):
    p.stdin.write(json.dumps(obj) + "\n")
    p.stdin.flush()


def reply():
    while True:
        line = p.stdout.readline()
        if not line:
            return None
        try:
            msg = json.loads(line)
        except json.JSONDecodeError:
            print("[qemu] " + line.rstrip())
            continue
        if "event" in msg or "QMP" in msg:
            continue
        return msg


p.stdout.readline()
send({"execute": "qmp_capabilities"})
reply()
status = 0
for step in steps:
    kind, _, arg = step.partition(":")
    if kind == "wait":
        deadline = time.time() + 60
        while time.time() < deadline and p.poll() is None:
            try:
                if arg in open(serial, errors="replace").read():
                    break
            except FileNotFoundError:
                pass
            time.sleep(0.1)
        else:
            print("step %s: TIMEOUT or QEMU ended" % step)
            status = 1
            break
    elif kind == "hmp":
        send({"execute": "human-monitor-command", "arguments": {"command-line": arg}})
        r = reply()
        print("(qemu) %s%s" % (arg, "" if r and "error" not in r else "  -> ERROR %s" % r))
        time.sleep(0.05)
    elif kind == "sleep":
        time.sleep(float(arg))
    elif kind == "exit":
        try:
            status = p.wait(timeout=60)
        except subprocess.TimeoutExpired:
            print("QEMU did not exit within 60 s; stopped by the script")
            p.kill()
            p.wait()
            status = 1
        sys.exit(status)
    elif kind == "quit":
        send({"execute": "quit"})
        p.wait(timeout=20)
        sys.exit(status)
if p.poll() is None:
    send({"execute": "quit"})
    p.wait(timeout=20)
sys.exit(status)
