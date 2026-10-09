// trap.cc - what happens after trap.S saved the registers: interrupts, exceptions,
// preemption and the crash dump.
#include "hooks.h"
#include "thread.h"

namespace k {

void (*volatile timer_hook)() = nullptr;

namespace {
const char* const kNames[32] = {
    "divide error", "debug", "NMI", "breakpoint", "overflow", "bound range",
    "invalid opcode", "device not available", "double fault", "coprocessor segment",
    "invalid TSS", "segment not present", "stack-segment fault", "general protection",
    "page fault", "reserved", "x87 FP error", "alignment check", "machine check",
    "SIMD FP error", "virtualization", "control protection", "reserved", "reserved",
    "reserved", "reserved", "reserved", "reserved", "hypervisor injection",
    "VMM communication", "security", "reserved"};

void print_pf_error(uint64_t e)   // page-fault error code bits
{
    kprintf("  error code 0x%lx: %s, %s, %s mode%s%s\n", e,
            (e & 1) ? "protection violation" : "page not present", (e & 2) ? "write" : "read",
            (e & 4) ? "user" : "supervisor", (e & 16) ? ", instruction fetch" : "",
            (e & 8) ? ", reserved bit set" : "");
}

[[noreturn]] void kernel_crash(TrapFrame* f, uint64_t cr2)
{
    Cpu& c = this_cpu();
    Thread* t = c.current;
    console_panic_mode();   // the console lock may be held by the code that crashed
    kprintf("\nKERNEL CRASH: %s (vector %lu) on cpu%d, thread %d '%s'\n",
            f->vector < 32 ? kNames[f->vector] : "interrupt", f->vector, c.id, t ? t->id : -1,
            t ? t->name : "?");
    if (f->vector == 14) {
        kprintf("  faulting address (CR2) = %016lx\n", cr2);
        print_pf_error(f->error);
    } else if (f->vector == 8) {
        kprintf("  CR2 (address of the last page fault) = %016lx\n", cr2);
    } else {
        kprintf("  error code 0x%lx\n", f->error);
    }
    kprintf("  rip=%016lx cs=%lx rflags=%lx rsp=%016lx ss=%lx\n", f->rip, f->cs, f->rflags,
            f->rsp, f->ss);
    kprintf("  rax=%016lx rbx=%016lx rcx=%016lx rdx=%016lx\n", f->rax, f->rbx, f->rcx, f->rdx);
    kprintf("  rsi=%016lx rdi=%016lx rbp=%016lx r8 =%016lx\n", f->rsi, f->rdi, f->rbp, f->r8);
    kprintf("  r12=%016lx r13=%016lx r14=%016lx r15=%016lx\n", f->r12, f->r13, f->r14, f->r15);
    if (t && t->stack_slot >= 0) {
        uint64_t top = t->kstack_top, guard = top - (kStackPages + 1) * kPage;
        kprintf("  thread stack: guard page %016lx-%016lx, stack %016lx-%016lx\n", guard,
                guard + kPage - 1, guard + kPage, top - 1);
    }
    kprintf("  backtrace (return addresses; use addr2line -e kernel.elf):\n    %016lx\n", f->rip);
    uint64_t bp = f->rbp;   // frame-pointer chain: [rbp] = caller's rbp, [rbp+8] = return
    for (int i = 0; i < 8 && t && bp >= t->kstack_top - kStackPages * kPage &&
                    bp + 16 <= t->kstack_top; ++i) {
        uint64_t ret = reinterpret_cast<uint64_t*>(bp)[1];
        kprintf("    %016lx\n", ret);
        bp = reinterpret_cast<uint64_t*>(bp)[0];
    }
    panic("unrecoverable %s in the kernel", f->vector < 32 ? kNames[f->vector] : "trap");
}

void exception(TrapFrame* f)
{
    uint64_t cr2 = (f->vector == 14 || f->vector == 8) ? read_cr2() : 0;
    if (f->vector == 14) {
        if (vm_handle_fault(f, cr2)) {
            return;   // demand paging mapped the page: retry the instruction
        }
        if (!from_user(f) && uaccess_fixup(f)) {
            return;   // a bad user pointer inside copy_from_user/copy_to_user
        }
    }
    if (from_user(f)) {
        user_fault(f, cr2);   // kill the process, the kernel keeps running (F3-29)
    }
    kernel_crash(f, cr2);
}
}  // namespace

extern "C" void trap_dispatch(TrapFrame* f)
{
    Cpu& c = this_cpu();
    if (f->vector < 32) {
        exception(f);
    } else {
        c.irq_depth++;
        switch (f->vector) {
        case kVecTimer:
            lapic_eoi();
            sched_timer_tick();
            if (timer_hook) {
                timer_hook();
            }
            break;
        case kVecTlb:
            tlb_ipi_handler();
            lapic_eoi();
            break;
        case kVecResched:
            c.need_resched = true;
            lapic_eoi();
            break;
        case kVecSpurious:
            break;   // no EOI for the spurious vector
        default:
            kprintf("unexpected interrupt vector %lu on cpu%d\n", f->vector, c.id);
            lapic_eoi();
        }
        c.irq_depth--;
        // Preempt only if this CPU holds no spinlock and is not nested in another handler.
        if (c.need_resched && c.preempt_count == 0 && c.irq_depth == 0) {
            schedule();
        }
    }
    if (from_user(f)) {
        user_return_check(f);
    }
}

}  // namespace k
