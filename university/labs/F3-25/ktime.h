// ktime.h - F3-25: kernel time keeping. One clock source for "now", the local APIC timer in
// one-shot (tickless) mode for wake-ups, and a timer queue that the timer interrupt drains.
#pragma once
#include <cstdint>
#include "clock.h"
#include "timerq.h"

namespace ktime {

bool init();                     // HPET (from ACPI) or PIT fallback; TSC and APIC timer calibration
uint64_t now_ns();               // monotonic nanoseconds since init()
const ClockSource& source();
bool add_timer(Timer& t);        // deadline_ns must be set; safe with interrupts on
void cancel_timer(Timer& t);
void sleep_ns(uint64_t ns);      // halts until the deadline has passed
uint64_t timer_interrupts();
uint64_t apic_timer_hz();
uint64_t tsc_hz();

} // namespace ktime
