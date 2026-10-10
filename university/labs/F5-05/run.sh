#!/usr/bin/env bash
# Extra runs for F5-05: the course lab "a TCP echo server and client".
# Inside a private network namespace (unshare -n): build net/echo_server.cpp and
# net/echo_client.cpp with the course flags, start the server for three clients, then run
# the client with 13 bytes, 1 MiB and 100 MiB and check the checksums.
set -u
CXXFLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
GXX="$(g++ --version | head -n 1)"
header() {   # header <name> <listing> <command>
    {
        echo "listing:   $2"
        echo "toolchain: $GXX"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "network:   private network namespace (unshare -n), loopback interface only"
    } > "$1.log"
}
if [ -z "${F5_IN_NETNS:-}" ]; then
    if unshare -n true 2>/dev/null; then
        F5_IN_NETNS=1 exec unshare -n "$0"
    fi
    for n in echo_server echo_client; do
        header "$n" "net/$n.cpp" "(not run)"
        echo "result:    untested in this environment: unshare -n was not permitted" >> "$n.log"
        echo "(not run)" > "$n.out"
    done
    exit 0
fi
python3 -c "import socket,fcntl,struct; fcntl.ioctl(socket.socket(),0x8914,struct.pack('16sH14s',b'lo',0x41,b''))"
for n in echo_server echo_client; do
    if ! g++ $CXXFLAGS "net/$n.cpp" -o ".bin_$n" > ".build_$n.txt" 2>&1; then
        header "$n" "net/$n.cpp" "g++ $CXXFLAGS net/$n.cpp -o $n"
        echo "result:    BUILD FAILED" >> "$n.log"; cat ".build_$n.txt" >> "$n.log"; exit 1
    fi
    rm -f ".build_$n.txt"
done
header echo_server "net/echo_server.cpp (with net/socket.hpp)" \
    "g++ $CXXFLAGS net/echo_server.cpp -o echo_server; ./echo_server 7100 3"
header echo_client "net/echo_client.cpp (with net/socket.hpp)" \
    "g++ $CXXFLAGS net/echo_client.cpp -o echo_client; ./echo_client localhost 7100 <bytes> for 13, 1048576 and 104857600 bytes"
./.bin_echo_server 7100 3 > echo_server.out 2>&1 &
srv=$!
for _ in $(seq 100); do grep -q listening echo_server.out 2>/dev/null && break; sleep 0.05; done
status=0
: > echo_client.out
for bytes in 13 1048576 104857600; do
    echo "\$ ./echo_client localhost 7100 $bytes" >> echo_client.out
    ./.bin_echo_client localhost 7100 "$bytes" >> echo_client.out 2>&1 || status=1
done
wait "$srv"; rc=$?
echo "exit code: $rc" >> echo_server.log
echo "exit code: $status (0 = all three clients passed)" >> echo_client.log
rm -f .bin_echo_server .bin_echo_client
[ "$rc" = 0 ] || status=1
exit $status
