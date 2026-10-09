#!/usr/bin/env bash
# Extra step for F2-14: show that the compiler made one function per type used.
# Writes nm_larger.out / nm_larger.log in the format of run_lab.sh.
set -u
cd "$(dirname "$0")"
name=nm_larger
{
    echo "listing:   larger.cpp (compiled to an object file, symbols listed with nm)"
    echo "toolchain: $(g++ --version | head -n 1); $(nm --version | head -n 1)"
    echo "command:   g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -c larger.cpp -o larger.o && nm -C larger.o | grep ' larger<'"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > "${name}.log"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -c larger.cpp -o .larger.o > "${name}.out" 2>&1
rc=$?
if [ $rc -eq 0 ]; then
    nm -C .larger.o | grep ' larger<' | sed 's/^[0-9a-f]* //' > "${name}.out"
    rc=${PIPESTATUS[0]}
fi
echo "exit code: $rc" >> "${name}.log"
rm -f .larger.o
exit $rc
