// d1_main.cc - F4-24: milestone D1 on QEMU virt. Find the console in the devicetree, report
// the exception level, run the arch-neutral tests, then provoke the two faults the D1
// acceptance test asks for (a data abort and an undefined instruction) and decode them.
#include <cstdint>
#include "arch.h"
#include "fdt.h"
#include "kprint.h"
#include "neutral_tests.h"
#include "pl011.h"
#include "psci.h"
#include "trap.h"

namespace {

// Console from /chosen/stdout-path, as F4-23's checklist row 6 found it. Returns false if
// there is no PL011 there (then nothing can be printed at all).
bool console_from_devicetree(const fdt::Blob& dt)
{
    fdt::Node chosen, uart;
    fdt::Prop p;
    if (!dt.find_path("/chosen", &chosen) || !dt.get_prop(chosen, "stdout-path", &p)) {
        return false;
    }
    char path[64];
    uint32_t i = 0;
    for (; i + 1 < sizeof path && i < p.len && p.data[i] != '\0' && p.data[i] != ':'; ++i) {
        path[i] = static_cast<char>(p.data[i]);
    }
    path[i] = '\0';
    uint64_t base = 0, size = 0;
    if (!dt.find_path(path, &uart) || !dt.is_compatible(uart, "arm,pl011") || !dt.reg(uart, 0, &base, &size)) {
        return false;
    }
    pl011::set_base(base);
    kprintf("console: %s (arm,pl011) at 0x%lx, from /chosen/stdout-path\n", path, base);
    return true;
}

} // namespace

extern "C" void kmain(const void* dtb, uint64_t entry_el)
{
    fdt::Blob dt;
    if (!dt.init(dtb) || !console_from_devicetree(dt)) {
        arch::halt_forever();            // no devicetree, no console: nothing we can report
    }
    kprintf("D1: entered at EL%lu, running at EL%lu\n", entry_el, arch::current_el());
    kprintf("devicetree at %p, %u bytes, version %u\n", dtb, dt.total_size(), dt.version());
    uint64_t midr = READ_SYSREG(midr_el1);
    kprintf("MIDR_EL1 0x%lx (implementer 0x%lx, part number 0x%lx)\n", midr, midr >> 24 & 0xff,
            midr >> 4 & 0xfff);
    kprintf("VBAR_EL1 0x%lx, SCTLR_EL1 0x%lx (bit 0 = MMU %s)\n", READ_SYSREG(vbar_el1),
            READ_SYSREG(sctlr_el1), (READ_SYSREG(sctlr_el1) & 1) ? "on" : "off");
    if (psci::init(dt)) {
        int64_t v = psci::call(psci::kVersion);
        kprintf("PSCI through %s: version %ld.%ld\n", psci::method(), v >> 16, v & 0xffff);
    }

    NeutralEnv env{arch::kName, dtb, arch::kPageSize};
    int failures = run_neutral_tests(env);

    // Acceptance test 2a: a data abort on an unmapped address. The first address after the
    // RAM range the devicetree reported has nothing behind it on this machine.
    fdt::Node mem;
    uint64_t ram = 0, ram_size = 0;
    dt.find_path("/memory", &mem);
    dt.reg(mem, 0, &ram, &ram_size);
    volatile uint32_t* hole = reinterpret_cast<volatile uint32_t*>(ram + ram_size);
    kprintf("test: read from 0x%lx (just past the end of RAM)\n", reinterpret_cast<uint64_t>(hole));
    trap::expect_fault();
    uint32_t v = *hole;
    uint64_t esr = 0, far = 0;
    bool abort_ok = trap::fault_seen(&esr, &far) && trap::ec(esr) == 0x25 &&
                    far == reinterpret_cast<uint64_t>(hole);
    kprintf("test: data abort %s (value read: 0x%x)\n", abort_ok ? "reported and decoded" : "NOT seen", v);

    // Acceptance test 2b: an undefined instruction. UDF #0 is permanently undefined.
    kprintf("test: execute UDF #0\n");
    trap::expect_fault();
    asm volatile("udf #0");
    bool udf_ok = trap::fault_seen(&esr, &far) && trap::ec(esr) == 0x00;
    kprintf("test: undefined instruction %s\n", udf_ok ? "reported and decoded" : "NOT seen");

    if (failures == 0 && abort_ok && udf_ok) {
        kprintf("D1 ok\n");
        arch::qemu_exit(arch::kExitPass);
    }
    kprintf("D1 FAILED\n");
    arch::qemu_exit(arch::kExitFail);
}
