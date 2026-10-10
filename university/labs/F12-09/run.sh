#!/usr/bin/env bash
# F12-09 run.sh - runs the CI pipeline (ci.sh) and the forensic evidence (ci_broken.sh), and
# keeps the serial logs. Every step writes <name>.out (real output) and <name>.log (run record).
set -u
cd "$(dirname "$0")"
status=0
export B=.build
rm -rf "$B"; mkdir -p "$B"
QEMUV="$(qemu-system-x86_64 --version | head -n 1)"
GXXV="$(g++ --version | head -n 1)"
LLDV="$(ld.lld --version | head -n 1)"
NVCCV="$(nvcc --version | tail -n 2 | head -n 1)"
SEAV="$(dpkg-query -W -f '${Version}' seabios 2>/dev/null)"
EMU="hardware:  untested on hardware; kernel tests ran in QEMU 8.2.2 (q35, 64 MiB, 1 CPU) with TCG emulation and the SeaBIOS firmware package $SEAV; the GPU run stage was skipped: this container has no GPU"
rec() {  # rec <name> <listing> <toolchain> <command> <exit code text> [extra line]
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
}
clean() { sed -i "s#$(pwd)/##g" "$1"; }

# 1. the whole pipeline
./ci.sh > ci.out 2>&1; rc=$?
clean ci.out
rec ci "ci.sh qemu_test.sh host_tests.cpp bitmap.hpp kmain.cc entry.S kernel.ld vadd.cu ../F12-07/fixed.cpp" \
    "$GXXV; $LLDV; $QEMUV; $NVCCV" "./ci.sh" "$rc" "$EMU"
[ "$rc" = 0 ] || status=1

# 2. serial logs kept by the pipeline (what the kernel printed in each QEMU test)
for t in all panic hang none; do
    cp "$B/serial_$t.log" "serial_$t.out"
    rec "serial_$t" "kmain.cc (serial port output)" "$QEMUV" \
        "qemu_test.sh $t (from ci.sh): serial log file" "see ci.out (verdict line for '$t')" "$EMU"
done

# 3. forensic evidence: the old QEMU stage, run on a commit whose kernel panics
./ci_broken.sh > ci_broken.out 2>&1; rc=$?
clean ci_broken.out
rec ci_broken "ci_broken.sh" "$QEMUV" "./ci_broken.sh (after ci.sh built .build/kernel.elf)" \
    "$rc (the old script reported success)" "$EMU"

# 4. the same commit through the current harness
(cd "$B" && ../qemu_test.sh old_commit "test=all,panic" 20 serial_old_commit.log) > forensic_fixed.out 2>&1; rc=$?
rec forensic_fixed "qemu_test.sh" "$QEMUV" "cd .build && ../qemu_test.sh old_commit 'test=all,panic' 20 serial_old_commit.log" \
    "$rc (expected: 1, the harness reports the failure)" "$EMU"
[ "$rc" = 1 ] || status=1

rm -rf "$B"
exit $status
