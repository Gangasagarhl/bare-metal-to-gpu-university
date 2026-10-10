#!/usr/bin/env python3
"""qemu_run.py - run QEMU with the serial port on stdout, a time limit, and clean text output.

    qemu_run.py [--timeout S] [--stamp] [--stop-after REGEX] -- qemu-system-x86_64 ...

The firmware draws its menus with terminal escape sequences; they are removed so that the
saved .out file is plain text. The UEFI Shell's five-line countdown is folded into one line
marked "[...]" (trimming is marked, guide AH-25). With --stamp every line starts with the host
time in seconds since QEMU was started (a measurement of this build container, not a property
of any real PC). Exit status: QEMU's own status, or 124 when the time limit stopped it.
"""
import os
import re
import select
import subprocess
import sys
import time

CSI = re.compile(rb"\x1b\[[0-9;?=]*[A-Za-z]")


def clean(data):
    """Cursor-positioning and screen-clearing sequences become line breaks; colours vanish."""
    return CSI.sub(lambda m: b"\n" if m.group(0)[-1:] in (b"H", b"J", b"f") else b"", data).replace(b"\r", b"")


def main():
    args = sys.argv[1:]
    timeout, stamp, stop = 30.0, False, None
    while args and args[0] != "--":
        opt = args.pop(0)
        if opt == "--timeout":
            timeout = float(args.pop(0))
        elif opt == "--stamp":
            stamp = True
        elif opt == "--stop-after":
            stop = re.compile(args.pop(0))
        else:
            sys.exit("unknown option " + opt)
    cmd = args[1:]
    t0 = time.monotonic()
    proc = subprocess.Popen(cmd, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    raw, pending, lines, status, countdown = b"", b"", [], None, False

    def emit(raw):
        nonlocal countdown
        text = raw.decode("utf-8", "replace").rstrip()
        if not text.strip():
            return
        if re.search(r"in \d seconds to skip|^ESC$|^startup\.nsh$|or any other key to continue|^Press$", text.strip()):
            if not countdown:
                lines.append(("%8.3f " % (time.monotonic() - t0) if stamp else "")
                             + "[...] (UEFI Shell countdown before startup.nsh: lines removed)")
                print(lines[-1], flush=True)
            countdown = True
            return
        lines.append(("%8.3f " % (time.monotonic() - t0) if stamp else "") + text)
        print(lines[-1], flush=True)

    while True:
        ready, _, _ = select.select([proc.stdout], [], [], 0.2)
        if ready:
            data = os.read(proc.stdout.fileno(), 65536)
            if not data:
                break
            raw += data
            # keep an escape sequence that is cut in half by the read until the rest arrives
            cut = len(raw)
            esc = raw.rfind(b"\x1b")
            if esc != -1 and len(raw) - esc < 16 and not CSI.match(raw, esc):
                cut = esc
            pending += clean(raw[:cut])
            raw = raw[cut:]
            while b"\n" in pending:
                line, pending = pending.split(b"\n", 1)
                before = len(lines)
                emit(line)
                if stop and len(lines) > before and stop.search(lines[-1]):
                    time.sleep(0.5)
                    proc.kill()
                    status = 124
        if time.monotonic() - t0 > timeout and proc.poll() is None:
            proc.kill()
            status = 124
        if proc.poll() is not None and not ready:
            break
    pending += clean(raw)
    for line in pending.split(b"\n"):
        emit(line)
    proc.wait()
    sys.exit(status if status is not None else proc.returncode)


if __name__ == "__main__":
    main()
