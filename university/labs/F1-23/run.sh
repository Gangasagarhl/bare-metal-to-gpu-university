#!/usr/bin/env bash
# F1-23 lab steps that run_lab.sh cannot do alone: run the U16 tool on a second input,
# and compile one C++ file for three real instruction sets, then disassemble the objects.
set -u -o pipefail
cd "$(dirname "$0")"
status=0
rec() {  # rec <name> <listing> <toolchain line> <command text> <exit code> [extra line]
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
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
# 1. the forensic program, run on the U16 machine of Listing 2
g++ $FLAGS asm_listing.cpp -o .bin_asm || status=1
./.bin_asm < forensic.hex > forensic.out 2>&1; rc=$?
rec forensic forensic.hex "$(g++ --version | head -n 1)" "./asm_listing < forensic.hex" "$rc" \
    "note:      exit code 0 = no assembler errors; the step limit message is the evidence"
rm -f .bin_asm
# 2. the same C++ function as machine code of three real ISAs (compiled, not run)
for t in x86 arm64 riscv64; do
    case $t in
        x86) cc=g++; od=objdump ;;
        arm64) cc=aarch64-linux-gnu-g++; od=aarch64-linux-gnu-objdump ;;
        riscv64) cc=riscv64-linux-gnu-g++; od=riscv64-linux-gnu-objdump ;;
    esac
    $cc -std=c++20 -O2 -c add3.cc -o .add3_$t.o && $od -d --no-addresses .add3_$t.o \
        | sed -n '/<_Z4add3iii>:/,$p' > real_$t.out; rc=$?
    rec real_$t add3.cc "$($cc --version | head -n 1); $($od --version | head -n 1)" \
        "$cc -std=c++20 -O2 -c add3.cc -o add3.o && $od -d --no-addresses add3.o" "$rc" \
        "hardware:  compiled and disassembled only; this object was not executed on any CPU"
    [ "$rc" = 0 ] || status=1
    rm -f .add3_$t.o
done
exit $status
