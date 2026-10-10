#!/usr/bin/env bash
# HW201-P runner (called by university/labs/run_lab.sh after exam_checks.cpp has been built
# and run). Runs the practical's reference solution and starting file, the final exam's
# forensic evidence, the reading question's module and the course project's reference,
# with Icarus Verilog, Verilator and Yosys, in the same record format as the chapter labs.
set -u
cd "$(dirname "$0")" || exit 2
status=0
IVERILOG_VER="$(iverilog -V 2>&1 | head -n 1)"
YOSYS_VER="$(yosys -V 2>&1 | head -n 1)"
VERILATOR_VER="$(verilator --version 2>&1 | head -n 1)"

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
check() {
    if grep -q "^RESULT: $2" "$1.out"; then
        if [ "$2" = PASS ]; then
            echo "check:     RESULT: PASS printed by the self-checking testbench" >> "$1.log"
        else
            echo "check:     RESULT: FAIL printed, as expected (starting file or forensic evidence)" >> "$1.log"
        fi
    else
        echo "check:     UNEXPECTED: RESULT: $2 not found in $1.out" >> "$1.log"; status=1
    fi
}

# step <name> <listings> <toolchain> <command>: run a command line exactly as written.
step() {
    local name="$1" listing="$2" tool="$3" cmd="$4"
    header "$name" "$listing" "$tool" "$cmd"
    bash -c "$cmd" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    [ "$rc" = 0 ] || status=1
}

# ------------------------------------------------------------ Practical (P): the dishwasher FSM
# the reference solution: must pass, lint clean, and its synthesis report
step dishwasher "dishwasher_tb.v dishwasher.v" "$IVERILOG_VER" \
    "iverilog -g2005 -Wall -o dishwasher.vvp dishwasher_tb.v dishwasher.v && vvp -n dishwasher.vvp"
check dishwasher PASS
step dishwasher_lint "dishwasher.v" "$VERILATOR_VER" \
    "verilator --lint-only -Wall dishwasher.v && echo 'verilator: no warnings, exit code 0'"
step dishwasher_synth "dishwasher.v" "$YOSYS_VER" \
    "yosys -q -p 'read_verilog dishwasher.v; synth -top dishwasher; tee -q -o /dev/stdout stat'"
# the starting file as handed out (start/dishwasher.v): compiles, fails the testbench
step dishwasher_start "dishwasher_tb.v start/dishwasher.v" "$IVERILOG_VER" \
    "iverilog -g2005 -Wall -o dishwasher_start.vvp dishwasher_tb.v start/dishwasher.v && vvp -n dishwasher_start.vvp"
check dishwasher_start FAIL
# the lint tool's view of the starting file (its web-link lines removed, as this university
# prints no URLs; verilator's own exit code is printed at the end)
step dishwasher_start_lint "start/dishwasher.v" "$VERILATOR_VER" \
    "cd start && verilator --lint-only -Wall dishwasher.v > ../dishwasher_start_lint.raw 2>&1; rc=\$?; cd ..; grep -v -e 'warning description' -e 'lint_off' dishwasher_start_lint.raw; rm -f dishwasher_start_lint.raw; echo \"[lines with web links removed] verilator exit code: \$rc\""

# ------------------------------------------------ Final, forensic question: the 010 detector
step seq010_bug "seq010_tb.v seq010_bug.v" "$IVERILOG_VER" \
    "iverilog -g2005 -Wall -DDUT=seq010_bug -o seq010_bug.vvp seq010_tb.v seq010_bug.v && vvp -n seq010_bug.vvp"
check seq010_bug FAIL
step seq010 "seq010_tb.v seq010.v" "$IVERILOG_VER" \
    "iverilog -g2005 -Wall -DDUT=seq010 -o seq010.vvp seq010_tb.v seq010.v && vvp -n seq010.vvp"
check seq010 PASS
step seq010_synth "seq010.v seq010_bug.v" "$YOSYS_VER" \
    "for m in seq010 seq010_bug; do yosys -q -p \"read_verilog \$m.v; synth -top \$m; tee -q -o /dev/stdout stat\" | grep -e '===' -e 'cells' -e 'DFF'; done"

# ------------------------------------------------ Final, question F14: reading read_me.v
step read_me_latch "read_me.v" "$YOSYS_VER" \
    "yosys -p 'read_verilog read_me.v; proc' | grep -c 'Latch inferred' | sed 's/^/Latch inferred lines: /'"
step read_me_synth "read_me.v" "$YOSYS_VER" \
    "yosys -q -p 'read_verilog read_me.v; synth -top read_me; tee -q -o /dev/stdout stat' | grep -e '===' -e 'cells' -e 'DFF' -e 'LATCH'"

# ------------------------------------------------ Course project: reference solution
step project "project_tb.v pc_branch.v ram256x8.v" "$IVERILOG_VER" \
    "iverilog -g2005 -Wall -o project.vvp project_tb.v pc_branch.v ram256x8.v && vvp -n project.vvp"
check project PASS
step project_lint "pc_branch.v ram256x8.v" "$VERILATOR_VER" \
    "verilator --lint-only -Wall pc_branch.v && verilator --lint-only -Wall ram256x8.v && echo 'verilator: no warnings, exit code 0'"
step project_synth "pc_branch.v" "$YOSYS_VER" \
    "yosys -q -p 'read_verilog pc_branch.v; synth -top pc_branch; tee -q -o /dev/stdout stat'"
step project_ram_mem "ram256x8.v" "$YOSYS_VER" \
    "yosys -q -p 'read_verilog ram256x8.v; proc; opt; memory -nomap; opt; tee -q -o /dev/stdout stat' | grep -e '===' -e 'cells' -e 'memor' -e '\\\$mem'"
step project_ram_lut "ram256x8.v" "$YOSYS_VER" \
    "yosys -q -p 'read_verilog ram256x8.v; synth -top ram256x8 -lut 4; tee -q -o /dev/stdout stat' | grep -e '===' -e 'cells' -e 'DFF' -e 'lut'"

rm -f ./*.vcd ./*.vvp
exit $status
