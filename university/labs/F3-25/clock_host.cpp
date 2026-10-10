// clock_host.cpp - F3-25: host tests of clock.h (exact conversion) and timerq.h (ordering,
// periodic re-arm, removal). The reference for the conversion is 128-bit arithmetic, which the
// kernel avoids; the host may use it.
#include <cstdint>
#include <cstdio>
#include <random>
#include "clock.h"
#include "timerq.h"

int g_fail = 0;
#define CHECK(c) do { bool ok_ = (c); if (!ok_) { std::printf("FAIL %s (line %d)\n", #c, __LINE__); ++g_fail; } } while (0)

__extension__ typedef unsigned __int128 u128;

int main()
{
    // 1. conversions against 128-bit reference arithmetic, at the rates this lab measured
    const uint64_t rates[] = {1193182, 100000000, 62448270, 2100102351, 3999999999, 17000000000};
    std::mt19937_64 rng(302);
    long checked = 0;
    for (uint64_t hz : rates) {
        for (int i = 0; i < 200000; ++i) {
            uint64_t c = rng() >> (rng() % 40);                   // small and huge counts
            u128 ref = static_cast<u128>(c) * 1000000000u / hz;
            if (ref > UINT64_MAX) {
                continue;
            }
            CHECK(cycles_to_ns(c, hz) == static_cast<uint64_t>(ref));
            uint64_t ns = rng() >> 4;
            u128 ref2 = static_cast<u128>(ns) * hz / 1000000000u;
            if (ref2 <= UINT64_MAX) {
                CHECK(ns_to_cycles(ns, hz) == static_cast<uint64_t>(ref2));
            }
            ++checked;
        }
    }
    std::printf("conversions: %ld random cases at %zu rates agree with 128-bit arithmetic\n", checked,
                sizeof rates / sizeof rates[0]);
    // the naive formula overflows: cycles * 1e9 wraps after 2^64 / 1e9 = 18.4 billion cycles
    uint64_t c = 3600ull * 2100102351ull;                          // one hour of a 2.1 GHz TSC
    uint64_t naive = c * 1000000000ull / 2100102351ull;
    std::printf("one hour of TSC: exact %lu ns, naive (c * 1e9 / hz) %lu ns\n",
                static_cast<unsigned long>(cycles_to_ns(c, 2100102351ull)), static_cast<unsigned long>(naive));
    CHECK(cycles_to_ns(c, 2100102351ull) == 3600000000000ull);

    // 2. the timer queue
    static int order[8], n = 0;
    Timer t[5];
    const uint64_t deadlines[5] = {50, 10, 40, 20, 30};
    TimerQueue<4> q;
    for (int i = 0; i < 5; ++i) {
        t[i].deadline_ns = deadlines[i];
        t[i].ctx = &t[i];
        t[i].fn = [](Timer& self) { order[n++] = static_cast<int>(self.deadline_ns); };
    }
    for (int i = 0; i < 4; ++i) {
        CHECK(q.add(t[i]));
    }
    CHECK(!q.add(t[4]));                                          // full
    CHECK(!q.add(t[0]));                                          // already queued
    CHECK(q.next_deadline() == 10);
    CHECK(q.remove(t[2]));                                        // cancel the 40
    CHECK(q.add(t[4]));
    CHECK(q.expire(25) == 2 && n == 2 && order[0] == 10 && order[1] == 20);
    CHECK(q.expire(25) == 0);
    CHECK(q.expire(1000) == 2 && order[2] == 30 && order[3] == 50 && q.empty());
    Timer tick;
    int ticks = 0;
    tick.deadline_ns = 10;
    tick.period_ns = 10;
    tick.ctx = &ticks;
    tick.fn = [](Timer& self) { ++*static_cast<int*>(self.ctx); };
    CHECK(q.add(tick));
    for (uint64_t now = 0; now <= 100; now += 5) {
        q.expire(now);
    }
    CHECK(ticks == 10 && tick.deadline_ns == 110);
    q.expire(1000);                                              // far behind: missed periods are skipped
    CHECK(ticks == 11 && tick.deadline_ns == 1010);
    std::printf("timer queue: ordering, capacity, cancel, periodic re-arm and catch-up checked\n");
    std::printf("%s: %d failure(s)\n", g_fail == 0 ? "PASS" : "FAIL", g_fail);
    return g_fail == 0 ? 0 : 1;
}
