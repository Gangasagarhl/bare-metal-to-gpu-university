#!/usr/bin/env bash
# SPDX-License-Identifier: LicenseRef-Uni-Lab
# golden.sh <stats_cli> <inputs file> <expected file>
# Compatibility test: for every line of valid inputs, the output and exit code must be
# byte-identical to the recorded output of version 0.1 (tests/golden_v0_1.txt).
set -u
cli="$1"; inputs="$2"; expected="$3"
actual="$(while IFS= read -r line; do
    # word splitting of the input line into arguments is intended
    out="$("$cli" $line 2>&1)"; code=$?
    printf '%s -> %s [exit %d]\n' "$line" "$out" "$code"
done < "$inputs")"
if diff <(printf '%s\n' "$actual") "$expected"; then
    echo "golden.sh: $(wc -l < "$inputs") cases identical to version 0.1"
else
    echo "golden.sh: output differs from version 0.1 (lines marked < are this build)"; exit 1
fi
