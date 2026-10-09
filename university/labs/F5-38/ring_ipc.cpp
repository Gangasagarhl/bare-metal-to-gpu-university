// ring_ipc.cpp - DS401 F5-38, Listing 1: two processes exchange 64-byte messages
// (a) through a socket pair (kernel path: one system call per send and per receive, and
//     the kernel copies the bytes into and out of its buffers), and
// (b) through two shared-memory rings polled by both sides (no system call on the data path).
// For each path: ping-pong latency (round trip) and one-way message rate.
// Usage: ring_ipc            both paths     ring_ipc socket | ring_ipc ring   (one path, for strace)
#include "spsc_ring.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
constexpr int kPingPongs = 20000;
constexpr int kStream = 200000;

bool readAll(int fd, char* p, size_t n)
{
    while (n > 0) {
        const ssize_t r = ::read(fd, p, n);
        if (r <= 0) {
            return false;
        }
        p += r;
        n -= static_cast<size_t>(r);
    }
    return true;
}

bool writeAll(int fd, const char* p, size_t n)
{
    while (n > 0) {
        const ssize_t r = ::write(fd, p, n);
        if (r <= 0) {
            return false;
        }
        p += r;
        n -= static_cast<size_t>(r);
    }
    return true;
}

void report(const char* path, std::vector<double>& rttUs, double streamSeconds)
{
    std::sort(rttUs.begin(), rttUs.end());
    std::printf("%-6s round trip: median %7.2f us, p99 %8.2f us | one-way stream: %6.2f million msgs/s\n", path,
                rttUs[rttUs.size() / 2], rttUs[rttUs.size() * 99 / 100], kStream / streamSeconds / 1e6);
}

int socketPath()
{
    int sv[2];
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        return 1;
    }
    const pid_t child = ::fork();
    char msg[kSlotBytes] = {};
    if (child == 0) {                                   // the echo / sink process
        ::close(sv[0]);
        for (int i = 0; i < kPingPongs; ++i) {
            if (!readAll(sv[1], msg, sizeof msg) || !writeAll(sv[1], msg, sizeof msg)) {
                ::_exit(1);
            }
        }
        for (int i = 0; i < kStream; ++i) {
            if (!readAll(sv[1], msg, sizeof msg)) {
                ::_exit(1);
            }
        }
        writeAll(sv[1], msg, 1);                        // "all received"
        ::_exit(0);
    }
    ::close(sv[1]);
    std::vector<double> rtt;
    for (int i = 0; i < kPingPongs; ++i) {
        const auto t0 = Clock::now();
        writeAll(sv[0], msg, sizeof msg);
        readAll(sv[0], msg, sizeof msg);
        rtt.push_back(std::chrono::duration<double, std::micro>(Clock::now() - t0).count());
    }
    const auto s0 = Clock::now();
    for (int i = 0; i < kStream; ++i) {
        writeAll(sv[0], msg, sizeof msg);
    }
    readAll(sv[0], msg, 1);
    const double secs = std::chrono::duration<double>(Clock::now() - s0).count();
    int st = 0;
    ::waitpid(child, &st, 0);
    ::close(sv[0]);
    report("socket", rtt, secs);
    return WIFEXITED(st) && WEXITSTATUS(st) == 0 ? 0 : 1;
}

int ringPath()
{
    Ring* toChild = mapSharedRing();
    Ring* toParent = mapSharedRing();
    const pid_t child = ::fork();
    char msg[kSlotBytes] = {};
    if (child == 0) {
        for (int i = 0; i < kPingPongs; ++i) {
            while (!tryPop(*toChild, msg)) {
            }
            while (!tryPush(*toParent, msg)) {
            }
        }
        for (int i = 0; i < kStream; ++i) {
            while (!tryPop(*toChild, msg)) {
            }
        }
        while (!tryPush(*toParent, msg)) {
        }
        ::_exit(0);
    }
    std::vector<double> rtt;
    for (int i = 0; i < kPingPongs; ++i) {
        const auto t0 = Clock::now();
        while (!tryPush(*toChild, msg)) {
        }
        while (!tryPop(*toParent, msg)) {
        }
        rtt.push_back(std::chrono::duration<double, std::micro>(Clock::now() - t0).count());
    }
    const auto s0 = Clock::now();
    for (int i = 0; i < kStream; ++i) {
        while (!tryPush(*toChild, msg)) {
        }
    }
    while (!tryPop(*toParent, msg)) {
    }
    const double secs = std::chrono::duration<double>(Clock::now() - s0).count();
    int st = 0;
    ::waitpid(child, &st, 0);
    report("ring", rtt, secs);
    unmapRing(toChild);
    unmapRing(toParent);
    return WIFEXITED(st) && WEXITSTATUS(st) == 0 ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv)
{
    const std::string only = argc > 1 ? argv[1] : "";
    std::printf("two processes, %zu-byte messages: %d ping-pongs, then %d messages one way\n", kSlotBytes,
                kPingPongs, kStream);
    std::fflush(stdout);   // flush before fork so the child does not print a copy
    int rc = 0;
    if (only.empty() || only == "socket") {
        rc |= socketPath();
        std::fflush(stdout);
    }
    if (only.empty() || only == "ring") {
        rc |= ringPath();
    }
    return rc;
}
