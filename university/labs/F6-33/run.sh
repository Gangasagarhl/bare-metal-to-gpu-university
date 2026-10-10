#!/usr/bin/env bash
# F6-33 extra lab steps (each writes <name>.out and <name>.log):
#   sass_counts      Listing 2 (count_sass.sh) on the SASS of roofline.cu for sm_80
#   roofline_points  Listing 3 on those counts
#   resources        ptxas resource report (registers, spills) for roofline.cu
#   sass_normalize   the fast paths of the two forensic kernels, real SASS for sm_80
#   sass_poly4       the real SASS of poly<4> (the Worked example reads it)
#   prec_div_help    nvcc's own description of --prec-div
#   sass_fastdiv     divideEach compiled with --prec-div=false
#   forensic         Listing 3 on forensic.in (fast-path counts read from sass_normalize)
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
GXX="$(g++ --version | head -n 1)"
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
COMPILED="compiled only: no GPU is needed to read generated code"
FMT="sed -E 's#^[[:space:]]+/\\*([0-9a-f]+)\\*/[[:space:]]+(.*[^[:space:]])[[:space:]]*;.*#\\1  \\2#'"
step sass_counts "count_sass.sh on roofline.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin roofline.cu -o .tmp.cubin && ./count_sass.sh .tmp.cubin"
step roofline_points "roofline_model.cpp with sass_counts.out" "$GXX" "model only: TG-1 is invented" \
  "g++ $FLAGS roofline_model.cpp -o .bin_roof && ./.bin_roof < sass_counts.out"
step resources "roofline.cu, ptxas resource report" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin -Xptxas -v roofline.cu -o .tmp.cubin 2>&1 | grep -E 'Compiling|Used|spill'"
step sass_normalize "normalize.cu" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin normalize.cu -o .tmp2.cubin && for f in _Z15multiplyInverseifPKfPf _Z10divideEachifPKfPf; do echo \"--- \$f\"; cuobjdump -sass -fun \$f .tmp2.cubin | grep -E '^[[:space:]]+/\\*[0-9a-f]{4}\\*/' | $FMT | grep -v -E '  NOP$' | head -n 27; done"
step sass_poly4 "roofline.cu, SASS of poly<4> for sm_80" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin roofline.cu -o .tmp.cubin && cuobjdump -sass -fun _Z4polyILi4EEviPKfPf .tmp.cubin | grep -E '^[[:space:]]+/\\*[0-9a-f]{4}\\*/' | $FMT | grep -v -E '  NOP$'"
step prec_div_help "nvcc --help (excerpt)" "$NVCC" "-" \
  "nvcc --help | grep -A5 -E '^--prec-div '"
step sass_fastdiv "normalize.cu built with --prec-div=false" "$NVCC" "$COMPILED" \
  "nvcc -arch=sm_80 -cubin --prec-div=false normalize.cu -o .tmp3.cubin && cuobjdump -sass -fun _Z10divideEachifPKfPf .tmp3.cubin | grep -E '^[[:space:]]+/\\*[0-9a-f]{4}\\*/' | $FMT | grep -v -E '  NOP$'"
step forensic "roofline_model.cpp with forensic.in" "$GXX" "model only: TG-1 is invented" \
  "./.bin_roof < forensic.in"
rm -f .tmp.cubin .tmp2.cubin .tmp3.cubin .bin_roof
exit $status
