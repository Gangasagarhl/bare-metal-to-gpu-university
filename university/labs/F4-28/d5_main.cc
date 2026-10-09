// d5_main.cc - F4-28: milestone D5 on QEMU's RISC-V virt machine. Console from the
// devicetree, SBI discovery, the arch-neutral tests, Sv39 paging with an identity map, then
// the two traps the D5 acceptance test asks for (illegal instruction, page fault), decoded.
#include <cstdint>
#include "arch.h"
#include "fdt.h"
#include "kprint.h"
#include "neutral_tests.h"
#include "sbi.h"
#include "sv39.h"
#include "trap.h"
#include "uart16550.h"

namespace {

alignas(4096) uint64_t g_root[512];   // Sv39 root page table (in .bss: starts all invalid)

bool console_from_devicetree(const fdt::Blob& dt)
{
#ifdef FORENSIC_FIXED_UART   // the F4-23 forensic kernel: "the UART is always at 0x10000000"
    uart16550::set_base(0x10000000, 0);
    (void)dt;
    return true;
#endif
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
    if (!dt.find_path(path, &uart) || !dt.is_compatible(uart, "ns16550a") || !dt.reg(uart, 0, &base, &size)) {
        return false;
    }
    fdt::Prop shift;
    uint32_t reg_shift = dt.get_prop(uart, "reg-shift", &shift) ? fdt::be32(shift.data) : 0;
    uart16550::set_base(base, reg_shift);
    kprintf("console: %s (ns16550a) at 0x%lx, reg-shift %u, from /chosen/stdout-path\n", path, base, reg_shift);
    return true;
}

void report_sbi()
{
    int64_t v = sbi::spec_version();
    kprintf("SBI specification %ld.%ld, implementation id %ld, implementation version 0x%lx\n",
            (v >> 24) & 0x7f, v & 0xffffff, sbi::impl_id(), sbi::impl_version());
    struct {
        uint64_t id;
        const char* name;
    } const ext[] = {{sbi::kTime, "TIME"}, {sbi::kIpi, "IPI"}, {sbi::kRfence, "RFENCE"},
                     {sbi::kHsm, "HSM"}, {sbi::kSrst, "SRST"}, {sbi::kDbcn, "DBCN"}};
    kprintf("SBI extensions:");
    for (const auto& e : ext) {
        kprintf(" %s=%s", e.name, sbi::probe(e.id) ? "yes" : "no");
    }
    kprintf("\n");
}

// Identity map: GiB 0 (devices: CLINT, PLIC, UART, virtio) read/write, GiB 2 (RAM, where the
// kernel runs) read/write/execute. GiB 1 stays unmapped on purpose: the page-fault test.
void enable_paging(uint64_t ram_base)
{
    g_root[0] = sv39::leaf(0, sv39::kR | sv39::kW);
    g_root[ram_base / sv39::kGiB] = sv39::leaf(ram_base & ~(sv39::kGiB - 1), sv39::kR | sv39::kW | sv39::kX);
    arch::sfence_vma();
    CSR_WRITE(satp, sv39::satp_value(g_root));
    arch::sfence_vma();
    kprintf("Sv39 on: satp 0x%lx (mode %lu, root table at 0x%lx)\n", CSR_READ(satp), CSR_READ(satp) >> 60,
            reinterpret_cast<uint64_t>(g_root));
}

} // namespace

extern "C" void kmain(uint64_t hartid, const void* dtb)
{
    fdt::Blob dt;
    if (!dt.init(dtb) || !console_from_devicetree(dt)) {
        arch::halt_forever();
    }
    kprintf("D5: hart %lu entered in S-mode, devicetree at %p (%u bytes)\n", hartid, dtb, dt.total_size());
    kprintf("stvec 0x%lx, sstatus 0x%lx, satp 0x%lx\n", CSR_READ(stvec), CSR_READ(sstatus), CSR_READ(satp));
    report_sbi();

    NeutralEnv env{arch::kName, dtb, arch::kPageSize};
    int failures = run_neutral_tests(env);

    fdt::Node mem;
    uint64_t ram = 0, ram_size = 0;
    dt.find_path("/memory", &mem);
    dt.reg(mem, 0, &ram, &ram_size);
    enable_paging(ram);
    kprintf("still running after satp write: kmain at 0x%lx\n", reinterpret_cast<uint64_t>(&kmain));

    // Acceptance test: an illegal instruction. Writing the read-only CSR "cycle" is illegal.
    uint64_t cause = 0, tval = 0;
    kprintf("test: csrw cycle, zero (write to a read-only CSR)\n");
    trap::expect_fault();
    asm volatile("csrw cycle, zero");
    bool ill_ok = trap::fault_seen(&cause, &tval) && cause == 2;
    kprintf("test: illegal instruction %s\n", ill_ok ? "reported and decoded" : "NOT seen");

    // Acceptance test: a page fault. 0x40000000 is in the unmapped second GiB.
    volatile uint64_t* hole = reinterpret_cast<volatile uint64_t*>(0x40000000);
    kprintf("test: load from 0x%lx (no page-table entry)\n", reinterpret_cast<uint64_t>(hole));
    trap::expect_fault();
    uint64_t v = *hole;
    bool pf_ok = trap::fault_seen(&cause, &tval) && cause == 13 && tval == reinterpret_cast<uint64_t>(hole);
    kprintf("test: load page fault %s (register value afterwards 0x%lx)\n", pf_ok ? "reported and decoded" : "NOT seen", v);

    if (failures == 0 && ill_ok && pf_ok) {
        kprintf("D5 ok\n");
        arch::qemu_exit(arch::kExitPass);
    }
    kprintf("D5 FAILED\n");
    arch::qemu_exit(arch::kExitFail);
}
