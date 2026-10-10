#!/usr/bin/env bash
# F12-19 run.sh: Listing 2 (cast_ub.cc) built twice: with run_lab.sh's default flags
# (cast_ub_default) and with the float-cast-overflow sanitizer added (cast_ub).
# Each step writes <name>.out and <name>.log.
set -u
cd "$(dirname "$0")"
BASE="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
status=0
step() {  # step <name> <extra flags>
    local name="$1" flags="$BASE${2:+,$2}"
    {
        echo "listing:   cast_ub.cc (run by run.sh)"
        echo "toolchain: $(g++ --version | head -n 1)"
        echo "command:   g++ $flags cast_ub.cc -o cast_ub; ./cast_ub"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$name.log"
    if ! g++ $flags cast_ub.cc -o ./.bin_cast > "$name.out" 2>&1; then
        echo "result:    BUILD FAILED" >> "$name.log"; status=1; return
    fi
    timeout 10 ./.bin_cast > "$name.out" 2>&1
    local rc=$?
    sed -i "s#$(pwd)/##g" "$name.out"
    echo "exit code: $rc" >> "$name.log"
    rm -f ./.bin_cast
}
step cast_ub_default ""
step cast_ub float-cast-overflow
grep -q "runtime error" cast_ub.out || status=1
exit $status
