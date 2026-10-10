#!/usr/bin/env bash
# F11-08 lab: bounds, lifetimes and ownership. Two bugs under AddressSanitizer
# (a dangling pointer after reallocation, and a use-after-free through a raw
# observer), then the safe patterns (safe.cpp, run by run_lab.sh first).
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"; status=0
GCCV="$(g++ --version | head -n 1)"
SAN="-std=c++20 -O1 -g -Wall -Wextra -fsanitize=address,undefined"
export ASAN_OPTIONS=abort_on_error=0:exitcode=99:detect_leaks=0
W="${LAB}/.work"; rm -rf "$W"; mkdir -p "$W"

rec() {
    local name="$1" listing="$2" cmd="$3" code="$4"; shift 4
    { echo "listing:   $listing"; echo "toolchain: $GCCV"; echo "command:   $cmd"
      echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
      echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
      echo "exit code: $code"; for l in "$@"; do echo "$l"; done; } > "${LAB}/${name}.log"
}
run_capture() {
    local name="$1" listing="$2"; shift 2
    g++ $SAN "${LAB}/${name}.cc" -o "$W/${name}" 2>/dev/null
    ( cd "$W" && "./${name}" ) > "${LAB}/${name}.out" 2>&1
    local rc=$?; sed -i "s#${W}/##g; s#${LAB}/##g" "${LAB}/${name}.out"
    rec "$name" "$listing" "g++ -fsanitize=address,undefined ${name}.cc; ./${name}" "$rc"
}

run_capture dangling "dangling.cc (-fsanitize=address,undefined)"
run_capture useafterfree "useafterfree.cc (-fsanitize=address,undefined)"

rm -rf "$W"
exit $status
