#!/usr/bin/env bash
# F11-07 lab: how memory bugs become attacks. Three demos, each built twice:
#   - plain -O0 -fno-stack-protector, to watch the corruption happen;
#   - with -fsanitize=address,undefined, to see what a sanitizer does (and, for
#     the intra-object overflow, does NOT) report.
# run_lab.sh builds and runs safe.cpp (the fixed versions) before this script.
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"; status=0
GCCV="$(g++ --version | head -n 1)"
PLAIN="-std=c++20 -O0 -g -fno-stack-protector -fno-pie -no-pie -Wall -Wextra"
SAN="-std=c++20 -O1 -g -Wall -Wextra -fsanitize=address,undefined"
export ASAN_OPTIONS=abort_on_error=0:exitcode=99:detect_leaks=0
export UBSAN_OPTIONS=halt_on_error=1:abort_on_error=0:print_stacktrace=0
W="${LAB}/.work"; rm -rf "$W"; mkdir -p "$W"

rec() {   # rec <name> <listing> <command> <exit text> [extra lines...]
    local name="$1" listing="$2" cmd="$3" code="$4"; shift 4
    {
        echo "listing:   $listing"; echo "toolchain: $GCCV"; echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $code"; for l in "$@"; do echo "$l"; done
    } > "${LAB}/${name}.log"
}
run_capture() {   # run_capture <name> <listing> <command...>
    local name="$1" listing="$2"; shift 2
    ( cd "$W" && "$@" ) > "${LAB}/${name}.out" 2>&1
    local rc=$?; sed -i "s#${W}/##g; s#${LAB}/##g" "${LAB}/${name}.out"
    rec "$name" "$listing" "$*" "$rc"
}

# Demo 1: adjacent flag, plain build. Safe name, then a 20-byte attack name.
g++ $PLAIN "${LAB}/adjacent.cc" -o "$W/adjacent_plain" 2>/dev/null
{
    echo "\$ ./adjacent amina"; ( cd "$W" && ./adjacent_plain amina )
    echo "\$ ./adjacent AAAAAAAAAAAAAAAAAAAA   # 20 bytes = sizeof(Account); fills name AND is_admin, still in bounds"
    ( cd "$W" && ./adjacent_plain AAAAAAAAAAAAAAAAAAAA )
} > "${LAB}/adjacent_plain.out" 2>&1
rec adjacent_plain "adjacent.cc (plain -O0, no stack protector)" "./adjacent amina; ./adjacent <20 A s>" 0

# Demo 1 again, under ASan: honest result -- an intra-object overflow is NOT caught.
g++ $SAN "${LAB}/adjacent.cc" -o "$W/adjacent_asan" 2>/dev/null
run_capture adjacent_asan "adjacent.cc (-fsanitize=address,undefined)" ./adjacent_asan AAAAAAAAAAAAAAAAAAAA

# Demo 2: control-flow hijack, plain build (deterministic).
g++ $PLAIN "${LAB}/hijack.cc" -o "$W/hijack_plain" 2>/dev/null
run_capture hijack_plain "hijack.cc (plain -O0, no stack protector)" ./hijack_plain

# Demo 3: integer overflow -> heap overflow, under ASan (this IS caught).
g++ $SAN "${LAB}/intoverflow.cc" -o "$W/intoverflow_asan" 2>/dev/null
run_capture intoverflow_asan "intoverflow.cc (-fsanitize=address,undefined)" ./intoverflow_asan 1073741825

rm -rf "$W"
exit $status
