#!/usr/bin/env bash
# F1-16 lab runner (called by university/labs/run_lab.sh after any .cpp files).
# Simulates every Verilog testbench of this chapter with Icarus Verilog (and runs the
# other tools named below) and records what really happened, in the same format as
# run_lab.sh: <name>.out = the real output, <name>.log = the run record.
# Generated files (.vvp, .vcd, netlists) are deleted at the end.
set -u
cd "$(dirname "$0")" || exit 2
status=0
IVERILOG_VER="$(iverilog -V 2>&1 | head -n 1)"
PY_VER="$(python3 --version 2>&1)"
YOSYS_VER="$(yosys -V 2>&1 | head -n 1)"

header() {  # header <name> <listings> <toolchain> <command>
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$1.log"
}

# check <name> <PASS|FAIL>: the testbench prints "RESULT: PASS" or "RESULT: FAIL ...".
# Working designs must pass; deliberately broken designs (forensic evidence) must fail.
check() {
    if grep -q "^RESULT: $2" "$1.out"; then
        if [ "$2" = PASS ]; then
            echo "check:     RESULT: PASS printed by the self-checking testbench" >> "$1.log"
        else
            echo "check:     RESULT: FAIL printed, as expected: deliberately broken design (forensic evidence)" >> "$1.log"
        fi
    else
        echo "check:     UNEXPECTED: RESULT: $2 not found in $1.out" >> "$1.log"; status=1
    fi
}

# sim <name> <verilog files...>: compile with iverilog (any warning counts as a failure),
# run with vvp, save everything it prints.
sim() {
    local name="$1"; shift
    header "$name" "$*" "$IVERILOG_VER" "iverilog -g2005 -Wall -o $name.vvp $* && vvp -n $name.vvp"
    if ! iverilog -g2005 -Wall -o "$name.vvp" "$@" > "$name.build.txt" 2>&1 || [ -s "$name.build.txt" ]; then
        echo "result:    BUILD FAILED (or warnings)" >> "$name.log"; cat "$name.build.txt" >> "$name.log"
        status=1
    else
        timeout 20 vvp -n "$name.vvp" > "$name.out" 2>&1
        local rc=$?
        echo "exit code: $rc" >> "$name.log"
        [ "$rc" = 0 ] || status=1
    fi
    rm -f "$name.build.txt" "$name.vvp"
}

# wave <name> <vcd> <step> <end> <signals...>: draw a text timing diagram from a VCD.
wave() {
    local name="$1" vcd="$2"; shift 2
    header "$name" "wave.py (reads $vcd)" "$PY_VER" "python3 -I wave.py $vcd $*"
    python3 -I wave.py "$vcd" "$@" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    [ "$rc" = 0 ] || status=1
}

# synth <name> <yosys commands> <verilog files...>: run Yosys quietly; the commands
# write their report with "tee -q -o <name>.out" so only the report is kept.
synth() {
    local name="$1" cmds="$2"; shift 2
    header "$name" "$*" "$YOSYS_VER" "yosys -q -p \"read_verilog $*; $cmds\""
    rm -f "$name.out"
    yosys -q -p "read_verilog $*; $cmds" > "$name.stderr.txt" 2>&1
    local rc=$?
    cat "$name.stderr.txt" >> "$name.out" 2>/dev/null
    rm -f "$name.stderr.txt"
    echo "exit code: $rc" >> "$name.log"
    [ "$rc" = 0 ] || status=1
}

# step <name> <listings> <toolchain> <command>: run any other command line exactly as
# written and keep what it prints.
step() {
    local name="$1" listing="$2" tool="$3" cmd="$4"
    header "$name" "$listing" "$tool" "$cmd"
    bash -c "$cmd" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    [ "$rc" = 0 ] || status=1
}

# ---------------------------------------------------------------- the steps of this lab
# Listing 1-2: the SR latch made of two NOR gates
sim sr_latch sr_latch_tb.v sr_latch.v

# Listings 3-6: D latch, D flip-flop and master-slave flip-flop driven by the same d
sim latch_vs_ff latch_vs_ff_tb.v d_latch.v d_flipflop.v ms_flipflop.v
check latch_vs_ff PASS
wave latch_vs_ff_wave latch_vs_ff.vcd 2/1 160 clk d q_latch q_ff q_ms

# Forensic lab: the selector that remembers (evidence), and the fixed version
step mux_bug "mux_tb.v mux_bug.v" "$IVERILOG_VER" \
    "iverilog -g2005 -Wall -DDUT=mux_bug -o mux_bug.vvp mux_tb.v mux_bug.v && vvp -n mux_bug.vvp"
check mux_bug FAIL
step mux_bug_yosys "mux_bug.v" "$YOSYS_VER" \
    "yosys -p 'read_verilog mux_bug.v; proc' | grep 'Latch inferred'"
step mux_fixed "mux_tb.v mux_fixed.v" "$IVERILOG_VER" \
    "iverilog -g2005 -Wall -DDUT=mux_fixed -o mux_fixed.vvp mux_tb.v mux_fixed.v && vvp -n mux_fixed.vvp"
check mux_fixed PASS
step mux_fixed_yosys "mux_fixed.v" "$YOSYS_VER" \
    "yosys -p 'read_verilog mux_fixed.v; proc' | grep -c 'Latch inferred' || true"

rm -f ./*.vcd ./*.vvp
exit $status
