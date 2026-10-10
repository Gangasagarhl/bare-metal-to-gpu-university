#!/usr/bin/env bash
# F2-33 lab: measure first (repeated timings, medians), then profile with two kinds of
# profiler (gprof samples, callgrind instruction counts), try perf, change the hot spot,
# and measure again. Forensic: the ticket queue that grows slow. All times are
# measurements on the shared build container, not properties of any program or machine.
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
T="${LAB}/.tmp"; rm -rf "$T"; mkdir -p "$T"
cp *.cc median.py "$T"/
status=0
begin() {
    OUT="${LAB}/$1.out"; LOG="${LAB}/$1.log"; : > "$OUT"
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container, shared, $(nproc) CPUs visible)"
    } > "$LOG"
}
c() {
    echo "\$ $1" >> "$OUT"
    (cd "$T" && bash -c "$1") >> "$OUT" 2>&1
    LAST=$?
}
end() {
    sed -i -E "s#${T}/#./#g; s#${LAB}/#./#g; s#==[0-9]+==#==<pid>==#g; s#\(PID [0-9]+#(PID <pid>#g" "$OUT"
    echo "exit code: ${LAST}" >> "$LOG"
    if [ $# -ge 2 ]; then echo "note:      $2" >> "$LOG"; fi
    if [ "$LAST" != "$1" ]; then echo "result:    UNEXPECTED (expected exit code $1)" >> "$LOG"; status=1; fi
}
GXX="$(g++ --version | head -n 1)"
VG="$(valgrind --version)"
B="g++ -std=c++20 -Wall -Wextra -Werror -O2 -g"
# callgrind, then the inclusive cost of this program's own functions (paths shortened)
CG="callgrind_annotate --inclusive=yes"

begin timing hotspot.cc "$GXX" "$B hotspot.cc -o hotspot; ./hotspot five times; median.py"
c "$B hotspot.cc -o hotspot"
c "for i in 1 2 3 4 5; do ./hotspot; done | tee runs.txt"
c "python3 median.py < runs.txt"
end 0 "times measured on the shared build container; they change from run to run"

begin perf_attempt hotspot.cc "$(perf --version 2>&1 | head -n 1)" "perf stat ./hotspot"
c "perf stat ./hotspot 2>&1 | head -n 6"
c "perf stat ./hotspot > /dev/null 2>&1; echo \"perf exit status: \$?\""
end 0 "perf does not work in this container: the perf front end found no tools for the running kernel"

begin callgrind hotspot.cc "$GXX; $VG" "valgrind --tool=callgrind ./hotspot; callgrind_annotate --inclusive=yes"
c "valgrind --tool=callgrind --callgrind-out-file=cg.out ./hotspot 2>&1 | grep -E 'refs|orders'"
c "$CG cg.out | grep -E 'PROGRAM TOTALS|hotspot.cc:(main|parse_line|sort_by_price|make_lines)' | sed -E 's#  [^ ]*hotspot.cc:#  hotspot.cc:#; s#\(std::.*##; s# \[.*##'"
end 0

begin gprof hotspot.cc "$GXX; $(gprof --version | head -n 1)" "g++ -pg; ./hotspot_pg N; gprof -b -p (flat profile), for N = 200000 and 2000000"
c "$B -pg hotspot.cc -o hotspot_pg"
for n in 200000 2000000; do
    c "./hotspot_pg $n > /dev/null && gprof -b -p hotspot_pg gmon.out | sed -n '1,12p' | cut -c1-110"
done
end 0 "gprof samples the program every 0.01 s; a short run gives few samples"

begin fast hotspot_fast.cc "$GXX; $VG" "$B hotspot_fast.cc -o hotspot_fast; five runs; median.py; callgrind"
c "$B hotspot_fast.cc -o hotspot_fast"
c "for i in 1 2 3 4 5; do ./hotspot_fast; done > runs_fast.txt; head -n 2 runs_fast.txt"
c "python3 median.py < runs_fast.txt"
c "valgrind --tool=callgrind --callgrind-out-file=cg_fast.out ./hotspot_fast 2>&1 | grep -E 'refs'"
c "$CG cg_fast.out | grep -E 'PROGRAM TOTALS|hotspot_fast.cc:(main|parse_line|sort_by_price|make_lines)' | sed -E 's#  [^ ]*hotspot_fast.cc:#  hotspot_fast.cc:#; s#\(std::.*##; s# \[.*##'"
end 0 "times measured on the shared build container; they change from run to run"

# ---- forensic -----------------------------------------------------------------------------
begin queue_scaling queue.cc "$GXX; $VG" "$B queue.cc -o queue; time and callgrind for 10000 and 20000 tickets"
c "$B queue.cc -o queue"
for n in 10000 20000; do
    c "TIMEFORMAT='wall time %3R s'; time ./queue $n"
done
for n in 10000 20000; do
    c "valgrind --tool=callgrind --callgrind-out-file=q$n.out ./queue $n 2>&1 | grep -E 'refs'"
done
end 0 "times measured once each on the shared build container"
begin queue_profile queue.cc "$VG" "callgrind_annotate q20000.out (inclusive, then self cost)"
c "$CG q20000.out | grep -E 'PROGRAM TOTALS|queue.cc:|memmove' | grep -v '=>' | head -n 6 | sed -E 's# \\[.*##; s#  [^ ]*/queue.cc:#  queue.cc:#' | cut -c1-118"
c "callgrind_annotate q20000.out | sed -n '/file:function/,+6p' | sed -E 's# \\[.*##' | cut -c1-118"
end 0
begin queue_fixed queue_fixed.cc "$GXX; $VG" "$B queue_fixed.cc -o queue_fixed; time and callgrind for 10000 and 20000"
c "$B queue_fixed.cc -o queue_fixed"
for n in 10000 20000; do
    c "TIMEFORMAT='wall time %3R s'; time ./queue_fixed $n"
    c "valgrind --tool=callgrind --callgrind-out-file=qf$n.out ./queue_fixed $n 2>&1 | grep -E 'refs'"
done
end 0 "for the answer key"
rm -rf "$T"
exit $status
