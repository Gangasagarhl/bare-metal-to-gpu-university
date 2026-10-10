#!/usr/bin/env bash
# F9-48 lab steps: build latency.cc with sanitizers (smoke run), then at -O2 without
# sanitizers, and measure wake-up latency in five configurations on this machine.
# latency.cc is a .cc file so that run_lab.sh does not build it with sanitizers only:
# sanitizers add work to every memory access and would distort the measurement.
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
TC="$(g++ --version | head -n 1)"
SAN="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined -pthread"
OPT="-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread"
MEAS="measured: on the build container (a virtual machine shared with other work, kernel without PREEMPT_RT), run as root; numbers change from run to run (AH-23)"
N="$(nproc)"
rec() {  # rec <name> <command> <exit code> [extra line]
    {
        echo "listing:   latency.cc"
        echo "toolchain: $TC"
        echo "command:   $2"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container), kernel $(uname -r) ($(uname -v)), $N CPUs"
        echo "exit code: $3"
        if [ $# -ge 4 ]; then echo "$4"; fi
    } > "$1.log"
}
step() {  # step <name> <arguments...>
    local name="$1"; shift
    timeout 60 $B/latency "$@" > "$name.out" 2>&1; local rc=$?
    rec "$name" "./latency $*" "$rc" "$MEAS"
    [ "$rc" = 0 ] || status=1
}

g++ $SAN latency.cc -o $B/latency_san && timeout 30 $B/latency_san --loops 200 --work reuse > smoke.out 2>&1
rc=$?; rec smoke "g++ $SAN latency.cc -o latency; ./latency --loops 200 --work reuse" "$rc" \
    "note:      sanitizer build, short run: checks memory and undefined behaviour; timing not used"
[ "$rc" = 0 ] || status=1

g++ $OPT latency.cc -o $B/latency || status=1
step idle_other  --policy other --loops 5000
step load_other  --policy other --loops 5000 --load "$N"
step load_fifo   --policy fifo --prio 80 --mlock --loops 5000 --load "$N"
step fault_fresh --policy fifo --prio 80 --mlock --loops 3000 --work fresh
step fault_reuse --policy fifo --prio 80 --mlock --loops 3000 --work reuse
rm -rf "$B"
exit $status
