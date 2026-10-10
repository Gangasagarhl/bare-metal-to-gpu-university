// arch_riscv64.cc - BR-06: the RISC-V arch layer of the probe (QEMU virt).
#include "probe.h"

extern "C" int try_read_misa(u64* out);
extern "C" void probe_trap();
extern "C" void fatal_trap();
extern "C" u64 g_scause;
u64 g_scause = 0;

namespace {

// 16550 transmit register. A probe constant: a kernel reads /chosen/stdout-path (F4-23).
volatile u8* const kUartThr = reinterpret_cast<volatile u8*>(uptr{0x10000000});
bool g_mmode = false;

} // namespace

const char* arch::name()
{
    return "riscv64 (QEMU virt)";
}

void arch::putc(char c)
{
    *kUartThr = static_cast<u8>(c);
}

[[noreturn]] void arch::exit(bool ok)
{
    if (g_mmode) {
        // M-mode, no firmware below us: QEMU virt's test device (riscv.sifive.test in the
        // memory map); 0x5555 = "pass", 0x3333 = "fail".
        *reinterpret_cast<volatile u32*>(uptr{0x100000}) = ok ? 0x5555 : 0x3333;
    } else {
        // S-mode: ask the SBI firmware. System Reset extension "SRST", function 0,
        // type 0 = shutdown, reason 0 (ok) or 1 (system failure).
        register u64 a0 asm("a0") = 0;
        register u64 a1 asm("a1") = ok ? 0 : 1;
        register u64 a6 asm("a6") = 0;
        register u64 a7 asm("a7") = 0x53525354;
        asm volatile("ecall" : "+r"(a0), "+r"(a1) : "r"(a6), "r"(a7) : "memory");
    }
    for (;;) {
        asm volatile("wfi");
    }
}

extern "C" [[noreturn]] void arch_entry(u64 hartid, u64 dtb)
{
    // Who are we? Try a machine-level CSR with an S-mode trap handler installed. Writing
    // stvec is legal in M-mode and in S-mode.
    asm volatile("csrw stvec, %0" : : "r"(reinterpret_cast<uptr>(&probe_trap)));
    u64 misa = 0;
    g_mmode = try_read_misa(&misa) != 0;
    asm volatile("csrw stvec, %0" : : "r"(reinterpret_cast<uptr>(&fatal_trap)));
    if (g_mmode) {
        asm volatile("csrw mtvec, %0" : : "r"(reinterpret_cast<uptr>(&fatal_trap)));
    }

    EntryState st{};
    st.reg_name[0] = "a0 (hart ID)           ";
    st.reg_name[1] = "a1 (devicetree address)";
    st.reg_name[2] = g_mmode ? "misa                   " : "scause of that trap    ";
    st.reg_value[0] = hartid;
    st.reg_value[1] = dtb;
    st.reg_value[2] = g_mmode ? misa : g_scause;
    st.nregs = 3;
    st.dtb = reinterpret_cast<const u8*>(uptr{dtb});
    st.privilege = g_mmode ? "M-mode" : "S-mode";
    st.how = g_mmode ? "reading the machine CSR misa worked" : "reading the machine CSR misa trapped";
    probe_main(st);
}

extern "C" [[noreturn]] void arch_fatal()
{
    u64 cause, tval, epc;
    if (g_mmode) {
        asm volatile("csrr %0, mcause" : "=r"(cause));
        asm volatile("csrr %0, mtval" : "=r"(tval));
        asm volatile("csrr %0, mepc" : "=r"(epc));
    } else {
        asm volatile("csrr %0, scause" : "=r"(cause));
        asm volatile("csrr %0, stval" : "=r"(tval));
        asm volatile("csrr %0, sepc" : "=r"(epc));
    }
    probe_fatal(cause == 5 ? "load access fault (cause 5)" : "trap", cause, tval, epc);
}
