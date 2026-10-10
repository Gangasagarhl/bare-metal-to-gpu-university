#!/usr/bin/env bash
# HW102 practical exam (P): HDL runs of this folder, called by run_lab.sh after exam_checks.cpp.
# Each run writes <name>.out (the real output) and <name>.log (the run record), in the
# format of the course's lab runners (university/labs/F1-14/run.sh).
set -u
cd "$(dirname "$0")" || exit 2
status=0
TOOL="$(iverilog -V 2>&1 | head -n 1); vvp from the same package"
HW="untested on hardware: gate-level simulation only; no physical logic chips or FPGA were used in this build (AH-26)"

# sim <name> <mode> <timeout seconds> <iverilog arguments...>
#   pass:  the testbench must print "ALL PASS"
#   fail:  evidence from a deliberately broken design or the starting file; must print "FAIL"
#   demo:  a measurement; its printed values are the evidence
sim() {
    local name="$1" mode="$2" tmo="$3"; shift 3
    local bin=".bin_${name}"
    {
        echo "listing:   $*"
        echo "toolchain: $TOOL"
        echo "command:   iverilog -g2012 -Wall -o ${name} $* && vvp -n ${name}"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "hardware:  $HW"
    } > "${name}.log"
    if ! iverilog -g2012 -Wall -o "$bin" "$@" > "${name}.out" 2>&1; then
        echo "result:    BUILD FAILED" >> "${name}.log"; status=1
        rm -f "$bin"; return
    fi
    timeout "$tmo" vvp -n "$bin" >> "${name}.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "${name}.log"
    case "$mode" in
        pass)
            if ! grep -q "^ALL PASS" "${name}.out"; then
                echo "result:    TEST FAILED (testbench did not print ALL PASS)" >> "${name}.log"; status=1
            fi ;;
        fail)
            if grep -q "FAIL" "${name}.out"; then
                echo "result:    failures recorded as intended (deliberately broken design or starting file, evidence)" >> "${name}.log"
            else
                echo "result:    UNEXPECTED PASS (the planted fault was not detected)" >> "${name}.log"; status=1
            fi ;;
        demo)
            [ "$rc" = 0 ] || status=1 ;;
    esac
    rm -f "$bin"
}

# synth <name> <top> <files...>: map a design onto NAND (and NOT) cells with Yosys and count them
synth() {
    local name="$1" top="$2"; shift 2
    local script="read_verilog $*; synth -top ${top}; abc -g NAND; opt_clean; tee -o ${name}.out stat"
    {
        echo "listing:   $*"
        echo "toolchain: $(yosys -V)"
        echo "command:   yosys -q -p \"${script}\""
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "hardware:  $HW"
    } > "${name}.log"
    yosys -q -p "$script" > "${name}.yosys.txt" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "${name}.log"
    [ "$rc" = 0 ] || { cat "${name}.yosys.txt" >> "${name}.log"; status=1; }
    rm -f "${name}.yosys.txt"
}

# The practical: reference solution, the starting file as handed out, the two planted faults
sim alu4_exam        pass 60 tb_alu4_exam.v full_adder.v alu4_ref.v
sim alu4_start       fail 60 tb_alu4_exam.v full_adder.v alu4_start.v
sim alu4_fault_cin   fail 60 tb_alu4_exam.v full_adder.v alu4_fault_cin.v
sim alu4_fault_carry fail 60 tb_alu4_exam.v full_adder.v alu4_fault_carry.v
synth yosys_alu4 alu4 full_adder.v alu4_ref.v

# Evidence for the final's forensic question and for the midterm/final reading questions
sim alu4_forensic    fail 60 tb_alu4_forensic.v full_adder.v alu4_forensic.v
sim inverter_bad2    fail 20 tb_inverter.v inverter_bad2.v
sim xor_bad2         fail 20 tb_xor.v xor_bad2.v
sim mux4_bad2        fail 20 tb_mux4_probe.v mux2_only.v mux4_bad2.v

# Reference for the final's design question (F19)
sim cmp4             pass 20 tb_cmp4.v cmp4.v full_adder.v alu4_ref.v

# Course project reference: the 16-bit structural ALU, its random test, timing and gate count
sim alu16_project    pass 400 tb_alu16_project.v alu_n_ref.v
sim alu16_timing     demo 60 -DTIMING tb_alu16_timing.v alu_n_ref.v
synth yosys_alu16 alu_n alu_n_ref.v
exit $status
