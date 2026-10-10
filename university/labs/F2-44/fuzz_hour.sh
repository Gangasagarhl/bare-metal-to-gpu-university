#!/usr/bin/env bash
# fuzz_hour.sh: the full acceptance run of milestone P2 -- one hour of fuzzing.
# Not called by run.sh (it takes an hour); run it by hand from the repository root:
#   university/labs/F2-44/fuzz_hour.sh
# It writes fuzz_hour.out and fuzz_hour.log in the same format as the other steps.
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"; W="${LAB}/.work_hour"; rm -rf "$W"; mkdir -p "$W"; cd "$W" || exit 2
g++ -std=c++20 -O1 -c "${LAB}/samples/hello.cpp" -o hello.o
g++ -std=c++20 -O1 "${LAB}/samples/hello.cpp" -o hello
g++ -std=c++20 -O1 -static "${LAB}/samples/hello.cpp" -o hello-static
g++ -std=c++20 -O1 -fPIC -shared "${LAB}/samples/greet.cpp" -o libgreet.so
strip hello -o hello-stripped
aarch64-linux-gnu-g++ -std=c++20 -O1 "${LAB}/samples/hello.cpp" -o hello-arm64
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined -O1"
g++ $FLAGS "${LAB}/elf/fuzz.cpp" -o fuzz || exit 1
CMD="./fuzz 3600 hello.o hello hello-static libgreet.so hello-stripped hello-arm64"
{
    echo "listing:   elf/fuzz.cpp elf/elf_reader.h"
    echo "toolchain: $(g++ --version | head -n 1)"
    echo "command:   g++ $FLAGS fuzz.cpp -o fuzz && $CMD"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ) (start)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > "${LAB}/fuzz_hour.log"
$CMD > "${LAB}/fuzz_hour.out" 2>&1
rc=$?
echo "end:       $(date -u +%Y-%m-%dT%H:%M:%SZ)" >> "${LAB}/fuzz_hour.log"
echo "exit code: $rc" >> "${LAB}/fuzz_hour.log"
cd "$LAB" && rm -rf "$W"
exit $rc
