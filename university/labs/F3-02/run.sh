#!/usr/bin/env bash
# Extra run for F3-02 (answer to Check yourself 6): Listing 2 without its fflush,
# with standard output redirected to a file.
set -u
gxx="$(g++ --version | head -n 1)"
flags="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
{
    echo "listing:   exec_child.cpp with the line 'std::fflush(stdout);' deleted (sed)"
    echo "toolchain: $gxx"
    echo "command:   sed '/std::fflush(stdout);/d' exec_child.cpp > noflush.cc && g++ $flags noflush.cc -o noflush && ./noflush > noflush.out"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > noflush.log
sed '/std::fflush(stdout);/d' exec_child.cpp > .noflush.cc
g++ $flags -x c++ .noflush.cc -o .bin_noflush || { echo "result:    BUILD FAILED" >> noflush.log; exit 1; }
./.bin_noflush > noflush.out 2>&1; echo "exit code: $?" >> noflush.log
rm -f .noflush.cc .bin_noflush
exit 0
