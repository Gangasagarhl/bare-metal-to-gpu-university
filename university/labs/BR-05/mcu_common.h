// mcu_common.h - BR-05 Listing 3: what the three emulated-microcontroller programs share:
// the period, a calibrated busy-work function (the "housekeeping" job), and the statistics
// of actuation lateness = (time the actuator was written) - (time the step was due).
#pragma once
#include <stdint.h>

#include "board.h"

namespace mcu {

inline constexpr uint32_t kReload = 24999;              // SysTick: 25,000 clocks per period
inline constexpr uint32_t kClocksPerPeriod = kReload + 1;
inline constexpr uint32_t kSteps = 1000;                // control steps per run

inline uint32_t loopsPerTenthPeriod = 0;

// Burn `tenths` x 0.1 period of processor time (never inlined: the calibrated code is the
// code that runs).
__attribute__((noinline)) inline void work(uint32_t tenths)
{
    for (volatile uint32_t i = 0; i < tenths * loopsPerTenthPeriod; i = i + 1) {
    }
}

// Time work(10) with SysTick running freely, then scale the loop count. Call before the
// program sets SysTick up for its own use.
inline void calibrate()
{
    board::reg(board::kSystRvr) = 0x00FFFFFF;
    board::reg(board::kSystCvr) = 0;
    board::reg(board::kSystCsr) = 0x5;                   // processor clock, no interrupt
    loopsPerTenthPeriod = 2000;                          // provisional: work(10) = 20,000 loops
    const uint32_t s0 = board::reg(board::kSystCvr);
    work(10);
    const uint32_t clocks = (s0 - board::reg(board::kSystCvr)) & 0x00FFFFFFu;
    board::reg(board::kSystCsr) = 0;
    loopsPerTenthPeriod = 20000u * (kClocksPerPeriod / 10) / clocks;
}

// Lateness statistics in processor clocks, with a histogram in fractions of the period.
struct Stats {
    uint32_t count = 0;
    uint32_t min = 0xFFFFFFFFu;
    uint32_t max = 0;
    uint32_t sum = 0;                                    // 32 bits: no 64-bit division helper
    uint32_t over = 0;                                   // steps later than one whole period
    uint32_t bins[6] = {};                               // <1%, <5%, <10%, <25%, <50%, >=50%
    uint32_t lateStep[6] = {}, lateBy[6] = {};           // the first six steps late by >= 1%
    uint32_t lateCount = 0;

    void add(uint32_t step, uint32_t late)
    {
        count = count + 1;
        if (late >= kClocksPerPeriod / 100 && lateCount < 6) {
            lateStep[lateCount] = step;
            lateBy[lateCount] = late;
            lateCount = lateCount + 1;
        }
        min = late < min ? late : min;
        max = late > max ? late : max;
        sum += late;
        if (late >= kClocksPerPeriod) {
            over = over + 1;
        }
        const uint32_t limits[5] = {kClocksPerPeriod / 100, kClocksPerPeriod / 20,
                                    kClocksPerPeriod / 10, kClocksPerPeriod / 4,
                                    kClocksPerPeriod / 2};
        uint32_t b = 0;
        while (b < 5 && late >= limits[b]) {
            ++b;
        }
        bins[b] = bins[b] + 1;
    }

    void print(const char* world, uint32_t checksum) const
    {
        static const char* const names[6] = {"  < 1% of period ", "  1-5%           ",
                                             "  5-10%          ", "  10-25%         ",
                                             "  25-50%         ", "  >= 50%         "};
        board::print("first steps late by 1% of the period or more:");
        for (uint32_t i = 0; i < lateCount; ++i) {
            board::print(" step ");
            board::printDec(lateStep[i]);
            board::print(" (");
            board::printDec(lateBy[i]);
            board::print(")");
        }
        board::print(lateCount == 0 ? " none\n" : "\n");
        board::print("actuation lateness histogram (steps):\n");
        for (uint32_t b = 0; b < 6; ++b) {
            board::print(names[b]);
            board::printDec(bins[b]);
            board::print("\n");
        }
        board::print("SUMMARY world=");
        board::print(world);
        board::print(" unit=clocks period=");
        board::printDec(kClocksPerPeriod);
        board::print(" samples=");
        board::printDec(count);
        board::print(" min=");
        board::printDec(min);
        board::print(" max=");
        board::printDec(max);
        board::print(" jitter=");
        board::printDec(max - min);
        board::print(" mean=");
        board::printDec(sum / (count == 0 ? 1 : count));
        board::print(" late_by_a_period_or_more=");
        board::printDec(over);
        board::print(" checksum=");
        board::printHex(checksum);
        board::print("\n");
    }
};

}  // namespace mcu
