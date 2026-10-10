#!/usr/bin/env bash
# F6-33 Listing 2: count the instructions that matter for a roofline point in the
# real SASS of every kernel in a cubin. Straight-line kernels only (no loops left
# after unrolling): each instruction then runs once per thread.
#   usage: count_sass.sh file.cubin
# FLOPs: FFMA = 2, FADD and FMUL = 1. Bytes: LDG/STG width from the opcode suffix
# (.64 = 8 bytes, .128 = 16 bytes, otherwise 4 bytes for these float kernels).
cuobjdump -sass "$1" | awk '
/Function :/ { if (name != "") out(); name = $3; ffma = fadd = fmul = mufu = ld = st = ins = 0; next }
/^[[:space:]]+\/\*[0-9a-f]+\*\// {
    op = $2; if (op ~ /^@/) op = $3
    sub(/;$/, "", op)                          # "NOP;" carries its ; with no space
    if (op == "NOP") next
    ins++
    w = 4; if (op ~ /\.64/) w = 8; if (op ~ /\.128/) w = 16
    if (op ~ /^FFMA/) ffma++
    else if (op ~ /^FADD/) fadd++
    else if (op ~ /^FMUL/) fmul++
    else if (op ~ /^MUFU/) mufu++
    else if (op ~ /^LDG/) ld += w
    else if (op ~ /^STG/) st += w
}
function out() {
    printf "kernel %-28s FFMA %3d FADD %3d FMUL %3d MUFU %2d LDGbytes %3d STGbytes %3d instr %4d flops %4d bytes %3d\n",
        name, ffma, fadd, fmul, mufu, ld, st, ins, 2 * ffma + fadd + fmul, ld + st
}
END { if (name != "") out() }'
