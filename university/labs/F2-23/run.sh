#!/usr/bin/env bash
# Builds the forensic program moving_crash.cc three ways and records each run (F2-23 forensic lab).
set -u
ver="$(g++ --version | head -n 1)"
here="$(pwd)"
step() {   # step <name> <description> <compile flags...>
    local name="$1" what="$2"; shift 2
    local cmd="g++ $* moving_crash.cc -o $name"
    {
        echo "listing:   moving_crash.cc ($what)"
        echo "toolchain: $ver"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$name.log"
    if ! g++ "$@" moving_crash.cc -o "./.bin_$name" > "${name}_build.out" 2>&1; then
        echo "result:    BUILD FAILED" >> "$name.log"; cat "${name}_build.out" >> "$name.log"; return 1
    fi
    sed -i "s#$here/##g" "${name}_build.out"
    {
        echo "listing:   moving_crash.cc (compiler messages of the $what build)"
        echo "toolchain: $ver"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: 0 (the compiler's own exit code: the build succeeded)"
    } > "${name}_build.log"
    local rc
    rc=$( { timeout 10 "./.bin_$name" < /dev/null > "$name.out" 2>&1; echo $?; } 2>/dev/null )
    sed -i "s#$here/##g; s#\./\.bin_#.bin_#g" "$name.out"
    if [ "$rc" -gt 128 ] && [ "$rc" != 124 ]; then
        echo "exit code: $rc (the program was killed by signal $((rc - 128)); 11 is SIGSEGV)" >> "$name.log"
    else
        echo "exit code: $rc" >> "$name.log"
    fi
    rm -f "./.bin_$name"
    if [ ! -s "${name}_build.out" ]; then rm -f "${name}_build.out" "${name}_build.log"; fi
}
step moving_release "release build: -O2, warnings on but not errors" -std=c++20 -Wall -Wextra -Wpedantic -O2
step moving_debug "debug-log build: the same plus -DKITCHEN_DEBUG" -std=c++20 -Wall -Wextra -Wpedantic -O2 -DKITCHEN_DEBUG
step moving_asan "AddressSanitizer build with the course flags" -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined
# a trimmed copy of view_dangle.out for the chapter page (AH-25: trims are marked)
total=$(wc -l < view_dangle.out)
awk '/^label|ERROR|READ of|in main |is located|^freed by|^previously|SUMMARY/ { if (s > 0) print "[... " s " line(s) trimmed ...]"; s = 0; print; next } { s++ } END { if (s > 0) print "[... " s " line(s) trimmed ...]" }' view_dangle.out > view_dangle_trimmed.out
{
    echo "listing:   view_dangle.cpp (output of the run_lab.sh run, trimmed)"
    echo "toolchain: $ver"
    echo "command:   awk filter keeping the report's headline, the frames in main and the summary"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    echo "exit code: 0 (of the trimming step; view_dangle.out had $total lines; the program's exit code is in view_dangle.log)"
} > view_dangle_trimmed.log
exit 0
