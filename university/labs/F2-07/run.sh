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

# 1. The converter, built the long way: compile each source file, then link.
begin converter_steps ok converter "converter/units.h converter/units.cpp converter/main.cpp" "g++ -c each file, g++ main.o units.o -o converter, ./converter < sample.in"
c "g++ $FLAGS -c units.cpp -o units.o"
c "g++ $FLAGS -c main.cpp -o main.o"
c "g++ $FLAGS main.o units.o -o converter"
c "./converter < sample.in"
finish

# 2. What each object file defines (T) and needs (U), without the standard library's names.
begin converter_nm ok converter "converter/units.cpp converter/main.cpp" "g++ -c, then nm -C (std:: lines removed with grep)"
c "g++ -std=c++20 -c units.cpp -o units.o"
c "g++ -std=c++20 -c main.cpp -o main.o"
c "nm -C units.o | grep ' T '"
c "nm -C main.o | grep -E ' [TU] ' | grep -v 'std::'"
finish

# 3. Forgetting a source file in the build command.
begin forgot_file fail converter "converter/main.cpp" "g++ $FLAGS main.cpp -o converter"
c "g++ $FLAGS main.cpp -o converter"
finish

# 4. A header without an include guard, reached twice.
begin no_guard fail no_guard "no_guard/recipe.h no_guard/shopping.h no_guard/main.cpp" "g++ $FLAGS main.cpp -o recipes"
c "g++ $FLAGS main.cpp -o recipes"
finish

# 5. A function body in a header included by two source files.
begin header_body fail header_body "header_body/banner.h header_body/report.cpp header_body/main.cpp" "g++ $FLAGS main.cpp report.cpp -o kitchen"
c "g++ $FLAGS main.cpp report.cpp -o kitchen"
finish

# 6. The same, with the header's function marked inline (a copy of the folder in inline/,
#    where sed adds the word inline): builds and runs.
begin header_inline ok header_body "header_body/banner.h with inline added, header_body/report.cpp, header_body/main.cpp" "copy to inline/, add inline with sed, g++ $FLAGS inline/main.cpp inline/report.cpp -o kitchen"
c "mkdir -p inline && cp main.cpp report.cpp inline/ && sed 's/^void print_banner/inline void print_banner/' banner.h > inline/banner.h"
c "grep -n 'print_banner()$' inline/banner.h"
c "g++ $FLAGS inline/main.cpp inline/report.cpp -o kitchen"
c "./kitchen"
rm -rf inline
finish

# Forensic evidence: Ayumi's build log.
begin forensic_build fail forensic "forensic/units.h forensic/units.cpp forensic/main.cpp" "g++ $FLAGS main.cpp units.cpp -o converter"
c "g++ $FLAGS main.cpp units.cpp -o converter"
finish

# Forensic evidence: the symbol tables of the two object files.
begin forensic_nm ok forensic "forensic/units.cpp forensic/main.cpp" "g++ -c, nm -C (std:: lines removed), nm without -C"
c "g++ -std=c++20 -c units.cpp -o units.o"
c "g++ -std=c++20 -c main.cpp -o main.o"
c "nm -C units.o | grep -E ' [TU] ' | grep -v 'std::'"
c "nm -C main.o | grep -E ' [TU] ' | grep -v 'std::'"
c "nm units.o | grep -E 'celsius|kilogram'"
c "nm main.o | grep -E 'celsius|kilogram'"
c "echo _Z21celsius_to_fahrenheitf _Z21celsius_to_fahrenheitd | c++filt"
finish
exit $status
