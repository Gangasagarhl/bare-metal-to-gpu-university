#!/usr/bin/env bash
# SPDX-License-Identifier: LicenseRef-Uni-Lab
# ci.sh: the one command that decides whether a change may be merged or released.
# Run from a clean clone. Three builds, so "works on my machine" cannot hide:
#   gcc-debug-asan  g++, Debug, AddressSanitizer + UBSan (stop on the first report)
#   gcc-release     g++, Release (optimised, NDEBUG)
#   clang-release   clang++, Release (a second compiler: different warnings, choices)
# then the release checks (versions, release notes, known issues, licences).
set -euo pipefail
cd "$(dirname "$0")"
run_config()
{
    local name="$1" cxx="$2"
    shift 2
    echo "== configuration: ${name} ($("${cxx}" --version | head -n 1))"
    cmake -S . -B "build/${name}" -DCMAKE_CXX_COMPILER="${cxx}" "$@" > "build-${name}.configure.txt"
    cmake --build "build/${name}" --parallel 2 > "build-${name}.build.txt"
    if ! ctest --test-dir "build/${name}" --output-on-failure > "build-${name}.test.txt" 2>&1; then
        cat "build-${name}.test.txt"
        echo "ci: FAILED in configuration ${name}"
        exit 1
    fi
    grep -E "tests passed" "build-${name}.test.txt"
}
mkdir -p build
run_config gcc-debug-asan g++ -DCMAKE_BUILD_TYPE=Debug -DUNI_SANITIZE=ON
run_config gcc-release g++ -DCMAKE_BUILD_TYPE=Release
run_config clang-release clang++ -DCMAKE_BUILD_TYPE=Release
echo "== release checks"
python3 tools/check_release.py .
echo "ci: all configurations and checks passed"
