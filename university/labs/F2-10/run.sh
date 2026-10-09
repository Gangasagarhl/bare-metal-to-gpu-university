#!/usr/bin/env bash
# Extra step for F2-10: which constructor and destructor calls did the compiler put into main()?
# Builds lifetime.cpp without sanitizers (so the machine code is easier to read), disassembles
# main with objdump, and keeps only the calls to Tracer, Station and the unwinder.
# Writes main_calls.out / main_calls.log in the format of run_lab.sh.
set -u
cd "$(dirname "$0")"
name=main_calls
STR='std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >'
{
    echo "listing:   lifetime.cpp (machine code of main, calls only)"
    echo "toolchain: $(g++ --version | head -n 1); $(objdump --version | head -n 1)"
    echo "command:   g++ -std=c++20 -O0 lifetime.cpp -o lifetime_plain && objdump -d -C --no-show-raw-insn lifetime_plain | (main only) | grep call | grep -E 'Tracer|Station|_Unwind_Resume' (the long std::string type name is shortened to std::string)"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > "${name}.log"
if g++ -std=c++20 -O0 lifetime.cpp -o .lifetime_plain > "${name}.out" 2>&1; then
    objdump -d -C --no-show-raw-insn .lifetime_plain | awk '/<main>:/,/^$/' | grep -E 'call' \
        | grep -E 'Tracer|Station|_Unwind_Resume' \
        | sed -E 's/^ *[0-9a-f]+:\s*call\s+[0-9a-f]+\s*/call /' | sed "s/$STR/std::string/g" > "${name}.out"
    echo "exit code: 0" >> "${name}.log"
    rc=0
else
    echo "exit code: 1" >> "${name}.log"
    rc=1
fi
rm -f .lifetime_plain
exit $rc
