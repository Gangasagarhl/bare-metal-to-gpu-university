// b5_main.cc - F3-22 test kernel for milestone B5 (kernel heap and C++ allocation).
//   (none)            containers, every operator new/delete form, the 1,000,000-operation stress
//   -DB5_UAF          a deliberate use-after-free, caught by the poison check
//   -DB5_FORENSIC     the forensic "name table" (see the chapter's forensic lab)
#include <cstddef>
#include <cstdint>
#include <new>
#include "arch.h"
#include "containers.h"
#include "gdt.h"
#include "interrupts.h"
#include "kheap.h"
#include "kprint.h"
#include "multiboot.h"
#include "paging.h"
#include "panic.h"
#include "pmm.h"
#include "serial.h"

namespace {

uint64_t g_rng = 88172645463325252ull;
uint64_t next_random()
{
    g_rng ^= g_rng << 13;
    g_rng ^= g_rng >> 7;
    g_rng ^= g_rng << 17;
    return g_rng;
}

struct Task {
    int id;
    ListLink link;
};

struct alignas(256) Aligned256 {
    uint8_t bytes[300];
};

[[maybe_unused]] void containers_and_operators()
{
    KVector<uint64_t> v;
    for (uint64_t i = 1; i <= 10000; ++i) {
        KASSERT(v.push_back(i));
    }
    uint64_t sum = 0;
    for (size_t i = 0; i < v.size(); ++i) {
        sum += v[i];
    }
    IntrusiveList<Task, &Task::link> run_queue;
    for (int i = 0; i < 100; ++i) {
        run_queue.push_back(*new Task{i, {}});
    }
    int ids = 0;
    while (Task* t = run_queue.pop_front()) {
        ids += t->id;
        delete t;
    }
    KHashMap<uint64_t> squares;
    for (uint64_t k = 0; k < 5000; ++k) {
        KASSERT(squares.put(k * 7919, k * k));
    }
    uint64_t* hit = squares.get(4999 * 7919);
    kprintf("containers: vector of %lu sums to %lu; list gave ids summing to %d; map has %lu entries, "
            "map[4999*7919] = %lu\n", v.size(), sum, ids, squares.size(), hit != nullptr ? *hit : 0);
    int* one = new int(7);
    int* many = new int[1000];
    auto* al = new Aligned256;
    auto* arr = new Aligned256[3];
    void* huge = ::operator new(size_t{1} << 42, std::nothrow);     // 4 TiB: must fail politely
    kprintf("operators: new int %p, new int[1000] %p, new Aligned256 %p (mod 256 = %lu), new "
            "Aligned256[3] %p (mod 256 = %lu), nothrow 4 TiB -> %p\n", static_cast<void*>(one),
            static_cast<void*>(many), static_cast<void*>(al), reinterpret_cast<uint64_t>(al) % 256,
            static_cast<void*>(arr), reinterpret_cast<uint64_t>(arr) % 256, huge);
    KASSERT(reinterpret_cast<uint64_t>(al) % 256 == 0 && reinterpret_cast<uint64_t>(arr) % 256 == 0);
    KASSERT(huge == nullptr);
    delete one;
    delete[] many;
    delete al;
    delete[] arr;
}

[[maybe_unused]] void stress()
{
    constexpr int kSlots = 4096;
    struct Live {
        uint8_t* p;
        size_t size;
        uint64_t tag;
    };
    static Live live[kSlots];
    kheap().trim();
    const uint64_t frames_before = pmm::stats().free_frames;
    const uint64_t tables_before = paging::kernel_space().tables();
    uint64_t allocs = 0, frees = 0, corrupt = 0, large = 0;
    while (allocs < 1000000) {
        Live& s = live[next_random() % kSlots];
        if (s.p != nullptr) {                       // check the pattern, then free
            if (s.p[0] != static_cast<uint8_t>(s.tag) || s.p[s.size - 1] != static_cast<uint8_t>(s.tag >> 8)) {
                ++corrupt;
            }
            kfree(s.p);
            s.p = nullptr;
            ++frees;
            continue;
        }
        uint64_t r = next_random();
        size_t size = (r % 1000 == 0) ? 4096 + r % 60000 : 2 + r % 2047;   // 0.1 % large
        size_t align = size_t{1} << (r >> 20) % 9;                            // 1 .. 256
        s.p = static_cast<uint8_t*>(kmalloc(size, align));
        KASSERT(s.p != nullptr && reinterpret_cast<uint64_t>(s.p) % align == 0);
        s.size = size;
        s.tag = r;
        s.p[0] = static_cast<uint8_t>(r);
        s.p[size - 1] = static_cast<uint8_t>(r >> 8);
        large += size > 2048 ? 1 : 0;
        ++allocs;
    }
    for (Live& s : live) {
        if (s.p != nullptr) {
            kfree(s.p);
            s.p = nullptr;
            ++frees;
        }
    }
    kheap().trim();
    kprintf("stress: %lu allocations (%lu large) and %lu frees, mixed sizes 2 B..64 KiB, alignments "
            "1..256; corrupted blocks %lu\n", allocs, large, frees, corrupt);
    kheap_report("stress end:");
    const uint64_t new_tables = paging::kernel_space().tables() - tables_before;
    const uint64_t frames_after = pmm::stats().free_frames;
    kprintf("stress: PMM free frames %lu before, %lu after; %lu new page-table pages (kept for the "
            "large-block region); %s\n", frames_before, frames_after, new_tables,
            frames_after + new_tables == frames_before ? "no frame leaked" : "FRAMES LEAKED");
    KASSERT(corrupt == 0 && kheap().live_objects() == 0 && frames_after + new_tables == frames_before);
}

#if defined(B5_FORENSIC)
// A table of device names. Each entry is a 64-byte object holding a C string.
char* copy_name(const char* name)
{
    size_t len = 0;
    while (name[len] != '\0') {
        ++len;
    }
    char* e = static_cast<char*>(kmalloc(len < 64 ? 64 : len));   // "names are at most 64"
    for (size_t i = 0; i <= len; ++i) {                            // copies the terminator too
        e[i] = name[i];
    }
    return e;
}
#endif

} // namespace

