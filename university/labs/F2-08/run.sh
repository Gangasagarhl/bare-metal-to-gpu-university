#!/usr/bin/env bash
# Multi-step builds of this lab (several source files, object files, nm, linker errors).
# Called by run_lab.sh after the single-file listings. Each step writes <name>.out (the
# real transcript: every command, then what it printed) and <name>.log (the run record).
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
status=0
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
TOOL="$(g++ --version | head -n 1)"

# begin <name> <expect: ok|fail> <folder> <listing files> <main command>
begin() {
    NAME="$1"; EXPECT="$2"; DIR="$3"; LAST=0; BAD=0
    OUT="${LAB}/${NAME}.out"; LOG="${LAB}/${NAME}.log"
    : > "$OUT"
    {
        echo "listing:   $4"
        echo "toolchain: $TOOL"
        echo "command:   $5"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$LOG"
    cd "${LAB}/${DIR}" || exit 2
}
# c '<shell command>': print it after "$ ", run it, record the exit code
c() {
    echo "\$ $1" >> "$OUT"
    bash -c "$1" >> "$OUT" 2>&1
    LAST=$?
    if [ "$LAST" != 0 ]; then echo "(exit code: $LAST)" >> "$OUT"; BAD=$((BAD + 1)); fi
}
finish() {
    # remove machine-specific text: the lab folder's path, process numbers, and the random
    # names of the compiler's temporary object files (placeholders, guide AH-25)
    sed -i -e "s#${LAB}/##g" -E -e 's/==[0-9]+==/==<pid>==/g' -e 's#/tmp/cc[A-Za-z0-9]+\.o#/tmp/cc<random>.o#g' "$OUT"
    echo "exit code: $LAST" >> "$LOG"
    if [ "$EXPECT" = ok ]; then
        if [ "$BAD" != 0 ]; then echo "result:    STEP FAILED ($BAD command(s) failed)" >> "$LOG"; status=1; fi
    else
        if [ "$LAST" != 0 ] && [ "$BAD" = 1 ]; then
            echo "result:    build failed as expected on the last command (messages saved in ${NAME}.out)" >> "$LOG"
        else
            echo "result:    UNEXPECTED (the last command was expected to fail, and only it)" >> "$LOG"; status=1
        fi
    fi
    rm -f ./*.o ./*.ii ./*.s
    find . -maxdepth 1 -type f -perm -u+x ! -name "*.sh" -delete   # the programs built in this step
    cd "$LAB" || exit 2
}

# 1. The unit tests: test_units.cpp + units.cpp, course flags (assertions on).
begin tests_pass ok converter "converter/units.h converter/units.cpp converter/test_units.cpp" "g++ $FLAGS test_units.cpp units.cpp -o test_units && ./test_units"
c "g++ $FLAGS test_units.cpp units.cpp -o test_units"
c "./test_units && echo '(the test program exited with code 0)'"
finish

# 2. The program itself: main.cpp + units.cpp (the same units.cpp the tests checked).
begin program_run ok converter "converter/units.h converter/units.cpp converter/main.cpp converter/sample.in" "g++ $FLAGS main.cpp units.cpp -o converter && ./converter < sample.in"
c "g++ $FLAGS main.cpp units.cpp -o converter"
c "./converter < sample.in"
finish

# 3. A failing assertion stops the program (abort).
begin assert_fail fail demo "demo/assert_fail.cpp" "g++ $FLAGS assert_fail.cpp -o assert_fail && ./assert_fail"
c "g++ $FLAGS assert_fail.cpp -o assert_fail"
c "./assert_fail"
finish

# 4. The same file built with -DNDEBUG: assert does nothing, the bug passes.
begin assert_ndebug ok demo "demo/assert_fail.cpp" "g++ $FLAGS -DNDEBUG assert_fail.cpp -o assert_fail && ./assert_fail"
c "g++ $FLAGS -DNDEBUG assert_fail.cpp -o assert_fail"
c "./assert_fail"
finish

# 5. A side effect inside assert: present in one build, gone in the other.
begin side_effect ok demo "demo/side_effect.cpp" "built and run twice: without and with -DNDEBUG"
c "g++ $FLAGS side_effect.cpp -o side_effect && ./side_effect"
c "g++ $FLAGS -DNDEBUG side_effect.cpp -o side_effect && ./side_effect"
finish

# Forensic evidence: the team's release build of the tests (optimised, -DNDEBUG) ...
begin forensic_release ok forensic "forensic/units.h forensic/units.cpp forensic/test_units.cpp forensic/main.cpp" "release flags -std=c++20 -O2 -DNDEBUG: build and run the tests, then the converter"
c "g++ -std=c++20 -O2 -DNDEBUG test_units.cpp units.cpp -o test_units"
c "./test_units"
c "g++ -std=c++20 -O2 -DNDEBUG main.cpp units.cpp -o converter"
c "echo '212 F C' | ./converter"
finish

# ... and the same sources built the course way (assertions on).
begin forensic_debug fail forensic "forensic/units.h forensic/units.cpp forensic/test_units.cpp" "g++ $FLAGS test_units.cpp units.cpp -o test_units && ./test_units"
c "g++ $FLAGS test_units.cpp units.cpp -o test_units"
c "./test_units"
finish
exit $status
