#!/usr/bin/env bash
# F3-51 run.sh: the forensic program forensic_workers.cpp again, this time on a terminal
# (a pseudo-terminal made by the script utility), to compare with forensic_workers.out, where
# standard output was a file. Also records which C library the reference runs used.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-50/oslib.sh
rm -rf .o; mkdir -p .o
g++ $HOSTFLAGS forensic_workers.cpp -o .o/fw; rc=$?
expect "$rc" 0 fw_build
script -q -c ./.o/fw /dev/null < /dev/null | tr -d '\r' > forensic_tty.out; rc=$?
rec forensic_tty "forensic_workers.cpp" "$GXX_VER; $(script --version | head -n 1)" \
    "script -q -c ./forensic_workers /dev/null   (standard output is a pseudo-terminal)" "$rc" \
    "note:      carriage returns added by the terminal were removed"
expect "$rc" 0 forensic_tty
{ echo "reference C library for every run in this chapter: $LDD_VER"
  echo "same program, standard output to a file  : $(wc -l < forensic_workers.out) lines"
  echo "same program, standard output to terminal: $(wc -l < forensic_tty.out) lines"; } > forensic_compare.out
rec forensic_compare "forensic_workers.out, forensic_tty.out" "$(wc --version | head -n 1)" "wc -l forensic_workers.out forensic_tty.out" 0
rm -rf .o
exit $status
