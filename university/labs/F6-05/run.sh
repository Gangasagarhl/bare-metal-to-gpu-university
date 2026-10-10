#!/usr/bin/env bash
# F6-05 extra lab steps:
#   sanitizer - Compute Sanitizer on oob.cu, attempted in the build container
#   sanitizer_help - the tool's own list of sub-tools and two options used in the chapter
#   header    - the CUDA 12.0 header's own words for the error codes used in this chapter
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
SAN="$(compute-sanitizer --version 2>&1 | grep -i version | head -n 1) (Compute Sanitizer); $NVCC"
NOGPU="untested on hardware: no GPU and no sanitizer injection library in the build container; this is the tool's real output"
step sanitizer "oob.cu" "$SAN" "$NOGPU" \
  "nvcc -std=c++17 -O2 -lineinfo oob.cu -o .bin_oob && compute-sanitizer --tool memcheck ./.bin_oob" allow-nonzero
step header "(header) /usr/include/driver_types.h" "CUDA 12.0 header, Ubuntu package nvidia-cuda-dev $(dpkg-query -W -f='${Version}' nvidia-cuda-dev 2>/dev/null)" "-" \
  "for e in cudaErrorInvalidConfiguration cudaErrorIllegalAddress cudaErrorLaunchFailure cudaErrorNoDevice; do awk -v e=\"\$e\" '/\\/\\*\\*/{buf=\"\"} {buf=buf \$0 \"\\n\"} \$0 ~ \"^ *\" e \" *=\" {printf \"%s\\n\", buf}' /usr/include/driver_types.h | sed -E 's/^ +//'; done"
step sanitizer_help "(tool) compute-sanitizer --help" "$SAN" "-" \
  "compute-sanitizer --help | grep -E -A5 '^ +--tool ' && compute-sanitizer --help | grep -E '^ +--(leak-check|report-api-errors) '"
rm -f .bin_oob
exit $status
