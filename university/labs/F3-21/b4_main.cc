// b4_main.cc - F3-21 test kernel for milestone B4 (paging and the kernel address space).
//   (none)          acceptance tests, prints the layout, then waits for the QEMU monitor
//   -DB4_OVERFLOW   runs on a kernel stack with a 4 KiB guard page and overflows it
//   -DB4_STALE_TLB  the forensic kernel: a driver unmaps its buffer without invalidating
//   -DB4_STALE_FIXED  the same driver with the fix (unmap() invalidates the TLB entry)
#include <cstdint>
#include "arch.h"
#include "gdt.h"
#include "interrupts.h"
#include "kprint.h"
#include "multiboot.h"
#include "paging.h"
#include "panic.h"
#include "pmm.h"
#include "serial.h"

// Runs BODY with ARG in rsi; if BODY faults, the exception handler resumes at label 1.
#define TRIGGER1(BODY, ARG)                                 \
    asm volatile("leaq 1f(%%rip), %%rax\n\t"                \
                 "movq %%rax, g_resume_rip(%%rip)\n\t"      \
                 BODY "\n"                                  \
                 "1:\n"                                     \
                 :                                          \
                 : "S"(ARG)                                 \
                 : "rax", "rcx", "rdx", "memory")

extern void (*g_double_fault_hook)(const InterruptFrame&);
extern "C" char __text_start[], __text_end[], __rodata_start[], __data_start[], __bss_end[];

