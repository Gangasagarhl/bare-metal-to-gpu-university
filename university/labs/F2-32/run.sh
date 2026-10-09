#!/usr/bin/env bash
# F2-32 lab: read compiler output of three kinds: error messages (g++ and clang++ on the
# same file), assembly (a local "compiler explorer": several compilers and levels side by
# side) and optimisation reports. The two failing listings are compiled by run_lab.sh as
# .expect-fail files; this script adds the comparisons.
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
T="${LAB}/.tmp"; rm -rf "$T"; mkdir -p "$T"
cp sort_list.cpp orders.cpp explore.cc saxpy.cc "$T"/
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
    (cd "$T" && bash -c "$1") >> "$OUT" 2>&1
    LAST=$?
}
end() {
    sed -i -E "s#${T}/#./#g; s#${LAB}/#./#g" "$OUT"
    echo "exit code: ${LAST}" >> "$LOG"
    if [ $# -ge 2 ]; then echo "note:      $2" >> "$LOG"; fi
    if [ "$LAST" != "$1" ]; then echo "result:    UNEXPECTED (expected exit code $1)" >> "$LOG"; status=1; fi
}
GXX="$(g++ --version | head -n 1)"
CLG="$(clang++ --version | head -n 1)"

begin sort_triage sort_list.cpp "$GXX" "g++ ... sort_list.cpp; count the lines; keep the error and 'required from here' lines"
c "g++ -std=c++20 -Wall -Wextra -c sort_list.cpp -o sort_list.o 2>&1 | wc -l"
c "g++ -std=c++20 -Wall -Wextra -c sort_list.cpp -o sort_list.o 2>&1 | grep -E 'required from here|error:' | cut -c1-150"
end 0 "the grep succeeded; the compile itself failed, as intended"
begin sort_clang sort_list.cpp "$CLG" "clang++ -std=c++20 -Wall -Wextra -c sort_list.cpp"
c "clang++ -std=c++20 -Wall -Wextra -c sort_list.cpp -o sort_list.o"
end 1 "the compile fails, as intended"

FILT="grep -v -E '^[[:space:]]*(\\.(file|text|globl|type|size|ident|section|p2align|align|cfi|LFB|LFE)|#|\\.LF)' | grep -v -E '^\\.(LF[BE][0-9]+|Lfunc_end[0-9]+):|^[[:space:]]*\\.addrsig' | c++filt"
begin explore explore.cc "$GXX; $CLG" "g++/clang++ -std=c++20 -S -O0|-O2 -fno-asynchronous-unwind-tables -fcf-protection=none, directives filtered, c++filt"
for cmd in "g++ -O0" "g++ -O2" "clang++ -O2"; do
    c "$cmd -std=c++20 -S -o - -fno-asynchronous-unwind-tables -fcf-protection=none explore.cc | $FILT"
done
end 0
begin explore_intel explore.cc "$GXX" "g++ -O2 -masm=intel -S"
c "g++ -O2 -std=c++20 -S -o - -masm=intel -fno-asynchronous-unwind-tables -fcf-protection=none explore.cc | $FILT | sed -n '/divide_by_8_signed/,/ret/p'"
end 0
begin names explore.cc "$GXX; $(nm --version | head -n 1)" "g++ -c explore.cc; nm; nm -C; c++filt"
c "g++ -std=c++20 -O2 -c explore.cc -o explore.o && nm explore.o"
c "nm -C explore.o"
c "echo _Z6squarei | c++filt"
end 0
begin remarks_gcc saxpy.cc "$GXX" "g++ -O3 -fopt-info-vec-optimized / -fopt-info-vec-missed"
c "g++ -std=c++20 -O3 -c saxpy.cc -o saxpy.o -fopt-info-vec-optimized"
c "g++ -std=c++20 -O3 -c saxpy.cc -o saxpy.o -fopt-info-vec-missed 2>&1 | head -n 3"
end 0
begin remarks_clang saxpy.cc "$CLG" "clang++ -O2 -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize"
c "clang++ -std=c++20 -O2 -c saxpy.cc -o saxpy.o -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize"
end 0
begin orders_clang orders.cpp "$CLG" "clang++ -std=c++20 -c orders.cpp, error lines only"
c "clang++ -std=c++20 -c orders.cpp -o orders.o 2>&1 | grep -E 'error|requested here|deleted here' | sed -E 's#/usr/bin/../lib/gcc/x86_64-linux-gnu/13/../../../../include/c[+][+]/13/#<libstdc++>/#' | cut -c1-170"
end 0 "for the answer key: the same error from the second compiler"
rm -rf "$T"
exit $status
