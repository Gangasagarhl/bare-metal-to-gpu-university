// mmu.cc - F4-25: identity-mapped translation tables for the QEMU virt memory map.
#include "mmu.h"
#include "arch.h"
#include "kprint.h"

extern "C" char __text_start[], __text_end[], __rodata_start[], __rodata_end[];

namespace {

alignas(4096) uint64_t g_l1[512];        // level 1: 1 GiB per entry
alignas(4096) uint64_t g_l2_ram[512];    // level 2 for GiB 1: 2 MiB per entry
alignas(4096) uint64_t g_l3_kernel[512]; // level 3 for the first 2 MiB of RAM: 4 KiB pages

constexpr uint64_t kGiB = uint64_t{1} << 30;
constexpr uint64_t k2MiB = uint64_t{1} << 21;
constexpr uint64_t kPage = 4096;
constexpr uint64_t kAddrMask = 0x0000fffffffff000ull;   // output address bits 47:12

uint64_t ram_page_attrs(uint64_t pa)
{
    uint64_t normal = mmu::attr(mmu::kAttrNormal) | mmu::kInnerShareable | mmu::kAccessFlag;
    auto text_lo = reinterpret_cast<uint64_t>(__text_start);
    auto text_hi = reinterpret_cast<uint64_t>(__text_end);
    auto ro_lo = reinterpret_cast<uint64_t>(__rodata_start);
    auto ro_hi = reinterpret_cast<uint64_t>(__rodata_end);
    if (pa >= text_lo && pa < text_hi) {
        return normal | mmu::kApReadOnly | mmu::kUXN;                       // R-X
    }
    if (pa >= ro_lo && pa < ro_hi) {
        return normal | mmu::kApReadOnly | mmu::kPXN | mmu::kUXN;           // R--
    }
    return normal | mmu::kPXN | mmu::kUXN;                                  // RW-
}

const char* kind(int level, uint64_t d)
{
    if ((d & mmu::kValid) == 0) {
        return "invalid";
    }
    if (level == 3) {
        return (d & mmu::kTableOrPage) ? "page" : "reserved";
    }
    return (d & mmu::kTableOrPage) ? "table" : "block";
}

} // namespace

namespace mmu {

void init(uint64_t ram_base, uint64_t ram_size)
{
    // GiB 0: the virt machine's devices (GIC, UART, virtio-mmio ...): Device memory, never executable.
    g_l1[0] = 0 | attr(kAttrDevice) | kAccessFlag | kPXN | kUXN | kValid;
    // GiB 1: RAM. First 2 MiB through 4 KiB pages (the kernel image lives there), the rest as
    // 2 MiB blocks, only as far as the devicetree says RAM goes.
    g_l1[ram_base / kGiB] = reinterpret_cast<uint64_t>(g_l2_ram) | kTableOrPage | kValid;
    for (uint64_t i = 0; i < 512; ++i) {
        g_l3_kernel[i] = (ram_base + i * kPage) | ram_page_attrs(ram_base + i * kPage) | kTableOrPage | kValid;
    }
    g_l2_ram[0] = reinterpret_cast<uint64_t>(g_l3_kernel) | kTableOrPage | kValid;
    for (uint64_t i = 1; i < 512 && i * k2MiB < ram_size; ++i) {
        g_l2_ram[i] = (ram_base + i * k2MiB) | attr(kAttrNormal) | kInnerShareable | kAccessFlag |
                      kPXN | kUXN | kValid;
    }

    enable_this_cpu();
    kprintf("MMU on: MAIR 0x%lx TCR 0x%lx TTBR0 0x%lx SCTLR 0x%lx (PARange %lu)\n", READ_SYSREG(mair_el1),
            READ_SYSREG(tcr_el1), READ_SYSREG(ttbr0_el1), READ_SYSREG(sctlr_el1),
            READ_SYSREG(id_aa64mmfr0_el1) & 0xf);
}

void enable_this_cpu()
{
    uint64_t mair = (uint64_t{0x00} << (8 * kAttrDevice)) | (uint64_t{0xff} << (8 * kAttrNormal));
    uint64_t parange = READ_SYSREG(id_aa64mmfr0_el1) & 0xf;
    uint64_t tcr = 25                       // T0SZ: 64 - 25 = 39-bit addresses, walk starts at level 1
                   | (1u << 8) | (1u << 10) // IRGN0, ORGN0: write-back cacheable table walks
                   | (3u << 12)             // SH0: inner shareable
                   | (0u << 14)             // TG0: 4 KiB granule
                   | (1u << 23)             // EPD1: no walks through TTBR1 (no upper half yet)
                   | (parange << 32);       // IPS: physical address size = what the CPU supports
    WRITE_SYSREG(mair_el1, mair);
    WRITE_SYSREG(tcr_el1, tcr);
    WRITE_SYSREG(ttbr0_el1, reinterpret_cast<uint64_t>(g_l1));
    asm volatile("dsb ish\n\tisb\n\ttlbi vmalle1\n\tdsb ish\n\tisb" : : : "memory");
    uint64_t sctlr = READ_SYSREG(sctlr_el1);
    sctlr |= (1u << 0) | (1u << 2) | (1u << 12);   // M: MMU on; C: data cache; I: instruction cache
    sctlr &= ~uint64_t{1u << 1};                   // A: no alignment checks for Normal memory
    WRITE_SYSREG(sctlr_el1, sctlr);
    arch::isb();
}

void explain(uint64_t va)
{
    kprintf("walk 0x%lx: indexes L1 %lu, L2 %lu, L3 %lu, offset 0x%lx\n", va, (va >> 30) & 511,
            (va >> 21) & 511, (va >> 12) & 511, va & 0xfff);
    const uint64_t* table = g_l1;
    for (int level = 1; level <= 3; ++level) {
        int shift = 39 - 9 * level;           // 30, 21, 12
        uint64_t d = table[(va >> shift) & 511];
        kprintf("  L%d[%lu] = 0x%016lx  %s\n", level, (va >> shift) & 511, d, kind(level, d));
        if ((d & kValid) == 0) {
            kprintf("  -> translation fault at level %d\n", level);
            return;
        }
        bool leaf = level == 3 || (d & kTableOrPage) == 0;
        if (leaf) {
            uint64_t size_mask = (uint64_t{1} << shift) - 1;
            kprintf("  -> PA 0x%lx, AttrIdx %lu, %s, %s\n", (d & kAddrMask & ~size_mask) | (va & size_mask),
                    (d >> 2) & 7, (d & kApReadOnly) ? "read-only" : "read-write",
                    (d & kPXN) ? "not executable" : "executable");
            return;
        }
        table = reinterpret_cast<const uint64_t*>(d & kAddrMask);
    }
}

} // namespace mmu
