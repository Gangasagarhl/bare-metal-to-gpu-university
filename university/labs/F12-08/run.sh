#!/usr/bin/env bash
# F12-08 run.sh - fuzzing with the university's own coverage-guided fuzzer.
#   fuzz_blind   : 300,000 mutations of the starting input, no coverage feedback
#   fuzz_guided  : the same budget and seed with coverage feedback (finds the planted bug)
#   regress_old  : the crash kept as a regression test, against the old parser (must fail)
#   regress_fixed: the same test against the fixed parser (must pass)
#   fuzz_fixed   : coverage-guided fuzzing of the fixed parser, same budget (no crash)
#   no_libfuzzer : what happens here when clang's libFuzzer is requested
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
F="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -O1 -fsanitize=address,undefined"
GXXV="$(g++ --version | head -n 1)"
export ASAN_OPTIONS=print_legend=0
rec() {  # rec <name> <listing> <toolchain> <command> <exit code text>
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $5"
    } > "$1.log"
}
clean() { sed -i "s#$(pwd)/##g; s#$(pwd)/$B/##g" "$1"; }

g++ $F -fsanitize-coverage=trace-pc -c pkt.cc -o $B/pkt.o &&
    g++ $F -fsanitize-coverage=trace-pc -c pkt_fixed.cc -o $B/pkt_fixed.o &&
    g++ $F -c minifuzz.cc -o $B/minifuzz.o &&
    g++ $F $B/pkt.o $B/minifuzz.o -o $B/minifuzz &&
    g++ $F $B/pkt_fixed.o $B/minifuzz.o -o $B/minifuzz_fixed &&
    g++ $F regress.cc pkt.cc -o $B/regress_old &&
    g++ $F regress.cc pkt_fixed.cc -o $B/regress_fixed || { echo "build failed"; exit 1; }
BUILD="g++ $F -fsanitize-coverage=trace-pc -c pkt.cc; g++ $F -c minifuzz.cc; g++ $F pkt.o minifuzz.o -o minifuzz"

timeout 120 $B/minifuzz blind 1 300000 > fuzz_blind.out 2>&1; rc=$?
clean fuzz_blind.out
rec fuzz_blind "minifuzz.cc pkt.cc pkt.hh" "$GXXV" "$BUILD; ASAN_OPTIONS=print_legend=0 ./minifuzz blind 1 300000" "$rc"
[ "$rc" = 0 ] || status=1

timeout 120 $B/minifuzz guided 1 300000 > fuzz_guided.out 2>&1; rc=$?
clean fuzz_guided.out
rec fuzz_guided "minifuzz.cc pkt.cc pkt.hh" "$GXXV" "$BUILD; ASAN_OPTIONS=print_legend=0 ./minifuzz guided 1 300000" \
    "$rc (expected: AddressSanitizer stops the run when the fuzzer finds the planted bug)"
[ "$rc" != 0 ] || status=1

timeout 60 $B/regress_old 2>&1 | grep -E '^regress|^SUMMARY|ERROR' > regress_old.out; rc=${PIPESTATUS[0]}
clean regress_old.out
rec regress_old "regress.cc pkt.cc pkt.hh" "$GXXV" \
    "g++ $F regress.cc pkt.cc -o regress_old; ./regress_old 2>&1 | grep -E '^regress|^SUMMARY|ERROR'" \
    "$rc (expected: the old parser fails the regression test)"
[ "$rc" != 0 ] || status=1

timeout 60 $B/regress_fixed > regress_fixed.out 2>&1; rc=$?
rec regress_fixed "regress.cc pkt_fixed.cc pkt.hh" "$GXXV" "g++ $F regress.cc pkt_fixed.cc -o regress_fixed; ./regress_fixed" "$rc"
[ "$rc" = 0 ] || status=1

timeout 120 $B/minifuzz_fixed guided 1 300000 > fuzz_fixed.out 2>&1; rc=$?
clean fuzz_fixed.out
rec fuzz_fixed "minifuzz.cc pkt_fixed.cc pkt.hh" "$GXXV" \
    "g++ $F -fsanitize-coverage=trace-pc -c pkt_fixed.cc; g++ $F pkt_fixed.o minifuzz.o -o minifuzz_fixed; ASAN_OPTIONS=print_legend=0 ./minifuzz_fixed guided 1 300000" "$rc"
[ "$rc" = 0 ] || status=1

echo 'extern "C" int LLVMFuzzerTestOneInput(const unsigned char*, unsigned long) { return 0; }' > $B/lf.cpp
clang++ -fsanitize=fuzzer,address $B/lf.cpp -o $B/lf > no_libfuzzer.out 2>&1; rc=$?
sed -i "s#$(pwd)/$B/##g; s#/tmp/[^ ]*\.o#<temporary object file>#g" no_libfuzzer.out
rec no_libfuzzer "(a one-line fuzz target written by run.sh)" "$(clang++ --version | head -n 1)" \
    "clang++ -fsanitize=fuzzer,address lf.cpp -o lf" "$rc (expected here: the libFuzzer runtime library is not installed in this build container)"

rm -rf "$B"
exit $status
