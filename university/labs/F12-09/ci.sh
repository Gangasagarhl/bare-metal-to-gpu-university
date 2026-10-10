#!/usr/bin/env bash
# ci.sh - the SE302 CI pipeline (F12-09). Run from the lab folder. Stages, in order:
#   1 host      host unit tests of the pure logic, with ASan and UBSan
#   2 kernel    freestanding build with warnings as errors; no undefined symbols
#   3 qemu      kernel tests in QEMU, each with a time limit
#   4 harness   self-tests of the harness: a panic and a hang MUST be reported as failures
#   5 gpu-build CUDA code compiles with warnings as errors; the machine code contains the kernel
#   6 gpu-run   runs only where a GPU exists; otherwise reported as SKIPPED, never as passed
#   7 robot     the robot's simulation scenario matrix (from F12-07) passes
# A stage that fails stops the pipeline. The last line is the overall result.
set -u -o pipefail
HERE=$(pwd)
B=${B:-.build}
mkdir -p "$B"
skipped=""
stage() { echo "== stage $1"; }
fail() { echo "CI stage $1: FAIL"; echo "CI result: FAIL"; exit 1; }
pass() { echo "CI stage $1: PASS"; }

stage host
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined \
    host_tests.cpp -o "$B/host_tests" && "$B/host_tests" || fail host
pass host

stage kernel
KF="-m32 -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -fno-pic -fno-pie"
KF="$KF -mgeneral-regs-only -fno-stack-protector -O2 -Wall -Wextra -Wpedantic -Werror"
g++ $KF -c kmain.cc -o "$B/kmain.o" && g++ -m32 -c entry.S -o "$B/entry.o" &&
    ld.lld -m elf_i386 -T kernel.ld -nostdlib -static -o "$B/kernel.elf" "$B/entry.o" "$B/kmain.o" ||
    fail kernel
undefined=$(readelf -sW "$B/kernel.elf" | awk '$7 == "UND" && $8 != ""' | wc -l)
echo "undefined symbols in kernel.elf: $undefined"
[ "$undefined" = 0 ] || fail kernel
pass kernel

stage qemu
(cd "$B" && "$HERE/qemu_test.sh" all "test=all" 20 serial_all.log) || fail qemu
(cd "$B" && "$HERE/qemu_test.sh" bitmap "test=bitmap" 20 serial_bitmap.log) || fail qemu
pass qemu

stage harness
if (cd "$B" && "$HERE/qemu_test.sh" panic "test=panic" 20 serial_panic.log); then
    echo "harness let a panicking kernel pass"; fail harness
fi
if (cd "$B" && "$HERE/qemu_test.sh" hang "test=hang" 10 serial_hang.log); then
    echo "harness let a hanging kernel pass"; fail harness
fi
if (cd "$B" && "$HERE/qemu_test.sh" none "" 20 serial_none.log); then
    echo "harness let a run with no tests pass"; fail harness
fi
echo "harness detected all three planted failures"
pass harness

stage gpu-build
nvcc -std=c++17 -O2 -Werror all-warnings vadd.cu -o "$B/vadd" || fail gpu-build
cuobjdump -sass "$B/vadd" | grep -E "Function : " || fail gpu-build
pass gpu-build

stage gpu-run
"$B/vadd" --probe; rc=$?
if [ "$rc" = 77 ]; then
    echo "CI stage gpu-run: SKIPPED (no GPU on this runner; the run is untested on hardware)"
    skipped="gpu-run"
elif [ "$rc" = 0 ] && "$B/vadd"; then
    pass gpu-run
else
    fail gpu-run
fi

stage robot
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined \
    ../F12-07/fixed.cpp -o "$B/robot_matrix" && "$B/robot_matrix" || fail robot
pass robot

if [ -n "$skipped" ]; then
    echo "CI result: PASS with skipped stages: $skipped"
else
    echo "CI result: PASS"
fi
