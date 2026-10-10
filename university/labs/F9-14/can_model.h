// F9-14: the university's frame-level CAN bus model (shared by bus_budget.cpp and
// can_sched.cpp). Time is counted in bit times. When the bus is idle, every node with a
// frame waiting takes part in arbitration and the lowest identifier wins (F1-47); a frame,
// once started, is never interrupted. Each frame is given its worst-case length.
#pragma once
#include <linux/can.h>  // CAN_SFF_MASK, CAN_MAX_DLEN (opened in this build)

#include <cstdint>

// Worst-case length in bits of a classical CAN data frame with an 11-bit identifier and
// n data bytes, including worst-case stuff bits and the 3-bit inter-frame space.
// NOT VERIFIED in this build: the formula is the one recorded in the chapter's unverified
// box (check the CAN specification and the schedulability paper named there).
constexpr int canFrameBitsWorst(int n)
{
    return 8 * n + 47 + (34 + 8 * n - 1) / 4;
}

static_assert(CAN_MAX_DLEN == 8, "classical CAN carries at most 8 data bytes");
