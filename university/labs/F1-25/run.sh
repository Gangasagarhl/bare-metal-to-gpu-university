#!/usr/bin/env bash
# F1-25: simulate the Verilog control unit (reference and forensic versions) with Icarus
# Verilog and compare each with the C++ table that the simulators use (Listing 1's output).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
rec() {  # rec <name> <listing> <command> <exit code> [extra line]
    {
        echo "listing:   $2"
        echo "toolchain: ${TC}"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $4"
        if [ $# -ge 5 ]; then echo "$5"; fi
    } > "$1.log"
}
# The C++ table without its header and name column, ALU names turned into numbers.
tail -n +2 control_table.out | awk '{a["add"]=0;a["sub"]=1;a["and"]=2;a["or"]=3;a["slt"]=4;
    printf "%2d %5d %6d %5d %5d %5d %8d %4d %4d %3d %4d %d\n",$1,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,a[$13]}' \
    > .expected.txt
for v in control control_bug; do
    TC="$(iverilog -V 2>&1 | head -n 1)"
    iverilog -g2012 -Wall -o .sim_$v $v.v control_tb.v > .build_$v.txt 2>&1 && vvp -n .sim_$v \
        | grep -v '\$finish' > ${v}_tb.out; rc=$?
    cat .build_$v.txt >> ${v}_tb.out
    rec ${v}_tb "$v.v + control_tb.v" "iverilog -g2012 -Wall -o sim $v.v control_tb.v && vvp -n sim" "$rc"
    [ "$rc" = 0 ] || status=1
    # compare with the C++ table; the difference (if any) is the evidence
    if diff .expected.txt ${v}_tb.out > ${v}_diff.out; then
        echo "no differences: the Verilog table equals the C++ table for all 16 opcodes" > ${v}_diff.out
        res="result:    tables equal"
    else
        res="result:    tables differ (lines starting < are the C++ table, > the Verilog table)"
    fi
    TC="$(diff --version | head -n 1)"
    rec ${v}_diff "control_table.out vs ${v}_tb.out" "diff expected.txt ${v}_tb.out" 0 "$res"
    rm -f .sim_$v .build_$v.txt
done
rm -f .expected.txt
# synthesise the reference control unit into generic gates and count them
TC="$(yosys -V | head -n 1)"
yosys -q -p "read_verilog control.v; synth -top control; tee -o control_synth.out stat"; rc=$?
rec control_synth control.v "yosys -q -p \"read_verilog control.v; synth -top control; tee -o control_synth.out stat\"" "$rc" \
    "note:      generic gate cells from yosys's own library; not a real chip's standard-cell library"
[ "$rc" = 0 ] || status=1
exit $status
