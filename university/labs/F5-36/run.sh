#!/usr/bin/env bash
# DS401 F5-36 extra steps (after run_lab.sh built and ran the model listings):
#   environment      what this machine offers for RDMA: packages, headers, devices
#   verbs_pingpong   Listing 1 (real libibverbs) built and started as the server;
#                    with no RDMA device it stops at ibv_get_device_list (untested on hardware)
set -u
cd "$(dirname "$0")"
status=0
ver="$(g++ --version | head -n 1)"
log()   # name listing toolchain command rc [hardware]
{
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        [ -n "${6:-}" ] && echo "hardware:  $6"
        echo "exit code: $5"
    } > "$1.log"
}
{
    echo "\$ dpkg-query -W libibverbs-dev libibverbs1 ibverbs-providers librdmacm1t64"
    dpkg-query -W libibverbs-dev libibverbs1 ibverbs-providers librdmacm1t64 2>&1
    echo "\$ ls /usr/include/infiniband"
    ls /usr/include/infiniband | tr '\n' ' '; echo
    echo "\$ ls /usr/lib/x86_64-linux-gnu/libibverbs   (user-space provider drivers)"
    ls /usr/lib/x86_64-linux-gnu/libibverbs 2>&1 | tr '\n' ' '; echo
    echo "\$ ls /sys/class/infiniband"
    ls /sys/class/infiniband 2>&1
    echo "\$ ls /lib/modules"
    ls /lib/modules 2>&1
} > environment.out
log environment "(shell commands)" "bash $(bash --version | head -n 1 | cut -d' ' -f4)" \
    "dpkg-query -W ...; ls /usr/include/infiniband; ls /usr/lib/x86_64-linux-gnu/libibverbs; ls /sys/class/infiniband; ls /lib/modules" 0

cmd="g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined verbs_pingpong.cc -o verbs_pingpong -libverbs"
if g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined verbs_pingpong.cc \
       -o .bin_verbs_pingpong -libverbs > verbs_pingpong.out 2>&1; then
    timeout 10 ./.bin_verbs_pingpong > verbs_pingpong.out 2>&1; rc=$?
    log verbs_pingpong verbs_pingpong.cc "$ver; libibverbs-dev $(dpkg-query -W -f='${Version}' libibverbs-dev)" \
        "$cmd; ./verbs_pingpong   (server role)" "$rc (2 = no RDMA device found, the expected result here)" \
        "untested on hardware: the build container has no RDMA device (no NIC, no Soft-RoCE); compiled and linked for real, stopped at ibv_get_device_list"
    [ "$rc" = 2 ] || status=1
else
    log verbs_pingpong verbs_pingpong.cc "$ver" "$cmd" "BUILD FAILED"
    echo "result:    BUILD FAILED" >> verbs_pingpong.log
    status=1
fi
rm -f .bin_verbs_pingpong
exit $status
