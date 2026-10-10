#!/usr/bin/env bash
# MP1 run.sh: the starter lab for milestone 1 (regression on x86-64 and AArch64 in QEMU).
#   bootargs_test (built and run by run_lab.sh itself, before this script)
#   build          every kernel of the matrix: the two MP1 kernels and the reused ones
#   matrix         the whole regression matrix, host-stamped serial logs, results.tsv
#   serial_x86, serial_a64   two stamped serial logs from that run
#   flaky          the quarantined row (OS303 B9 threads test): 5 attempts, then 3 on a loaded host
#   regress_matrix the MP1 rows again with a kernel built with -DMP1_MAX_CPUS=2 (a bad change)
#   regress_compare  baseline against that run: the harness names the regression
#   forensic_*     evidence for the forensic lab (the slow AArch64 boot)
#   budget         the boot-time budget check of the answer key (prevention)
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-23/dlab.sh                      # rec, toolchain strings, HW_NOTE
status=0
HERE="$(pwd)"
PYV="$(python3 --version)"
QX86V="$(qemu-system-x86_64 --version | head -n 1)"
GXXV="$(g++ --version | head -n 1); $(ld --version | head -n 1)"
TOOLS="$QX86V; $A64_VER; $GXXV; $PYV"
scrub() { sed -i "s#$HERE/##g; s#$LABS/#labs/#g" "$@"; }

# 1. build every kernel
rm -rf .build .build_bug .ci
(
    ./build_all.sh .build 2>&1; rc=$?
    echo "== kernels (size, Berkeley format) =="
    (cd .build && size mp1_x86.elf mp1_a64.elf d1.elf d3.elf os303/kernel.elf)
    echo "== the two MP1 kernel images (sha256) =="
    (cd .build && sha256sum mp1_x86.bin mp1_a64.bin)
    echo "== reused source files, compiled unchanged (sha256) =="
    (cd "$LABS" && sha256sum F3-18/boot.S F3-18/kprint.cc F3-18/serial.cc F3-18/panic.cc \
        F3-18/cxxrt.cc F3-24/madt.h F4-23/neutral_tests.cc F4-24/boot.S F4-24/psci.cc \
        F4-26/virtio_mmio.cc)
    exit $rc
) > build.out 2>&1; rc=$?
scrub build.out
rec build "build_all.sh: mp1_core.cc mp1_x86.cc mp1_a64.cc bootargs.h + reused F3-18, F3-24, F4-23, F4-24, F4-26, OS303 files" \
    "$A64_VER; $GXXV" "./build_all.sh .build" "$rc"
[ "$rc" = 0 ] || { echo "build failed"; exit 1; }

# a 1 MiB scratch disk; sector 0 starts with "D" because the reused D3 kernel checks that
printf 'DR402 F4-26: sector 0, read through virtio-mmio (MP1 matrix disk)' > .disk.img
truncate -s 1M .disk.img
MODS="$(cd .build/os303 && ls u_*.elf | sed "s#^#$HERE/.build/os303/#" | tr '\n' ',' | sed 's/,$//')"
SETS=(--set DISK="$HERE/.disk.img" --set MODS="$MODS")

# 2. the whole matrix
python3 -I mp1ci.py run matrix.txt .build .ci/base "${SETS[@]}" > matrix.out 2>&1; rc=$?
rec matrix "mp1ci.py run matrix.txt (11 rows; the ~ row is quarantined)" "$TOOLS" \
    "python3 -I mp1ci.py run matrix.txt .build .ci/base --set DISK=.disk.img --set MODS=<the OS303 user programs>" \
    "$rc" "note:      0 = every counted row passed; seconds per row are wall time on this container and vary" \
    "$HW_NOTE"
[ "$rc" = 0 ] || status=1
for a in x86 a64; do
    cp ".ci/base/mp1-$a-smp4.serial" "serial_$a.out"
    rec "serial_$a" "mp1_core.cc + mp1_$a.cc (row mp1-$a-smp4)" "$TOOLS" \
        "mp1ci.py run (row mp1-$a-smp4); host stamp in ms since QEMU started | serial line" "0" \
        "note:      host stamps vary from run to run" "$HW_NOTE"
done

