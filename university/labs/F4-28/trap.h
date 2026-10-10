// trap.h - F4-28: the frame trapvec.S saves, and the scause decoder.
#pragma once
#include <cstdint>

struct TrapFrame {
    uint64_t x[32];     // x[0] unused (x0 is always zero); x[2] = interrupted sp
    uint64_t sepc;      // where the trap happened
    uint64_t sstatus;
    uint64_t scause;    // bit 63 = interrupt; low bits = exception or interrupt code
    uint64_t stval;     // faulting address, or the instruction bits, or 0
};
static_assert(sizeof(TrapFrame) == 288, "must match FRAME_SIZE in trapvec.S");

namespace trap {

inline bool is_interrupt(uint64_t scause) { return (scause >> 63) != 0; }
inline uint64_t code(uint64_t scause) { return scause & ~(uint64_t{1} << 63); }
const char* exception_name(uint64_t code);
const char* interrupt_name(uint64_t code);

void expect_fault();                               // next exception: report and skip it
bool fault_seen(uint64_t* scause, uint64_t* stval);

using IrqHandler = void (*)(TrapFrame*, uint64_t code);
void set_irq_handler(IrqHandler h);                // F4-29: timer, software, external

} // namespace trap
