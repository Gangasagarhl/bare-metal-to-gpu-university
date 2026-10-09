#!/usr/bin/env bash
# Extra runs for F5-24: a real OCI container started with runc, with no image registry
# and no internet. The "image" is a folder (rootfs) holding one static program.
#   runc_help        the commands runc lists in its own help text
#   runc_spec        the config.json that `runc spec` generates, summarised by bundle.py
#   host_view        inside.cc run directly on the host
#   container_view   the same binary run by runc in its own namespaces
#   oom_host         eat.cc on the host (no limit)
#   oom_container    eat.cc in a container whose config sets a 64 MiB memory limit
#   container_limit  inside.cc in a container with the same limit: the limit seen from inside
#   oom_cgroup       eat.cc in a hand-made cgroup v1 memory group with the same limit, plus
#                    the kernel's counters for that group
# Binaries and the bundle folder are deleted at the end.
set -u
GXX="$(g++ --version | head -n 1)"
RUNC="$(runc --version 2>&1 | head -n 1) ($(runc --version 2>&1 | grep '^spec' ))"
FLAGS="-std=c++20 -O2 -static -Wall -Wextra -Wpedantic -Werror"
hdr() {   # hdr <name> <listing> <toolchain> <command>
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container, running as root)"
    } > "$1.log"
}
status=0
B=.bundle
rm -rf "$B"; mkdir -p "$B/rootfs"/{proc,sys,dev,tmp}
for p in inside eat; do
    if ! g++ $FLAGS "$p.cc" -o "$B/rootfs/$p" > .build.txt 2>&1; then
        hdr "$p" "$p.cc" "$GXX" "g++ $FLAGS $p.cc"; echo "result:    BUILD FAILED" >> "$p.log"
        cat .build.txt >> "$p.log"; rm -rf .build.txt "$B"; exit 1
    fi
done
rm -f .build.txt

# 0. the runtime's own list of commands
hdr runc_help "(none: the tool's help text)" "$RUNC" "runc --help   (COMMANDS section only)"
runc --help 2>&1 | sed -n '/^COMMANDS/,/^GLOBAL/p' | sed '$d' > runc_help.out; rc=${PIPESTATUS[0]}
echo "exit code: $rc" >> runc_help.log; [ "$rc" = 0 ] || status=1

# 1. the generated configuration
hdr runc_spec "bundle.py" "$RUNC; $(python3 --version)" "cd bundle; runc spec; python3 bundle.py summary config.json"
( cd "$B" && runc spec ) > .spec.txt 2>&1 && python3 bundle.py summary "$B/config.json" > runc_spec.out 2>&1
rc=$?; echo "exit code: $rc" >> runc_spec.log; [ "$rc" = 0 ] || { cat .spec.txt >> runc_spec.out; status=1; }
rm -f .spec.txt

# 2. the same program, on the host and in a container
hdr host_view inside.cc "$GXX" "g++ $FLAGS inside.cc -o rootfs/inside; ./rootfs/inside"
"$B/rootfs/inside" > host_view.out 2>&1; rc=$?; echo "exit code: $rc" >> host_view.log; [ "$rc" = 0 ] || status=1

python3 bundle.py run "$B/config.json" /inside box1
hdr container_view inside.cc "$RUNC" "python3 bundle.py run config.json /inside box1; runc run ds303-view"
( cd "$B" && timeout 30 runc run ds303-view ) > container_view.out 2>&1; rc=$?
echo "exit code: $rc" >> container_view.log; [ "$rc" = 0 ] || status=1

# 3. the forensic pair: no limit on the host, 64 MiB inside the container
hdr oom_host eat.cc "$GXX" "g++ $FLAGS eat.cc -o rootfs/eat; ./rootfs/eat"
"$B/rootfs/eat" > oom_host.out 2>&1; rc=$?; echo "exit code: $rc" >> oom_host.log; [ "$rc" = 0 ] || status=1

python3 bundle.py run "$B/config.json" /eat worker 67108864
hdr oom_container eat.cc "$RUNC" "python3 bundle.py run config.json /eat worker 67108864; runc run ds303-eat; echo \"exit status \$?\""
( cd "$B" && timeout 30 runc run ds303-eat ) > oom_container.out 2>&1; rc=$?
echo "runc exit status: $rc" >> oom_container.out
echo "exit code: $rc (expected: the container's process is killed; see the chapter)" >> oom_container.log
echo "result:    exit status recorded as evidence for the forensic lab" >> oom_container.log
python3 bundle.py summary "$B/config.json" | grep resources >> oom_container.out
python3 bundle.py run "$B/config.json" /inside worker 67108864
hdr container_limit inside.cc "$RUNC" "python3 bundle.py run config.json /inside worker 67108864; runc run ds303-limit"
( cd "$B" && timeout 30 runc run ds303-limit ) > container_limit.out 2>&1; rc=$?
echo "exit code: $rc" >> container_limit.log; [ "$rc" = 0 ] || status=1
runc delete -f ds303-view > /dev/null 2>&1; runc delete -f ds303-eat > /dev/null 2>&1; runc delete -f ds303-limit > /dev/null 2>&1

# 4. the same limit set by hand in a cgroup v1 memory group, to read the kernel's counters
CG=/sys/fs/cgroup/memory/ds303-oom
hdr oom_cgroup eat.cc "$GXX; Linux $(uname -r) cgroup v1 memory controller" \
    "mkdir $CG; echo 67108864 > $CG/memory.limit_in_bytes; sh -c 'echo \$\$ > $CG/cgroup.procs; exec ./eat'; cat counters"
if mkdir -p "$CG" 2>/dev/null && echo 67108864 > "$CG/memory.limit_in_bytes" 2>/dev/null; then
    sh -c "echo \$\$ > $CG/cgroup.procs && exec $B/rootfs/eat" > oom_cgroup.out 2>&1; rc=$?
    {
        echo "exit status of eat: $rc"
        echo "memory.limit_in_bytes:     $(cat $CG/memory.limit_in_bytes)"
        echo "memory.max_usage_in_bytes: $(cat $CG/memory.max_usage_in_bytes)"
        echo "memory.failcnt:            $(cat $CG/memory.failcnt)"
        echo "memory.oom_control:"; sed 's/^/  /' "$CG/memory.oom_control"
    } >> oom_cgroup.out
    echo "exit code: $rc (expected: killed by the kernel; counters printed after it)" >> oom_cgroup.log
    sleep 1; rmdir "$CG" 2>/dev/null
else
    echo "(not run: the memory cgroup could not be created here)" > oom_cgroup.out
    echo "result:    untested in this environment: no writable cgroup v1 memory controller" >> oom_cgroup.log
fi
rm -rf "$B"
exit $status
