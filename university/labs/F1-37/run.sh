#!/usr/bin/env bash
# F1-37: the two real multi-threaded experiments (-O2, -pthread: observations on this machine)
# and the disassembly of Peterson's lock loop for both memory-order choices.
set -u -o pipefail
cd "$(dirname "$0")"
TC="$(g++ --version | head -n 1)"
MACH="$(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs); CPU model: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')"
NOTE="note:      observed on the build container (a shared cloud virtual machine), one session; counts vary from run to run and are not guarantees"
rec() {
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $MACH"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
}
status=0
OPT="g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread"
$OPT store_buffer.cc -o .bin_sb || exit 1
$OPT peterson.cc -o .bin_pt || exit 1
timeout 120 ./.bin_sb > store_buffer.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec store_buffer store_buffer.cc "$TC" "$OPT store_buffer.cc -o store_buffer && ./store_buffer" $rc "$NOTE"
timeout 120 ./.bin_pt > peterson.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec peterson peterson.cc "$TC" "$OPT peterson.cc -o peterson && ./peterson" $rc "$NOTE"
OD="objdump -d --no-show-raw-insn -M intel -C"
SHORT='s/<std::thread::_State_impl.*_M_run\(\)(\+0x[0-9a-f]+)?>/<run\1>/'
$OD .bin_pt | awk '/experiment<\(std::memory_order\)3, \(std::memory_order\)2>\(char const\*\)::\{lambda\(\)#1\}> > >::_M_run\(\)>:/,/^$/' \
    | sed -E "$SHORT" > peterson_asm_relacq.out; rc=$?
rec peterson_asm_relacq peterson.cc "$(objdump --version | head -n 1)" \
    "$OD peterson  (thread 0's loop, release/acquire version; long symbol names shortened to <run>)" $rc
$OD .bin_pt | awk '/experiment<\(std::memory_order\)5, \(std::memory_order\)5>\(char const\*\)::\{lambda\(\)#1\}> > >::_M_run\(\)>:/,/^$/' \
    | sed -E "$SHORT" > peterson_asm_seqcst.out; rc=$?
rec peterson_asm_seqcst peterson.cc "$(objdump --version | head -n 1)" \
    "$OD peterson  (thread 0's loop, seq_cst version; long symbol names shortened to <run>)" $rc
rm -f .bin_sb .bin_pt
exit $status
