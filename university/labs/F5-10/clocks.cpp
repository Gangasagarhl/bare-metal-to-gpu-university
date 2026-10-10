// clocks.cpp - the two clocks a Linux program can read, and what each one promises.
// CLOCK_REALTIME: time of day; can be set or stepped (by an administrator or a time daemon).
// CLOCK_MONOTONIC: counts from an unspecified start; cannot be set; use it for durations.
// Numbers printed here are measurements of the build container (AH-23), not specifications.
#include <time.h>

#include <cstdint>
#include <cstdio>

std::int64_t ns(clockid_t id)
{
    timespec t{};
    clock_gettime(id, &t);
    return std::int64_t{t.tv_sec} * 1000000000 + t.tv_nsec;
}

int main()
{
    timespec res{};
    clock_getres(CLOCK_REALTIME, &res);
    std::printf("CLOCK_REALTIME  resolution reported: %ld ns\n", res.tv_nsec);
    clock_getres(CLOCK_MONOTONIC, &res);
    std::printf("CLOCK_MONOTONIC resolution reported: %ld ns\n", res.tv_nsec);

    // Read the monotonic clock many times; it must never go backwards.
    const int reads = 1000000;
    std::int64_t prev = ns(CLOCK_MONOTONIC);
    int backwards = 0;
    int equal = 0;
    for (int i = 0; i < reads; ++i) {
        const std::int64_t now = ns(CLOCK_MONOTONIC);
        backwards += now < prev ? 1 : 0;
        equal += now == prev ? 1 : 0;
        prev = now;
    }
    std::printf("%d consecutive CLOCK_MONOTONIC reads: went backwards %d times, "
                "same value twice in a row %s\n", reads, backwards, equal > 0 ? "yes" : "no");

    // Measure a 50 ms sleep with both clocks.
    const std::int64_t r0 = ns(CLOCK_REALTIME);
    const std::int64_t m0 = ns(CLOCK_MONOTONIC);
    timespec nap{0, 50000000};
    nanosleep(&nap, nullptr);
    const double rMs = (ns(CLOCK_REALTIME) - r0) / 1e6;
    const double mMs = (ns(CLOCK_MONOTONIC) - m0) / 1e6;
    std::printf("a 50 ms sleep measured: monotonic %s 50 ms, realtime %s 50 ms "
                "(a step of the realtime clock during the sleep would make them disagree)\n",
                mMs >= 50.0 ? ">=" : "<", rMs >= 50.0 ? ">=" : "<");
    return 0;
}
