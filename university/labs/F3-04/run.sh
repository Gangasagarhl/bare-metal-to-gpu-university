#!/usr/bin/env bash
# Extra runs for F3-04: plain (no sanitizer) builds, because sanitizers change the address space.
set -u
gxx="$(g++ --version | head -n 1)"
flags="-std=c++20 -Wall -Wextra -Wpedantic -Werror -O0"
one() {   # one <name> <source>
    {
        echo "listing:   $2 (built without sanitizers)"
        echo "toolchain: $gxx"
        echo "command:   g++ $flags $2 -o $1 && ./$1"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$1.log"
    g++ $flags "$2" -o "$1" || { echo "result:    BUILD FAILED" >> "$1.log"; return 1; }
    "./$1" > "$1.out" 2>&1; echo "exit code: $?" >> "$1.log"
    sed -i "s#$(pwd)/##g" "$1.out"
    rm -f "./$1"
}
one lazy_pages_plain lazy_pages.cpp || exit 1
one show_maps show_maps.cc || exit 1
exit 0
