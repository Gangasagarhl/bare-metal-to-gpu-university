#!/usr/bin/env bash
# F2-28 lab: GDB and LLDB sessions on a program with a logic bug, what optimisation does
# to debugging, the debuggers' own help texts, and the forensic crash investigation.
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
T="${LAB}/.tmp"; rm -rf "$T"; mkdir -p "$T"
cp config_mine.in config_new.in streak.cc config.cc config_fixed.cc "$T"/
status=0
begin() {
    OUT="${LAB}/$1.out"; LOG="${LAB}/$1.log"; : > "$OUT"
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
    (cd "$T" && bash -c "$1") >> "$OUT" 2>&1
    LAST=$?
}
end() {
    # placeholders: the lab folder and process ids differ on every machine and run
    sed -i -E "s#${T}/#./#g; s#${LAB}/#./#g; s#(process|Process) [0-9]+#\1 <pid>#g; s#line [0-9]+: +[0-9]+ #line <n>: <pid> #g" "$OUT"
    echo "exit code: ${LAST}" >> "$LOG"
    if [ $# -ge 2 ]; then echo "note:      $2" >> "$LOG"; fi
    if [ "$LAST" != "$1" ]; then echo "result:    UNEXPECTED (expected exit code $1)" >> "$LOG"; status=1; fi
}
GXX="$(g++ --version | head -n 1)"
GDB="$(gdb --version | head -n 1)"
LLDB="$(lldb --version | head -n 1)"

begin streak_run streak.cc "$GXX" "g++ -std=c++20 -g -O0 streak.cc -o streak && ./streak"
c "g++ -std=c++20 -Wall -Wextra -g -O0 streak.cc -o streak && ./streak"
end 0 "the program runs without error and prints a wrong answer (expected 4)"

begin gdb_session "session.gdb, streak.cc" "$GDB; $GXX" "gdb -q -batch -x session.gdb ./streak"
c "gdb -q -batch -x $LAB/session.gdb ./streak"
end 0

begin lldb_session "session.lldb, streak.cc" "$LLDB; $GXX" "lldb -b -s session.lldb ./streak"
c "lldb -b -s $LAB/session.lldb ./streak"
end 0

begin optimised "streak.cc" "$GDB; $GXX" "g++ -g -O2, then gdb: breakpoints on lines 21 and 13, info locals, continue"
c "g++ -std=c++20 -g -O2 streak.cc -o streak_O2"
c "gdb -q -batch -ex 'break streak.cc:21' -ex 'break streak.cc:13' -ex run -ex 'info locals' -ex 'delete 2' -ex continue ./streak_O2"
end 0 "what a debugger can show in an optimised build: the line-21 breakpoint is never reported as hit"

begin debugger_help "(none: built-in help)" "$GDB; $LLDB" "gdb -batch -ex 'help <command>'; lldb -b -o 'help <command>'"
for h in break watch backtrace frame "info locals" next step finish print continue; do
    c "gdb -q -batch -ex 'help $h' | head -n 3"
done
for h in "breakpoint set" "watchpoint set variable" "frame variable" "thread backtrace"; do
    c "lldb -b -o 'help $h' | sed -n '2,3p'"
done
end 0

begin config_mine "config.cc, config_mine.in" "$GXX" "g++ -std=c++20 -g -O0 config.cc -o config && ./config < config_mine.in"
c "g++ -std=c++20 -Wall -Wextra -g -O0 config.cc -o config && ./config < config_mine.in"
end 0
begin config_new "config.cc, config_new.in" "$GXX" "./config < config_new.in"
c "cat config_new.in"
c "./config < config_new.in"
end 139 "exit status 139 = 128 + 11, the program was stopped by signal 11 (SIGSEGV)"
begin crash_gdb "crash.gdb, config.cc, config_new.in" "$GDB; $GXX" "gdb -q -batch -x crash.gdb ./config"
c "gdb -q -batch -x $LAB/crash.gdb ./config"
end 0
begin config_fixed "config_fixed.cc" "$GXX" "g++ ... config_fixed.cc -o config_fixed; run on both files"
c "g++ -std=c++20 -Wall -Wextra -Werror -g -fsanitize=address,undefined config_fixed.cc -o config_fixed"
c "./config_fixed < config_mine.in"
c "./config_fixed < config_new.in"
end 2 "the fixed program reports the missing key and exits with status 2 on the new machine's file"
rm -rf "$T"
exit $status
