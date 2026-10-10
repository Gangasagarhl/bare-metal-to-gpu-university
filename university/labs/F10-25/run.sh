#!/usr/bin/env bash
# F10-25 lab: build the log tools, simulate two flights that differ only in one
# parameter, write both as ULog-style files, and analyse them. Each step writes
# <step>.out and <step>.log in the run_lab.sh format. The .ulg files and binaries are
# deleted at the end (they are regenerated exactly by every run).
set -u
cd "$(dirname "$0")"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
VER="$(g++ --version | head -n 1)"
status=0

header() {   # step, listing, command
    {
        echo "listing:   $2"
        echo "toolchain: $VER"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$1.log"
}

for tool in flight_sim ulog_info innov_report; do
    if ! g++ $FLAGS "$tool.cc" -o ".bin_$tool" > ".build_$tool.txt" 2>&1; then
        header "$tool" "$tool.cc" "g++ $FLAGS $tool.cc -o $tool"
        echo "result:    BUILD FAILED" >> "$tool.log"; cat ".build_$tool.txt" >> "$tool.log"
        status=1
    fi
    rm -f ".build_$tool.txt"
done
[ "$status" = 0 ] || exit 1

step() {   # step name, listing, shown command, then the real command
    local name="$1" listing="$2" shown="$3"; shift 3
    header "$name" "$listing" "g++ $FLAGS $listing -o ${listing%.cc}; $shown"
    timeout 60 "$@" > "$name.out" 2>&1; local rc=$?
    echo "exit code: $rc" >> "$name.log"
    [ "$rc" = 0 ] || status=1
}

step flight_sim flight_sim.cc "./flight_sim good.ulg 0.15 && ./flight_sim bad.ulg 1.5" \
    bash -c './.bin_flight_sim good.ulg 0.15 && ./.bin_flight_sim bad.ulg 1.5'
step header_bytes flight_sim.cc "od -A d -t x1 -N 48 bad.ulg" od -A d -t x1 -N 48 bad.ulg
sed -i "s/^listing: .*/listing:   bad.ulg as written by flight_sim.cc (first 48 bytes)/; s/^toolchain: .*/toolchain: $(od --version | head -n 1)/; s/^command: .*/command:   od -A d -t x1 -N 48 bad.ulg/" header_bytes.log
step ulog_info ulog_info.cc "./ulog_info bad.ulg" ./.bin_ulog_info bad.ulg
step params_diff ulog_info.cc "./ulog_info good.ulg bad.ulg" ./.bin_ulog_info good.ulg bad.ulg
step innov_bad innov_report.cc "./innov_report bad.ulg" ./.bin_innov_report bad.ulg
step innov_good innov_report.cc "./innov_report good.ulg" ./.bin_innov_report good.ulg

rm -f .bin_flight_sim .bin_ulog_info .bin_innov_report good.ulg bad.ulg
exit $status
