// pmm.h - F3-20: the kernel's physical memory manager (a buddy allocator over the
// normalized firmware memory map).
#pragma once
#include <cstdint>
#include "buddy.h"
#include "multiboot.h"

namespace pmm {

inline constexpr uint64_t kDirectMap = 0xffff800000000000;   // boot.S maps physical 0..8 GiB here
inline constexpr uint64_t kDirectMapLimit = 8ull << 30;

inline void* phys_to_virt(uint64_t phys)
{
    return reinterpret_cast<void*>(kDirectMap + phys);
}

struct Accounting {
    uint64_t conventional;   // bytes of type-1 ("available") RAM in the firmware's map
    uint64_t low_kept;       // usable bytes below 1 MiB that we keep out of the allocator
    uint64_t kernel;         // the kernel image, including .bss, rounded to pages
    uint64_t boot_info;      // the Multiboot information and memory map pages
    uint64_t metadata;       // the buddy allocator's state bytes
    uint64_t rounding;       // partial pages at the ends of ranges
    uint64_t managed;        // bytes handed to the allocator
};

void init(const mb::Info* info, bool check_order);
bool alloc(int order, uint64_t& phys);       // 2^order contiguous 4 KiB frames
bool free(uint64_t phys, int order);
const Buddy::Stats& stats();
const Accounting& accounting();
void print_stats(const char* tag);

} // namespace pmm
