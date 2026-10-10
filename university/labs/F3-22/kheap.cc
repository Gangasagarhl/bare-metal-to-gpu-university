// kheap.cc - F3-22: connects heap.h to the PMM (F3-20) and the kernel page tables (F3-21).
#include "kheap.h"
#include "kprint.h"
#include "paging.h"
#include "panic.h"
#include "pmm.h"

namespace {

constexpr uint64_t kLargeRegion = 0xffffd00000000000;   // large blocks are mapped here
constexpr uint64_t kLargeRegionSize = 1ull << 40;
uint64_t g_large_next = kLargeRegion;                   // bump pointer; ranges are not reused
Heap g_heap;

void* get_slab()
{
    uint64_t phys;
    if (!pmm::alloc(2, phys)) {                          // order 2 = 16 KiB, aligned to 16 KiB
        return nullptr;
    }
    return pmm::phys_to_virt(phys);
}

void put_slab(void* p)
{
    KASSERT(pmm::free(reinterpret_cast<uint64_t>(p) - pmm::kDirectMap, 2));
}

void* get_pages(size_t pages)
{
    if (g_large_next + (pages + 1) * 4096 > kLargeRegion + kLargeRegionSize) {
        return nullptr;
    }
    uint64_t virt = g_large_next + 4096;                 // leave an unmapped guard page before
    for (size_t i = 0; i < pages; ++i) {
        uint64_t phys;
        if (!pmm::alloc(0, phys)) {
            for (size_t k = 0; k < i; ++k) {             // undo the partial mapping
                uint64_t pk;
                KASSERT(paging::kernel_space().translate(virt + k * 4096, pk));
                KASSERT(paging::kernel_space().unmap(virt + k * 4096));
                KASSERT(pmm::free(pk, 0));
            }
            return nullptr;
        }
        KASSERT(paging::kernel_space().map(virt + i * 4096, phys, paging::kWrite | paging::kNoExec | paging::kGlobal));
    }
    g_large_next = virt + pages * 4096;
    return reinterpret_cast<void*>(virt);
}

void put_pages(void* p, size_t pages)
{
    uint64_t virt = reinterpret_cast<uint64_t>(p);
    for (size_t i = 0; i < pages; ++i) {
        uint64_t phys;
        KASSERT(paging::kernel_space().translate(virt + i * 4096, phys));
        KASSERT(paging::kernel_space().unmap(virt + i * 4096));
        KASSERT(pmm::free(phys, 0));
    }
}

bool is_large(const void* p)
{
    uint64_t v = reinterpret_cast<uint64_t>(p);
    return v >= kLargeRegion && v < kLargeRegion + kLargeRegionSize;
}

void report(const char* what, const char* cache, const void* obj, size_t offset, uint8_t value)
{
    PANIC("heap: %s in cache %s, object %p, byte %lu = 0x%02x", what, cache, obj, offset, unsigned{value});
}

} // namespace

void kheap_init()
{
    g_heap.init(HeapEnv{get_slab, put_slab, get_pages, put_pages, is_large, report});
}

void* kmalloc(size_t size, size_t align)
{
    return g_heap.alloc(size, align);
}

void kfree(void* p)
{
    g_heap.free(p);
}

Heap& kheap()
{
    return g_heap;
}

void kheap_report(const char* tag)
{
    Heap::CacheStats st[Heap::kClasses];
    int n = g_heap.cache_stats(st);
    kprintf("%s live objects %lu; per cache (live/slabs):", tag, g_heap.live_objects());
    for (int i = 0; i < n; ++i) {
        kprintf(" %lu:%lu/%lu", st[i].size, st[i].live, st[i].slabs);
    }
    kprintf("; large %lu blocks, %lu pages; PMM free frames %lu\n", g_heap.large_live(),
            g_heap.large_pages(), pmm::stats().free_frames);
}
