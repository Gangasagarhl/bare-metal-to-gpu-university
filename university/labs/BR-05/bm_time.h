// bm_time.h - BR-05 Listing 4: the bare-metal time base: SysTick interrupts once per period
// and counts periods; now() adds the part of the current period that SysTick has counted
// (the same method as uRTOS's os::now() in F3-39). Time 0 = when startTicks() ran.
#pragma once
#include <stdint.h>

#include "board.h"
#include "mcu_common.h"

namespace bm {

inline volatile uint32_t ticks = 0;
inline constexpr uintptr_t kIcsr = 0xE000ED04;           // interrupt control and state

inline uint32_t now()
{
    uint32_t primask;
    asm volatile("mrs %0, primask\n cpsid i" : "=r"(primask) :: "memory");
    uint32_t t = ticks;
    const uint32_t cvr = board::reg(board::kSystCvr);
    if ((board::reg(kIcsr) & (1u << 26)) != 0 && (cvr == 0 || cvr > mcu::kReload / 2)) {
        t += 1;                                          // wrapped, handler not run yet
    }
    asm volatile("msr primask, %0" :: "r"(primask) : "memory");
    const uint32_t inPeriod = cvr == 0 ? 0 : mcu::kReload + 1 - cvr;
    return t * mcu::kClocksPerPeriod + inPeriod;
}

inline void startTicks()
{
    board::reg(board::kSystRvr) = mcu::kReload;
    board::reg(board::kSystCvr) = 0;
    board::reg(board::kSystCsr) = 0x7;                   // processor clock, interrupt, enable
}

}  // namespace bm