# 2b. the quarantined row, five more times: is the B9 fairness bound stable under TCG?
(
    for i in 1 2 3 4 5; do
        python3 -I mp1ci.py run matrix.txt .build ".ci/flaky$i" --only '^~os303-threads$' "${SETS[@]}" \
            | grep -E '^~os303' | awk -v i="$i" '{printf "attempt %d: %s exit %s, %s s; ", i, $3, $4, $5}'
        grep -h 'fairness: max/min' ".ci/flaky$i/~os303-threads.serial" | sed 's/^ *[0-9.]* | //'
    done
    # attempts 6-8: the same row while three other QEMU runs load the host, as parallel CI jobs do
    LOAD=(qemu-system-x86_64 -M pc -cpu qemu64 -m 256M -smp 1 -display none -no-reboot -monitor none
          -nic none -kernel .build/os303/kernel32.elf -initrd "$MODS" -serial null -append test=sync)
    for i in 6 7 8; do
        for j in 1 2 3; do timeout 20 "${LOAD[@]}" > /dev/null 2>&1 & done
        python3 -I mp1ci.py run matrix.txt .build ".ci/flaky$i" --only '^~os303-threads$' "${SETS[@]}" \
            | grep -E '^~os303' | awk -v i="$i" '{printf "attempt %d (host loaded): %s exit %s, %s s; ", i, $3, $4, $5}'
        grep -h 'fairness: max/min' ".ci/flaky$i/~os303-threads.serial" | sed 's/^ *[0-9.]* | //'
        wait
    done
) > flaky.out 2>&1
rec flaky "row ~os303-threads (F3-26 B9 test, unchanged): 5 attempts, then 3 with the host loaded" "$TOOLS" \
    "for i in 1..8: python3 -I mp1ci.py run matrix.txt .build .ci/flaky\$i --only '^~os303-threads\$'; attempts 6-8 with 3 extra QEMU runs (test=sync, 20 s limit) in the background" \
    "0" "note:      an observation, not a pass condition: the count of passing attempts varies" "$HW_NOTE"

# 3. a bad change: the kernel tracks at most 2 CPUs. Only the MP1 rows are run again.
./build_all.sh .build_bug "-DMP1_MAX_CPUS=2" > /dev/null 2>&1 || status=1
python3 -I mp1ci.py run matrix.txt .build_bug .ci/bug --only '^mp1-' "${SETS[@]}" > regress_matrix.out 2>&1; rc=$?
rec regress_matrix "mp1_core.cc built with -DMP1_MAX_CPUS=2" "$TOOLS" \
    "./build_all.sh .build_bug -DMP1_MAX_CPUS=2; python3 -I mp1ci.py run matrix.txt .build_bug .ci/bug --only '^mp1-' ..." \
    "$rc" "result:    1 is the expected result: the injected change must fail the 4-CPU rows" "$HW_NOTE"
[ "$rc" = 1 ] || status=1
python3 -I mp1ci.py compare .ci/base/results.tsv .ci/bug/results.tsv --only '^mp1-' > regress_compare.out 2>&1; rc=$?
rec regress_compare "mp1ci.py compare" "$PYV" \
    "python3 -I mp1ci.py compare .ci/base/results.tsv .ci/bug/results.tsv --only '^mp1-'" "$rc" \
    "result:    1 is the expected result: two regressions are named"
[ "$rc" = 1 ] || status=1

# 4. forensic evidence: the AArch64 rows after a "tidy-up" of the matrix (matrix_after.txt)
python3 -I mp1ci.py run matrix_after.txt .build .ci/after "${SETS[@]}" > forensic_matrix.out 2>&1; rc=$?
rec forensic_matrix "matrix_after.txt (the AArch64 MP1 rows after the change)" "$TOOLS" \
    "python3 -I mp1ci.py run matrix_after.txt .build .ci/after" "$rc" \
    "note:      0: both rows pass, which is the point of the forensic lab" "$HW_NOTE"
[ "$rc" = 0 ] || status=1
cp .ci/base/mp1-a64-smp4.serial forensic_before.out
cp .ci/after/mp1-a64-smp4.serial forensic_after.out
rec forensic_before "row mp1-a64-smp4, matrix.txt (before the change)" "$TOOLS" "mp1ci.py run, stamped serial log" "0" "$HW_NOTE"
rec forensic_after "row mp1-a64-smp4, matrix_after.txt (after the change)" "$TOOLS" "mp1ci.py run, stamped serial log" "0" "$HW_NOTE"
python3 -I mp1ci.py timeline .ci/base/mp1-a64-smp4.serial .ci/after/mp1-a64-smp4.serial \
    --labels before,after > forensic_timeline.out 2>&1; rc=$?
rec forensic_timeline "mp1ci.py timeline" "$PYV" \
    "python3 -I mp1ci.py timeline base/mp1-a64-smp4.serial after/mp1-a64-smp4.serial --labels before,after" "$rc"
[ "$rc" = 0 ] || status=1

# 5. prevention (answer key): a boot-time budget turns the slow boot into a red row
(
    echo "== before the change, budget 1000 ms =="
    python3 -I mp1ci.py timeline .ci/base/mp1-a64-smp4.serial .ci/base/mp1-a64-smp4.serial \
        --labels before,before --budget-ms 1000 | tail -n 1
    echo "== after the change, budget 1000 ms =="
    python3 -I mp1ci.py timeline .ci/base/mp1-a64-smp4.serial .ci/after/mp1-a64-smp4.serial \
        --labels before,after --budget-ms 1000 | tail -n 1
    exit "${PIPESTATUS[0]}"
) > budget.out 2>&1; rc=$?
rec budget "mp1ci.py timeline --budget-ms" "$PYV" \
    "python3 -I mp1ci.py timeline <before> <before|after> --budget-ms 1000" "$rc" \
    "result:    1 is the expected result: the slow boot exceeds the budget"
[ "$rc" = 1 ] || status=1

scrub matrix.out regress_matrix.out regress_compare.out forensic_matrix.out serial_x86.out \
    serial_a64.out forensic_before.out forensic_after.out
rm -rf .build .build_bug .ci .disk.img __pycache__
exit $status
