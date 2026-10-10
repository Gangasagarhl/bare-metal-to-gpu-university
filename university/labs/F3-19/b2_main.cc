// b2_main.cc - F3-19 test kernel for milestone B2. Build variants (kbuild.sh flags):
//   (none)            install GDT, TSS, IDT and exit: the "normal boot" for QEMU's interrupt log
//   -DB2_TESTS        trigger #DE, #UD, #BP, #GP and two #PF in turn, then overflow the stack
//   -DB2_FORGET_IST   as B2_TESTS, but the IDT gives #DF no IST stack (forensic lab)
#include <cstdint>
#include "arch.h"
#include "gdt.h"
#include "interrupts.h"
#include "kprint.h"
#include "panic.h"
#include "serial.h"

extern "C" uint64_t boot_pd[];   // boot.S: the page directory for physical 0..8 GiB
extern void (*g_double_fault_hook)(const InterruptFrame&);

// Runs the instructions in BODY; if they fault, the handler resumes at label 1.
#define TRIGGER(BODY)                                       \
    asm volatile("leaq 1f(%%rip), %%rax\n\t"                \
                 "movq %%rax, g_resume_rip(%%rip)\n\t"      \
                 BODY "\n"                                  \
                 "1:\n"                                     \
                 :                                          \
                 :                                          \
                 : "rax", "rcx", "rdx", "memory")

namespace {

constexpr uint64_t kDirectMap = 0xffff800000000000;

[[gnu::noinline, maybe_unused]] uint64_t recurse(uint64_t depth)
{
    volatile uint8_t frame_filler[256];          // makes each call use more stack
    frame_filler[0] = static_cast<uint8_t>(depth);
    if (depth > 100000000) {                     // never reached: the stack runs out first
        return depth;
    }
    return recurse(depth + 1) + frame_filler[0];
}

[[maybe_unused]] void stack_overflow_test()
{
    // Make physical 6..8 MiB a guard: clear its 2 MiB entry in the boot page directory
    // (shared by the identity map, the direct map and the kernel window), then run on a
    // fresh 16 KiB stack at physical 8 MiB, so the stack grows down into the guard.
    boot_pd[3] = 0;
    arch::write_cr3(arch::read_cr3());           // flush the TLB (boot pages are not global)
    uint64_t top = kDirectMap + 0x800000 + 16384;
    kprintf("stack overflow test: guard at phys 0x600000-0x7fffff, new stack top %p\n",
            reinterpret_cast<void*>(top));
    uint64_t (*fn)(uint64_t) = recurse;
    asm volatile("mov %0, %%rsp\n\t"
                 "xor %%edi, %%edi\n\t"
                 "call *%1\n\t"
                 :
                 : "r"(top), "r"(fn)
                 : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11", "rax", "memory");
    PANIC("the recursion returned");
}

[[maybe_unused]] void double_fault_seen(const InterruptFrame& f)
{
    kprintf("B2 double fault test: reported on the IST stack (frame at %p); B2 ok\n",
            static_cast<const void*>(&f));
    arch::qemu_exit(arch::kExitPass);
}

} // namespace

extern "C" void kmain(uint32_t, uint32_t)
{
    serial::init();
#if defined(B2_FORGET_IST)
    bool use_ist = false;
#else
    bool use_ist = true;
#endif
    gdt::init(use_ist);
    idt::init(use_ist);
    idt::dump_gate(8);
    idt::dump_gate(14);
    kprintf("tss: ist1 %016lx ist2 %016lx ist3 %016lx\n", gdt::tss().ist[0], gdt::tss().ist[1],
            gdt::tss().ist[2]);
#if defined(B2_TESTS) || defined(B2_FORGET_IST)
    kprintf("-- 1: divide by zero\n");
    TRIGGER("xorl %%edx, %%edx\n\tmovl $1, %%eax\n\txorl %%ecx, %%ecx\n\tdivl %%ecx");
    kprintf("-- 2: invalid opcode\n");
    TRIGGER("ud2");
    kprintf("-- 3: breakpoint (a trap: execution continues by itself)\n");
    asm volatile("int3");
    kprintf("-- 4: general protection (non-canonical address)\n");
    TRIGGER("movabsq $0x0000800000000000, %%rcx\n\tmovq (%%rcx), %%rcx");
    kprintf("-- 5: page fault, read of an unmapped address\n");
    TRIGGER("movabsq $0x0000000400000000, %%rcx\n\tmovq (%%rcx), %%rcx");
    kprintf("-- 6: page fault, write to an unmapped address\n");
    TRIGGER("movabsq $0x0000000400000000, %%rcx\n\tmovq $1, (%%rcx)");
    kprintf("-- 7: kernel stack overflow into a guard page\n");
    g_double_fault_hook = double_fault_seen;
    stack_overflow_test();
#endif
    kprintf("B2 tables installed; no exception expected during this boot\n");
    arch::qemu_exit(arch::kExitPass);
}
