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

# The silent conversion of silent.cpp, caught by asking for one more warning (-Wconversion).
begin wconversion fail . "silent.cpp" "g++ $FLAGS -Wconversion silent.cpp -o silent"
c "g++ $FLAGS -Wconversion silent.cpp -o silent"
finish
exit $status
