#!/usr/bin/env bash
# F6-34 extra lab steps: real PTX and SASS of kernels.cu and rowsums.cu for sm_80.
# Nothing here needs a GPU: the compiler and the binary utilities do all the work.
#   ptx_axpy        PTX bodies of axpyPlain and axpyRestrict
#   sass_axpy       SASS of the same two kernels
#   sass_lineinfo   nvdisasm with source-line annotations (built with -lineinfo)
#   sass_copy4      the global loads and stores of copy4
#   sass_blocksum   shared-memory accesses and barriers of blockSum
#   sass_nofma      axpyRestrict built with --fmad=false
#   arch_help       nvcc's own description of -arch=sm_XY
#   fatbin_contents the SASS (ELF) and PTX images inside the built executable
#   forensic_before / forensic_after   rowsums.cu built two ways (forensic evidence)
set -u
cd "$(dirname "$0")"
status=0
step() {  # step <name> <listing> <toolchain line> <hardware note or -> <command>
    local name="$1" listing="$2" tc="$3" hw="$4" cmd="$5"
    {
        echo "listing:   $listing"
        echo "toolchain: $tc"
        echo "command:   $cmd"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        if [ "$hw" != "-" ]; then echo "hardware:  $hw"; fi
    } > "$name.log"
    bash -c "$cmd" > "$name.out" 2>&1
    local rc=$?
    echo "exit code: $rc" >> "$name.log"
    if [ "$rc" -ne 0 ]; then status=1; fi
}
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
C="compiled only: no GPU is needed to read generated code"
FMT="sed -E 's#^[[:space:]]+/\\*([0-9a-f]+)\\*/[[:space:]]+(.*[^[:space:]])[[:space:]]*;.*#\\1  \\2#' | grep -v -E '  NOP$'"
SASSLINES="grep -E '^[[:space:]]+/\\*[0-9a-f]{4}\\*/'"
nvcc -arch=sm_80 -cubin kernels.cu -o .k.cubin || status=1
step ptx_axpy "kernels.cu" "$NVCC" "$C" \
  "nvcc -arch=sm_80 -ptx kernels.cu -o .k.ptx && for f in _Z9axpyPlainifPKfPf _Z12axpyRestrictifPKfPf; do echo \"--- \$f\"; sed -n \"/entry \$f(/,/^}/p\" .k.ptx | grep -E '^[[:space:]]+(ld|st|fma|mul|add|cvta|mad|setp|mov)'; done"
step sass_axpy "kernels.cu" "$NVCC" "$C" \
  "for f in _Z9axpyPlainifPKfPf _Z12axpyRestrictifPKfPf; do echo \"--- \$f\"; cuobjdump -sass -fun \$f .k.cubin | $SASSLINES | $FMT; done"
step sass_lineinfo "kernels.cu (built with -lineinfo)" "$NVCC" "$C" \
  "nvcc -arch=sm_80 -cubin -lineinfo kernels.cu -o .kl.cubin && nvdisasm -g .kl.cubin | awk '/^\\.text\\._Z12axpyRestrictifPKfPf:/{p=1} p&&/^\\.L_x_/{exit} p' | sed -E 's#File \".*/#File \"#; s#[[:space:]]*;[[:space:]]*\$##'"
step sass_copy4 "kernels.cu" "$NVCC" "$C" \
  "cuobjdump -sass -fun _Z5copy4PK6float4PS_i .k.cubin | $SASSLINES | $FMT | grep -E 'LDG|STG'"
step sass_blocksum "kernels.cu" "$NVCC" "$C" \
  "cuobjdump -sass -fun _Z8blockSumPKfPfi .k.cubin | $SASSLINES | $FMT | grep -E 'LDG|STG|LDS|STS|BAR' | head -n 12; echo '[...]'; echo -n 'BAR.SYNC count: '; cuobjdump -sass -fun _Z8blockSumPKfPfi .k.cubin | grep -c 'BAR.SYNC'"
step sass_nofma "kernels.cu built with --fmad=false" "$NVCC" "$C" \
  "nvcc -arch=sm_80 -cubin --fmad=false kernels.cu -o .kf.cubin && cuobjdump -sass -fun _Z12axpyRestrictifPKfPf .kf.cubin | $SASSLINES | $FMT | grep -E 'LDG|STG|FFMA|FMUL|FADD'"
step arch_help "nvcc --help (excerpt)" "$NVCC" "-" \
  "nvcc --help | grep -A18 -E '^--gpu-architecture ' | sed -n '13,18p'"
step fatbin_contents "kernels.cu built as a program; what the executable carries" "$NVCC" "$C" \
  "nvcc -arch=sm_80 kernels.cu -o kernels_exe && cuobjdump --list-elf --list-ptx kernels_exe | sed -E 's/tmpxft_[0-9a-f_]+-/tmpxft_<temporary-id>-/'"
for v in before after; do
    if [ "$v" = before ]; then opt=""; else opt="-maxrregcount=16"; fi
    step forensic_$v "rowsums.cu built with: nvcc -arch=sm_80 -cubin $opt" "$NVCC" "$C" \
      "nvcc -arch=sm_80 -cubin $opt -Xptxas -v rowsums.cu -o .r.cubin 2>&1 | grep -E 'warning|Used|spill' | sed 's/^ *//'; echo '--- opcode counts'; cuobjdump -sass .r.cubin | $SASSLINES | grep -oE '(LDG|STG|STL|LDL|FFMA|FMUL|FADD|BRA)[.A-Z0-9]*' | sort | uniq -c; echo '--- memory instructions and branches, in order'; cuobjdump -sass .r.cubin | $SASSLINES | $FMT | grep -E 'LDG|STG|STL|LDL|BRA'"
done
rm -f .k.cubin .kl.cubin .kf.cubin .k.ptx .r.cubin kernels_exe
exit $status
