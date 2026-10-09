#!/usr/bin/env bash
# HDL steps of this lab, called by run_lab.sh after any C++ listings.
# Each step writes <name>.out (the real output) and <name>.log (the run record).
set -u
cd "$(dirname "$0")" || exit 2
status=0
TOOL="$(iverilog -V 2>&1 | head -n 1); vvp from the same package"
HW="untested on hardware: switch-level and gate-level simulation only; no physical transistors or logic chips were used in this build (AH-26)"

# sim <name> <mode> <iverilog arguments...>
#   pass:      the testbench must print "ALL PASS"
#   fail:      forensic evidence from a deliberately broken design; must print "FAIL"
#   demo:      a measurement or demonstration; its printed values are the evidence
#   buildfail: the compile is expected to fail; its messages are the evidence
sim() {
    local name="$1" mode="$2"; shift 2
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
        if [ "$mode" = buildfail ]; then
            echo "result:    compile failed as expected (messages saved in ${name}.out)" >> "${name}.log"
        else
            echo "result:    BUILD FAILED" >> "${name}.log"; status=1
        fi
        rm -f "$bin"; return
    fi
    if [ "$mode" = buildfail ]; then
        echo "result:    UNEXPECTED SUCCESS (compile was expected to fail)" >> "${name}.log"; status=1
        rm -f "$bin"; return
    fi
    timeout 20 vvp -n "$bin" >> "${name}.out" 2>&1
    echo "exit code: $?" >> "${name}.log"
    case "$mode" in
        pass)
            if ! grep -q "^ALL PASS" "${name}.out"; then
                echo "result:    TEST FAILED (testbench did not print ALL PASS)" >> "${name}.log"; status=1
            fi ;;
        fail)
            if grep -q "FAIL" "${name}.out"; then
                echo "result:    failures recorded as intended (deliberately broken design, forensic evidence)" >> "${name}.log"
            else
                echo "result:    UNEXPECTED PASS (the planted fault was not detected)" >> "${name}.log"; status=1
            fi ;;
        demo) ;;
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

sim ripple_delay demo tb_ripple_delay.v delay_adder.v
sim carry_trace  demo tb_carry_trace.v delay_adder.v
sim glitch       demo tb_glitch.v glitch_mux.v
sim sample_early fail tb_sample.v delay_adder.v
sim sample_late  pass -P tb_sample.WAIT=20 tb_sample.v delay_adder.v
exit $status
