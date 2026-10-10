// paging.cc - F3-21: building, changing and switching to the kernel's own page tables.
#include "paging.h"
#include "arch.h"
#include "kprint.h"
#include "multiboot.h"
#include "panic.h"
#include "pmm.h"

extern "C" char __text_start[], __text_end[], __rodata_start[], __data_start[], __bss_end[];

namespace paging {
namespace {

constexpr uint64_t kKernelVma = 0xffffffff80000000;
AddressSpace g_kernel;

uint64_t* table_virt(uint64_t phys)
{
    return static_cast<uint64_t*>(pmm::phys_to_virt(phys));
}

} // namespace

bool AddressSpace::init()
{
    if (!pmm::alloc(0, pml4_)) {
        return false;
    }
    uint64_t* t = table_virt(pml4_);
    for (int i = 0; i < 512; ++i) {
        t[i] = 0;
    }
    tables_ = 1;
    return true;
}

// Returns the entry for 'virt' in the table of the given level (4 = PML4, 3 = PDPT, 2 = PD,
// 1 = PT), creating missing intermediate tables when 'create' is true.
uint64_t* AddressSpace::entry(uint64_t virt, int level, bool create)
{
    uint64_t* table = table_virt(pml4_);
    for (int l = 4; l > level; --l) {
        unsigned idx = (virt >> (12 + 9 * (l - 1))) & 511;
        uint64_t e = table[idx];
        if (!(e & kPresent)) {
            if (!create) {
                return nullptr;
            }
            uint64_t phys;
            if (!pmm::alloc(0, phys)) {
                return nullptr;
            }
            uint64_t* nt = table_virt(phys);
            for (int i = 0; i < 512; ++i) {
                nt[i] = 0;
            }
            ++tables_;
            // Intermediate entries allow everything; the leaf entry restricts.
            table[idx] = phys | kPresent | kWrite | kUser;
            e = table[idx];
        } else if (e & kHuge) {
            return nullptr;                         // a 2 MiB page is in the way
        }
        table = table_virt(e & kAddrMask);
    }
    return &table[(virt >> (12 + 9 * (level - 1))) & 511];
}

bool AddressSpace::map(uint64_t virt, uint64_t phys, uint64_t flags)
{
    uint64_t* e = entry(virt, 1, true);
    if (e == nullptr || (*e & kPresent)) {
        return false;                               // no memory, or already mapped
    }
    *e = (phys & kAddrMask) | flags | kPresent;
    return true;
}

bool AddressSpace::map_2m(uint64_t virt, uint64_t phys, uint64_t flags)
{
    uint64_t* e = entry(virt, 2, true);
    if (e == nullptr || (*e & kPresent)) {
        return false;
    }
    *e = (phys & kAddrMask) | flags | kPresent | kHuge;
    return true;
}

bool AddressSpace::unmap_no_flush(uint64_t virt)
{
    uint64_t* e = entry(virt, 1, false);
    if (e == nullptr || !(*e & kPresent)) {
        return false;
    }
    *e = 0;
    return true;
}

bool AddressSpace::unmap(uint64_t virt)
{
    if (!unmap_no_flush(virt)) {
        return false;
    }
    arch::invlpg(virt);                             // drop any cached translation of 'virt'
    return true;
}

bool AddressSpace::protect(uint64_t virt, uint64_t flags)
{
    uint64_t* e = entry(virt, 1, false);
    if (e == nullptr || !(*e & kPresent)) {
        return false;
    }
    *e = (*e & kAddrMask) | flags | kPresent;
    arch::invlpg(virt);
    return true;
}

bool AddressSpace::translate(uint64_t virt, uint64_t& phys, uint64_t* leaf) const
{
    const uint64_t* table = table_virt(pml4_);
    for (int l = 4; l >= 1; --l) {
        unsigned shift = 12 + 9 * (l - 1);
        uint64_t e = table[(virt >> shift) & 511];
        if (!(e & kPresent)) {
            return false;
        }
        if (l == 1 || (l == 2 && (e & kHuge))) {
            uint64_t page_mask = (1ull << shift) - 1;
            phys = ((e & kAddrMask) & ~page_mask) | (virt & page_mask);
            if (leaf != nullptr) {
                *leaf = e;
            }
            return true;
        }
        table = table_virt(e & kAddrMask);
    }
    return false;
}

void AddressSpace::activate() const
{
    arch::write_cr3(pml4_);
}

void enable_cpu_features()
{
    arch::wrmsr(0xC0000080, arch::rdmsr(0xC0000080) | (1ull << 11));   // EFER.NXE
    arch::write_cr0(arch::read_cr0() | (1ull << 16));                   // CR0.WP
    arch::write_cr4(arch::read_cr4() | (1ull << 7));                    // CR4.PGE
    kprintf("paging: EFER %lx, CR0 %lx, CR4 %lx (NXE, WP, PGE set), PAT MSR %016lx\n",
            arch::rdmsr(0xC0000080), arch::read_cr0(), arch::read_cr4(), arch::rdmsr(0x277));
}

AddressSpace& kernel_space()
{
    return g_kernel;
}

bool build_kernel_space(uint64_t mbi_phys)
{
    enable_cpu_features();
    if (!g_kernel.init()) {
        return false;
    }
    auto map_range = [](uint64_t vstart, uint64_t vend, uint64_t flags) {
        for (uint64_t v = vstart & ~0xFFFull; v < vend; v += 4096) {
            KASSERT(g_kernel.map(v, v - kKernelVma, flags));
        }
    };
    // The kernel image, section by section, with the least rights each section needs.
    map_range(reinterpret_cast<uint64_t>(__text_start), reinterpret_cast<uint64_t>(__text_end), kGlobal);
    map_range(reinterpret_cast<uint64_t>(__rodata_start), reinterpret_cast<uint64_t>(__data_start),
              kGlobal | kNoExec);
    map_range(reinterpret_cast<uint64_t>(__data_start), reinterpret_cast<uint64_t>(__bss_end),
              kGlobal | kWrite | kNoExec);
    // The direct map: every "available" range of the firmware map, read/write, never executable.
    const auto* info = static_cast<const mb::Info*>(pmm::phys_to_virt(mbi_phys));
    uint64_t p = info->mmap_addr;
    uint64_t pages_4k = 0, pages_2m = 0;
    while (p < uint64_t{info->mmap_addr} + info->mmap_length) {
        const auto* e = static_cast<const mb::MmapEntry*>(pmm::phys_to_virt(p));
        p += e->size + 4;
        // Available RAM: read/write. Other ranges below 4 GiB (firmware data such as the ACPI
        // and SMBIOS tables, BIOS ROM): read-only. Ranges above 4 GiB that are not RAM: skipped.
        if (e->type != 1 && e->addr >= (1ull << 32)) {
            continue;
        }
        uint64_t a = e->addr & ~0xFFFull;
        uint64_t end = (e->addr + e->len + 0xFFF) & ~0xFFFull;
        while (a < end) {
            uint64_t flags = (e->type == 1 ? kWrite : 0) | kNoExec | kGlobal;
            if ((a & 0x1FFFFF) == 0 && a + 0x200000 <= end) {
                KASSERT(g_kernel.map_2m(pmm::kDirectMap + a, a, flags));
                a += 0x200000;
                ++pages_2m;
            } else {
                uint64_t already;                       // a partial page shared by two ranges
                if (!g_kernel.translate(pmm::kDirectMap + a, already)) {
                    KASSERT(g_kernel.map(pmm::kDirectMap + a, a, flags));
                    ++pages_4k;
                }
                a += 4096;
            }
        }
    }
    // The legacy BIOS area 0xE0000-0xFFFFF is not always listed; the SMBIOS and ACPI entry
    // points are searched there (F3-23, F3-24). Map what is missing, read-only.
    for (uint64_t a = 0xE0000; a < 0x100000; a += 4096) {
        uint64_t dummy;
        if (!g_kernel.translate(pmm::kDirectMap + a, dummy)) {
            KASSERT(g_kernel.map(pmm::kDirectMap + a, a, kNoExec | kGlobal));
            ++pages_4k;
        }
    }
    kprintf("paging: direct map built with %lu pages of 2 MiB and %lu pages of 4 KiB; "
            "%lu page-table pages in total\n", pages_2m, pages_4k, g_kernel.tables());
    g_kernel.activate();
    kprintf("paging: CR3 = %016lx, the boot page tables are no longer used\n", arch::read_cr3());
    return true;
}

} // namespace paging
