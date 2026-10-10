#!/usr/bin/env bash
# F12-14 run.sh - whole-system profile of the kvsvc service (Listing 1), real runs:
#   kvsvc_whole   / kvsvc_whole_key   : snapshot holds the store lock for the whole store
#   kvsvc_chunked / kvsvc_chunked_key : first fix attempt, 1,000 entries per hold, back to back
#   kvsvc_paced   / kvsvc_paced_key   : 1,000 entries per hold, 0.2 ms pause between holds
#   kvsvc_strace  : system-call summary of a short paced run (strace -f -c)
#   kvsvc_tsan    : a short run built with ThreadSanitizer (data-race check)
# Timing builds use -O2 without sanitizers; every step writes <name>.out and <name>.log.
set -u
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B/data"
GXXV="$(g++ --version | head -n 1)"
CPU="$(grep -m1 'model name' /proc/cpuinfo | sed 's/.*: //')"
NOTE="measured on the build container (a shared cloud virtual machine, other jobs running); timings change from run to run and are not a specification (AH-23)"
rec() {  # rec <name> <listing> <toolchain> <command> <exit code text> [note]
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs visible, $CPU)"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "note:      $6"; fi
    } > "$1.log"
}

BUILD="g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -pthread kvsvc.cc -o kvsvc"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -pthread kvsvc.cc -o $B/kvsvc || exit 1

for mode in whole chunked paced; do
    timeout 120 $B/kvsvc --mode $mode --dir $B/data --key "kvsvc_${mode}_key.out" > "kvsvc_${mode}.out" 2>&1; rc=$?
    rec "kvsvc_${mode}" kvsvc.cc "$GXXV" "$BUILD; ./kvsvc --mode $mode --dir data --key kvsvc_${mode}_key.out" "$rc" "$NOTE"
    rec "kvsvc_${mode}_key" kvsvc.cc "$GXXV" "(same run as kvsvc_${mode}: the analysis part written by --key)" "$rc" "$NOTE"
    [ "$rc" = 0 ] || status=1
done

SV="$(strace --version | head -n 1)"
timeout 120 strace -f -c -o kvsvc_strace.out $B/kvsvc --mode paced --seconds 2 --dir $B/data > /dev/null 2>&1; rc=$?
rec kvsvc_strace kvsvc.cc "$SV; $GXXV" "strace -f -c -o kvsvc_strace.out ./kvsvc --mode paced --seconds 2 --dir data" "$rc" \
    "system-call counts and times for all threads; strace slows every system call, so its times are inflated (AH-23)"
[ "$rc" = 0 ] || status=1

g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -O1 -fsanitize=thread -pthread kvsvc.cc -o $B/kvsvc_tsan || exit 1
timeout 120 $B/kvsvc_tsan --mode paced --seconds 1 --rate 500 --dir $B/data > $B/tsan_report.txt 2>&1; rc=$?
{
    echo "ThreadSanitizer run: exit code $rc"
    if grep -q "WARNING: ThreadSanitizer" $B/tsan_report.txt; then
        grep -A3 "WARNING: ThreadSanitizer" $B/tsan_report.txt | head -n 20
    else
        echo "no ThreadSanitizer warnings in the run (report lines checked: 'WARNING: ThreadSanitizer')"
    fi
    grep -A3 "^== 1" $B/tsan_report.txt | head -n 2
} > kvsvc_tsan.out
rec kvsvc_tsan kvsvc.cc "$GXXV" "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -O1 -fsanitize=thread -pthread kvsvc.cc -o kvsvc_tsan; ./kvsvc_tsan --mode paced --seconds 1 --rate 500 --dir data" \
    "$rc" "data-race check only; timings under ThreadSanitizer are meaningless"
[ "$rc" = 0 ] || status=1
grep -q "WARNING: ThreadSanitizer" $B/tsan_report.txt && status=1

rm -rf "$B"
exit $status
