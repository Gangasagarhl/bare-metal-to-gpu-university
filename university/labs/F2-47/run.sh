#!/usr/bin/env bash
# F2-47 lab: ABIs and calling conventions, read from real compiler output.
#   step sysv     : abi_demo.cpp for x86-64 System V (g++), disassembled
#   step win64    : the same file for x86-64 Windows (clang, MSVC-compatible target)
#   step arm64    : the same file for AArch64 Linux (aarch64-linux-gnu-g++)
#   step redzone  : a leaf function with and without -mno-red-zone
#   step call_asm : C++ calling a hand-written assembly function (sanitizers on)
#   step vtable   : Itanium C++ ABI: mangled names and the vtable's layout
#   step forensic_O0 / forensic_O2 / forensic_fixed : the "release build crashes" evidence
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
status=0
TOOL="$(g++ --version | head -n 1); $(objdump --version | head -n 1)"
W="${LAB}/.work"; rm -rf "$W"; mkdir -p "$W"
SAN="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"

begin() {   # begin <name> <listing> <command summary> [extra toolchain]
    NAME="$1"; LAST=0; BAD=0
    OUT="${LAB}/${NAME}.out"; LOG="${LAB}/${NAME}.log"; : > "$OUT"
    {
        echo "listing:   $2"
        echo "toolchain: $TOOL${4:-}"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$LOG"
    cd "$W" || exit 2
}
c() {
    echo "\$ $1" >> "$OUT"
    bash -c "$1" >> "$OUT" 2>&1
    LAST=$?
    if [ "$LAST" != "${2:-0}" ]; then BAD=$((BAD + 1)); fi
    if [ "$LAST" != 0 ]; then echo "(exit code: $LAST)" >> "$OUT"; fi
}
finish() {
    sed -i -e "s#${W}/##g" -e "s#${LAB}/##g" -e 's/^bash: line [0-9]*: *[0-9]* /bash: /' "$OUT"
    echo "exit code: $LAST" >> "$LOG"
    [ -n "${1:-}" ] && echo "$1" >> "$LOG"
    if [ "$BAD" != 0 ]; then echo "result:    STEP FAILED ($BAD command(s) did not give the expected exit code)" >> "$LOG"; status=1; fi
    cd "$LAB" || exit 2
}
A="${LAB}/abi"
# show <objdump command> <object> <function names...>: disassemble only the named functions
show() {
    local cmd="$1" obj="$2"; shift 2
    for f in "$@"; do
        c "$cmd --no-show-raw-insn -d $obj | sed -n '/<$f>:/,/^\$/p'"
    done
}

begin sysv "abi/abi_demo.cpp" "g++ -std=c++20 -O2 -c abi_demo.cpp; objdump -d"
c "g++ -std=c++20 -O2 -fcf-protection=none -c $A/abi_demo.cpp -o sysv.o"
show objdump sysv.o _Z4sum8llllllll _Z7pairSum4Pair _Z7makeBigl _Z3mixidid _Z8callsOutl
finish

begin win64 "abi/abi_demo.cpp" "clang++ --target=x86_64-pc-windows-msvc -std=c++20 -O2 -c abi_demo.cpp; llvm-objdump -d" "; $(clang++ --version | head -n 1)"
c "clang++ --target=x86_64-pc-windows-msvc -std=c++20 -O2 -c $A/abi_demo.cpp -o win64.obj"
c "llvm-nm win64.obj"
show llvm-objdump win64.obj '?sum8@@YAJJJJJJJJJ@Z' '?pairSum@@YAJUPair@@@Z' '?makeBig@@YA?AUBig@@J@Z' '?callsOut@@YAJJ@Z'
finish

begin arm64 "abi/abi_demo.cpp" "aarch64-linux-gnu-g++ -std=c++20 -O2 -c abi_demo.cpp; aarch64-linux-gnu-objdump -d" "; $(aarch64-linux-gnu-g++ --version | head -n 1)"
c "aarch64-linux-gnu-g++ -std=c++20 -O2 -c $A/abi_demo.cpp -o arm64.o"
show aarch64-linux-gnu-objdump arm64.o _Z4sum8llllllll _Z7makeBigl _Z8callsOutl
finish

begin redzone "abi/redzone.cpp" "g++ -std=c++20 -O2 -fcf-protection=none -fno-stack-protector -c redzone.cpp (then also -mno-red-zone); objdump -d"
c "g++ -std=c++20 -O2 -fcf-protection=none -fno-stack-protector -c $A/redzone.cpp -o rz.o"
show objdump rz.o _Z13leafWithArrayi
c "g++ -std=c++20 -O2 -fcf-protection=none -fno-stack-protector -mno-red-zone -c $A/redzone.cpp -o norz.o"
show objdump norz.o _Z13leafWithArrayi
finish

begin call_asm "abi/sum6.S abi/call_asm.cpp" "g++ $SAN call_asm.cpp sum6.S -o call_asm && ./call_asm"
c "g++ $SAN $A/call_asm.cpp $A/sum6.S -o call_asm"
c "./call_asm"
finish

begin vtable "abi/vtable.cpp" "g++ -std=c++20 -O2 -c vtable.cpp; nm; nm -C; readelf -rW (vtable relocations)"
c "g++ -std=c++20 -O2 -c $A/vtable.cpp -o vtable.o"
c "nm vtable.o | grep -E ' (V|W|T|U) '"
c "nm -C vtable.o | grep -E 'vtable|typeinfo|Thermometer::|makeThermometer'"
c "readelf -SW vtable.o | grep -E 'ZTV11Thermometer'"
c "readelf -rW vtable.o | sed -n '/_ZTV11Thermometer/,/^\$/p'"
c "clang++ --target=x86_64-pc-windows-msvc -std=c++20 -O2 -c $A/vtable.cpp -o vtable.obj && llvm-nm vtable.obj | grep -E 'makeThermometer|read@Thermometer'"
finish

F="${LAB}/forensic"
begin forensic_O0 "forensic/totals.cpp forensic/scale.S" "g++ -std=c++20 -O0 totals.cpp scale.S -o totals_O0 && ./totals_O0 (no sanitizers)"
c "g++ -std=c++20 -O0 $F/totals.cpp $F/scale.S -o totals_O0"
c "./totals_O0"
finish

begin forensic_O2 "forensic/totals.cpp forensic/scale.S" "g++ -std=c++20 -O2 totals.cpp scale.S -o totals_O2 && ./totals_O2 (no sanitizers); objdump -d"
c "g++ -std=c++20 -O2 -fcf-protection=none $F/totals.cpp $F/scale.S -o totals_O2"
c "./totals_O2" 139
c "objdump -d --no-show-raw-insn totals_O2 | sed -n '/<_Z5totalPKli>:/,/^\$/p'"
c "objdump -d --no-show-raw-insn totals_O2 | sed -n '/<scale>:/,/^\$/p'"
c "gdb -q -batch -ex run -ex 'info registers rip rbx' -ex 'x/i \$pc' ./totals_O2 2>&1 | grep -v -E '^\\[|libthread_db|^Using|^Download|^$'" 0
finish "note:      exit code 139 (128 + 11, SIGSEGV) from ./totals_O2 is expected: that is the bug"

begin forensic_fixed "forensic/totals.cpp forensic/scale_fixed.S" "g++ -std=c++20 -O2 totals.cpp scale_fixed.S; then the sanitizer build"
c "g++ -std=c++20 -O2 $F/totals.cpp $F/scale_fixed.S -o totals_fixed && ./totals_fixed"
c "g++ $SAN -O2 $F/totals.cpp $F/scale_fixed.S -o totals_fixed_san && ./totals_fixed_san"
finish
rm -rf "$W"
exit $status
