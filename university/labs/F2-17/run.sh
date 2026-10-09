#!/usr/bin/env bash
# Extra step for F2-17, logged in the format of run_lab.sh:
#   nm_lambdas : lambdas.cpp compiled to an object file (-O0, no sanitizers); nm -C lists the
#                symbols, keeping the lines for the lambdas' call operators.
set -u
cd "$(dirname "$0")"
name=nm_lambdas
STR='std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >'
{
    echo "listing:   lambdas.cpp (symbols of the object file, lambda call operators only)"
    echo "toolchain: $(g++ --version | head -n 1); $(nm --version | head -n 1)"
    echo "command:   g++ -std=c++20 -O0 -c lambdas.cpp -o lambdas.o && nm -C lambdas.o | grep 'lambda' | grep 'operator()' | (keep main's lambdas only) (long std::string type shortened to std::string)"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > "${name}.log"
if g++ -std=c++20 -O0 -x c++ -c lambdas.cpp -o .lambdas.o > "${name}.out" 2>&1; then
    nm -C .lambdas.o | grep 'lambda' | grep 'operator()' | sed -E 's/^[0-9a-f]+ //' | grep -E '^. (auto )?main::' \
        | sed "s/$STR/std::string/g" | sort > "${name}.out"
    echo "exit code: 0" >> "${name}.log"
    rc=0
else
    echo "exit code: 1" >> "${name}.log"
    rc=1
fi
rm -f .lambdas.o
exit $rc
