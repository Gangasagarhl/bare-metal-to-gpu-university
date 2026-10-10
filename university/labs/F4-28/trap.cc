// trap.cc - F4-28: decode and report S-mode traps. scause codes after the RISC-V Instruction
// Set Manual, Volume II, supervisor cause register (title only, pending verification).
// The lab run checks the codes QEMU reports for the two faults the D5 acceptance test asks for.
#include "trap.h"
#include "arch.h"
#include "kprint.h"
#include "panic.h"

namespace {
bool g_expecting = false;
bool g_seen = false;
uint64_t g_cause = 0, g_tval = 0;
trap::IrqHandler g_irq = nullptr;
} // namespace

namespace trap {

const char* exception_name(uint64_t c)
{
    switch (c) {
    case 0: return "instruction address misaligned";
    case 1: return "instruction access fault";
    case 2: return "illegal instruction";
    case 3: return "breakpoint";
    case 4: return "load address misaligned";
    case 5: return "load access fault";
    case 6: return "store/AMO address misaligned";
    case 7: return "store/AMO access fault";
    case 8: return "environment call from U-mode";
    case 9: return "environment call from S-mode";
    case 12: return "instruction page fault";
    case 13: return "load page fault";
    case 15: return "store/AMO page fault";
    default: return "(not decoded by this kernel)";
    }
}

const char* interrupt_name(uint64_t c)
{
    switch (c) {
    case 1: return "supervisor software interrupt";
    case 5: return "supervisor timer interrupt";
    case 9: return "supervisor external interrupt";
    default: return "(not decoded by this kernel)";
    }
}

void expect_fault()
{
    g_expecting = true;
    g_seen = false;
}

bool fault_seen(uint64_t* scause, uint64_t* stval)
{
    *scause = g_cause;
    *stval = g_tval;
    return g_seen;
}

void set_irq_handler(IrqHandler h)
{
    g_irq = h;
}

} // namespace trap

extern "C" void trap_entry(TrapFrame* f)
{
    uint64_t c = trap::code(f->scause);
    if (trap::is_interrupt(f->scause)) {
        if (g_irq != nullptr) {
            g_irq(f, c);
            return;
        }
        PANIC("unexpected interrupt %lu (%s)", c, trap::interrupt_name(c));
    }
    kprintf("trap: scause 0x%lx = exception %lu, %s\n", f->scause, c, trap::exception_name(c));
    kprintf("  sepc 0x%lx, stval 0x%lx, sstatus 0x%lx\n", f->sepc, f->stval, f->sstatus);
    if (g_expecting) {
        g_expecting = false;
        g_seen = true;
        g_cause = f->scause;
        g_tval = f->stval;
        // Instructions are 2 or 4 bytes long (the C extension): the two low bits of the first
        // halfword are 11 only for a 4-byte instruction.
        uint16_t first = *reinterpret_cast<const uint16_t*>(f->sepc);
        f->sepc += (first & 3) == 3 ? 4 : 2;
        return;
    }
    PANIC("unexpected exception");
}
