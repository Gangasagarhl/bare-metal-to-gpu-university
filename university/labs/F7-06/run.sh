#!/usr/bin/env bash
# F7-06 extra lab steps (no GPU needed). Build trees go to a temporary folder.
#   hipcc_driver    - what hipcc really runs (HIPCC_VERBOSE=1) for one file
#   hipcc_default   - a build WITHOUT --offload-arch: which GPU target is inside?
#   cmake_configure - proj/ configured with find_package(hip), GPU_TARGETS=gfx90a;gfx1030
#   cmake_build     - proj/ built; the compile lines and the bundle of the executable
#   cmake_run       - the CMake-built program run
#   cmake_notargets - proj/ configured and built with GPU_TARGETS left empty (forensic)
#   cmake_hiplang   - proj_hiplang/ with CMake's own HIP language (fails in this container)
#   cmake_nvidia    - the same proj/ with -DHIP_BACKEND=NVIDIA: configured, built (nvcc line shown)
#   cmake_nvidia_run - the NVIDIA-back-end executable run
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
    sed -i "s#$(pwd)/##g; s#$T/##g; s#$T#<build>#g; s#file://##g" "$name.out"
    echo "exit code: $rc" >> "$name.log"
    return $rc
}
T="$(mktemp -d)"
export T
HIPV="$(hipcc --version 2>/dev/null | head -n 1)"
CMV="$(cmake --version | head -n 1)"
COMPILED="compiled only: no GPU is needed to read a build"
NOGPU="untested on hardware: the build container has no GPU (AH-26); the build is real, the run shows the runtime's own error"
BUNDLE="sed -E 's/#offset=.*//'"
step hipcc_driver "proj/src/saxpy_kernel.hip" "$HIPV" "$COMPILED" \
  "HIPCC_VERBOSE=1 hipcc -std=c++17 -O2 --offload-arch=gfx90a -Iproj/src -c proj/src/saxpy_kernel.hip -o \$T/k.o 2>&1 | sed -E -e 's/^hipcc-cmd: //' -e \"s#\$T/##g\" | tr -s ' '" || status=1
step hipcc_default "proj/src/saxpy_kernel.hip, proj/src/main.cpp" "$HIPV" "$COMPILED" \
  "hipcc -std=c++17 -O2 -Iproj/src proj/src/saxpy_kernel.hip proj/src/main.cpp -o \$T/nodefault && roc-obj-ls \$T/nodefault | $BUNDLE" || status=1
step cmake_configure "proj/CMakeLists.txt" "$CMV; clang++-17 as CMAKE_CXX_COMPILER; $HIPV" "-" \
  "cmake -S proj -B \$T/b -DCMAKE_CXX_COMPILER=clang++-17 -DGPU_TARGETS='gfx90a;gfx1030' -DCMAKE_BUILD_TYPE=Release" || status=1
step cmake_build "proj/CMakeLists.txt" "$CMV; $HIPV" "$COMPILED" \
  "cmake --build \$T/b -v 2>&1 | grep -E '^/usr/bin/clang\+\+-17' | sed -E -e 's# -MD -MT [^ ]+ -MF [^ ]+##' -e \"s#\$(pwd)/##g\" | tr -s ' ' && echo && echo 'bundle of the executable:' && roc-obj-ls \$T/b/saxpy | $BUNDLE" || status=1
step cmake_run "proj (CMake build)" "$CMV; $HIPV" "$NOGPU" "\$T/b/saxpy"
step cmake_notargets "proj/CMakeLists.txt" "$CMV; $HIPV" "$COMPILED" \
  "cmake -S proj -B \$T/n -DCMAKE_CXX_COMPILER=clang++-17 > /dev/null && cmake --build \$T/n > /dev/null && grep -E '^GPU_TARGETS' \$T/n/CMakeCache.txt && echo 'bundle of the executable:' && roc-obj-ls \$T/n/saxpy | $BUNDLE" || status=1
# expected to fail (exit code 1) in this container; the output is CMake's own message
step cmake_hiplang "proj_hiplang/CMakeLists.txt" "$CMV; $HIPV" "-" \
  "cmake -S proj_hiplang -B \$T/h 2>&1 | sed -n '/CMake Error/,/Call Stack/p'"
NVV="$(nvcc --version | tail -n 2 | head -n 1)"
step cmake_nvidia "proj/CMakeLists.txt" "$CMV; $NVV; $HIPV headers" "$COMPILED" \
  "cmake -S proj -B \$T/v -DHIP_BACKEND=NVIDIA -DCMAKE_CUDA_ARCHITECTURES=80 -DCMAKE_BUILD_TYPE=Release > /dev/null && cmake --build \$T/v -v 2>&1 | grep -E '^/usr/bin/nvcc' | sed -E -e 's# -MD -MT [^ ]+ -MF [^ ]+##' -e \"s#\$(pwd)/##g\" | tr -s ' ' && echo && echo 'GPU code in the executable (cuobjdump --list-elf):' && cuobjdump --list-elf \$T/v/saxpy | sed -E 's/tmpxft_[0-9a-f]+_[0-9]+-//'" || status=1
step cmake_nvidia_run "proj (CMake build, HIP_BACKEND=NVIDIA)" "$CMV; $NVV" "$NOGPU" "\$T/v/saxpy"
rm -rf "$T"
exit $status
