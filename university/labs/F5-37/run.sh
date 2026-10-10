#!/usr/bin/env bash
# DS401 F5-37 extra steps (after run_lab.sh built and ran the listings with sanitizers):
#   bench_O2        Listing 1 rebuilt with -O2 and no sanitizers (the measurement build)
#   fabric_whatif   Listing 2 with the "whatif" argument (forensic answer key)
#   perftest_check  is the perftest package installed here? (it is not)
set -u
cd "$(dirname "$0")"
status=0
ver="$(g++ --version | head -n 1)"
log()   # name listing command rc
{
    {
        echo "listing:   $2"
        echo "toolchain: $ver"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container, $(nproc) CPUs visible)"
        echo "exit code: $4"
    } > "$1.log"
}
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror bench_tcp.cpp -o .bin_bench -pthread || status=1
timeout 60 ./.bin_bench > bench_O2.out 2>&1; rc=$?
log bench_O2 bench_tcp.cpp "g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror bench_tcp.cpp -o bench_tcp -pthread; ./bench_tcp" $rc
[ $rc = 0 ] || status=1

g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined fabric_model.cpp -o .bin_fabric || status=1
./.bin_fabric whatif < fabric_model.in > fabric_whatif.out 2>&1; rc=$?
log fabric_whatif fabric_model.cpp "g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined fabric_model.cpp -o fabric_model; ./fabric_model whatif < fabric_model.in" $rc
[ $rc = 0 ] || status=1
{
    echo "\$ for t in ib_write_bw ib_write_lat ib_send_bw ib_send_lat ib_read_bw ib_read_lat; do command -v \$t || echo \"\$t: not installed\"; done"
    for t in ib_write_bw ib_write_lat ib_send_bw ib_send_lat ib_read_bw ib_read_lat; do command -v $t || echo "$t: not installed"; done
    echo "\$ dpkg-query -W perftest"
    dpkg-query -W perftest 2>&1
} > perftest_check.out
log perftest_check "(shell commands)" "command -v ib_write_bw ...; dpkg-query -W perftest" 0
rm -f .bin_bench .bin_fabric
exit $status
