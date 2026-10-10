#!/usr/bin/env bash
# F6-40 lab steps (no GPU, no Triton, no PyTorch in this build).
#   tile_model    - Listing 2, the NumPy block-program model: reduction, softmax, matmul
#   forensic      - Listing 2 with the softmax fill value of the forensic lab
#   pycheck       - Listing 1 compiled to Python byte code (syntax check only; nothing imported)
#   triton_import - records that Triton is not installed here (expected to fail)
#   loc           - Listing 3: lines of code of the softmax kernels in CUDA (F6-38) and in Triton
set -u
cd "$(dirname "$0")"
status=0
step() {  # step <name> <listing> <toolchain line> <hardware note or -> <command> [may-fail]
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
    if [ "$rc" -ne 0 ] && [ "${6:-}" != "may-fail" ]; then status=1; fi
}
PY="$(python3 --version 2>&1)"
step tile_model "tile_model.py" "$PY" "-" "python3 -I tile_model.py"
step forensic "tile_model.py" "$PY" "-" "python3 -I tile_model.py --forensic"
step pycheck "triton_kernels.py" "$PY" "untested: Triton, PyTorch and a GPU are not available in this build; syntax check only" \
  "python3 -I -c \"import py_compile; py_compile.compile('triton_kernels.py', cfile='.tmp_tk.pyc', doraise=True); print('triton_kernels.py: valid Python syntax (not imported, not run)')\""
step triton_import "-" "$PY" "untested: Triton is not installed in this build" \
  "python3 -I -c 'import triton'" may-fail
step loc "loc.py" "$PY" "-" \
  "python3 -I loc.py 'CUDA softmax (F6-38):../F6-38/softmax_fused.cu:mergeMaxSum,softmaxRows' 'Triton softmax (Listing 1):triton_kernels.py:softmax_kernel,softmax' 'Triton matmul (Listing 1):triton_kernels.py:matmul_kernel,matmul' 'CUDA attention (F6-39):../F6-39/attn_fwd.cu:attnForward'"
rm -f .tmp_tk.pyc
exit $status
