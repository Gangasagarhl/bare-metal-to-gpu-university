#!/usr/bin/env bash
# SPDX-License-Identifier: LicenseRef-Uni-Lab
# expect.sh <exit code> <regex> <command> [args...]
# A CLI test for CTest that checks BOTH the exact exit code and the output. CTest's
# PASS_REGULAR_EXPRESSION alone ignores the exit code ("cmake --help-property
# PASS_REGULAR_EXPRESSION"), so a wrong exit status after the right text would pass
# (BR-09 lab step regex_ignores_exit).
set -u
want_code="$1"; want_text="$2"; shift 2
out="$("$@" 2>&1)"; code=$?
printf '%s\n' "$out"
if [ "$code" != "$want_code" ]; then
    echo "expect.sh: exit code ${code}, expected ${want_code}"; exit 1
fi
if ! printf '%s\n' "$out" | grep -Eq -- "$want_text"; then
    echo "expect.sh: output does not match /${want_text}/"; exit 1
fi
echo "expect.sh: ok (exit code ${code}, output matches)"
