#!/usr/bin/env bash
# F11-09 lab: sanitizers and fuzzing (curriculum P2: "a fuzzing run finds nothing"
# on the fixed parser). It builds the university's tiny coverage-guided fuzzer
# against two targets, each in a buggy and a fixed build:
#   - elf_target.cc : a cut-down ELF section reader (the P2 reader's name path)
#   - mav_target.cc : a MAVLink-v1-style frame scanner (untrusted radio input)
# Steps, per target:
#   *_fuzz        : fuzz the BUGGY build -> finds a crash, saves the input
#   *_minimise    : shrink the crashing input while it still crashes
#   *_report      : replay the minimised input -> the AddressSanitizer report
#   *_fixed_fuzz  : fuzz the FIXED build for a while -> no crash
# run_lab.sh builds and runs elf_tests.cpp and mav_tests.cpp (the fixed readers'
# unit tests) before this script.
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
status=0
GCCV="$(g++ --version | head -n 1)"
SAN="-std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined"
COV="-fsanitize-coverage=trace-pc"
export ASAN_OPTIONS=abort_on_error=0:exitcode=99:detect_leaks=0
export UBSAN_OPTIONS=halt_on_error=1:abort_on_error=0:exitcode=99
W="${LAB}/.work"; rm -rf "$W"; mkdir -p "$W"

rec() {   # rec <name> <listing> <command> <exit text> [extra lines...]
    local name="$1" listing="$2" cmd="$3" code="$4"; shift 4
    {
        echo "listing:   $listing"
        echo "toolchain: $GCCV"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $code"
        for l in "$@"; do echo "$l"; done
    } > "${LAB}/${name}.log"
}

# Seeds: a small well-formed ELF and a valid MAVLink-style frame.
g++ -std=c++20 -O1 -Wall -Wextra -Werror "${LAB}/mkelf.cc"  -o "$W/mkelf"  && "$W/mkelf"  "$W/seed_elf.bin"
g++ -std=c++20 -O1 -Wall -Wextra -Werror "${LAB}/mkseed.cc" -o "$W/mkseed" && "$W/mkseed" "$W/seed_mav.bin"

# --- target builds -------------------------------------------------------------
# fuzzer + coverage runtime: NO coverage instrumentation.
g++ $SAN -c "${LAB}/fuzz.cc" -o "$W/fuzz.o"       || status=1
g++ $SAN -c "${LAB}/cov.cc"  -o "$W/cov.o"        || status=1
# targets: WITH coverage instrumentation, buggy and fixed.
g++ $SAN $COV -c "${LAB}/elf_target.cc"        -o "$W/elf_bug.o"   || status=1
g++ $SAN $COV -c "${LAB}/elf_target.cc" -DFIXED -o "$W/elf_fix.o"  || status=1
g++ $SAN $COV -c "${LAB}/mav_target.cc"        -o "$W/mav_bug.o"   || status=1
g++ $SAN $COV -c "${LAB}/mav_target.cc" -DFIXED -o "$W/mav_fix.o"  || status=1
g++ $SAN "$W/fuzz.o" "$W/cov.o" "$W/elf_bug.o" -o "$W/fuzz_elf_bug" || status=1
g++ $SAN "$W/fuzz.o" "$W/cov.o" "$W/elf_fix.o" -o "$W/fuzz_elf_fix" || status=1
g++ $SAN "$W/fuzz.o" "$W/cov.o" "$W/mav_bug.o" -o "$W/fuzz_mav_bug" || status=1
g++ $SAN "$W/fuzz.o" "$W/cov.o" "$W/mav_fix.o" -o "$W/fuzz_mav_fix" || status=1
# minimiser + repro against the buggy targets.
g++ $SAN "${LAB}/minimise.cc" "$W/cov.o" "$W/elf_bug.o" -o "$W/min_elf" || status=1
g++ $SAN "${LAB}/repro.cc" "$W/cov.o" "$W/elf_bug.o" -o "$W/repro_elf" || status=1
g++ $SAN "${LAB}/minimise.cc" "$W/cov.o" "$W/mav_bug.o" -o "$W/min_mav" || status=1
g++ $SAN "${LAB}/repro.cc" "$W/cov.o" "$W/mav_bug.o" -o "$W/repro_mav" || status=1

