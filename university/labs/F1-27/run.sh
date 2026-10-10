#!/usr/bin/env bash
# F1-27: run Listing 1 on the reordered program, the forensic program and its fix, and run
# the U16 computer (F1-24 Listing 2) to show that reordering did not change the answers.
set -u -o pipefail
cd "$(dirname "$0")"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
g++ $FLAGS hazards.cpp -o .bin_h || exit 1
g++ $FLAGS ../F1-24/cpu.cpp -o .bin_cpu || exit 1
step() {  # step <name> <listing text> <command text> <binary + args...> < input
    local name=$1 listing=$2 cmdtext=$3; shift 3
    "$@" > "$name.out" 2>&1; local rc=$?
    {
        echo "listing:   $listing"
        echo "toolchain: $(g++ --version | head -n 1)"
        echo "command:   $cmdtext"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $rc"
    } > "$name.log"
    [ "$rc" = 0 ] || status=1
}
step reordered "reordered.s (run with hazards.cpp)" "./hazards 10 < reordered.s" ./.bin_h 10 < reordered.s
step forensic "forensic.s (run with hazards.cpp)" "./hazards 14 < forensic.s" ./.bin_h 14 < forensic.s
step forensic_fixed "forensic_fixed.s (run with hazards.cpp)" "./hazards 14 < forensic_fixed.s" \
    ./.bin_h 14 < forensic_fixed.s
for s in hazards.in reordered.s forensic.s forensic_fixed.s; do
    n=answer_${s%%.*}
    step "$n" "$s (run with ../F1-24/cpu.cpp)" "./cpu < $s" ./.bin_cpu < "$s"
done
rm -f .bin_h .bin_cpu
exit $status
