#!/usr/bin/env bash
# BR-09 lab: take the SP202 course project (the F2-26 template, version 0.1.0) and make it
# production-grade (prod/, version 1.0.0). Every trap of the bridge is run for real:
# failure paths, two "works on my machine" cases, undocumented assumptions, copied code
# without a licence; then CI, compatibility, release checks, review rules, a dependency
# inventory and a monitor. Each step writes <name>.out (transcript: "$ command" lines and
# what they printed) and <name>.log (run record). Work folders are deleted at the end.
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
TEMPLATE="$(cd ../F2-26/template && pwd)"   # the earlier project, read-only
WORK="${LAB}/.work"
rm -rf "$WORK"; mkdir -p "$WORK"
status=0
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
CLANG_FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g"   # clang's sanitizer runtime is not installed here (step clang_asan_missing)

begin() {
    NAME="$1"; OUT="${LAB}/$1.out"; LOG="${LAB}/$1.log"; : > "$OUT"
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
    (cd "$HERE" && bash -c "$1") >> "$OUT" 2>&1
    LAST=$?
}
end() {
    sed -i -E "s#${WORK}#<work>#g; s#${LAB}#<lab>#g; s#${TEMPLATE}#<F2-26 template>#g; \
s#line [0-9]+: +[0-9]+ Aborted#line N: <pid> Aborted#g; \
s#\(0x[0-9a-f]+\)#(<address>)#g" "$OUT"
    echo "exit code: ${LAST}" >> "$LOG"
    if [ $# -ge 2 ]; then echo "note:      $2" >> "$LOG"; fi
    if [ "$LAST" != "$1" ]; then echo "result:    UNEXPECTED (expected exit code $1)" >> "$LOG"; status=1; fi
}
GXX="$(g++ --version | head -n 1)"
CLX="$(clang++ --version | head -n 1)"
CMK="$(cmake --version | head -n 1)"
PY="$(python3 --version)"
GIT="$(git --version)"
# run each argument line of a file through a stats_cli binary: "$ stats_cli <args>", output, status
cases() {
    local bin="$1"; shift
    for a in "$@"; do
        echo "echo '\$ stats_cli $a'; $bin $a; echo \"exit status: \$?\""
    done
}

# ---- 0. reading sources: what the tools themselves say ------------------------------------
HERE="$LAB"
begin docs "(CMake and git built-in help)" "$CMK; $GIT" "cmake --help-property/--help-command/--help-variable; git tag -h"
c "cmake --help-property PASS_REGULAR_EXPRESSION | sed -n '1,5p'"
c "cmake --help-property WILL_FAIL | sed -n '1,9p'"
c "cmake --help-command add_test | sed -n '/^..WORKING_DIRECTORY..\$/,/^\$/p'"
c "cmake --help-variable PROJECT_VERSION | sed -n '1,8p'"
c "git tag -h 2>&1 | sed -n '1,4p'; true"
end 0

# ---- 1. the earlier project (version 0.1.0): failure paths and undocumented assumptions ----
cp -r "$TEMPLATE" "$WORK/v01"
HERE="$WORK/v01"
c_quiet() { (cd "$HERE" && bash -c "$1") > /dev/null 2>&1; }
c_quiet "cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build"
begin before_failpaths "F2-26/template (version 0.1.0): app/main.cpp, src/stats.cpp" "$GXX; $CMK" \
    "build the F2-26 template (Debug), then run stats_cli on happy-path and failure-path inputs"
while IFS= read -r cmd; do c "$cmd"; done < <(cases ./build/stats_cli "3 1 2" "" "abc" "1e999" \
    "3abc 1 2" "0x10" "4,4 4.2" "nan 1 2" "1 nan 2" "1 2 nan" "1e308 1e308" "1234567 1234568")
end 0 "the crashes (exit status 134) are the evidence; the last command succeeds"

begin before_coverage "F2-26/template app/main.cpp" "$GXX; $(gcov --version | head -n 1); $CMK" \
    "build with --coverage, run the project's own ctest, gcov app/main.cpp"
rm -rf "$WORK/v01cov"; cp -r "$TEMPLATE" "$WORK/v01cov"; HERE="$WORK/v01cov"
c "cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS=--coverage > /dev/null && cmake --build build > /dev/null && ctest --test-dir build | grep 'tests passed'"
c "cd build/CMakeFiles/stats_cli.dir/app && gcov -o . main.cpp.gcda 2>&1 | grep -A1 \"app/main.cpp'\""
c "cd build/CMakeFiles/stats_cli.dir/app && sed -n '/ 8:int main/,\$p' main.cpp.gcov"
end 0

begin regex_ignores_exit "F2-26/template CMakeLists.txt (test stats.cli)" "$GXX; $CMK" \
    "plant a wrong exit status, then a crash, after the correct output; run the template's CLI test each time"
rm -rf "$WORK/v01exit" "$WORK/v01abort"; cp -r "$TEMPLATE" "$WORK/v01exit"; cp -r "$TEMPLATE" "$WORK/v01abort"
HERE="$WORK/v01exit"
c "grep -n 'stats.cli' CMakeLists.txt"
c "sed -i 's|    return 0;|    return 3;  // planted: wrong exit status after correct output|' app/main.cpp && grep -n 'planted' app/main.cpp"
c "cmake -S . -B build > /dev/null && cmake --build build > /dev/null && ./build/stats_cli 3 1 2; echo \"exit status: \$?\""
c "ctest --test-dir build -R stats.cli | grep -E 'Test #|tests passed'"
HERE="$WORK/v01abort"
c "sed -i 's|    return 0;|    std::abort();  // planted: crash after correct output|' app/main.cpp && sed -i 's|#include <cstdio>|#include <cstdio>\\n#include <cstdlib>|' app/main.cpp && grep -n 'planted' app/main.cpp"
c "cmake -S . -B build > /dev/null && cmake --build build > /dev/null && ctest --test-dir build -R stats.cli 2>&1 | grep -E 'Test #|tests passed'; true"
end 0 "exit status 3 passes the regex test (exit code ignored); the abort is still caught as a system-level failure"

# ---- 2. "works on my machine" -------------------------------------------------------------
HERE="$LAB/traps"
begin wom_order "traps/wom_order.cc" "$GXX; $CLX" "g++ and clang++ with the university flags, run both"
c "g++ $FLAGS wom_order.cc -o $WORK/order_gcc && $WORK/order_gcc; echo \"exit status: \$?\""
c "clang++ $CLANG_FLAGS wom_order.cc -o $WORK/order_clang && $WORK/order_clang; echo \"exit status: \$?\""
end 0 "no warning from either compiler; g++ build FAILS its test, clang++ build PASSES"
begin clang_asan_missing "traps/wom_order.cc" "$CLX" "clang++ with -fsanitize=address,undefined in this container"
c "clang++ $FLAGS wom_order.cc -o $WORK/order_clang_asan 2>&1 | sed -n '1,4p'; exit \${PIPESTATUS[0]}"
end 1 "expected: the build machine lacks clang's sanitizer runtime libraries, an environment difference"
begin wom_order_fixed "traps/wom_order_fixed.cc" "$GXX; $CLX" "the fix, both compilers"
c "g++ $FLAGS wom_order_fixed.cc -o $WORK/fixed_gcc && $WORK/fixed_gcc"
c "clang++ $CLANG_FLAGS wom_order_fixed.cc -o $WORK/fixed_clang && $WORK/fixed_clang"
end 0
mkdir -p "$WORK/pathproj/tests/data" "$WORK/pathproj/build"
echo "12.1 11.8 12.4" > "$WORK/pathproj/tests/data/sample.txt"
g++ $FLAGS traps/wom_path.cc -o "$WORK/pathproj/build/wom_path"
HERE="$WORK/pathproj"
begin wom_path "traps/wom_path.cc" "$GXX" "run the same test binary from the project folder and from build/"
c "pwd; ./build/wom_path; echo \"exit status: \$?\""
c "cd build && pwd && ./wom_path; echo \"exit status: \$?\""
end 0 "the second run fails: same binary, different current directory"

# ---- 3. the production-grade project (version 1.0.0) ----------------------------------------
begin golden_record "prod/tests/golden_v0_1.txt" "$GXX; $CMK" \
    "regenerate the expected outputs from the 0.1.0 binary and compare with the committed file"
HERE="$WORK/v01"
c "while IFS= read -r line; do out=\"\$(./build/stats_cli \$line 2>&1)\"; code=\$?; printf '%s -> %s [exit %d]\\n' \"\$line\" \"\$out\" \"\$code\"; done < $LAB/prod/tests/golden_inputs.txt > $WORK/golden_now.txt; cat $WORK/golden_now.txt"
c "cmp $WORK/golden_now.txt $LAB/prod/tests/golden_v0_1.txt && echo 'golden file = real output of version 0.1.0'"
end 0

rm -rf "$WORK/ci"; cp -r prod "$WORK/ci"; rm -rf "$WORK/ci/build"
HERE="$WORK/ci"
begin ci "prod/ci.sh" "$GXX; $CLX; $CMK; $PY" "./ci.sh (from a fresh copy)"
c "./ci.sh"
end 0

HERE="$WORK/ci"
begin after_failpaths "prod/app/main.cpp, prod/src/parse.cpp, prod/src/stats.cpp" "$GXX; $CMK" \
    "the same inputs as before_failpaths, through version 1.0.0 (gcc-debug-asan build from ci.sh)"
while IFS= read -r cmd; do c "$cmd"; done < <(cases ./build/gcc-debug-asan/stats_cli "3 1 2" "" \
    "abc" "1e999" "3abc 1 2" "0x10" "4,4 4.2" "nan 1 2" "1 nan 2" "1 2 nan" "1e308 1e308" \
    "1234567 1234568" "--version")
end 0

begin ctest_list "prod/CMakeLists.txt" "$CMK" "ctest -N (the list of tests of version 1.0.0)"
c "ctest --test-dir build/gcc-release -N | sed -n '/Test *#/p;/Total Tests/p'"
end 0

begin compat "F2-26/template/tests/test_stats.cpp against prod/ (1.0.0)" "$GXX" \
    "compile version 0.1's unit tests unchanged against the 1.0.0 library and run them"
HERE="$WORK/ci"
c "g++ $FLAGS -Iinclude -Itests $TEMPLATE/tests/test_stats.cpp src/stats.cpp src/parse.cpp -o $WORK/compat && $WORK/compat"
end 0

begin compat_break "prod/include/uni/stats.h, changed by sed (a planted breaking change)" "$GXX" \
    "change median() to return double, then compile version 0.1's tests against it"
rm -rf "$WORK/break"; cp -r prod "$WORK/break"; HERE="$WORK/break"
c "sed -i 's|^std::optional<double> median(std::vector<double> xs);|double median(std::vector<double> xs);  // \"simpler\"|' include/uni/stats.h && grep -n 'median(' include/uni/stats.h"
c "g++ $FLAGS -Iinclude -Itests -c $TEMPLATE/tests/test_stats.cpp -o $WORK/break.o 2>&1 | grep -E 'error' | sed -n '1,4p'; exit \${PIPESTATUS[0]}"
end 1 "expected: the old callers no longer compile, so this change needs a new MAJOR version"

# ---- 4. licences of copied code ---------------------------------------------------------
rm -rf "$WORK/lic"; cp -r prod "$WORK/lic"; rm -rf "$WORK/lic/build"; HERE="$WORK/lic"
begin licence_scan "prod/tools/check_release.py; traps/fast_median.cpp" "$PY" \
    "copy a pasted file into src/, run the release checks; then the same file with a GPL line; then edit the copied harness"
c "cp $LAB/traps/fast_median.cpp src/ && python3 tools/check_release.py ."
c "sed -i '1i // SPDX-License-Identifier: GPL-2.0-only' src/fast_median.cpp && python3 tools/check_release.py ."
c "rm src/fast_median.cpp && echo '// local tweak' >> tests/uni_test.h && python3 tools/check_release.py ."
end 1 "expected: findings, exit status 1"

begin release_check_bad "prod/tools/check_release.py" "$PY" "bump the version in CMakeLists.txt only"
rm -rf "$WORK/rel"; cp -r prod "$WORK/rel"; HERE="$WORK/rel"
c "sed -i 's/VERSION 1.0.0/VERSION 1.0.1/' CMakeLists.txt && python3 tools/check_release.py ."
end 1 "expected: version mismatch finding"

# ---- 5. review rules on real git history ------------------------------------------------
rm -rf "$WORK/repo"; mkdir -p "$WORK/repo" "$WORK/home"; HERE="$WORK/repo"
export HOME="$WORK/home" GIT_CONFIG_NOSYSTEM=1
export GIT_AUTHOR_NAME="Amara Okafor" GIT_AUTHOR_EMAIL="amara@uni.invalid"
export GIT_COMMITTER_NAME="Amara Okafor" GIT_COMMITTER_EMAIL="amara@uni.invalid"
begin review "prod/tools/check_release.py --review" "$GIT; $PY" \
    "history 0.1.0 -> 1.0.0 -> a quick fix; run the review rules on each step"
c "git init -q -b main . && cp -r $TEMPLATE/. . && git add -A && GIT_AUTHOR_DATE=2026-10-01T10:00:00Z GIT_COMMITTER_DATE=2026-10-01T10:00:00Z git commit -q -m 'stats 0.1.0 (SP202 course project)' && git tag v0.1.0"
c "git rm -q -r . && cp -r $LAB/prod/. . && rm -rf build && git add -A && GIT_AUTHOR_DATE=2026-10-08T10:00:00Z GIT_COMMITTER_DATE=2026-10-08T10:00:00Z git commit -q -m 'stats 1.0.0: strict parsing, failure-path tests, CI, release notes' && git tag v1.0.0"
c "sed -i 's|return std::midpoint(xs\\[mid - 1\\], xs\\[mid\\]);|return (xs[mid - 1] + xs[mid]) / 2.0;  // quick fix: faster|' src/stats.cpp && GIT_AUTHOR_DATE=2026-10-09T23:50:00Z GIT_COMMITTER_DATE=2026-10-09T23:50:00Z git commit -q -am 'quick fix' && git log --oneline"
c "cmake -S . -B build -DCMAKE_BUILD_TYPE=Release > /dev/null && cmake --build build > /dev/null && ./build/test_stats huge; echo \"exit status: \$?\""
c "python3 tools/check_release.py . --review v0.1.0 v1.0.0"
c "python3 tools/check_release.py . --review v1.0.0 HEAD"
end 1 "expected: the quick fix has findings (exit status 1)"
unset GIT_AUTHOR_NAME GIT_AUTHOR_EMAIL GIT_COMMITTER_NAME GIT_COMMITTER_EMAIL

# ---- 6. security updates: what does the shipped program depend on? -------------------------
HERE="$WORK/ci"
begin deps "prod (gcc-release build of stats_cli)" "$GXX; $(ldd --version | head -n 1)" \
    "ldd on the release binary; installed package versions of the runtime libraries; toolchain"
c "ldd build/gcc-release/stats_cli | sed -E 's/^[[:space:]]+//'"
c "dpkg-query -W -f='\${Package} \${Version}\\n' libc6 libstdc++6 libgcc-s1 2>&1"
c "g++ --version | head -n 1; clang++ --version | head -n 1; cmake --version | head -n 1"
end 0

# ---- 7. monitoring: a day of calls from a downstream script ------------------------------
g++ $FLAGS monitor.cc -o "$WORK/monitor"
HERE="$WORK"
begin monitor_v0 "monitor.cc; fleet_inputs.txt; F2-26 template stats_cli (0.1.0)" "$GXX" \
    "run 0.1.0 on each input line, log 'exit=<status> <first stderr line>', feed the log to monitor 5"
c "while IFS= read -r line; do v01/build/stats_cli \$line > /dev/null 2> err.txt; code=\$?; echo \"exit=\$code \$(head -n 1 err.txt)\"; done < $LAB/fleet_inputs.txt > fleet_v0.log; grep -v '^exit=0 \$' fleet_v0.log"
c "./monitor 5 < fleet_v0.log"
end 0
begin monitor_v1 "monitor.cc; fleet_inputs.txt; prod stats_cli (1.0.0)" "$GXX" "the same day through version 1.0.0"
c "while IFS= read -r line; do ci/build/gcc-release/stats_cli \$line > /dev/null 2> err.txt; code=\$?; echo \"exit=\$code \$(head -n 1 err.txt)\"; done < $LAB/fleet_inputs.txt > fleet_v1.log; grep -v '^exit=0 \$' fleet_v1.log"
c "./monitor 5 < fleet_v1.log"
end 0

rm -rf "$WORK"
exit $status
