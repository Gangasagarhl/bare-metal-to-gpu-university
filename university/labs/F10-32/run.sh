#!/usr/bin/env bash
# F10-32 lab steps: fly the simulated survey and write a UDF log, look at its first bytes,
# read it back with dfread.py, review it with analyse.py, read a damaged copy, then make the
# forensic log (GNSS glitch) and extract its evidence pack and the answer-key analysis.
# The binary logs stay in a temporary folder and are deleted at the end.
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
rec() {  # rec <name> <listing> <toolchain> <command> <exit code> [extra line]
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
}
GXX="$(g++ --version | head -n 1)"
PY="$(python3 --version 2>&1); numpy $(python3 -c 'import numpy; print(numpy.__version__)')"
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
NOTE="note:      UDF is the university's own log format, modelled on the idea of DataFlash logs; no ArduPilot log or tool was used"
run() {  # run <name> <listing> <toolchain> <command...>
    local name="$1" listing="$2" tc="$3"; shift 3
    "$@" > "$name.out" 2>&1; local rc=$?
    sed -i "s#$B/##g" "$name.out"
    rec "$name" "$listing" "$tc" "$(echo "$*" | sed "s#$B/##g")" "$rc" "$NOTE"
    [ "$rc" = 0 ] || status=1
}

g++ $CXXFLAGS fly_log.cc -o $B/fly_log > $B/build.txt 2>&1 || { cat $B/build.txt; status=1; }
run fly_log "fly_log.cc udf.hpp" "$GXX" $B/fly_log $B/flight.bin normal
run hexdump "hexdump.py" "$PY" python3 hexdump.py $B/flight.bin 96
run read_summary "dfread.py" "$PY" python3 dfread.py $B/flight.bin --summary
run read_bat "dfread.py" "$PY" python3 dfread.py $B/flight.bin --type BAT --from 0 --to 6
run analyse "analyse.py dfread.py" "$PY" python3 analyse.py $B/flight.bin

# a damaged copy: 7 bytes of junk inserted at offset 4000, file cut at 9000 bytes
python3 -c "import sys; d = open(sys.argv[1], 'rb').read(); open(sys.argv[2], 'wb').write((d[:4000] + bytes(range(7)) + d[4000:])[:9000])" $B/flight.bin $B/damaged.bin
run read_damaged "dfread.py" "$PY" python3 dfread.py $B/damaged.bin --summary

# forensic: the GNSS-glitch flight
"$B/fly_log" $B/glitch.bin glitch > /dev/null 2>&1 || status=1
run forensic_summary "dfread.py on the forensic log" "$PY" python3 dfread.py $B/glitch.bin --summary
{
    for t in PARM MODE MSG ERR; do echo "--- $t"; python3 dfread.py $B/glitch.bin --type $t; done
} > forensic_events.out 2>&1; rc=$?
rec forensic_events "dfread.py --type PARM/MODE/MSG/ERR on the forensic log" "$PY" "python3 dfread.py glitch.bin --type <PARM|MODE|MSG|ERR>" "$rc" "$NOTE"
run forensic_gps "dfread.py (answer key)" "$PY" python3 dfread.py $B/glitch.bin --type GPS --from 23.9 --to 26.3
run forensic_analyse "analyse.py (answer key)" "$PY" python3 analyse.py $B/glitch.bin
rm -rf "$B" __pycache__
exit $status
