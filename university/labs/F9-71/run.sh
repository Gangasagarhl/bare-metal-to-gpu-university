#!/usr/bin/env bash
# F9-71 run.sh: the forensic evidence (the simulator and the bench robot of release 6) and the
# delay-margin cases of the worked example.
# (Listing 1 with sim2real.in runs through run_lab.sh itself.)
set -u -o pipefail
cd "$(dirname "$0")"
. ../F9-67/rblib.sh
B=.build
status=0
rm -rf "$B"; mkdir -p "$B"
g++ $HOSTFLAGS sim2real.cpp -o $B/sim2real || exit 1
timeout 10 $B/sim2real < sim2real_forensic.in > forensic_sim2real.out 2>&1; rc=$?
rec forensic_sim2real "sim2real.cpp" "$GXX_VER" "g++ $HOSTFLAGS sim2real.cpp -o sim2real; ./sim2real < sim2real_forensic.in" "$rc" \
    "stdin:     sim2real_forensic.in" "hardware:  both 'robots' are models in sim2real.cpp; no real motor was used (untested on hardware)"
[ "$rc" = 0 ] || status=1
timeout 10 $B/sim2real < sim2real_margin.in > margin.out 2>&1; rc=$?
rec margin "sim2real.cpp" "$GXX_VER" "g++ $HOSTFLAGS sim2real.cpp -o sim2real; ./sim2real < sim2real_margin.in" "$rc" \
    "stdin:     sim2real_margin.in" "hardware:  models only (untested on hardware)"
[ "$rc" = 0 ] || status=1
rm -rf "$B"
exit $status
