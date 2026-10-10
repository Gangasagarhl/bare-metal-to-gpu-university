#!/usr/bin/env bash
# F0-63 timing steps: built with -O2 and WITHOUT sanitizers (sanitizers change what is
# measured), so they are .cc files that run_lab.sh does not compile itself.
set -u -o pipefail
cd "$(dirname "$0")"
TC="$(g++ --version | head -n 1)"
MACH="$(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs); CPU model: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')"
NOTE="note:      measured on the build container (a shared cloud virtual machine), one session; clocks not fixed (not possible here); not a specification (AH-23)"
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
W="-std=c++20 -Wall -Wextra -Wpedantic -Werror -O2"
for name in timing30 capture; do
    g++ $W $name.cc -o .bin_$name || exit 1
    timeout 120 ./.bin_$name > $name.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
    rec $name $name.cc "$TC" "g++ $W $name.cc -o $name && ./$name" $rc "$NOTE"
done
rm -f .bin_timing30 .bin_capture
exit $status
