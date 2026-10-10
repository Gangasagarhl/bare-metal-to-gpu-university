#!/usr/bin/env python3
"""sample.py - watch where the CPU is while the firmware starts (an observation, not a trace).

    sample.py <seconds> <interval ms> -- qemu-system-x86_64 ... -S -monitor unix:<sock>,server,nowait

QEMU starts halted (-S). The script lets it run and, every <interval> milliseconds, asks the
QEMU monitor for the registers. From each sample it computes the linear address of the next
instruction (code-segment base + instruction pointer) and the CPU mode (CR0.PE, CR0.PG,
EFER.LMA), and prints a line only when the mode or the memory region changes. Sampling can
miss short phases: the interval decides what you can see.
"""
import re
import socket
import subprocess
import sys
import time


def ask(sock, command):
    sock.sendall(command.encode() + b"\n")
    data = b""
    while not data.rstrip().endswith(b"(qemu)"):
        data += sock.recv(65536)
    text = re.sub(r"\x1b\[[0-9;]*[A-Za-z]", "", data.decode(errors="replace")).replace("\r", "")
    return text[text.rfind(command) + len(command):]


def region(linear, flash_base):
    if flash_base <= linear < 1 << 32:
        return "flash (firmware image, executed in place)"
    if linear < 1 << 20:
        return "RAM below 1 MiB"
    return "RAM above 1 MiB"


def mode(cr0, efer):
    if not cr0 & 1:
        return "16-bit real mode"
    if efer >> 10 & 1:
        return "64-bit long mode, paging on"
    return "32-bit protected mode, paging %s" % ("on" if cr0 >> 31 & 1 else "off")


def main():
    seconds, interval = float(sys.argv[1]), float(sys.argv[2]) / 1000
    cmd = sys.argv[4:]
    flash_base = (1 << 32) - int(subprocess.check_output(
        ["stat", "-c", "%s", re.search(r"readonly=on,file=([^ ,]+)", " ".join(cmd)).group(1)]))
    path = re.search(r"unix:([^,]+)", " ".join(cmd)).group(1)
    qemu = subprocess.Popen(cmd, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)
    for _ in range(50):
        try:
            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            sock.connect(path)
            break
        except OSError:
            time.sleep(0.1)
    data = b""
    while not data.rstrip().endswith(b"(qemu)"):
        data += sock.recv(4096)
    print("flash image mapped at 0x%08x-0xffffffff; sampling every %.0f ms for %.0f s"
          % (flash_base, interval * 1000, seconds))
    print("    time   linear address      mode / region")
    last, samples = None, 0
    ask(sock, "cont")
    t0 = time.monotonic()
    while time.monotonic() - t0 < seconds:
        regs = ask(sock, "info registers")
        samples += 1
        ip = int(re.search(r"[RE]IP=([0-9a-f]+)", regs).group(1), 16)
        cs_base = int(re.search(r"CS =[0-9a-f]+ ([0-9a-f]+)", regs).group(1), 16)
        cr0 = int(re.search(r"CR0=([0-9a-f]+)", regs).group(1), 16)
        efer = int(re.search(r"EFER=([0-9a-f]+)", regs).group(1), 16)
        linear = (cs_base + ip) & 0xffffffffffffffff
        state = (mode(cr0, efer), region(linear, flash_base))
        if state != last:
            print("%8.3f s  0x%016x  %s / %s" % (time.monotonic() - t0, linear, state[0], state[1]))
            last = state
        time.sleep(interval)
    print("%d samples taken" % samples)
    ask(sock, "stop")
    regs = ask(sock, "info registers")
    print("final state (virtual machine stopped): %s, %s"
          % (re.search(r"[RE]IP=[0-9a-f]+", regs).group(0), re.search(r"CS =[0-9a-f]+ [0-9a-f]+", regs).group(0)))
    print("the 16 bytes at the instruction pointer:")
    for line in ask(sock, "x /16xb $pc").split("\n"):
        if line.strip() and not line.startswith("(qemu)"):
            print("  " + line.strip())
    sock.sendall(b"quit\n")
    qemu.wait(timeout=10)


if __name__ == "__main__":
    main()
