#!/usr/bin/env bash
# F2-55 steps: try perf, profile hotspot.cc with gprof (sampling) and callgrind
# (instrumentation), then the forensic pair orders_v1 / orders_v2.
set -u -o pipefail
cd "$(dirname "$0")"
TC="$(g++ --version | head -n 1)"
VG="$(valgrind --version)"
GP="$(gprof --version | head -n 1)"
MACH="$(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs); CPU model: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')"
NOTE="note:      measured on the build container (a shared cloud virtual machine, other jobs may run at the same time), one session; not a specification (AH-23)"
rec() {
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $MACH"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
}
status=0
W="-std=c++20 -Wall -Wextra -Wpedantic -Werror"
C="sed -E -f compact.sed"
# 1. Does the perf tool work here? Record whatever happens (expected: it does not).
perf stat -e task-clock -- true > perf_try.out 2>&1; rc=$?
rec perf_try "(none)" "perf wrapper at $(command -v perf); kernel $(uname -r)" "perf stat -e task-clock -- true" $rc \
    "result:    perf could not run in this build container (see output); untested in this build"
# 2. gprof: the program samples itself (-pg), then gprof prints the flat profile.
g++ $W -O2 -pg hotspot.cc -o .bin_hs_pg || exit 1
./.bin_hs_pg > .hs.txt 2>&1; rc=$?; [ $rc = 0 ] || status=1
{ cat .hs.txt; echo "--- gprof -b -p (flat profile, first 12 lines, names shortened by compact.sed)";
  gprof -b -p ./.bin_hs_pg gmon.out 2>&1 | head -n 12 | $C | cut -c1-150; } > hotspot_gprof.out
rec hotspot_gprof hotspot.cc "$GP; $TC" "g++ $W -O2 -pg hotspot.cc -o hotspot && ./hotspot && gprof -b -p hotspot gmon.out" $rc
# 3. callgrind: every instruction counted; self (exclusive) and inclusive views.
g++ $W -O2 hotspot.cc -o .bin_hs || exit 1
valgrind --tool=callgrind --callgrind-out-file=.cg.hs ./.bin_hs > .hs.txt 2> .hs.err; rc=$?; [ $rc = 0 ] || status=1
{ cat .hs.txt; echo "--- callgrind_annotate (self cost per function, top 10)";
  callgrind_annotate .cg.hs 2>&1 | grep -E '^ *[0-9,]+ \(' | grep -v 'PROGRAM TOTALS' | head -n 10 | $C | cut -c1-150
  echo "--- callgrind_annotate --inclusive=yes (top 12)";
  callgrind_annotate --inclusive=yes .cg.hs 2>&1 | grep -E '^ *[0-9,]+ \(' | grep -v 'PROGRAM TOTALS' | head -n 12 | $C | cut -c1-150; } > hotspot_cg.out
rec hotspot_cg hotspot.cc "$VG; $TC" "g++ $W -O2 hotspot.cc -o hotspot && valgrind --tool=callgrind ./hotspot && callgrind_annotate [--inclusive=yes]" $rc \
    "note:      instruction counts (Ir) from callgrind's instrumentation; they are not times"
# 4. Forensic: the two releases, timed and profiled, plus the diff between them.
diff -u orders_v1.cc orders_v2.cc | sed -E '1,2s/\t.*$//' > orders_diff.out; rc=$?
rec orders_diff "orders_v1.cc orders_v2.cc" "$(diff --version | head -n 1)" "diff -u orders_v1.cc orders_v2.cc" $rc \
    "note:      exit code 1 from diff means 'files differ' (the expected result)"
for v in 1 2; do
    g++ $W -O2 orders_v$v.cc -o .bin_o$v || exit 1
    timeout 300 ./.bin_o$v > orders_v${v}_time.out 2>&1; rc=$?; [ $rc = 0 ] || status=1
    rec orders_v${v}_time orders_v$v.cc "$TC" "g++ $W -O2 orders_v$v.cc -o orders_v$v && ./orders_v$v" $rc "$NOTE"
    valgrind --tool=callgrind --toggle-collect='summarise*' --callgrind-out-file=.cg.o$v ./.bin_o$v 1 > .o.txt 2> .o.err; rc=$?; [ $rc = 0 ] || status=1
    { cat .o.txt; echo "--- callgrind: instructions counted inside summarise (and everything it calls)"; grep -E 'Collected' .o.err | $C
      echo "--- callgrind_annotate (self cost per function, top 10)";
      callgrind_annotate .cg.o$v 2>&1 | grep -E '^ *[0-9,]+ \(' | grep -v 'PROGRAM TOTALS' | head -n 10 | $C | cut -c1-150
      echo "--- callgrind_annotate --inclusive=yes (top 12)";
      callgrind_annotate --inclusive=yes .cg.o$v 2>&1 | grep -E '^ *[0-9,]+ \(' | grep -v 'PROGRAM TOTALS' | head -n 12 | $C | cut -c1-150; } > orders_v${v}_cg.out
    rec orders_v${v}_cg orders_v$v.cc "$VG; $TC" "g++ $W -O2 orders_v$v.cc -o orders_v$v && valgrind --tool=callgrind --toggle-collect='summarise*' ./orders_v$v 1 && callgrind_annotate [--inclusive=yes]" $rc \
        "note:      instruction counts (Ir) from callgrind's instrumentation, collected only inside summarise(); one timed run (argument 1)"
done
rm -f .bin_hs_pg .bin_hs .bin_o1 .bin_o2 gmon.out .hs.txt .hs.err .cg.hs .cg.o1 .cg.o2 .o.txt .o.err
exit $status
