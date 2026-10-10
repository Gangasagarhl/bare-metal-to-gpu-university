#!/usr/bin/env bash
# F2-51 timing steps: built with optimisation and WITHOUT sanitizers (sanitizers change
# what is measured), so they are .cc files that run_lab.sh does not compile itself.
set -u -o pipefail
cd "$(dirname "$0")"
TC="$(g++ --version | head -n 1)"
MACH="$(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs); CPU model: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')"
NOTE="note:      measured on the build container (a shared cloud virtual machine), one session; not a specification (AH-23)"
rec() {  # name listing toolchain command exitcode [extra line]
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
W="-std=c++20 -Wall -Wextra -Wpedantic -Werror"
for lvl in O0 O2; do
    g++ $W -$lvl dce.cc -o .bin_dce_$lvl || exit 1
    timeout 120 ./.bin_dce_$lvl > dce_$lvl.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
    rec dce_$lvl dce.cc "$TC" "g++ $W -$lvl dce.cc -o dce && ./dce" $rc "$NOTE"
done
# Forensic evidence: what the -O2 compiler made of the two functions.
CMD="objdump -d -C --no-show-raw-insn dce | awk '/^[0-9a-f]+ <(sumDiscarded|sumKept)/,/^\$/'"
objdump -d -C --no-show-raw-insn .bin_dce_O2 | awk '/^[0-9a-f]+ <(sumDiscarded|sumKept)/,/^$/' > dce_asm.out 2>&1; rc=$?
rec dce_asm dce.cc "$(objdump --version | head -n 1)" "$CMD" $rc "note:      disassembly of the -O2 build above; addresses are this build's"
g++ $W -O2 -pthread noise.cc -o .bin_noise || exit 1
timeout 120 ./.bin_noise 0 > noise_quiet.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec noise_quiet noise.cc "$TC" "g++ $W -O2 -pthread noise.cc -o noise && ./noise 0" $rc "$NOTE"
timeout 120 ./.bin_noise "$(nproc)" > noise_busy.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
rec noise_busy noise.cc "$TC" "g++ $W -O2 -pthread noise.cc -o noise && ./noise $(nproc)" $rc "$NOTE"
rm -f .bin_dce_O0 .bin_dce_O2 .bin_noise
exit $status
