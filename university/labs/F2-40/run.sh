#!/usr/bin/env bash
# F2-40: (1) ThreadSanitizer runs of the fixed transfers and the checker test; (2) the
# inverted pair under ThreadSanitizer; (3) the forensic ledger: start it, let it deadlock,
# attach gdb, dump every thread's stack, then kill it.
set -u
cd "$(dirname "$0")"
TC="$(g++ --version | head -n 1)"
W="-std=c++20 -Wall -Wextra -Wpedantic -Werror"
rec() {
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container), $(nproc) CPUs visible"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "note:      $6"; fi
    } > "$1.log"
}
clean() { sed -i "s#$(pwd)/##g; s# (BuildId: [0-9a-f]*)##g; s#\.bin_##g" "$1"; }
status=0
g++ $W -O1 -g -fsanitize=thread ordered_transfer.cpp -o .bin_ordered_transfer || status=1
timeout 120 ./.bin_ordered_transfer > ordered_transfer_tsan.out 2>&1; rc=$?; clean ordered_transfer_tsan.out
rec ordered_transfer_tsan ordered_transfer.cpp "$TC" "g++ $W -O1 -g -fsanitize=thread ordered_transfer.cpp -o ordered_transfer_tsan && ./ordered_transfer_tsan" "$rc" \
    "no WARNING lines and exit code 0 = ThreadSanitizer detected no race and no lock-order problem in this run"
[ "$rc" = 0 ] || status=1
g++ $W -O1 -g -fsanitize=thread checker_test.cpp -o .bin_checker_test || status=1
timeout 120 ./.bin_checker_test > checker_test_tsan.out 2>&1; rc=$?; clean checker_test_tsan.out
rec checker_test_tsan checker_test.cpp "$TC" "g++ $W -O1 -g -fsanitize=thread checker_test.cpp -o checker_test_tsan && ./checker_test_tsan" "$rc" \
    "expected: ThreadSanitizer ALSO reports the deliberate inversion (exit code 66 is its own); the checker's PASS line still appears"
[ "$rc" = 66 ] || status=1
g++ $W -O1 -g -fsanitize=thread inversion.cc -o .bin_inversion || status=1
timeout 60 ./.bin_inversion > inversion_tsan.out 2>&1; rc=$?; clean inversion_tsan.out
rec inversion_tsan inversion.cc "$TC" "g++ $W -O1 -g -fsanitize=thread inversion.cc -o inversion_tsan && ./inversion_tsan" "$rc" \
    "exit code 66 is ThreadSanitizer's own after it reported a problem"
# forensic evidence: a real deadlock and a real thread dump
GV="$(gdb --version | head -n 1)"
g++ $W -O0 -g ledger.cc -o .bin_ledger || status=1
./.bin_ledger > ledger.out 2>&1 &
pid=$!
sleep 2
if kill -0 "$pid" 2>/dev/null; then state="still running after 2 s"; else state="exited"; fi
timeout 60 gdb -q -batch -p "$pid" -ex "info threads" -ex "thread apply all bt" \
    -ex "echo \\n=== locals of transfer() in each thread ===\\n" \
    -ex "thread apply all frame apply all -s print from.name" -ex "thread apply all frame apply all -s -q print &from.m" \
    -ex "thread apply all frame apply all -s -q print to.name" -ex "thread apply all frame apply all -s -q print &to.m" > thread_dump.raw 2>&1
kill -9 "$pid" 2>/dev/null; wait "$pid" 2>/dev/null
# keep the dump readable: drop gdb's start-up chatter and the C++ library's thread-start frames
grep -v -E '^\[New LWP|^\[Thread debugging|^Using host libthread_db|^warning: [0-9]+\s|No such file or directory|^\[Inferior 1 .* detached\]' thread_dump.raw \
  | sed -E "s#$(pwd)/##g; s#\.bin_##g; s#0x[0-9a-f]{12} in ##" \
  | awk '/^#[0-9]+ / { if ($0 ~ /std::__invoke|std::thread::_|_M_run|start_thread|clone3|in \?\? \(\) from .*libstdc/) { if (!cut) { print "    [...] (frames of the thread-start machinery removed)"; cut=1 } ; next } } { if ($0 !~ /^#/) cut=0; print }' \
  > thread_dump.out
rm -f thread_dump.raw
echo "ledger process: $state; killed after the dump" >> ledger.out
rec thread_dump ledger.cc "$TC; $GV" "g++ $W -O0 -g ledger.cc -o ledger; ./ledger & sleep 2; gdb -q -batch -p <pid> -ex 'info threads' -ex 'thread apply all bt' -ex 'thread apply all frame apply all -s print from.name' (and &from.m, to.name, &to.m)" 0 \
    "the dump is trimmed: gdb start-up lines and the C++ library's thread-start frames are replaced by [...]; addresses vary per run"
rec ledger ledger.cc "$TC" "g++ $W -O0 -g ledger.cc -o ledger && ./ledger (killed by run.sh after the thread dump)" 137 \
    "137 = killed with SIGKILL by run.sh, because the program never finished"
rm -f .bin_*
exit $status
