#!/usr/bin/env bash
# F7-20 extra lab steps.
#   macros        - which macros exist in the host pass and in each device pass (hipcc and nvcc)
#   fatbin        - the code objects inside one AMD binary built for gfx90a and gfx1100
#   nv_ballot_v0  - the first try (plain __ballot) through HIP's NVIDIA path: expected to fail
#   nv_build      - Listing 1 through HIP's NVIDIA path for sm_80: build, run, SASS vote/atomic
#   half_isa      - forensic evidence: metadata of Priya's histogram kernel for gfx90a
set -u
cd "$(dirname "$0")"
status=0
step() {  # step <name> <listing> <toolchain line> <hardware note or -> <expect ok|fail|any> <command>
    local name="$1" listing="$2" tc="$3" hw="$4" expect="$5" cmd="$6"
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
    sed -i -E '/argument unused during compilation/d; s#/tmp/tmpxft_[0-9a-f_]+-[0-9]+_#<tmp>/#g' "$name.out"
    if [ "$expect" = "fail" ]; then
        if [ "$rc" -ne 0 ]; then
            echo "result:    compile failed as expected (messages saved in $name.out)" >> "$name.log"
        else
            echo "result:    UNEXPECTED SUCCESS (compile was expected to fail)" >> "$name.log"; status=1
        fi
    else
        echo "exit code: $rc" >> "$name.log"
        if [ "$rc" -ne 0 ] && [ "$expect" = "ok" ]; then status=1; fi
    fi
}
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
CLANG="$(clang++-17 --version | head -n 1) (the compiler that hipcc drives; HIP 5.7)"
COMPILED="compiled only: no GPU is needed to read generated code"
PAT="__HIP_PLATFORM_AMD__|__HIP_PLATFORM_NVIDIA__|__HIP_DEVICE_COMPILE__|__AMDGCN_WAVEFRONT_SIZE|__gfx[0-9a-f]+__|__CUDA_ARCH__"

step macros "portable.hip" "$CLANG; $NVCC" "$COMPILED" ok \
  "echo '--- hipcc host pass'; hipcc -std=c++17 --offload-arch=gfx90a --cuda-host-only -dM -E portable.hip 2>/dev/null | grep -E '#define ($PAT) ' | sort; for t in gfx90a gfx1100; do echo \"--- hipcc device pass \$t\"; hipcc -std=c++17 --offload-arch=\$t --cuda-device-only -dM -E portable.hip 2>/dev/null | grep -E '#define ($PAT) ' | sort; done; echo '--- nvcc device pass sm_80 (value of __CUDA_ARCH__ printed by a test macro)'; printf '#include <cstdio>\n__global__ void k(int* o){\n#ifdef __CUDA_ARCH__\n*o = __CUDA_ARCH__;\n#endif\n}\nint main(){\n#ifdef __CUDA_ARCH__\nstd::puts(\"host pass sees __CUDA_ARCH__\");\n#else\nstd::puts(\"host pass: __CUDA_ARCH__ not defined\");\n#endif\n}\n' > .tmp_m.cu && nvcc -arch=sm_80 .tmp_m.cu -o .tmp_m && ./.tmp_m && nvcc -arch=sm_80 -ptx .tmp_m.cu -o - | grep -E 'mov.u32.*800;'"
step fatbin "portable.hip" "$CLANG" "untested on hardware: no AMD GPU (AH-26); the listing of code objects is real, the run shows the runtime's own error" any \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a --offload-arch=gfx1100 portable.hip -o .tmp_p 2>/dev/null && objcopy --dump-section .hip_fatbin=.tmp_fb .tmp_p && /usr/lib/llvm-17/bin/clang-offload-bundler --list --type=o --input=.tmp_fb && echo '--- run' && ./.tmp_p"
step nv_ballot_v0 "ballot_v0.hip.inc" "$NVCC (with the HIP 5.7 headers, NVIDIA platform)" "$COMPILED" fail \
  "nvcc -std=c++17 -O2 -arch=sm_80 -x cu -D__HIP_PLATFORM_NVIDIA__ -c ballot_v0.hip.inc -o .tmp_v0.o"
step nv_build "portable.hip" "$NVCC (with the HIP 5.7 headers, NVIDIA platform)" \
  "untested on hardware: the build container has no NVIDIA GPU (AH-26); the build and SASS are real, the run shows the runtime's own error" any \
  "nvcc -std=c++17 -O2 -arch=sm_80 -x cu -D__HIP_PLATFORM_NVIDIA__ portable.hip -o .tmp_nv && { ./.tmp_nv; echo \"(program exit code \$?)\"; } && echo 'SASS of countPositive (vote, popcount, atomic):' && cuobjdump -sass .tmp_nv | grep -o -E 'VOTE[.A-Z]*|POPC|RED[.A-Z0-9]*|ATOM[.A-Z0-9]*' | sort | uniq -c | sed -E 's/^ +//'"
step half_isa "half_lanes.hip.inc" "$CLANG" "$COMPILED" ok \
  "hipcc -x hip -std=c++17 -O2 --offload-arch=gfx90a --cuda-device-only -S half_lanes.hip.inc -o .tmp.s && grep -E '^[[:space:]-]+\\.(name|max_flat_workgroup_size|wavefront_size|group_segment_fixed_size|vgpr_count):' .tmp.s | sed -E 's/^[[:space:]-]+//'"
rm -f .tmp.s .tmp_p .tmp_fb .tmp_nv .tmp_v0.o .tmp_m .tmp_m.cu
exit $status
