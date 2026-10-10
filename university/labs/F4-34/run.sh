#!/usr/bin/env bash
# F4-34 run.sh: the forensic evidence. boardscore.cpp itself is built and run by run_lab.sh
# (with boardscore.in); here the evidence pack's records are scored twice: with the tool version
# the team used (an older gate check, recreated with sed) and with the current Listing 1.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-31/lablib.sh
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
sed 's/    if (get(b, "boot_chain") != "open" \&\& get(b, "boot_chain") != "payload") {   \/\/ only known-good values pass/    if (get(b, "boot_chain") == "locked" || get(b, "boot_chain") == "unknown") {/' \
    boardscore.cpp > $B/boardscore_old.cpp
diff boardscore.cpp $B/boardscore_old.cpp | sed "s#$B/##" > forensic_diff.out
hostbuild $B/old $B/boardscore_old.cpp > $B/b1.txt 2>&1 && hostbuild $B/new boardscore.cpp > $B/b2.txt 2>&1 || status=1
$B/old < forensic.in > forensic_old.out 2>&1; rc=$?
rec forensic_old "boardscore.cpp as the team ran it (older gate check)" "$GXX_VER" "g++ $HOSTFLAGS boardscore_old.cpp -o old; ./old < forensic.in" "$rc"
$B/new < forensic.in > forensic_new.out 2>&1; rc=$?
rec forensic_new "boardscore.cpp (Listing 1)" "$GXX_VER" "g++ $HOSTFLAGS boardscore.cpp -o new; ./new < forensic.in" "$rc"
rec forensic_diff "boardscore.cpp vs the older version" "$(diff --version | head -n 1)" "diff boardscore.cpp boardscore_old.cpp" "0"
rm -rf "$B"
exit $status
