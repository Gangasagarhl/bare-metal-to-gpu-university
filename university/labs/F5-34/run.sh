#!/usr/bin/env bash
# DS401 F5-34 extra steps (run by run_lab.sh after the .cpp listings):
#   paths_O2       Listing 1 rebuilt with -O2 and no sanitizers (measurement build)
#   syscalls_tcp   strace -c of Listing 1, TCP path only
#   syscalls_shm   strace -c of Listing 1, shared-memory path only
#   nagle_trace    strace of the forensic client (3 requests): the system calls and their times
#   nagle_nodelay  the forensic client with TCP_NODELAY (answer key)
#   nagle_onewrite the forensic client with one writev per request (answer key)
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
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror paths.cpp -o .bin_paths -pthread || status=1
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror nagle.cpp -o .bin_nagle -pthread || status=1

./.bin_paths > paths_O2.out 2>&1; rc=$?
log paths_O2 paths.cpp "$ver" "g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror paths.cpp -o paths -pthread; ./paths" $rc
[ $rc = 0 ] || status=1

for m in tcp shm; do
    strace -f -c -o "syscalls_$m.out" ./.bin_paths "$m" > /dev/null 2>&1; rc=$?
    log "syscalls_$m" paths.cpp "$sver; $ver" "strace -f -c -o syscalls_$m.out ./paths $m   (the -O2 build above)" $rc
    [ $rc = 0 ] || status=1
done

strace -f -r -e trace=sendto,recvfrom,writev,setsockopt,getsockopt ./.bin_nagle trace > nagle_trace.out 2>&1; rc=$?
# process ids differ from run to run; replace them so the evidence is stable to read
sed -i -E 's/^\[pid +[0-9]+\]/[pid N]/; s/^strace: Process [0-9]+ attached/strace: Process N attached/' nagle_trace.out
log nagle_trace nagle.cpp "$sver; $ver" "strace -f -r -e trace=sendto,recvfrom,writev,setsockopt,getsockopt ./nagle trace   (process ids replaced by N)" $rc
[ $rc = 0 ] || status=1

for m in nodelay onewrite; do
    ./.bin_nagle "$m" > "nagle_$m.out" 2>&1; rc=$?
    log "nagle_$m" nagle.cpp "$ver" "./nagle $m   (-O2 build)" $rc
    [ $rc = 0 ] || status=1
done
rm -f .bin_paths .bin_nagle
exit $status
