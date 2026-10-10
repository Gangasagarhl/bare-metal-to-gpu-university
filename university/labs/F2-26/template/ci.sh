#!/usr/bin/env bash
# ci.sh: the one command that checks this repository from a clean clone.
# Two configurations: Debug with AddressSanitizer + UBSan, and Release (optimised,
# NDEBUG). Each is configured, built and tested; any failure stops the script.
set -euo pipefail
cd "$(dirname "$0")"
run_config()
{
    local name="$1"
    shift
    echo "== configuration: ${name}"
    cmake -S . -B "build/${name}" "$@" > "build-${name}.configure.txt"
    cmake --build "build/${name}" --parallel 2 > "build-${name}.build.txt"
    ctest --test-dir "build/${name}" --output-on-failure
}
mkdir -p build
run_config debug-asan -DCMAKE_BUILD_TYPE=Debug -DUNI_SANITIZE=ON
run_config release -DCMAKE_BUILD_TYPE=Release
echo "ci: all configurations passed"
