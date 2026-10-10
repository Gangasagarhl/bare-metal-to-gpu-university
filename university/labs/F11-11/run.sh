#!/usr/bin/env bash
# F11-11 lab: coding standards. A static analyser (clang-tidy) over a careless
# parser and over the standard-conforming rewrite; and a demonstration of
# "warnings as errors" catching a narrowing conversion at build time.
# run_lab.sh builds and runs good.cpp (the clean rewrite) first.
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"; status=0
GCCV="$(g++ --version | head -n 1)"
TIDYV="$(clang-tidy --version 2>/dev/null | grep -oiE 'version [0-9.]+' | head -n1)"
CHECKS='-*,cert-*,clang-analyzer-*,bugprone-*'
W="${LAB}/.work"; rm -rf "$W"; mkdir -p "$W"

rec() {
    local name="$1" listing="$2" cmd="$3" code="$4"; shift 4
    { echo "listing:   $listing"; echo "toolchain: $GCCV; clang-tidy $TIDYV"; echo "command:   $cmd"
      echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
      echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
      echo "exit code: $code"; for l in "$@"; do echo "$l"; done; } > "${LAB}/${name}.log"
}

# clang-tidy over the careless file: expect findings.
clang-tidy --quiet -checks="$CHECKS" "${LAB}/bad.cc" -- -std=c++20 > "${LAB}/tidy_bad.out" 2>/dev/null
rc=$?; sed -i "s#${LAB}/##g" "${LAB}/tidy_bad.out"
nfind=$(grep -c "warning:" "${LAB}/tidy_bad.out")
echo "" >> "${LAB}/tidy_bad.out"; echo "($nfind clang-tidy warnings)" >> "${LAB}/tidy_bad.out"
rec tidy_bad "bad.cc checked by clang-tidy ($CHECKS)" \
    "clang-tidy --quiet -checks='$CHECKS' bad.cc -- -std=c++20" "$rc" "found:     $nfind warnings"
[ "$nfind" -ge 3 ] || status=1

# clang-tidy over the clean rewrite: expect no findings.
clang-tidy --quiet -checks="$CHECKS" "${LAB}/good.cpp" -- -std=c++20 > "${LAB}/tidy_good.out" 2>/dev/null
rc=$?; sed -i "s#${LAB}/##g" "${LAB}/tidy_good.out"
gfind=$(grep -c "warning:" "${LAB}/tidy_good.out")
[ -s "${LAB}/tidy_good.out" ] || echo "(no clang-tidy warnings)" > "${LAB}/tidy_good.out"
rec tidy_good "good.cpp checked by clang-tidy ($CHECKS)" \
    "clang-tidy --quiet -checks='$CHECKS' good.cpp -- -std=c++20" "$rc" "found:     $gfind warnings"
[ "$gfind" -eq 0 ] || status=1

# narrowing: builds quietly without the extra warnings...
{
    echo "# g++ -std=c++20 -Wall -Wextra narrowing.cc  (builds; the bug is silent):"
    if g++ -std=c++20 -Wall -Wextra "${LAB}/narrowing.cc" -o "$W/narrow" 2>&1; then
        echo "build: OK"; echo "\$ ./narrow"; ( cd "$W" && ./narrow )
    fi
    echo
    echo "# g++ -std=c++20 -Wall -Wextra -Wconversion -Wsign-conversion -Werror narrowing.cc  (fails):"
    g++ -std=c++20 -Wall -Wextra -Wconversion -Wsign-conversion -Werror "${LAB}/narrowing.cc" -o "$W/narrow2" 2>&1
    echo "build exit code: $?"
} > "${LAB}/narrowing.out" 2>&1
sed -i "s#${W}/##g; s#${LAB}/##g" "${LAB}/narrowing.out"
rec narrowing "narrowing.cc built two ways" \
    "g++ ... narrowing.cc; then g++ ... -Wconversion -Wsign-conversion -Werror narrowing.cc" 0 \
    "note:      the second build is expected to fail; that failure is the point"
grep -q "build exit code: 0" "${LAB}/narrowing.out" && { echo "narrowing build unexpectedly succeeded with -Werror" >> "${LAB}/narrowing.log"; status=1; }

rm -rf "$W"
exit $status
