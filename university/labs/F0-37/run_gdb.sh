#!/usr/bin/env bash
# Runs the two non-interactive gdb sessions of F0-37 and saves their transcripts.
# Run from this folder:  bash run_gdb.sh
# Each program is built with exactly the course's flags (the same as run_lab.sh),
# debugged with the commands in <name>.gdb, and the temporary program is deleted.
set -u
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
for name in walk steps cooking_time; do
    g++ $FLAGS "$name.cpp" -o "${name}_dbg" || exit 1
    {
        echo "\$ g++ --version | head -n 1"
        g++ --version | head -n 1
        echo "\$ gdb --version | head -n 1"
        gdb --version | head -n 1
        echo "\$ g++ $FLAGS $name.cpp -o ${name}_dbg"
        echo "\$ gdb -q -batch -x $name.gdb ./${name}_dbg"
        gdb -q -batch -x "$name.gdb" "./${name}_dbg" 2>&1 | sed -e 's/(process [0-9]*)/(process <pid>)/'
    } > "${name}_gdb.txt"
    rm -f "${name}_dbg"
done
# gdb's own short help for the commands used in the chapter
{
    echo "\$ gdb --version | head -n 1"
    gdb --version | head -n 1
    for c in break run next step print continue kill; do
        echo "\$ gdb -q -batch -ex \"help $c\""
        gdb -q -batch -ex "help $c"
    done
} > gdb_help.txt 2>&1