namespace {

using paging::AddressSpace;
constexpr uint64_t kStackRegion = 0xffffff0000000000;   // guard page here, stack above it
constexpr uint64_t kTestRegion = 0xffffc00000000000;    // random test mappings
const volatile uint64_t kReadOnlyWord = 0x524f4e4c59;   // in .rodata (hex digits spell "RONLY")
volatile uint64_t g_walk_target = 0x5452414e534c4154;   // in .data (hex digits spell "TRANSLAT")
uint32_t g_mbi = 0;

uint64_t g_rng = 0x2545F4914F6CDD1Dull;
[[maybe_unused]] uint64_t next_random()          // xorshift64: a deterministic sequence, the same every run
{
    g_rng ^= g_rng << 13;
    g_rng ^= g_rng >> 7;
    g_rng ^= g_rng << 17;
    return g_rng;
}

uint64_t new_frame()
{
    uint64_t phys;
    KASSERT(pmm::alloc(0, phys));
    return phys;
}

#if !defined(B4_STALE_TLB) && !defined(B4_STALE_FIXED)
[[maybe_unused]] void test_rodata_write()
{
    kprintf("-- B4 test 1: write to .rodata at %016lx\n", reinterpret_cast<uint64_t>(&kReadOnlyWord));
    TRIGGER1("movq $1, (%%rsi)", &kReadOnlyWord);
    kprintf("   value still %lx\n", kReadOnlyWord);
}

[[maybe_unused]] void test_nx()
{
    uint64_t phys = new_frame();
    auto* code = static_cast<uint8_t*>(pmm::phys_to_virt(phys));
    code[0] = 0xC3;                                         // the instruction "ret"
    kprintf("-- B4 test 2: call a data page (direct map %p) containing 'ret'\n", static_cast<void*>(code));
    asm volatile("leaq 1f(%%rip), %%rax\n\t"
                 "movq %%rax, g_resume_rip(%%rip)\n\t"
                 "movq %%rsp, %%rdx\n\t"                    // the call pushes; restore after
                 "call *%%rsi\n"
                 "1:\n\t"
                 "movq %%rdx, %%rsp\n"
                 :
                 : "S"(code)
                 : "rax", "rcx", "rdx", "memory");
    KASSERT(pmm::free(phys, 0));
}

[[maybe_unused]] void test_translate()
{
    AddressSpace& as = paging::kernel_space();
    constexpr int kCount = 1000;
    static uint64_t virts[kCount], physs[kCount];
    int agree = 0, cpu_agree = 0;
    for (int i = 0; i < kCount; ++i) {
        uint64_t v;
        uint64_t dummy;
        do {
            v = kTestRegion + ((next_random() & 0x3FFFFFFFull) << 12);   // 2^30 pages = 4 TiB
        } while (as.translate(v, dummy));
        uint64_t p = new_frame();
        KASSERT(as.map(v, p, paging::kWrite | paging::kNoExec));
        *static_cast<volatile uint64_t*>(pmm::phys_to_virt(p)) = v ^ 0xA5A5A5A5A5A5A5A5ull;
        virts[i] = v;
        physs[i] = p;
    }
    for (int i = 0; i < kCount; ++i) {
        uint64_t off = next_random() & 0xFF8;                 // a random 8-byte slot in the page
        uint64_t got;
        if (as.translate(virts[i] + off, got) && got == physs[i] + off) {
            ++agree;
        }
        if (*reinterpret_cast<volatile uint64_t*>(virts[i]) == (virts[i] ^ 0xA5A5A5A5A5A5A5A5ull)) {
            ++cpu_agree;                                       // the MMU found the same frame
        }
    }
    kprintf("-- B4 test 3: %d random mappings: software walk agrees %d, CPU access agrees %d; "
            "page-table pages now %lu\n", kCount, agree, cpu_agree, as.tables());
    kprintf("   example: virt %016lx -> phys %016lx\n", virts[0], physs[0]);
    KASSERT(agree == kCount && cpu_agree == kCount);
    for (int i = 0; i < kCount; ++i) {
        KASSERT(as.unmap(virts[i]));
        KASSERT(pmm::free(physs[i], 0));
    }
}

[[maybe_unused]] void test_unmap_faults()
{
    AddressSpace& as = paging::kernel_space();
    uint64_t v = kTestRegion + 0x123456000ull;
    uint64_t p = new_frame();
    KASSERT(as.map(v, p, paging::kWrite | paging::kNoExec));
    *reinterpret_cast<volatile uint64_t*>(v) = 42;            // the TLB now caches v -> p
    KASSERT(as.unmap(v));
    kprintf("-- B4 test 4: read %016lx after unmap (expect a page fault)\n", v);
    TRIGGER1("movq (%%rsi), %%rcx", v);
    KASSERT(pmm::free(p, 0));
}

[[maybe_unused]] void print_layout()
{
    kprintf("B4 layout (virtual ranges):\n");
    kprintf("  %016lx-%016lx kernel .text   R X global\n", reinterpret_cast<uint64_t>(__text_start),
            reinterpret_cast<uint64_t>(__text_end) - 1);
    kprintf("  %016lx-%016lx kernel .rodata R NX global\n", reinterpret_cast<uint64_t>(__rodata_start),
            reinterpret_cast<uint64_t>(__data_start) - 1);
    kprintf("  %016lx-%016lx kernel .data/.bss RW NX global\n", reinterpret_cast<uint64_t>(__data_start),
            reinterpret_cast<uint64_t>(__bss_end) - 1);
    kprintf("  %016lx-...              direct map of available RAM, RW NX global\n", pmm::kDirectMap);
    kprintf("  %016lx                  guard page (never mapped) below the 16 KiB kernel stack\n",
            kStackRegion);
    kprintf("walk target: virt %016lx holds %016lx\n", reinterpret_cast<uint64_t>(&g_walk_target), g_walk_target);
}
#endif

[[maybe_unused]] void double_fault_seen(const InterruptFrame&)
{
    kprintf("B4 guard page test: the overflow hit the guard page and was reported on IST1\n");
    arch::qemu_exit(arch::kExitPass);
}

[[gnu::noinline, maybe_unused]] uint64_t recurse(uint64_t depth)
{
    volatile uint8_t filler[256];
    filler[0] = static_cast<uint8_t>(depth);
    if (depth > 100000000) {
        return depth;
    }
    return recurse(depth + 1) + filler[0];
}

#if defined(B4_STALE_TLB) || defined(B4_STALE_FIXED)
// The forensic driver: a DMA-style buffer is mapped at a fixed virtual address, used,
// unmapped and its frame returned. Then another subsystem gets the same frame.
void stale_tlb_driver()
{
    AddressSpace& as = paging::kernel_space();
    const uint64_t buf = kTestRegion + 0x40000000ull;
    for (int round = 1; round <= 3; ++round) {
        uint64_t frame = new_frame();
        KASSERT(as.map(buf, frame, paging::kWrite | paging::kNoExec));
        volatile uint64_t* b = reinterpret_cast<volatile uint64_t*>(buf);
        b[0] = 0xD00D0000 + round;                            // the driver uses its buffer
#if defined(B4_STALE_FIXED)
        KASSERT(as.unmap(buf));                               // the fix: unmap() does invlpg
#else
        KASSERT(as.unmap_no_flush(buf));                      // "release" (bug: no invlpg)
#endif
        KASSERT(pmm::free(frame, 0));
        uint64_t owner;
        KASSERT(pmm::alloc(0, owner));                        // the next allocation, elsewhere
        auto* table = static_cast<volatile uint64_t*>(pmm::phys_to_virt(owner));
        table[0] = 0x600D;                                    // the new owner initialises it
        uint64_t pte_phys;
        bool mapped = as.translate(buf, pte_phys);
#if defined(B4_STALE_FIXED)
        TRIGGER1("movq $0xBAD0000, (%%rsi)", b);              // the late write now faults
#else
        b[0] = 0xBAD0000 + round;                             // a late write by the old driver
#endif
        kprintf("round %d: driver frame %016lx, new owner frame %016lx; software walk of the "
                "buffer: %s; new owner's word 0 = %lx\n", round, frame, owner,
                mapped ? "mapped" : "not mapped", table[0]);
        KASSERT(pmm::free(owner, 0));
    }
}
#endif

void tests_on_new_stack()
{
#if defined(B4_OVERFLOW)
    kprintf("-- B4 guard test: recursion on the 16 KiB kernel stack\n");
    g_double_fault_hook = double_fault_seen;
    recurse(0);
#elif defined(B4_STALE_TLB) || defined(B4_STALE_FIXED)
    stale_tlb_driver();
    arch::qemu_exit(arch::kExitPass);
#else
    test_rodata_write();
    test_nx();
    test_translate();
    test_unmap_faults();
    print_layout();
    pmm::print_stats("pmm after the tests:");
    kprintf("B4 ok; waiting for the QEMU monitor\n");
    for (;;) {
        arch::hlt();                                           // interrupts are off: stays here
    }
#endif
}

} // namespace

extern "C" void kmain(uint32_t magic, uint32_t info_phys)
{
    serial::init();
    KASSERT(magic == mb::kBootMagic);
    g_mbi = info_phys;
    gdt::init(true);
    idt::init(true);
    pmm::init(static_cast<const mb::Info*>(pmm::phys_to_virt(info_phys)), true);
    KASSERT(paging::build_kernel_space(info_phys));
    // A 16 KiB kernel stack with an unmapped guard page below it.
    for (uint64_t i = 1; i <= 4; ++i) {
        KASSERT(paging::kernel_space().map(kStackRegion + i * 4096, new_frame(),
                                           paging::kWrite | paging::kNoExec | paging::kGlobal));
    }
    uint64_t top = kStackRegion + 5 * 4096;
    kprintf("kernel stack: %016lx-%016lx, guard page at %016lx\n", kStackRegion + 4096, top - 1, kStackRegion);
    void (*fn)() = tests_on_new_stack;
    asm volatile("mov %0, %%rsp\n\t"
                 "call *%1\n\t"
                 :
                 : "r"(top), "r"(fn)
                 : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11", "rax", "memory");
    PANIC("the test function returned");
}
