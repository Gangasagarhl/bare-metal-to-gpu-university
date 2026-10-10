#!/usr/bin/env bash
# F5-47 run.sh: membership + Raft + scheduler on three QEMU nodes of the DS403 kernel.
#   1. build        kernel f547.elf (F5-44 base + sm.cc raft.cc naive.cc f547_main.cc)
#   2. sched        four jobs; node 3 halts at tick 700 in the middle of its job
#   3. split        Raft; the switch cuts {1} from {2,3} between 6 s and 16 s; two clients race
#   4. accept       checks on 2 and 3
#   5. splitbrain   the same partition with the "lowest live id leads" design (forensic)
set -u -o pipefail
cd "$(dirname "$0")"
. ../F5-44/lablib.sh
status=0
K=../F5-44
KSRC="$K/boot.S $K/k.cc $K/e1000.cc $K/msg.cc $K/member.cc sm.cc raft.cc naive.cc f547_main.cc"

{ kbuild f547.elf $KSRC && hostbuild "$K/.labswitch" "$K/labswitch.cc"; } > build.out 2>&1; rc=$?
size -A f547.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)' >> build.out
rec build "F5-44: boot.S k.cc e1000.cc msg.cc member.cc; F5-47: sm.cc raft.cc naive.cc f547_main.cc" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o f547.elf *.o" "$rc"
[ "$rc" = 0 ] || status=1

run_cluster() {  # run_cluster <tag> <switch partition args or nothing> -- <command lines>
    local tag="$1"; shift
    local sw=(); while [ "$1" != "--" ]; do sw+=("$1"); shift; done; shift
    CL=("$@")
    cluster "$tag" 60000 f547.elf "${sw[@]}" -- "${CL[@]}"
    for i in 1 2 3; do
        mv "${tag}_n$i.txt" "${tag}_n$i.out"
        rc="$(awk -v n=n$i '$1==n {print $2}' "${tag}_rc.txt")"
        rec "${tag}_n$i" "f547_main.cc sm.cc raft.cc naive.cc (kernel f547.elf), node $i" "$QEMU_VER" \
            "$(qemu_node $i "$CLUSTER_BASE" f547.elf "${CL[$((i - 1))]}")" "$rc" \
            "note:      exit code 33 = the kernel wrote 0x10 to isa-debug-exit (its planned halt)" "$HW_NOTE"
        [ "$rc" = 33 ] || status=1
    done
    mv "${tag}_switch.txt" "${tag}_switch.out"
    rec "${tag}_switch" "labswitch.cc (F5-44)" "$GXX_VER" "labswitch 3 $CLUSTER_BASE 60000 ${sw[*]}" 0 "$HW_NOTE"
    rm -f "${tag}_rc.txt"
}

run_cluster sched -- "node=1 nodes=3 scen=sched life=1500" "node=2 nodes=3 scen=sched life=1500" "node=3 nodes=3 scen=sched life=700"
run_cluster split 6000 16000 1 23 -- "node=1 nodes=3 scen=split mode=raft life=2200" \
    "node=2 nodes=3 scen=split mode=raft life=2200" "node=3 nodes=3 scen=split mode=raft life=2200"

{
    echo "scheduler run:"
    for i in 1 2; do
        grep -q 'state (final): job1=done(669) job2=done(1229) job3=done(1754) job4=done(2262)' sched_n$i.out \
            && echo "  node $i: all four jobs done, results = primes <= 5000, 10000, 15000, 20000: PASS" || echo "  node $i: FAIL"
    done
    grep -q 'scheduler: node 3 left the view; job 3 must run again' sched_n1.out sched_n2.out sched_n3.out \
        && echo "  job 3 was placed again after node 3 left: PASS" || echo "  re-placement: FAIL"
    echo "  host check of the job results: $(python3 -c "print([sum(1 for k in range(2,n+1) if all(k%d for d in range(2,int(k**0.5)+1))) for n in (5000,10000,15000,20000)])")"
    echo "partition run (Raft):"
    ok="$(grep -h 'client: request 1 (CAS lock' split_n*.out | grep -c ': OK')"
    echo "  clients told OK: $ok (expected 1: only the majority side can commit)"
    [ "$ok" = 1 ] && echo "  PASS" || echo "  FAIL"
    grep -q "client: request 1 (CAS lock ''->n1): no answer in time; outcome UNKNOWN" split_n1.out \
        && echo "  node 1 (minority) could not commit and said so: PASS" || echo "  minority client: FAIL"
    finals="$(grep -h 'state (final)' split_n*.out | sed 's/.*state (final)[:]//' | sort -u)"
    echo "  final states: $(echo "$finals" | tr '\n' ' ')"
    [ "$(echo "$finals" | wc -l)" = 1 ] && echo "  all three nodes agree: PASS" || echo "  nodes disagree: FAIL"
} > accept.out
grep -q FAIL accept.out && { rc=1; status=1; } || rc=0
rec accept "run.sh step 4 (grep and Python checks)" "$(python3 --version)" "see run.sh" "$rc"

run_cluster splitbrain 6000 16000 1 23 -- "node=1 nodes=3 scen=split mode=naive life=2200" \
    "node=2 nodes=3 scen=split mode=naive life=2200" "node=3 nodes=3 scen=split mode=naive life=2200"
{
    echo "clients told OK in the naive design: $(grep -h 'client: request 1' splitbrain_n*.out | grep -c 'OK')"
    echo "final states:"; grep -h 'state (final)' splitbrain_n*.out
} > splitbrain_check.out
grep -q 'told OK in the naive design: 2' splitbrain_check.out && r="safety violated, as the forensic lab needs" || { r="the race did not happen this time"; status=1; }
rec splitbrain_check "run.sh step 5 (grep of splitbrain_n*.out)" "$(grep --version | head -n 1)" "see run.sh" 0 "result:    expected-fail: $r"
rm -f f547.elf "$K/.labswitch"
exit $status
