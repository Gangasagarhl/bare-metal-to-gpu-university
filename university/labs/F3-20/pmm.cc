// pmm.cc - F3-20: build the allocator from the Multiboot memory map.
#include "pmm.h"
#include "kprint.h"
#include "memmap.h"
#include "panic.h"

extern "C" char __bss_end_phys[];    // linker.ld: physical end of the kernel image

namespace pmm {
namespace {

Buddy g_buddy;
Accounting g_acc;

const char* type_name(uint32_t t)
{
    return t == 1 ? "available" : t == 2 ? "reserved" : t == 3 ? "ACPI reclaimable"
         : t == 4 ? "ACPI NVS" : t == 5 ? "bad memory" : "other";
}

} // namespace

void init(const mb::Info* info, bool check_order)
{
    KASSERT(info->flags & mb::kFlagMmap);
    memmap::RangeList ram;
    g_acc = Accounting{};
    kprintf("firmware memory map (Multiboot, from the BIOS):\n");
    uint64_t p = info->mmap_addr;
    while (p < uint64_t{info->mmap_addr} + info->mmap_length) {
        const auto* e = static_cast<const mb::MmapEntry*>(phys_to_virt(p));
        kprintf("  %016lx-%016lx %10lu KiB  type %u %s\n", e->addr, e->addr + e->len - 1,
                e->len / 1024, e->type, type_name(e->type));
        if (e->type == 1 && ram.n < memmap::kMaxRanges) {
            ram.r[ram.n++] = memmap::Range{e->addr, e->addr + e->len};
            g_acc.conventional += e->len;
        }
        p += e->size + 4;                         // 'size' does not count itself
    }
    memmap::normalize(ram);
    g_acc.rounding = g_acc.conventional - ram.bytes();
    auto take = [&](uint64_t base, uint64_t end, uint64_t& counter) {
        uint64_t before = ram.bytes();
        memmap::subtract(ram, base, end);
        counter += before - ram.bytes();
    };
    take(0, 0x100000, g_acc.low_kept);
    take(0x100000, reinterpret_cast<uint64_t>(__bss_end_phys), g_acc.kernel);
    take(reinterpret_cast<uint64_t>(info) - kDirectMap, reinterpret_cast<uint64_t>(info) - kDirectMap + sizeof(mb::Info),
         g_acc.boot_info);
    take(info->mmap_addr, uint64_t{info->mmap_addr} + info->mmap_length, g_acc.boot_info);
    uint64_t top = ram.r[ram.n - 1].end;
    if (top > kDirectMapLimit) {
        kprintf("pmm: RAM above 8 GiB ignored (not in the boot direct map)\n");
        memmap::subtract(ram, kDirectMapLimit, top);
        top = kDirectMapLimit;
    }
    uint64_t nframes = top / memmap::kPage;
    uint64_t meta_bytes = (nframes + memmap::kPage - 1) & ~(memmap::kPage - 1);
    uint64_t meta = 0;
    for (int i = 0; i < ram.n && meta == 0; ++i) {
        if (ram.r[i].end - ram.r[i].base >= meta_bytes) {
            meta = ram.r[i].base;
        }
    }
    KASSERT(meta != 0);
    take(meta, meta + meta_bytes, g_acc.metadata);
    g_buddy.init(static_cast<uint8_t*>(phys_to_virt(0)), static_cast<uint8_t*>(phys_to_virt(meta)),
                 nframes, check_order);
    kprintf("usable ranges handed to the buddy allocator:\n");
    for (int i = 0; i < ram.n; ++i) {
        kprintf("  %016lx-%016lx %10lu KiB\n", ram.r[i].base, ram.r[i].end - 1,
                (ram.r[i].end - ram.r[i].base) / 1024);
        g_buddy.add_frames(ram.r[i].base / memmap::kPage, (ram.r[i].end - ram.r[i].base) / memmap::kPage);
    }
    g_acc.managed = ram.bytes();
}

bool alloc(int order, uint64_t& phys)
{
    uint64_t f;
    if (!g_buddy.alloc(order, f)) {
        return false;
    }
    phys = f * memmap::kPage;
    return true;
}

bool free(uint64_t phys, int order)
{
    return g_buddy.free(phys / memmap::kPage, order);
}

const Buddy::Stats& stats()
{
    return g_buddy.stats();
}

const Accounting& accounting()
{
    return g_acc;
}

void print_stats(const char* tag)
{
    const Buddy::Stats& s = g_buddy.stats();
    kprintf("%s free %lu frames (%lu KiB), allocs %lu, frees %lu, free blocks by order:", tag,
            s.free_frames, s.free_frames * 4, s.allocs, s.frees);
    for (int o = 0; o <= Buddy::kMaxOrder; ++o) {
        kprintf(" %lu", s.free_blocks[o]);
    }
    kprintf("\n");
}

} // namespace pmm
