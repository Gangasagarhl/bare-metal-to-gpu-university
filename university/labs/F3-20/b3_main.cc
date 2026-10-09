// b3_main.cc - F3-20 test kernel for milestone B3 (physical memory manager).
//   (none)      accounting report, allocate-everything test, contiguous/aligned test
//   -DB3_LEAK   the forensic "network driver" that leaks frames (statistics over time)
#include <cstdint>
#include "arch.h"
#include "kprint.h"
#include "multiboot.h"
#include "panic.h"
#include "pmm.h"
#include "serial.h"

namespace {

#if !defined(B3_LEAK)
void allocate_everything_test()
{
    const Buddy::Stats before = pmm::stats();
    uint64_t count = 0;
    uint64_t phys;
    uint64_t first_page_seen = ~0ull;
    while (pmm::alloc(0, phys)) {
        // keep a list of every frame inside the frames themselves: frame i holds frame i-1
        *static_cast<uint64_t*>(pmm::phys_to_virt(phys)) = first_page_seen;
        first_page_seen = phys;
        ++count;
    }
    kprintf("allocate-all: got %lu frames, free count now %lu\n", count, pmm::stats().free_frames);
    while (first_page_seen != ~0ull) {
        uint64_t prev = *static_cast<uint64_t*>(pmm::phys_to_virt(first_page_seen));
        KASSERT(pmm::free(first_page_seen, 0));
        first_page_seen = prev;
    }
    const Buddy::Stats& after = pmm::stats();
    bool same = after.free_frames == before.free_frames;
    for (int o = 0; o <= Buddy::kMaxOrder; ++o) {
        same = same && after.free_blocks[o] == before.free_blocks[o];
    }
    kprintf("free-all: free count %lu (start %lu), free blocks back to the start shape: %s\n",
            after.free_frames, before.free_frames, same ? "yes" : "NO");
    KASSERT(same && count == before.free_frames);
}

void contiguous_test()
{
    const int orders[] = {4, 9};                  // 64 KiB and 2 MiB, for DMA buffers
    for (int order : orders) {
        uint64_t phys;
        KASSERT(pmm::alloc(order, phys));
        uint64_t size = 4096ull << order;
        kprintf("contiguous: order %d (%lu KiB) at %016lx, aligned to its size: %s\n", order,
                size / 1024, phys, (phys % size) == 0 ? "yes" : "NO");
        KASSERT(phys % size == 0);
        KASSERT(!pmm::free(phys, order - 1));     // wrong order: refused (order checking on)
        KASSERT(pmm::free(phys, order));
        KASSERT(!pmm::free(phys, order));         // double free: refused
    }
    kprintf("contiguous: wrong-order and double frees were refused\n");
}
#else
// A toy network driver: every packet gets an 8 KiB receive buffer (order 1). After a change
// of the buffer size from 4 KiB to 8 KiB, the release path still passes order 0.
constexpr int kRxOrderAlloc = 1;
constexpr int kRxOrderFree = 0;

void leaking_driver()
{
    pmm::print_stats("t=0      ");
    for (int packet = 1; packet <= 20000; ++packet) {
        uint64_t buf;
        if (!pmm::alloc(kRxOrderAlloc, buf)) {
            PANIC("out of memory at packet %d", packet);
        }
        static_cast<uint8_t*>(pmm::phys_to_virt(buf))[0] = 0x45;   // "receive" a packet
        pmm::free(buf, kRxOrderFree);
        if (packet % 4000 == 0) {
            char tag[16];
            ksnprintf(tag, sizeof tag, "t=%6d ", packet);
            pmm::print_stats(tag);
        }
    }
}
#endif

} // namespace

extern "C" void kmain(uint32_t magic, uint32_t info_phys)
{
    serial::init();
    KASSERT(magic == mb::kBootMagic);
    const auto* info = static_cast<const mb::Info*>(pmm::phys_to_virt(info_phys));
#if !defined(B3_LEAK)
    pmm::init(info, true);
    const pmm::Accounting& a = pmm::accounting();
    kprintf("accounting (KiB): conventional %lu = managed %lu + below 1 MiB %lu + kernel %lu"
            " + boot info %lu + metadata %lu + rounding %lu\n",
            a.conventional / 1024, a.managed / 1024, a.low_kept / 1024, a.kernel / 1024,
            a.boot_info / 1024, a.metadata / 1024, a.rounding / 1024);
    uint64_t sum = a.managed + a.low_kept + a.kernel + a.boot_info + a.metadata + a.rounding;
    kprintf("check: sum of parts %s conventional; allocator reports %lu KiB free\n",
            sum == a.conventional ? "equals" : "DIFFERS FROM", pmm::stats().free_frames * 4);
    KASSERT(sum == a.conventional && pmm::stats().free_frames * 4096 == a.managed);
    pmm::print_stats("after boot:");
    allocate_everything_test();
    contiguous_test();
    kprintf("B3 ok\n");
#else
    pmm::init(info, false);
    leaking_driver();
    kprintf("driver test finished\n");
#endif
    arch::qemu_exit(arch::kExitPass);
}
