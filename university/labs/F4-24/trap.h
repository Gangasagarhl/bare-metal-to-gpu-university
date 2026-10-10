// trap.h - F4-24: the register frame vectors.S saves, and the exception decoder.
#pragma once
#include <cstdint>

struct TrapFrame {
    uint64_t x[31];    // x0 ... x30
    uint64_t elr;      // where the exception happened (or the next instruction, for SVC)
    uint64_t spsr;     // saved PSTATE
    uint64_t esr;      // syndrome: what happened
    uint64_t far;      // faulting address, for aborts
    uint64_t pad;
};
static_assert(sizeof(TrapFrame) == 288, "must match FRAME_SIZE in vectors.S");

namespace trap {

// Exception class: ESR_EL1 bits 31:26.
inline uint32_t ec(uint64_t esr) { return static_cast<uint32_t>(esr >> 26) & 0x3f; }
// Instruction-specific syndrome: bits 24:0. For aborts, bits 5:0 are the fault status code.
inline uint32_t iss(uint64_t esr) { return static_cast<uint32_t>(esr) & 0x1ffffff; }
const char* ec_name(uint32_t ec);
const char* fault_status_name(uint32_t fsc);

// A test may announce that the next synchronous exception is expected; the handler then
// reports it, records it and resumes at the following instruction instead of panicking.
void expect_fault();
bool fault_seen(uint64_t* esr, uint64_t* far);

// Interrupt handler hook for later chapters (F4-25 installs the GIC handler here).
using IrqHandler = void (*)(TrapFrame*);
void set_irq_handler(IrqHandler h);

} // namespace trap
