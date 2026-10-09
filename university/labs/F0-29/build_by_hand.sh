#!/usr/bin/env bash
# Builds hello.cpp the short way, by hand, and looks at the file the compiler made.
# Run from this folder:  bash build_by_hand.sh > build_by_hand.txt 2>&1
set -u
echo '$ g++ --version | head -n 1'
g++ --version | head -n 1
echo '$ g++ hello.cpp -o hello'
g++ hello.cpp -o hello
echo '$ ./hello'
./hello
echo "(exit code: $?)"
echo '$ file hello'
file hello | sed 's/BuildID\[sha1\]=[0-9a-f]*/BuildID[sha1]=<hash removed>/'
echo '$ od -A d -t x1 -N 16 hello'
od -A d -t x1 -N 16 hello
echo '$ od -A d -c -N 4 hello'
od -A d -c -N 4 hello
echo '$ g++ --help | grep -E "^  -o |^  -std="'
g++ --help | grep -E "^  -o |^  -std="
rm -f hello
