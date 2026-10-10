// gtimer.h - F4-25: the Arm generic timer, EL1 physical timer. Register names after the
// Arm Architecture Reference Manual, generic timer chapter (title only, pending verification).
// The frequency is read from CNTFRQ_EL0 (set by firmware or by QEMU), never assumed.
#pragma once
#include <cstdint>
#include "arch.h"
#include "fdt.h"

namespace gtimer {

inline uint64_t freq() { return READ_SYSREG(cntfrq_el0); }
inline uint64_t now()
{
    arch::isb();                               // do not read the counter early
    return READ_SYSREG(cntpct_el0);
}
// Fire after `ticks` counter ticks: TVAL is a down-counter view of the compare value.
inline void arm(uint64_t ticks)
{
    WRITE_SYSREG(cntp_tval_el0, ticks);
    WRITE_SYSREG(cntp_ctl_el0, 1);             // ENABLE = 1, IMASK = 0
    arch::isb();
}
inline void stop() { WRITE_SYSREG(cntp_ctl_el0, 0); }
inline uint64_t ctl() { return READ_SYSREG(cntp_ctl_el0); }   // bit 2 ISTATUS: condition met

// The timer's PPI as the devicetree's "arm,armv8-timer" node lists it (second entry:
// non-secure EL1 physical timer), converted to a GIC INTID (PPI n = INTID 16 + n).
uint32_t intid_from_devicetree(const fdt::Blob& dt);

} // namespace gtimer
