// bypass_sleep.cpp - DS401 F5-38, forensic evidence generator ("kernel bypass made it slower").
// Version 2 of a two-process service: requests and replies travel through shared-memory
// rings (spsc_ring.hpp), but the server "saves CPU" by sleeping 1 microsecond whenever its
// ring is empty. The program prints round-trip latency and the server's CPU use.
// Usage: bypass_sleep          version 2 as deployed (sleeps when idle)
//        bypass_sleep spin     the server busy-polls instead (answer key)
#include "spsc_ring.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <sys/resource.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

int main(int argc, char** argv)
{
    const bool spin = argc > 1 && std::string(argv[1]) == "spin";
    constexpr int kRequests = 2000;
    Ring* req = mapSharedRing();
    Ring* rep = mapSharedRing();
    std::printf("service v2: shared-memory rings; server idle policy: %s\n",
                spin ? "busy poll" : "sleep_for(1 us) when the ring is empty");
    std::fflush(stdout);
    const pid_t child = ::fork();
    char msg[kSlotBytes] = {};
    if (child == 0) {
        const auto w0 = std::chrono::steady_clock::now();
        for (int i = 0; i < kRequests; ++i) {
            while (!tryPop(*req, msg)) {
                if (!spin) {
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
            }
            while (!tryPush(*rep, msg)) {
            }
        }
        const double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - w0).count();
        rusage ru{};
        ::getrusage(RUSAGE_SELF, &ru);
        const double cpu = static_cast<double>(ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) +
                           static_cast<double>(ru.ru_utime.tv_usec + ru.ru_stime.tv_usec) / 1e6;
        std::printf("server: CPU time %.3f s during %.3f s of wall time (%.0f %% of one core)\n", cpu, wall,
                    100.0 * cpu / wall);
        std::fflush(stdout);
        ::_exit(0);
    }
    std::vector<double> us;
    for (int i = 0; i < kRequests; ++i) {
        const auto t0 = std::chrono::steady_clock::now();
        while (!tryPush(*req, msg)) {
        }
        while (!tryPop(*rep, msg)) {
        }
        us.push_back(std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count());
        std::this_thread::sleep_for(std::chrono::microseconds(200));   // requests arrive spaced out
    }
    int st = 0;
    ::waitpid(child, &st, 0);
    std::sort(us.begin(), us.end());
    std::printf("client: %d requests, round trip min %.2f us, median %.2f us, p99 %.2f us\n", kRequests, us.front(),
                us[us.size() / 2], us[us.size() * 99 / 100]);
    unmapRing(req);
    unmapRing(rep);
    return WIFEXITED(st) && WEXITSTATUS(st) == 0 ? 0 : 1;
}