extern "C" void kmain(uint32_t magic, uint32_t info_phys)
{
    serial::init();
    KASSERT(magic == mb::kBootMagic);
    gdt::init(true);
    idt::init(true);
    pmm::init(static_cast<const mb::Info*>(pmm::phys_to_virt(info_phys)), true);
    KASSERT(paging::build_kernel_space(info_phys));
    kheap_init();
#if defined(B5_UAF)
    char* p = static_cast<char*>(kmalloc(48));
    kprintf("use-after-free test: object %p from kmalloc(48)\n", static_cast<void*>(p));
    kfree(p);
    p[20] = 'X';                                   // the bug: writing through a dangling pointer
    void* again = kmalloc(48);                     // the poison check runs here
    kprintf("not reached: %p\n", again);
#elif defined(B5_FORENSIC)
    const char* names[] = {"uart0", "rtc", "pci-0000:00:01.0-isa-bridge",
                           "pci-0000:00:1f.2-ata-1.0-scsi-0:0:0:0-disk-serial-QM00001-part12", "nvme0n1", "e1000e-eth0", "hpet", "ps2-keyboard"};
    char* table[8];
    for (int i = 0; i < 8; ++i) {
        table[i] = copy_name(names[i]);
        kprintf("name %d at %p: %s\n", i, static_cast<void*>(table[i]), table[i]);
    }
    for (int i = 0; i < 8; ++i) {
        kfree(table[i]);
    }
    kprintf("all names freed\n");
#else
    kheap_report("after init:");
    containers_and_operators();
    kheap_report("after containers:");
    stress();
    kprintf("B5 ok\n");
#endif
    arch::qemu_exit(arch::kExitPass);
}
