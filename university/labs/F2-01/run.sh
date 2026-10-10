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

# Step 1: the four stages, one at a time (inspection flags: no -g, no sanitizers, so that
# the files stay small and nm shows only what the program itself uses).
begin stages ok kitchen "kitchen/main.cpp kitchen/prices.cpp" "g++ -E, -S, -c, nm, link, run (see stages.out)"
c "g++ -std=c++20 -E main.cpp -o main.ii"
c "wc -l main.cpp main.ii"
c "grep -n 'dish_price' main.ii"
c "g++ -std=c++20 -S prices.cpp -o prices.s"
c "sed -n '/^_Z10dish_pricei:/,/^.LFE0:/p' prices.s | grep -v cfi_"
c "g++ -std=c++20 -Wall -Wextra -c prices.cpp -o prices.o"
c "g++ -std=c++20 -Wall -Wextra -c main.cpp -o main.o"
c "file main.o prices.o"
c "nm prices.o"
c "nm -C prices.o"
c "nm -C main.o"
c "g++ main.o prices.o -o kitchen"
c "./kitchen"
finish

# Step 2: link main.o alone: the definition of dish_price is missing.
begin link_missing fail kitchen "kitchen/main.cpp" "g++ -c main.cpp, then g++ main.o -o kitchen (prices.o left out)"
c "g++ -std=c++20 -Wall -Wextra -c main.cpp -o main.o"
c "g++ main.o -o kitchen"
finish

# Step 3: the course build: both files in one command, all warnings as errors, sanitizers on.
begin full_build ok kitchen "kitchen/main.cpp kitchen/prices.cpp" "g++ $FLAGS main.cpp prices.cpp -o kitchen && ./kitchen"
c "g++ $FLAGS main.cpp prices.cpp -o kitchen"
c "./kitchen"
finish

# Forensic evidence: Sam builds the price list on its own, as if it were a program.
begin forensic_no_main fail kitchen "kitchen/prices.cpp" "g++ $FLAGS prices.cpp -o prices"
c "g++ $FLAGS prices.cpp -o prices"
finish

# Forensic follow-up: what the object file contains (the method the hints suggest).
begin forensic_nm ok kitchen "kitchen/prices.cpp kitchen/main.cpp" "g++ -c, then nm on each object file"
c "g++ -std=c++20 -c prices.cpp -o prices.o"
c "nm -C prices.o"
c "nm -C prices.o | grep -c ' T main' || true"
c "g++ -std=c++20 -c main.cpp -o main.o"
c "nm -C main.o | grep -E ' (T|U) (main|dish_price)'"
finish
exit $status