run_capture() {   # run_capture <name> <listing> <command...>
    local name="$1" listing="$2"; shift 2
    ( cd "$W" && "$@" ) > "${LAB}/${name}.out" 2>&1
    local rc=$?
    sed -i "s#${W}/##g; s#${LAB}/##g" "${LAB}/${name}.out"
    rec "$name" "$listing" "$*" "$rc"
    return $rc
}

mkdir -p "$W/elf_crashes" "$W/mav_crashes"

# === ELF target ================================================================
run_capture elf_fuzz "fuzz.cc cov.cc elf_target.cc (buggy)" \
    ./fuzz_elf_bug 45 elf_crashes seed_elf.bin
if ! ls "$W"/elf_crashes/crash-*.bin >/dev/null 2>&1; then
    echo "note: ELF fuzzer found no crash in 45 s this run; retrying longer" >> "${LAB}/elf_fuzz.out"
    ( cd "$W" && ./fuzz_elf_bug 90 elf_crashes seed_elf.bin ) >> "${LAB}/elf_fuzz.out" 2>&1
    sed -i "s#${W}/##g; s#${LAB}/##g" "${LAB}/elf_fuzz.out"
fi
ELFCRASH="$(ls "$W"/elf_crashes/crash-*.bin 2>/dev/null | head -n 1)"
if [ -n "$ELFCRASH" ]; then
    run_capture elf_minimise "minimise.cc elf_target.cc (buggy)" \
        ./min_elf "$ELFCRASH" elf_crashes/min.bin
    run_capture elf_report "repro.cc elf_target.cc (buggy)" \
        ./repro_elf elf_crashes/min.bin || true    # ASan exits 99: that IS the evidence
    cp "$W/elf_crashes/min.bin" "${LAB}/elf_crash_min.bin" 2>/dev/null || true
else
    echo "ELF fuzzer found no crash; see elf_fuzz.out" > "${LAB}/elf_minimise.out"; status=1
fi
run_capture elf_fixed_fuzz "fuzz.cc cov.cc elf_target.cc (FIXED)" \
    ./fuzz_elf_fix 30 elf_crashes seed_elf.bin
grep -q "no crash" "${LAB}/elf_fixed_fuzz.out" || { echo "FIXED ELF reader still crashed!" >> "${LAB}/elf_fixed_fuzz.log"; status=1; }

# === MAVLink target ============================================================
run_capture mav_fuzz "fuzz.cc cov.cc mav_target.cc (buggy)" \
    ./fuzz_mav_bug 30 mav_crashes seed_mav.bin
if ! ls "$W"/mav_crashes/crash-*.bin >/dev/null 2>&1; then
    ( cd "$W" && ./fuzz_mav_bug 60 mav_crashes seed_mav.bin ) >> "${LAB}/mav_fuzz.out" 2>&1
    sed -i "s#${W}/##g; s#${LAB}/##g" "${LAB}/mav_fuzz.out"
fi
MAVCRASH="$(ls "$W"/mav_crashes/crash-*.bin 2>/dev/null | head -n 1)"
if [ -n "$MAVCRASH" ]; then
    run_capture mav_minimise "minimise.cc mav_target.cc (buggy)" \
        ./min_mav "$MAVCRASH" mav_crashes/min.bin
    run_capture mav_report "repro.cc mav_target.cc (buggy)" \
        ./repro_mav mav_crashes/min.bin || true
    cp "$W/mav_crashes/min.bin" "${LAB}/mav_crash_min.bin" 2>/dev/null || true
else
    echo "MAVLink fuzzer found no crash; see mav_fuzz.out" > "${LAB}/mav_minimise.out"; status=1
fi
run_capture mav_fixed_fuzz "fuzz.cc cov.cc mav_target.cc (FIXED)" \
    ./fuzz_mav_fix 30 mav_crashes seed_mav.bin
grep -q "no crash" "${LAB}/mav_fixed_fuzz.out" || { echo "FIXED MAVLink scanner still crashed!" >> "${LAB}/mav_fixed_fuzz.log"; status=1; }

rm -rf "$W"
exit $status
