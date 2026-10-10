// trap.cc - F4-24: decode and report AArch64 exceptions taken to EL1.
// Exception-class and fault-status values after the Arm Architecture Reference Manual,
// ESR_EL1 description (title only, pending verification). The lab run checks the values
// QEMU reports for the two faults the D1 acceptance test asks for.
#include "trap.h"
#include "arch.h"
#include "kprint.h"
#include "panic.h"

namespace {
bool g_expecting = false;
bool g_seen = false;
uint64_t g_esr = 0, g_far = 0;
trap::IrqHandler g_irq = nullptr;

const char* const kKind[4] = {"synchronous", "IRQ", "FIQ", "SError"};
const char* const kSource[4] = {"current EL, SP_EL0", "current EL, SP_ELx", "lower EL, AArch64",
                                "lower EL, AArch32"};
} // namespace

namespace trap {

const char* ec_name(uint32_t ec)
{
    switch (ec) {
    case 0x00: return "unknown reason (for example an undefined instruction)";
    case 0x01: return "trapped WFI or WFE";
    case 0x0e: return "illegal execution state";
    case 0x15: return "SVC from AArch64";
    case 0x16: return "HVC from AArch64";
    case 0x17: return "SMC from AArch64";
    case 0x18: return "trapped MSR, MRS or system instruction";
    case 0x20: return "instruction abort from a lower EL";
    case 0x21: return "instruction abort, same EL";
    case 0x22: return "PC alignment fault";
    case 0x24: return "data abort from a lower EL";
    case 0x25: return "data abort, same EL";
    case 0x26: return "SP alignment fault";
    case 0x2f: return "SError";
    case 0x3c: return "BRK instruction";
    default: return "(not decoded by this kernel)";
    }
}

const char* fault_status_name(uint32_t fsc)
{
    if (fsc >= 0x04 && fsc <= 0x07) {
        return "translation fault";
    }
    if (fsc >= 0x09 && fsc <= 0x0b) {
        return "access flag fault";
    }
    if (fsc >= 0x0d && fsc <= 0x0f) {
        return "permission fault";
    }
    switch (fsc) {
    case 0x00: return "address size fault, level 0";
    case 0x10: return "synchronous external abort";
    case 0x21: return "alignment fault";
    default: return "(not decoded by this kernel)";
    }
}

void expect_fault()
{
    g_expecting = true;
    g_seen = false;
}

bool fault_seen(uint64_t* esr, uint64_t* far)
{
    *esr = g_esr;
    *far = g_far;
    return g_seen;
}

void set_irq_handler(IrqHandler h)
{
    g_irq = h;
}

} // namespace trap

extern "C" void exception_entry(TrapFrame* f, uint64_t index)
{
    uint64_t kind = index & 3;
    if (kind == 1 && g_irq != nullptr) {
        g_irq(f);
        return;
    }
    uint32_t ec = trap::ec(f->esr);
    kprintf("exception: %s from %s (vector entry %lu, offset 0x%lx)\n", kKind[kind],
            kSource[(index >> 2) & 3], index, index * 0x80);
    kprintf("  ESR_EL1 0x%lx: EC 0x%x = %s, IL %lu, ISS 0x%x\n", f->esr, ec, trap::ec_name(ec),
            (f->esr >> 25) & 1, trap::iss(f->esr));
    if (ec == 0x20 || ec == 0x21 || ec == 0x24 || ec == 0x25) {
        uint32_t fsc = trap::iss(f->esr) & 0x3f;
        kprintf("  fault status 0x%x = %s; %s; FAR_EL1 0x%lx\n", fsc, trap::fault_status_name(fsc),
                (ec >= 0x24 && (f->esr & (1u << 6))) ? "write" : "read or fetch", f->far);
    }
    kprintf("  ELR_EL1 0x%lx, SPSR_EL1 0x%lx\n", f->elr, f->spsr);
    if (kind == 0 && g_expecting) {
        g_expecting = false;
        g_seen = true;
        g_esr = f->esr;
        g_far = f->far;
        if (ec == 0x20 || ec == 0x21) {
            f->elr = f->x[30];   // a failed fetch: there is no instruction to skip; return to the caller
        } else {
            f->elr += 4;         // resume after the faulting instruction (all A64 instructions are 4 bytes)
        }
        return;
    }
    PANIC("unexpected exception");
}
