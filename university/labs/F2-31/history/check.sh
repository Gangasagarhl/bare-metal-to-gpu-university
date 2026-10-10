#!/usr/bin/env bash
# check.sh: used by "git bisect run". Exit status 0 means "this commit is good";
# a non-zero status (here: the tests failed) means "this commit is bad".
cmake -S . -B build/bisect -DCMAKE_BUILD_TYPE=Release > /dev/null || exit 125
cmake --build build/bisect > /dev/null || exit 125
ctest --test-dir build/bisect > /dev/null
