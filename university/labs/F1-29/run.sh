#!/usr/bin/env bash
# F1-29: build accum.cc with optimisation (-O2), run it (a real timing on this machine),
# and disassemble its two hot loops so the timing can be explained from the instructions.
set -u -o pipefail
cd "$(dirname "$0")"
CMD="g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror accum.cc -o accum"
rec() {
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container); CPU model: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
}
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror accum.cc -o .bin_accum || exit 1
./.bin_accum > accum.out 2>&1; rc=$?
rec accum accum.cc "$(g++ --version | head -n 1)" "$CMD && ./accum" "$rc" \
    "note:      times are one measurement session on a shared cloud container (AH-23), not a CPU specification"
objdump -d --no-show-raw-insn -M intel -C .bin_accum \
    | awk '/^[0-9a-f]+ <sumOne\(/,/^$/' > accum_asm_one.out; rc1=$?
objdump -d --no-show-raw-insn -M intel -C .bin_accum \
    | awk '/^[0-9a-f]+ <sumFour\(/,/^$/' > accum_asm_four.out; rc2=$?
rec accum_asm_one accum.cc "$(objdump --version | head -n 1)" \
    "objdump -d --no-show-raw-insn -M intel -C accum  (function sumOne)" "$rc1"
rec accum_asm_four accum.cc "$(objdump --version | head -n 1)" \
    "objdump -d --no-show-raw-insn -M intel -C accum  (function sumFour)" "$rc2"
rm -f .bin_accum
[ "$rc" = 0 ] && [ "$rc1" = 0 ] && [ "$rc2" = 0 ]
