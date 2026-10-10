#!/usr/bin/env bash
# BR-08 run.sh: the steps that need more than "compile one .cpp and run it".
# run_lab.sh has already built and run every .cpp of this folder (paired_run, the four traps,
# worked, forensic_gen, forensic_replay), which wrote the .csv logs used below.
set -u -o pipefail
cd "$(dirname "$0")"
GXX_VER="$(g++ --version | head -n 1)"
PY_VER="$(python3 --version) with numpy $(python3 -I -c 'import numpy; print(numpy.__version__)')"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
NOTE="hardware:  untested on hardware; the 'kit' is the stand-in model kit_model.hpp, no motor kit was used"
status=0

rec() { # rec NAME LISTING TOOLCHAIN COMMAND EXIT [extra lines...]
    local name="$1" listing="$2" tool="$3" cmd="$4" rc="$5"
    shift 5
    {
        echo "listing:   $listing"
        echo "toolchain: $tool"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $rc"
        for extra in "$@"; do echo "$extra"; done
    } > "$name.log"
    [ "$rc" = 0 ] || status=1
}

# 1. Compare the paired logs (Listing 4).
python3 -I compare_logs.py sim_run.csv kit_run.csv identified.txt > compare.out 2>&1
rec compare compare_logs.py "$PY_VER" \
    "python3 -I compare_logs.py sim_run.csv kit_run.csv identified.txt" "$?" "$NOTE"

# 2. The updated simulator, one effect at a time (Listing 5).
B=.build
rm -rf "$B"; mkdir -p "$B"
if g++ $FLAGS resim.cc -o "$B/resim" > "$B/resim.txt" 2>&1; then
    timeout 60 "$B/resim" identified.txt kit_run.csv > resim.out 2>&1
    rec resim resim.cc "$GXX_VER" "g++ $FLAGS resim.cc -o resim; ./resim identified.txt kit_run.csv" \
        "$?" "$NOTE"
else
    cp "$B/resim.txt" resim.out
    rec resim resim.cc "$GXX_VER" "g++ $FLAGS resim.cc -o resim" "BUILD FAILED"
fi

# 3. The first lines of the kit's log: the run record and the first rows.
head -n 14 kit_run.csv > kit_log_head.out
rec kit_log_head kit_run.csv "$(head --version | head -n 1)" "head -n 14 kit_run.csv" "$?" "$NOTE"

# 4. The forensic logs through the same comparison (used in the answer key).
python3 -I compare_logs.py forensic_morning.csv forensic_afternoon.csv "$B/forensic_id.txt" \
    morning afternoon > forensic_compare.out 2>&1
rec forensic_compare compare_logs.py "$PY_VER" \
    "python3 -I compare_logs.py forensic_morning.csv forensic_afternoon.csv id.txt morning afternoon" \
    "$?" "$NOTE"

# 5. The C++ listings run by run_lab.sh use the stand-in kit too: say so in their records.
for name in paired_run trap_gains trap_bench trap_estop trap_two_changes forensic_gen forensic_replay; do
    [ -f "$name.log" ] && echo "$NOTE" >> "$name.log"
done

rm -rf "$B"
exit $status
