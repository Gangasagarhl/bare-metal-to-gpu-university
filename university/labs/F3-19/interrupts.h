// interrupts.h - F3-19: the IDT, the saved-register frame and handler registration.
#pragma once
#include <cstdint>

struct InterruptFrame {        // pushed by isr.S (r15 first in memory) and by the CPU
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vector, error_code;
    uint64_t rip, cs, rflags, rsp, ss;   // the CPU's part, the same for every vector here
};
static_assert(sizeof(InterruptFrame) == 22 * 8, "isr.S and this struct must agree");

using IrqHandler = void (*)(InterruptFrame&);

namespace idt {
void init(bool use_ist);                       // all 256 gates; IST for #DF, NMI and #MC
void set_handler(uint8_t vector, IrqHandler h);  // for vectors 32..255 (devices, timers)
void dump_gate(uint8_t vector);                  // prints the gate's two raw 64-bit halves
}

// Test support ("exception fixup"): when non-zero, an exception handler reports the
// exception and then resumes at this address instead of panicking.
extern "C" volatile uint64_t g_resume_rip;
const char* exception_name(uint64_t vector);
void dump_frame(const InterruptFrame& f);
