#!/usr/bin/env bash
# DS401 F5-38 extra steps (after run_lab.sh built and ran the listings with sanitizers):
#   ring_O2          Listing 1 rebuilt with -O2 and no sanitizers (measurement build)
#   syscalls_socket  strace -f -c of Listing 1, socket path only
#   syscalls_ring    strace -f -c of Listing 1, ring path only
#   bypass_syscalls  strace -f -c of the forensic program, as deployed (evidence)
#   bypass_sleep_O2  the forensic program as deployed, -O2 build (evidence, same build as bypass_spin)
#   bypass_spin      the forensic program with a busy-polling server (answer key)
set -u
cd "$(dirname "$0")"
status=0
ver="$(g++ --version | head -n 1)"
sver="$(strace -V | head -n 1)"
log()   # name listing toolchain command rc
{
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs visible)"
        echo "exit code: $5"
    } > "$1.log"
}
build="g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror"
$build ring_ipc.cpp -o .bin_ring || status=1
$build bypass_sleep.cpp -o .bin_bypass -pthread || status=1

./.bin_ring > ring_O2.out 2>&1; rc=$?
log ring_O2 ring_ipc.cpp "$ver" "$build ring_ipc.cpp -o ring_ipc; ./ring_ipc" $rc
[ $rc = 0 ] || status=1

for m in socket ring; do
    strace -f -c -o "syscalls_$m.out" ./.bin_ring "$m" > /dev/null 2>&1; rc=$?
    log "syscalls_$m" ring_ipc.cpp "$sver; $ver" "strace -f -c -o syscalls_$m.out ./ring_ipc $m   (the -O2 build)" $rc
    [ $rc = 0 ] || status=1
done

strace -f -c -o bypass_syscalls.out ./.bin_bypass > /dev/null 2>&1; rc=$?
log bypass_syscalls bypass_sleep.cpp "$sver; $ver" "strace -f -c -o bypass_syscalls.out ./bypass_sleep   (-O2 build, as deployed)" $rc
[ $rc = 0 ] || status=1

./.bin_bypass > bypass_sleep_O2.out 2>&1; rc=$?
log bypass_sleep_O2 bypass_sleep.cpp "$ver" "$build bypass_sleep.cpp -o bypass_sleep -pthread; ./bypass_sleep" $rc
[ $rc = 0 ] || status=1

./.bin_bypass spin > bypass_spin.out 2>&1; rc=$?
log bypass_spin bypass_sleep.cpp "$ver" "$build bypass_sleep.cpp -o bypass_sleep -pthread; ./bypass_sleep spin" $rc
[ $rc = 0 ] || status=1
rm -f .bin_ring .bin_bypass
exit $status
