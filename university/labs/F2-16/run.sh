#!/usr/bin/env bash
# Extra steps for F2-16, logged in the format of run_lab.sh:
#   expected23     : expected23.cc built with -std=c++23 (std::expected), then run
#   no_exceptions  : no_exceptions.cc built with -fno-exceptions (as kernels do); must fail
#   fry_calls      : unwinding.cpp built without sanitizers; the calls inside fry(), from objdump
set -u
cd "$(dirname "$0")"
FLAGS="-Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
ver="$(g++ --version | head -n 1)"
header() {
    echo "listing:   $1"
    echo "toolchain: $ver"
    echo "command:   $2"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
}
status=0

cmd="g++ -std=c++23 $FLAGS expected23.cc -o expected23"
header expected23.cc "$cmd" > expected23.log
if g++ -std=c++23 $FLAGS -x c++ expected23.cc -o .bin_expected23 > expected23.build.txt 2>&1; then
    ./.bin_expected23 > expected23.out 2>&1; rc=$?
    echo "exit code: $rc" >> expected23.log
    [ $rc -eq 0 ] || status=1
else
    echo "result:    BUILD FAILED" >> expected23.log; cat expected23.build.txt >> expected23.log; status=1
fi
rm -f expected23.build.txt .bin_expected23

cmd="g++ -std=c++20 -fno-exceptions -Wall -Wextra -Wpedantic -Werror -c no_exceptions.cc"
header no_exceptions.cc "$cmd" > no_exceptions.log
if g++ -std=c++20 -fno-exceptions -Wall -Wextra -Wpedantic -Werror -x c++ -c no_exceptions.cc \
        -o .no_exceptions.o > no_exceptions.out 2>&1; then
    echo "result:    UNEXPECTED SUCCESS (compile was expected to fail)" >> no_exceptions.log; status=1
else
    echo "result:    compile failed as expected (messages saved in no_exceptions.out)" >> no_exceptions.log
fi
rm -f .no_exceptions.o

STR='std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >'
{
    header "unwinding.cpp (machine code of fry, calls only)" \
        "g++ -std=c++20 -O0 unwinding.cpp -o unwinding_plain && objdump -d -C --no-show-raw-insn unwinding_plain | (fry only) | grep call (long std::string type shortened)"
} > fry_calls.log
if g++ -std=c++20 -O0 -x c++ unwinding.cpp -o .unwinding_plain > fry_calls.out 2>&1; then
    objdump -d -C --no-show-raw-insn .unwinding_plain | awk '/<fry\(int\)>:/,/^$/' \
        | grep -E 'call' | sed -E 's/^ *[0-9a-f]+:\s*call\s+[0-9a-f]+\s*/call /' \
        | sed "s/$STR/std::string/g" > fry_calls.out
    echo "exit code: 0" >> fry_calls.log
else
    echo "exit code: 1" >> fry_calls.log; status=1
fi
rm -f .unwinding_plain
exit $status
