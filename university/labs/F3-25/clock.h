// clock.h - F3-25: the clock-source abstraction and the exact cycles-to-nanoseconds conversion.
// Pure logic, so clock_host.cpp tests it on the host.
#pragma once
#include <cstdint>

struct ClockSource {
    const char* name;
    uint64_t (*read)();      // a counter that only goes up (wraps after centuries at these rates)
    uint64_t hz;             // counts per second, measured or read from the hardware
};

// cycles * 1e9 / hz without overflow: split cycles into whole seconds and a remainder.
// The remainder is below hz, and hz stays below 1.8e10 here, so remainder * 1e9 fits in 64 bits.
constexpr uint64_t cycles_to_ns(uint64_t cycles, uint64_t hz)
{
    return (cycles / hz) * 1000000000ull + (cycles % hz) * 1000000000ull / hz;
}

constexpr uint64_t ns_to_cycles(uint64_t ns, uint64_t hz)
{
    return (ns / 1000000000ull) * hz + (ns % 1000000000ull) * hz / 1000000000ull;
}
