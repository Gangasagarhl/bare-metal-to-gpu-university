#!/usr/bin/env bash
# F1-31: compile funcs.cc for three ISAs at several -O levels and disassemble each function;
# build and run the forensic benchmark at -O0 and -O2 and disassemble its main at -O2.
set -u -o pipefail
cd "$(dirname "$0")"
status=0
rec() {  # rec <name> <listing> <toolchain> <command> <exit code> [extra]
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
    [ "$5" = 0 ] || status=1
}
# asm <name> <compiler> <objdump> <-O level> <objdump options>
asm() {
    local name=$1 cc=$2 od=$3 opt=$4 odopt=$5
    $cc -std=c++20 $opt -c funcs.cc -o .f.o && $od -d --no-show-raw-insn $odopt -C .f.o \
        | sed -n '/^[0-9a-f]* <add(int, int)>:/,$p' > "$name.out"; local rc=$?
    rec "$name" funcs.cc "$($cc --version | head -n 1); $($od --version | head -n 1)" \
        "$cc -std=c++20 $opt -c funcs.cc -o funcs.o && $od -d --no-show-raw-insn $odopt -C funcs.o" \
        "$rc" "hardware:  compiled and disassembled only (not executed)"
    rm -f .f.o
}
asm x86_O0 g++ objdump -O0 "-M intel"
asm x86_O1 g++ objdump -O1 "-M intel"
asm x86_O2 g++ objdump -O2 "-M intel"
asm x86_O3 g++ objdump -O3 "-M intel"
asm x86_O2_att g++ objdump -O2 ""
asm clang_x86_O2 clang++ objdump -O2 "-M intel"
asm arm64_O0 aarch64-linux-gnu-g++ aarch64-linux-gnu-objdump -O0 ""
asm arm64_O2 aarch64-linux-gnu-g++ aarch64-linux-gnu-objdump -O2 ""
asm riscv64_O0 riscv64-linux-gnu-g++ riscv64-linux-gnu-objdump -O0 ""
asm riscv64_O2 riscv64-linux-gnu-g++ riscv64-linux-gnu-objdump -O2 ""
# the forensic benchmark: build, run, and look at main at -O2
for o in O0 O2; do
    g++ -std=c++20 -$o -Wall -Wextra -Wpedantic -Werror vanish.cc -o .bin_v_$o \
        && ./.bin_v_$o > vanish_$o.out 2>&1; rc=$?
    rec vanish_$o vanish.cc "$(g++ --version | head -n 1)" \
        "g++ -std=c++20 -$o -Wall -Wextra -Wpedantic -Werror vanish.cc -o vanish && ./vanish" "$rc" \
        "note:      a time measured once on a shared cloud container (AH-23); it changes run to run"
done
objdump -d --no-show-raw-insn -M intel -C .bin_v_O2 | awk '/^[0-9a-f]+ <main>:/,/^$/' \
    > vanish_main_O2.out; rc=$?
rec vanish_main_O2 vanish.cc "$(objdump --version | head -n 1)" \
    "objdump -d --no-show-raw-insn -M intel -C vanish   (function main, -O2 build)" "$rc"
rm -f .bin_v_O0 .bin_v_O2
exit $status
