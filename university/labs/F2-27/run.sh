#!/usr/bin/env bash
# F2-27 lab: the same buggy file compiled with more and more warning flags, with two
# compilers and two optimisation levels; what -Wall really enables; and the forensic
# "warning nobody read". Transcript-style .out files, run records in .log files.
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
T="${LAB}/.tmp"; rm -rf "$T"; mkdir -p "$T"
status=0
begin() {
    OUT="${LAB}/$1.out"; LOG="${LAB}/$1.log"; : > "$OUT"
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$LOG"
}
c() {
    echo "\$ $1" >> "$OUT"
    bash -c "$1" >> "$OUT" 2>&1
    LAST=$?
}
end() {
    sed -i "s#${T}/#./#g" "$OUT"
    echo "exit code: ${LAST}" >> "$LOG"
    if [ $# -ge 2 ]; then echo "note:      $2" >> "$LOG"; fi
    if [ "$LAST" != "$1" ]; then echo "result:    UNEXPECTED (expected exit code $1)" >> "$LOG"; status=1; fi
}
GXX="$(g++ --version | head -n 1)"
CLG="$(clang++ --version | head -n 1)"
W='grep -E "warning|error" || true'

begin ladder bugs.cc "$GXX" "g++ -std=c++20 <flags> -O0 -c bugs.cc, four flag sets"
for f in "" "-Wall" "-Wall -Wextra" "-Wall -Wextra -Wconversion -Wshadow"; do
    c "g++ -std=c++20 $f -O0 -c bugs.cc -o $T/bugs.o 2>&1 | $W"
done
end 0 "the compile succeeds every time: warnings do not stop a build without -Werror"

begin clang_ladder bugs.cc "$CLG" "clang++ -std=c++20 -Wall -Wextra -Wconversion -Wshadow -O0 -c bugs.cc"
c "clang++ -std=c++20 -Wall -Wextra -Wconversion -Wshadow -O0 -c bugs.cc -o $T/bugs.o 2>&1 | $W"
end 0

begin opt_level bugs.cc "$GXX" "g++ -std=c++20 -Wall at -O0 and at -O2"
c "g++ -std=c++20 -Wall -O0 -c bugs.cc -o $T/bugs.o 2>&1 | grep -c warning"
c "g++ -std=c++20 -Wall -O2 -c bugs.cc -o $T/bugs.o 2>&1 | grep -c warning"
c "g++ -std=c++20 -Wall -O2 -c bugs.cc -o $T/bugs.o 2>&1 | grep -A3 'maybe-uninitialized'"
end 0

begin enabled "(none: asks the compiler)" "$GXX" "g++ <flags> -Q --help=warnings -x c++ /dev/null, filtered"
SEL="'-W(return-type|sign-compare|parentheses|uninitialized|maybe-uninitialized|conversion|float-conversion|shadow|unused-variable) '"
for f in "" "-Wall" "-Wall -Wextra"; do
    c "g++ $f -Q --help=warnings -x c++ /dev/null | grep -E -- $SEL"
done
c "g++ --help=warnings | grep -E -- '^  -W(all|extra|pedantic|error|conversion|shadow|sign-conversion|return-type|maybe-uninitialized|unused-variable) '"
end 0

begin bugs_run bugs.cc "$GXX" "g++ -std=c++20 -O0 bugs.cc -o bugs && ./bugs (no sanitizer)"
c "g++ -std=c++20 -O0 bugs.cc -o $T/bugs 2>/dev/null && $T/bugs"
end 0 "expected 1 2.5 2 2000 1; the printed values show the bugs; the third value reads an uninitialised variable (undefined behaviour) and may differ on other machines"

begin fixed_strict fixed.cpp "$GXX; $CLG" "both compilers, -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Werror, -O0 and -O2"
for cc in g++ clang++; do for o in -O0 -O2; do
    c "$cc -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Werror $o fixed.cpp -o $T/fixed && $T/fixed"
done; done
end 0

begin invoice_build invoice.cc "$GXX" "g++ -std=c++20 -O2 -Wall -Wextra -Wshadow invoice.cc -o invoice"
c "g++ -std=c++20 -O2 -Wall -Wextra -Wshadow invoice.cc -o $T/invoice"
end 0 "the build succeeds with warnings (no -Werror)"
begin invoice_run invoice.cc "$GXX" "./invoice"
c "$T/invoice"
end 0
begin invoice_fixed invoice_fixed.cc "$GXX" "g++ -std=c++20 -O2 -Wall -Wextra -Wshadow -Werror invoice_fixed.cc -o invoice && ./invoice"
c "g++ -std=c++20 -O2 -Wall -Wextra -Wshadow -Werror invoice_fixed.cc -o $T/invoice_fixed && $T/invoice_fixed"
end 0
rm -rf "$T"
exit $status
