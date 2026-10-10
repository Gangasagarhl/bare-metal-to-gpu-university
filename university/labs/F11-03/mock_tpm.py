#!/usr/bin/env python3
"""mock_tpm.py - copied unchanged in its code from the DR302 F4-13 lab, where it is explained.
A tiny stand-in for swtpm, the software TPM QEMU's "emulator" TPM backend talks to. The build
container has no swtpm, so this script plays it: just enough of swtpm's control protocol for
QEMU 8.2 to start, and just enough TPM 2.0 commands (Startup, SelfTest, GetCapability, GetRandom,
PCR_Extend, PCR_Read) for the firmware. It is NOT a TPM: no keys, no sessions, no authorisation
checks, no persistence, no quotes, no sealing. SHA-256 comes from Python's hashlib; the PCR bank
is SHA-256 only, PCR 0-23. Its control protocol was written from memory of swtpm's tpm_ioctl.h
and is proven only by QEMU 8.2.2 accepting it. In F11-03 it serves OVMF, which measured into it.
Usage: mock_tpm.py <control socket path> <log file>"""
import array, hashlib, os, socket, struct, sys

CTRL = {1: "GET_CAPABILITY", 2: "INIT", 3: "SHUTDOWN", 4: "GET_TPMESTABLISHED", 5: "SET_LOCALITY",
        9: "CANCEL_TPM_CMD", 11: "RESET_TPMESTABLISHED", 14: "STOP", 15: "GET_CONFIG",
        16: "SET_DATAFD", 17: "SET_BUFFERSIZE"}
CC = {0x144: "Startup", 0x143: "SelfTest", 0x17A: "GetCapability", 0x17B: "GetRandom",
      0x182: "PCR_Extend", 0x17E: "PCR_Read", 0x129: "HierarchyChangeAuth", 0x121: "HierarchyControl",
      0x145: "Shutdown", 0x146: "StirRandom"}
ST_NO_SESSIONS, ST_SESSIONS = 0x8001, 0x8002
RC_SUCCESS, RC_INITIALIZE, RC_COMMAND_CODE, RC_VALUE = 0x000, 0x100, 0x143, 0x084
ALG_SHA256 = 0x000B

pcr = [bytes(32) for _ in range(24)]
started = False
locality = 0
log = open(sys.argv[2], "a", buffering=1)

def say(msg):
    log.write("mock-tpm: " + msg + "\n")

def recv_exact(s, n):
    b = b""
    while len(b) < n:
        c = s.recv(n - len(b))
        if not c:
            raise EOFError
        b += c
    return b

def respond(tag, rc, body=b""):
    return struct.pack(">HII", tag, 10 + len(body), rc) + body

def auth_response():                     # an empty password-session response: nonce, attributes, hmac
    return struct.pack(">HBH", 0, 1, 0)

def parse_selection(b, off):
    count, = struct.unpack_from(">I", b, off); off += 4
    sel = []
    for _ in range(count):
        alg, size = struct.unpack_from(">HB", b, off); off += 3
        bits = b[off:off + size]; off += size
        sel.append((alg, bits))
    return sel, off

