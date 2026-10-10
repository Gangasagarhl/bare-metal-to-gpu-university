// arch_aarch64.cc - BR-06: the AArch64 arch layer of the probe (QEMU virt).
#include "probe.h"

namespace {

// PL011 data register. A probe constant: a kernel finds the UART through
// /chosen/stdout-path in the devicetree (F4-23), never like this.
volatile u32* const kUartData = reinterpret_cast<volatile u32*>(uptr{0x09000000});
u64 g_el = 0;

u64 current_el()
{
    u64 v;
    asm volatile("mrs %0, CurrentEL" : "=r"(v));
    return (v >> 2) & 3;
}

} // namespace

const char* arch::name()
{
    return "aarch64 (QEMU virt)";
}

void arch::putc(char c)
{
    *kUartData = static_cast<u8>(c);
}

[[noreturn]] void arch::exit(bool ok)
{
    // PSCI SYSTEM_OFF (function ID 0x84000008). QEMU's conduit is hvc for an EL1 guest and
    // smc when it emulates EL2 (F4-24 reads it from /psci instead).
    register u64 x0 asm("x0") = 0x84000008;
    (void)ok;
    if (g_el == 2) {
        asm volatile("smc #0" : "+r"(x0) : : "memory");
    } else {
        asm volatile("hvc #0" : "+r"(x0) : : "memory");
    }
    for (;;) {
        asm volatile("wfe");
    }
}

extern "C" [[noreturn]] void arch_entry(u64 x0, u64 x1, u64 x2, u64 x3, u64 el_reg)
{
    (void)x3;
    g_el = (el_reg >> 2) & 3;
    EntryState st{};
    st.reg_name[0] = "x0 (devicetree address)";
    st.reg_name[1] = "x1 (reserved, 0)       ";
    st.reg_name[2] = "x2 (reserved, 0)       ";
    st.reg_value[0] = x0;
    st.reg_value[1] = x1;
    st.reg_value[2] = x2;
    st.nregs = 3;
    st.dtb = reinterpret_cast<const u8*>(uptr{x0});
    st.privilege = g_el == 2 ? "EL2" : g_el == 1 ? "EL1" : "EL3 or EL0 (unexpected)";
    st.how = "CurrentEL system register, bits 3:2";
    probe_main(st);
}

extern "C" [[noreturn]] void arch_fatal()
{
    u64 esr, far, elr;
    if (current_el() == 2) {
        asm volatile("mrs %0, esr_el2" : "=r"(esr));
        asm volatile("mrs %0, far_el2" : "=r"(far));
        asm volatile("mrs %0, elr_el2" : "=r"(elr));
    } else {
        asm volatile("mrs %0, esr_el1" : "=r"(esr));
        asm volatile("mrs %0, far_el1" : "=r"(far));
        asm volatile("mrs %0, elr_el1" : "=r"(elr));
    }
    const u64 ec = (esr >> 26) & 0x3f;
    probe_fatal(ec == 0x25 ? "data abort at the current EL (ESR class 0x25)" : "synchronous exception",
                esr, far, elr);
}
