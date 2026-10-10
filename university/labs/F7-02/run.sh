#!/usr/bin/env bash
# F7-02 extra lab steps (no GPU needed):
#   system          - the facts a compatibility check starts from: OS, kernel, where ROCm came from
#   check           - the readiness checklist check_rocm.sh on this machine
#   devquery_nvidia - devquery.hip built through HIP's NVIDIA back end, then run
#   bundle_targets  - which GPU targets a build contains: one target, then two
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
    sed -i "s#$(pwd)/##g; s#file://##g" "$name.out"
    echo "exit code: $rc" >> "$name.log"
    return $rc
}
HIPV="$(hipcc --version 2>/dev/null | head -n 1)"
NVV="$(nvcc --version | tail -n 2 | head -n 1)"
NOGPU="untested on hardware: the build container has no GPU (AH-26); the build is real, the run shows the runtime's own error"
step system "-" "dpkg-query, apt-cache, uname" "-" \
  "echo \"OS: \$(. /etc/os-release && echo \$PRETTY_NAME)\"; echo \"kernel release: \$(uname -r)\"; echo 'ROCm packages installed (name version):'; dpkg-query -W -f='  \${Package} \${Version}\n' hipcc libamdhip64-5 libamdhip64-dev libhsa-runtime64-1 rocminfo rocm-device-libs-17 libamd-comgr2 2>/dev/null; echo \"archive section of hipcc: \$(apt-cache policy hipcc 2>/dev/null | grep -m1 -oE '[a-z]+/(main|universe|restricted|multiverse)')\"; if [ -d /opt/rocm ]; then echo '/opt/rocm: present'; else echo '/opt/rocm: absent (packages install under /usr)'; fi; echo \"hipconfig --rocmpath: \$(hipconfig --rocmpath 2>/dev/null)\"" || status=1
# expected to report FAIL lines here (exit code 1): this container has no GPU
step check "check_rocm.sh" "$HIPV; rocminfo package $(dpkg-query -W -f='${Version}' rocminfo 2>/dev/null)" \
  "untested on hardware: run on a machine without a GPU; the FAIL lines are this container's real state" \
  "./check_rocm.sh"
step devquery_nvidia "devquery.hip" "$HIPV with HIP_PLATFORM=nvidia; $NVV" "$NOGPU" \
  "HIP_PLATFORM=nvidia CUDA_PATH=/usr hipcc -std=c++17 -x cu devquery.hip -o .tmp_dq_nv && ./.tmp_dq_nv"
step bundle_targets "devquery.hip" "$HIPV" "compiled only: no GPU is needed to list a bundle" \
  "hipcc -std=c++17 -O2 --offload-arch=gfx90a devquery.hip -o .tmp_one && echo 'built with --offload-arch=gfx90a:' && roc-obj-ls .tmp_one | sed -E 's/#offset=.*//' && hipcc -std=c++17 -O2 --offload-arch=gfx90a --offload-arch=gfx1030 devquery.hip -o .tmp_two && echo 'built with --offload-arch=gfx90a --offload-arch=gfx1030:' && roc-obj-ls .tmp_two | sed -E 's/#offset=.*//'" || status=1
rm -f .tmp_dq_nv .tmp_one .tmp_two
exit $status
