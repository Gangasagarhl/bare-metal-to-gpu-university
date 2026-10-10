#!/usr/bin/env bash
# F4-13 run.sh: milestone C13 in QEMU (q35, tpm-tis, "emulator" backend = mock_tpm.py).
#   sha256check : host program, sha256.h against the FIPS 180-4 examples (run_lab.sh)
#   hashcmp     : sha256.h against Python's hashlib on 2000 random messages of 0-300 bytes (every length)
#   build       : the F4-13 kernel (default) and the forensic variant
#   notpm       : the kernel on a machine without a TPM
#   tpm         : with a TPM: TIS probe, firmware event log replay vs PCR 0-7, PCR 8 measurement
#   verify      : the host verifier (verify.cc) on the attestation report of the tpm run
#   forensic, forensic_verify : the same with -DF413_SIZEOF_BUG
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-01/lablib.sh
status=0
SRC="../F4-01/boot.S ../F4-01/kbase.cc ../F4-01/div64.cc ../F4-02/pci.cc ../F4-02/acpi.cc \
../F4-08/isr256.S ../F4-08/intr.cc tpm.cc f413_main.cc"
PY_VER="$(python3 --version)"
SHOWN="-chardev socket,id=chrtpm,path=ctrl.sock -tpmdev emulator,id=tpm0,chardev=chrtpm -device tpm-tis,tpmdev=tpm0"

# sha256.h against hashlib: a small driver prints digests of messages read from stdin.
cat > .hashcmp.cc <<'CC'
#include "sha256.h"
#include <cstdio>
#include <iostream>
#include <string>
int main() {
    std::string hexmsg;
    while (std::cin >> hexmsg) {
        std::string m;
        for (size_t i = 0; i + 1 < hexmsg.size(); i += 2) m += static_cast<char>(std::stoi(hexmsg.substr(i, 2), nullptr, 16));
        if (hexmsg == "-") m.clear();
        uint8_t d[32];
        sha::hash(m.data(), m.size(), d);
        for (uint8_t b : d) std::printf("%02x", b);
        std::printf("\n");
    }
}
CC
hostbuild .hashcmp .hashcmp.cc -I. > .hb.txt 2>&1 || { cat .hb.txt; status=1; }
python3 - > hashcmp.out 2>&1 <<'PY'
import hashlib, random, subprocess
r = random.Random(13)
msgs = [bytes(r.randrange(256) for _ in range(n)) for n in range(301)]           # every length 0-300
msgs += [bytes(r.randrange(256) for _ in range(r.randrange(301))) for _ in range(1699)]
out = subprocess.run(["./.hashcmp"], input="\n".join(m.hex() or "-" for m in msgs) + "\n",
                     capture_output=True, text=True).stdout.split()
bad = sum(1 for m, h in zip(msgs, out) if hashlib.sha256(m).hexdigest() != h)
lens = sorted({len(m) for m in msgs})
print("messages: %d, every length from %d to %d bytes at least once (%d distinct lengths)" % (
    len(msgs), lens[0], lens[-1], len(lens)))
print("sha256.h vs hashlib: %d of %d digests differ" % (bad + abs(len(out) - len(msgs)), len(msgs)))
raise SystemExit(1 if bad or len(out) != len(msgs) else 0)
PY
rc=$?
rec hashcmp "sha256.h (through a small driver) vs Python hashlib" "$GXX_VER; $PY_VER" \
    "g++ $HOSTFLAGS hashcmp.cc -o hashcmp; python3: 2000 random messages through both" "$rc"
[ "$rc" = 0 ] || status=1

kbuild k413.elf $SRC > .kb.txt 2>&1; rc=$?
KEXTRA="-DF413_SIZEOF_BUG" kbuild k413f.elf $SRC >> .kb.txt 2>&1 || rc=1
{ cat .kb.txt; size -A k413.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)'; } > build.out
rec build "$SRC kernel.ld" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k413.elf *.o (and again with -DF413_SIZEOF_BUG)" "$rc"
[ "$rc" = 0 ] || status=1

qboot notpm.out 30 q35 k413.elf; rc=$?
rec notpm "f413_main.cc (kernel k413.elf), machine without a TPM" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k413.elf" "$rc" "note:      exit code 33 = pass" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

tpmboot() {  # $1 out, $2 kernel
    rm -f .ctrl.sock .mock.txt
    python3 mock_tpm.py "$PWD/.ctrl.sock" .mock.txt &
    local mp=$! i
    for i in $(seq 50); do [ -S .ctrl.sock ] && break; python3 -c 'import time; time.sleep(0.1)'; done
    qboot "$1" 60 q35 "$2" -chardev socket,id=chrtpm,path=.ctrl.sock -tpmdev emulator,id=tpm0,chardev=chrtpm \
        -device tpm-tis,tpmdev=tpm0
    local rc=$?
    wait $mp
    { echo "== mock_tpm.py (host side: what the TPM received) =="; cat .mock.txt; } >> "$1"
    rm -f .ctrl.sock .mock.txt
    return $rc
}
hostbuild .verify verify.cc > .hb.txt 2>&1 || { cat .hb.txt; status=1; }

tpmboot tpm.out k413.elf; rc=$?
rec tpm "f413_main.cc (kernel k413.elf); TPM played by mock_tpm.py" "$QEMU_VER; $PY_VER" \
    "python3 mock_tpm.py ctrl.sock mock.log & $QEMU -machine q35 $QCOMMON -serial stdio -kernel k413.elf $SHOWN" "$rc" \
    "note:      exit code 33 = pass; the TPM is a mock (no swtpm in the build container)" "$HW_NOTE"
[ "$rc" = 33 ] || status=1
grep '^report:' tpm.out | ./.verify > verify.out 2>&1; rc=$?
rec verify "verify.cc on the report: lines of tpm.out" "$GXX_VER" "grep '^report:' tpm.out | ./verify" "$rc" \
    "note:      exit code 0 = ACCEPT"
[ "$rc" = 0 ] || status=1

tpmboot forensic.out k413f.elf; rc=$?
rec forensic "f413_main.cc built with -DF413_SIZEOF_BUG (kernel k413f.elf); TPM played by mock_tpm.py" "$QEMU_VER; $PY_VER" \
    "python3 mock_tpm.py ctrl.sock mock.log & $QEMU -machine q35 $QCOMMON -serial stdio -kernel k413f.elf $SHOWN" "$rc" \
    "note:      exit code 33 = the kernel finished (it does not check its own measurement)" "$HW_NOTE"
[ "$rc" = 33 ] || status=1
grep '^report:' forensic.out | ./.verify > forensic_verify.out 2>&1; rc=$?
rec forensic_verify "verify.cc on the report: lines of forensic.out" "$GXX_VER" "grep '^report:' forensic.out | ./verify" "$rc" \
    "note:      exit code 1 = REJECT: expected for the forensic build"
[ "$rc" = 1 ] || status=1

rm -f k413.elf k413f.elf .kb.txt .hb.txt .hashcmp .hashcmp.cc .verify
exit $status
