// cpu.h - per-CPU data, descriptor tables and the trap frame of the OS303 kernel.
// Every CPU owns one Cpu object. Its address is loaded into the GS base register
// (MSR IA32_GS_BASE), so "this CPU's data" is one instruction away: mov %gs:0, %rax.
#pragma once
#include "kbase.h"

namespace k {

struct Thread;

// Kernel GDT layout. The order 0x18 user data, 0x20 user code is the one SYSRET
// needs (F3-29): SYSRET loads SS = STAR[63:48] + 8 and CS = STAR[63:48] + 16.
constexpr uint16_t kKernelCS = 0x08, kKernelDS = 0x10, kUserDS = 0x18 | 3, kUserCS = 0x20 | 3;
constexpr uint16_t kTssSel = 0x28;

struct __attribute__((packed)) Tss {   // 64-bit TSS (Intel SDM Vol. 3, "Task Management")
    uint32_t reserved0;
    uint64_t rsp0;          // stack loaded when an interrupt arrives from ring 3
    uint64_t rsp1, rsp2;
    uint64_t reserved1;
    uint64_t ist[7];        // ist[0] = IST1, used for double faults
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
};
static_assert(sizeof(Tss) == 104);

constexpr int kMaxCpus = 16;
constexpr int kMaxHeld = 8;

struct Cpu {
    // Offsets 0, 8 and 16 are used by assembly (syscall.S): keep them first.
    Cpu* self;                 // %gs:0
    uint64_t kernel_rsp;       // %gs:8   top of the current thread's kernel stack
    uint64_t user_rsp;         // %gs:16  scratch for the SYSCALL entry
    int id;                    // 0 = bootstrap processor
    uint32_t apic_id;
    Thread* current;
    Thread* idle;
    volatile uint64_t ticks;   // this CPU's timer interrupts
    int slice;                 // ticks the current thread has run
    bool need_resched;
    int preempt_count;         // spinlocks held: no preemption while > 0
    int irq_depth;             // > 0 while handling an interrupt
    volatile bool online;
    const char* spinning_on;   // lock this CPU is waiting for (for lockup reports)
    uint64_t spin_since;
    uint64_t gdt[7];
    Tss tss;
    alignas(16) uint8_t df_stack[4096];   // IST1 stack for double faults
};

inline Cpu& this_cpu()
{
    Cpu* c;
    asm volatile("mov %%gs:0, %0" : "=r"(c));
    return *c;
}
Cpu& cpu_by_index(int i);
int cpu_count();                        // CPUs started so far
void cpu_setup(Cpu& c, int id, uint32_t apic_id);   // GDT, TSS, IDT, GS base for this CPU
void idt_init();                        // build the shared IDT once

// Trap frame pushed by trap.S: general registers, vector, error code, then what the
// CPU pushed (RIP, CS, RFLAGS, RSP, SS) (Intel SDM Vol. 3, "Interrupt and Exception
// Handling", 64-bit mode stack frame).
struct TrapFrame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vector, error;
    uint64_t rip, cs, rflags, rsp, ss;
};
inline bool from_user(const TrapFrame* f) { return (f->cs & 3) == 3; }

// Interrupt vectors used by this kernel.
constexpr int kVecTimer = 32, kVecTlb = 0xF0, kVecResched = 0xF1, kVecSpurious = 0xFF;

}  // namespace k