def tpm_command(cmd):
    global started
    tag, size, cc = struct.unpack_from(">HII", cmd, 0)
    name = CC.get(cc, "0x%x" % cc)
    if cc == 0x144:                                         # Startup(TPM_SU)
        su, = struct.unpack_from(">H", cmd, 10)
        started = True
        say("Startup(%s)" % ("CLEAR" if su == 0 else "STATE"))
        return respond(ST_NO_SESSIONS, RC_SUCCESS)
    if not started:
        say("%s before Startup -> TPM_RC_INITIALIZE" % name)
        return respond(ST_NO_SESSIONS, RC_INITIALIZE)
    if cc in (0x143, 0x145, 0x146):
        say(name)
        return respond(ST_NO_SESSIONS, RC_SUCCESS)
    if cc == 0x17A:                                         # GetCapability(cap, property, count)
        cap, prop, count = struct.unpack_from(">III", cmd, 10)
        if cap == 5:                                        # TPM_CAP_PCRS: one SHA-256 bank, all 24 PCRs
            body = struct.pack(">BI", 0, 5) + struct.pack(">I", 1) + struct.pack(">HB", ALG_SHA256, 3) + b"\xff\xff\xff"
        else:                                               # anything else: an empty list
            body = struct.pack(">BII", 0, cap, 0)
        say("GetCapability(cap 0x%x, property 0x%x, count %d)" % (cap, prop, count))
        return respond(ST_NO_SESSIONS, RC_SUCCESS, body)
    if cc == 0x17B:                                         # GetRandom(bytesRequested)
        n, = struct.unpack_from(">H", cmd, 10)
        n = min(n, 32)
        say("GetRandom(%d)" % n)
        return respond(ST_NO_SESSIONS, RC_SUCCESS, struct.pack(">H", n) + os.urandom(n))
    if cc == 0x182:                                         # PCR_Extend(pcrHandle, auth, TPML_DIGEST_VALUES)
        handle, auth_size = struct.unpack_from(">II", cmd, 10)
        off = 18 + auth_size
        count, = struct.unpack_from(">I", cmd, off); off += 4
        for _ in range(count):
            alg, = struct.unpack_from(">H", cmd, off); off += 2
            dlen = {ALG_SHA256: 32, 0x0004: 20, 0x000C: 48, 0x000D: 64}.get(alg)
            if dlen is None:
                say("PCR_Extend: unknown hash algorithm 0x%x" % alg)
                return respond(ST_NO_SESSIONS, RC_VALUE)
            d = cmd[off:off + dlen]; off += dlen
            if alg == ALG_SHA256 and handle < 24:
                pcr[handle] = hashlib.sha256(pcr[handle] + d).digest()
                say("PCR_Extend(PCR %d, sha256 %s) -> %s" % (handle, d.hex(), pcr[handle].hex()))
            else:
                say("PCR_Extend(PCR %d, alg 0x%x) ignored: no such bank" % (handle, alg))
        return respond(ST_SESSIONS, RC_SUCCESS, struct.pack(">I", 0) + auth_response())
    if cc == 0x17E:                                         # PCR_Read(TPML_PCR_SELECTION)
        sel, _ = parse_selection(cmd, 10)
        out_sel, digests = [], []
        for alg, bits in sel:
            chosen = bytearray(len(bits))
            for i in range(len(bits) * 8):
                if alg == ALG_SHA256 and i < 24 and bits[i // 8] & (1 << (i % 8)) and len(digests) < 8:
                    chosen[i // 8] |= 1 << (i % 8)
                    digests.append(pcr[i])
            out_sel.append((alg, bytes(chosen)))
        body = struct.pack(">I", 1) + struct.pack(">I", len(out_sel))
        for alg, bits in out_sel:
            body += struct.pack(">HB", alg, len(bits)) + bits
        body += struct.pack(">I", len(digests)) + b"".join(struct.pack(">H", 32) + d for d in digests)
        say("PCR_Read(%s) -> %d digest(s)" % (", ".join("alg 0x%x bits %s" % (a, b.hex()) for a, b in sel), len(digests)))
        return respond(ST_NO_SESSIONS, RC_SUCCESS, body)
    if cc in (0x129, 0x121):
        say("%s -> success (not modelled)" % name)
        return respond(ST_SESSIONS, RC_SUCCESS, struct.pack(">I", 0) + auth_response())
    say("command %s -> TPM_RC_COMMAND_CODE" % name)
    return respond(ST_NO_SESSIONS, RC_COMMAND_CODE)

def serve_data(fd):
    s = socket.socket(fileno=fd)
    while True:
        try:
            hdr = recv_exact(s, 10)
        except (EOFError, OSError):
            return
        tag, size, cc = struct.unpack(">HII", hdr)
        body = recv_exact(s, size - 10) if size > 10 else b""
        s.sendall(tpm_command(hdr + body))

def main():
    global locality
    path = sys.argv[1]
    try:
        os.unlink(path)
    except FileNotFoundError:
        pass
    srv = socket.socket(socket.AF_UNIX)
    srv.bind(path)
    srv.listen(1)
    say("listening on the control socket")
    c, _ = srv.accept()
    import threading
    while True:
        try:
            msg, fds, _, _ = socket.recv_fds(c, 4, 4)
        except OSError:
            break
        if len(msg) < 4:
            break
        code, = struct.unpack(">I", msg)
        name = CTRL.get(code, "ctrl %d" % code)
        if code == 1:
            caps = (1 << 14) - 1                            # every capability bit 0..13
            c.sendall(struct.pack(">Q", caps))
        elif code == 2:
            flags, = struct.unpack(">I", recv_exact(c, 4))
            c.sendall(struct.pack(">I", 0))
        elif code in (3, 9, 14):
            c.sendall(struct.pack(">I", 0))
        elif code == 4:
            c.sendall(struct.pack(">IB3x", 0, 0))
        elif code in (5, 11):
            loc = recv_exact(c, 4)[0]
            if code == 5:
                locality = loc
            c.sendall(struct.pack(">I", 0))
        elif code == 15:
            c.sendall(struct.pack(">II", 0, 0))
        elif code == 16:
            if fds:
                threading.Thread(target=serve_data, args=(fds[0],), daemon=True).start()
            c.sendall(struct.pack(">I", 0))
        elif code == 17:
            want, = struct.unpack(">I", recv_exact(c, 4))
            size = want if want else 4096
            c.sendall(struct.pack(">IIII", 0, size, 128, 4096))
        else:
            say("control command %s not handled" % name)
            c.sendall(struct.pack(">I", 1))
        if code not in (5,):
            say("control %s" % name)
    say("QEMU closed the control socket; final SHA-256 PCRs 0-10:")
    for i in range(11):
        say("  PCR %2d %s" % (i, pcr[i].hex()))

main()
