#!/usr/bin/env bash
# Extra steps for F5-42 (run by run_lab.sh after the .cpp listings):
#  1. dashcheck: parse the static HTML report written by dashboard.cpp with Python's html.parser
#     and check that it is balanced, has no script, and contains the expected chart and table.
#  2. dashboard_head: a trimmed copy of the report for the chapter page (trimming is marked).
set -u
py="$(python3 --version 2>&1)"
header() {   # header <name> <listing> <command> <toolchain>
    {
        echo "listing:   $2"
        echo "toolchain: $4"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$1.log"
}
header dashcheck "dashcheck.py" "python3 -I dashcheck.py dashboard.out" "$py"
python3 -I dashcheck.py dashboard.out > dashcheck.out 2>&1
rc=$?
echo "exit code: $rc" >> dashcheck.log

header dashboard_head "dashboard.out from the run of dashboard.cpp, trimmed" \
    "head -n 14 dashboard.out; a marker line; tail -n 4 dashboard.out" "coreutils head/tail"
total=$(wc -l < dashboard.out)
{ head -n 14 dashboard.out; echo "[... $((total - 18)) line(s) trimmed: the other bars, the axis labels and the first table rows ...]"; tail -n 4 dashboard.out; } > dashboard_head.out
echo "exit code: 0 (of the trimming step; dashboard.out had $total lines)" >> dashboard_head.log
exit $rc
