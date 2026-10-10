#!/usr/bin/env bash
# F5-44 run.sh: build the DS403 cluster kernel and the lab switch, boot three QEMU nodes,
# and record the single-system-image services (cluster process list, remote run).
#   1. build            kernel f544.elf + host program labswitch
#   2. ssi              three nodes; node 3 halts at tick 600 (a crash, as far as others know)
#   3. accept           checks on the run's output
#   4. forensic (flap)  the same cluster with heartbeat period 60 ticks and suspicion 50 ticks
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u -o pipefail
cd "$(dirname "$0")"
. ./lablib.sh
status=0
KSRC="boot.S k.cc e1000.cc msg.cc member.cc ssi.cc f544_main.cc"

# 1. build
{ kbuild f544.elf $KSRC && hostbuild .labswitch labswitch.cc; } > build.txt 2>&1; rc=$?
{ cat build.txt; size -A f544.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)'; } > build.out
rec build "$KSRC kernel.ld labswitch.cc" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each kernel file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o f544.elf *.o; g++ $HOSTFLAGS labswitch.cc -o labswitch" "$rc"
[ "$rc" = 0 ] || status=1
rm -f build.txt

# 2. three nodes, node 3 stops at tick 600
CL=("node=1 nodes=3 role=ctl life=1200" "node=2 nodes=3 life=1200" "node=3 nodes=3 life=600")
cluster ssi 40000 f544.elf -- "${CL[@]}"
for i in 1 2 3; do
    mv ssi_n$i.txt ssi_n$i.out
    rc="$(awk -v n=n$i '$1==n {print $2}' ssi_rc.txt)"
    rec ssi_n$i "f544_main.cc (kernel f544.elf), node $i" "$QEMU_VER" \
        "$(qemu_node $i "$CLUSTER_BASE" f544.elf "${CL[$((i - 1))]}")" "$rc" \
        "note:      exit code 33 = the kernel wrote 0x10 to isa-debug-exit (its planned halt)" \
        "note:      UDP ports $((CLUSTER_BASE + 1))-$((CLUSTER_BASE + 13)) were chosen at random by lablib.sh for this run" "$HW_NOTE"
    [ "$rc" = 33 ] || status=1
done
mv ssi_switch.txt switch.out
rec switch "labswitch.cc" "$GXX_VER" "labswitch 3 $CLUSTER_BASE 40000 (stopped by SIGTERM when every node has halted)" 0 "$HW_NOTE"
rm -f ssi_rc.txt

# 3. acceptance checks
{
    c1="$(sed -n '/cps #1: asking/,/cps #1 done/p' ssi_n1.out | grep -cE '^  [1-3]\.[0-9]')"
    echo "cps #1 lists $c1 processes (expected 12: four on each of three nodes)"
    [ "$c1" = 12 ] && echo "  PASS" || echo "  FAIL"
    c2="$(sed -n '/cps #2: asking/,/cps #2 done/p' ssi_n1.out | grep -cE '^  [1-3]\.[0-9]')"
    echo "cps #2 lists $c2 processes (expected 8: node 3 has halted)"
    [ "$c2" = 8 ] && grep -q 'cps #2: node 3 not in the view' ssi_n1.out && echo "  PASS" || echo "  FAIL"
    for job in "1 20000" "2 30000" "3 10000"; do
        set -- $job
        want="$(python3 -c "n=$2; print(sum(1 for k in range(2,n+1) if all(k%d for d in range(2,int(k**0.5)+1))))")"
        got="$(grep -oE "job $1 ran on node [0-9]: primes<=$2 = [0-9]+" ssi_n1.out | awk '{print $NF}')"
        echo "job $1: kernel says $got, Python on the host says $want"
        [ -n "$got" ] && [ "$got" = "$want" ] && echo "  PASS" || echo "  FAIL"
    done
    grep -q 'rrun job 3 (primes<=10000) -> node 2' ssi_n1.out && echo "job 3 placed on node 2 (node 3 no longer in the view): PASS" || echo "job 3 placement: FAIL"
} > accept.out
grep -q FAIL accept.out; rc=$?
rec accept "run.sh step 3 (grep and Python checks of ssi_n1.out)" "$(python3 --version)" "see run.sh" "$([ $rc = 1 ] && echo 0 || echo 1)"
[ "$rc" = 1 ] || status=1

# 4. forensic evidence: heartbeat period longer than the suspicion timeout
cluster flap 40000 f544.elf -- "node=1 nodes=3 role=ctl life=1200 hb=60 suspect=50" "node=2 nodes=3 life=1200 hb=60 suspect=50" "node=3 nodes=3 life=1200 hb=60 suspect=50"
for i in 1 2 3; do
    mv flap_n$i.txt flap_n$i.out
    rc="$(awk -v n=n$i '$1==n {print $2}' flap_rc.txt)"
    rec flap_n$i "f544_main.cc (kernel f544.elf), node $i, forensic configuration (see the answer key)" "$QEMU_VER" \
        "qemu ... -append \"node=$i nodes=3$([ $i = 1 ] && echo ' role=ctl') life=1200 hb=60 suspect=50\" (other options as in ssi_n$i.log)" "$rc" "$HW_NOTE"
    [ "$rc" = 33 ] || status=1
done
mv flap_switch.txt flap_switch.out
rec flap_switch "labswitch.cc" "$GXX_VER" "labswitch 3 $CLUSTER_BASE 40000" 0 "$HW_NOTE"
{
    echo "view changes per node (a stable cluster has 2: start, then everyone joined):"
    for i in 1 2 3; do echo "  node $i: $(grep -c '] view ' flap_n$i.out) lines with 'view'"; done
    echo "first 24 lines of flap_n1.out:"
    head -n 24 flap_n1.out
    echo "lines of flap_n1.out about remote runs:"
    grep -E 'rrun|ran on|no answer' flap_n1.out
} > flap_summary.out
rec flap_summary "run.sh step 4 (grep of flap_n1.out ... flap_n3.out)" "$(grep --version | head -n 1)" "see run.sh" 0
rm -f flap_rc.txt f544.elf .labswitch
exit $status
