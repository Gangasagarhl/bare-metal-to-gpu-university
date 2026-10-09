// paging.h - F3-21: x86-64 4-level page tables owned by the kernel.
// Entry bits from Intel SDM Vol. 3, "Paging" (4-level paging, page-table entry formats);
// pending verification (see F3-21 D1). The lab's runs check them against QEMU.
#pragma once
#include <cstdint>

namespace paging {

inline constexpr uint64_t kPresent = 1ull << 0;
inline constexpr uint64_t kWrite = 1ull << 1;
inline constexpr uint64_t kUser = 1ull << 2;
inline constexpr uint64_t kWriteThrough = 1ull << 3;   // PWT
inline constexpr uint64_t kCacheDisable = 1ull << 4;   // PCD
inline constexpr uint64_t kAccessed = 1ull << 5;
inline constexpr uint64_t kDirty = 1ull << 6;
inline constexpr uint64_t kHuge = 1ull << 7;           // PS in a PDE: a 2 MiB page
inline constexpr uint64_t kPat4K = 1ull << 7;          // PAT bit in a 4 KiB PTE
inline constexpr uint64_t kGlobal = 1ull << 8;
inline constexpr uint64_t kNoExec = 1ull << 63;        // needs EFER.NXE
inline constexpr uint64_t kAddrMask = 0x000FFFFFFFFFF000ull;

// The four 9-bit indices and the 12-bit offset of a canonical 48-bit virtual address.
struct Split {
    unsigned pml4, pdpt, pd, pt;
    uint64_t offset;
};
constexpr Split split(uint64_t virt)
{
    return Split{static_cast<unsigned>((virt >> 39) & 511), static_cast<unsigned>((virt >> 30) & 511),
                 static_cast<unsigned>((virt >> 21) & 511), static_cast<unsigned>((virt >> 12) & 511),
                 virt & 0xFFF};
}

class AddressSpace {
public:
    bool init();                                             // allocates an empty PML4
    bool map(uint64_t virt, uint64_t phys, uint64_t flags);  // one 4 KiB page
    bool map_2m(uint64_t virt, uint64_t phys, uint64_t flags);
    bool unmap(uint64_t virt);                               // clears the PTE and invlpg
    bool unmap_no_flush(uint64_t virt);                      // the forensic lab's bug
    bool protect(uint64_t virt, uint64_t flags);
    // Software page walk: physical address and the leaf entry's flags, false if unmapped.
    bool translate(uint64_t virt, uint64_t& phys, uint64_t* leaf = nullptr) const;
    uint64_t root() const { return pml4_; }
    void activate() const;                                   // load CR3
    uint64_t tables() const { return tables_; }

private:
    uint64_t* entry(uint64_t virt, int level, bool create);  // level 1 = PT ... 3 = PDPT
    uint64_t pml4_ = 0;                                      // physical address
    uint64_t tables_ = 0;                                    // page-table pages allocated
};

void enable_cpu_features();   // EFER.NXE, CR0.WP, CR4.PGE; prints what it set
AddressSpace& kernel_space();
bool build_kernel_space(uint64_t mbi_phys);   // kernel image + direct map, then switch CR3

} // namespace paging
