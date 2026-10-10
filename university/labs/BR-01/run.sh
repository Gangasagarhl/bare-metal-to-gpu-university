#!/usr/bin/env bash
# BR-01 extra lab steps (run by run_lab.sh after the .cpp/.cu listings):
#   step2_nvcc - Listing 1 unchanged, copied to a .cu file and built by nvcc instead of g++
#   split      - nvcc --dryrun on Listing 5: the tools nvcc runs, one line per tool
#   fatbin     - what the finished executable carries: device code images and the host stub
#   diffs      - each deliberate break, as a diff against Listing 5
#   device_library_check - Listing 6 without its two refused lines, and with the relaxed-constexpr flag
#   nvcc_help  - nvcc's own help text for the options this chapter mentions
set -u
cd "$(dirname "$0")"
status=0
step() {  # step <name> <listing> <toolchain line> <hardware note or -> <command> [allow-nonzero]
    local name="$1" listing="$2" tc="$3" hw="$4" cmd="$5" allow="${6:-no}"
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
    if [ "$rc" -ne 0 ] && [ "$allow" != "allow-nonzero" ]; then status=1; fi
}
NVCC="$(nvcc --version | tail -n 2 | head -n 1)"
CPUONLY="runs on the CPU only: the program has no kernel launch and no CUDA runtime call, so no GPU is needed"

step step2_nvcc "step1_cpu.cpp (copied unchanged to step2_same.cu)" "$NVCC" "$CPUONLY" \
  "cp step1_cpu.cpp .step2_same.cu && nvcc -std=c++17 -O2 -Werror all-warnings .step2_same.cu -o .bin_step2 && ./.bin_step2"

# One line per tool that nvcc would run (filtered by split_filter.py).
step split "step5_kernel.cu" "$NVCC" "compile-only listing of tool steps; nothing is executed" \
  "nvcc -std=c++17 --dryrun step5_kernel.cu -o step5_kernel 2>&1 | python3 -I split_filter.py"

step fatbin "step5_kernel.cu" "$NVCC; $(cuobjdump --version | tail -n 2 | head -n 1) (cuobjdump); $(nm --version | head -n 1)" \
  "untested on hardware: the executable is built and inspected; it is not run" \
  "nvcc -std=c++17 -O2 step5_kernel.cu -o fat_kernel && echo '--- cuobjdump --list-elf --list-ptx fat_kernel' && cuobjdump --list-elf --list-ptx fat_kernel && echo '--- nm -C fat_kernel: host symbols that mention vecAdd (addresses removed)' && nm -C fat_kernel | grep 'vecAdd' | grep -v 'vecAddCpu' | sed -E 's/^[0-9a-f]+ //'"

step diffs "break1_no_qualifier.cu, break2_exception.cu, break_host_pointer.cu against step5_kernel.cu" \
  "$(diff --version | head -n 1)" "-" \
  "for b in break1_no_qualifier break2_exception break_host_pointer; do diff -U1 --label step5_kernel.cu --label \$b.cu step5_kernel.cu \$b.cu; done" allow-nonzero

step device_library_check "device_library.cu (two variants)" "$NVCC" "compile-only; nothing is executed" \
  "sed '15,16d' device_library.cu > .dl_ok.cu && nvcc -std=c++17 -O2 -Werror all-warnings -c .dl_ok.cu -o .dl_ok.o && echo 'variant A, lines 15 and 16 deleted: compiled without errors or warnings (sqrt and printf kept)' && nvcc -std=c++17 -O2 -Werror all-warnings --expt-relaxed-constexpr -c device_library.cu -o .dl_relaxed.o && echo 'variant B, all four lines, with --expt-relaxed-constexpr: compiled without errors or warnings'"

step nvcc_help "(tool) nvcc --help" "$NVCC" "-" \
  "nvcc --help | grep -E -A3 '^--(dryrun|expt-relaxed-constexpr|x) '"

rm -f .bin_step2 .bin_split fat_kernel .step2_same.cu .dl_ok.cu .dl_ok.o .dl_relaxed.o
exit $status
