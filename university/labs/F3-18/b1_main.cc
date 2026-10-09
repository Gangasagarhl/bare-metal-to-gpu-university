// b1_main.cc - F3-18 test kernel for milestone B1: constructors run, printing works,
// a failed assertion panics and ends QEMU with the failure status.
#include <cstdint>
#include "arch.h"
#include "kprint.h"
#include "multiboot.h"
#include "panic.h"
#include "serial.h"

extern "C" char __text_start[], __rodata_start[], __data_start[], __bss_start[], __bss_end[];

namespace {

struct B1Probe {
    int value;
    B1Probe() : value(42)
    {
        serial::init();   // the first constructor to run may not assume anything else ran
        kprintf("B1 ok: a global constructor ran before kmain\n");
    }
};
B1Probe g_probe;          // constructed by run_global_constructors() in boot.S

uint64_t g_zeroed[4];     // in .bss: must read as zero because boot.S cleared it

} // namespace

extern "C" void kmain(uint32_t magic, uint32_t info_phys)
{
    kprintf("kmain: magic=0x%x info=0x%x probe.value=%d\n", magic, info_phys, g_probe.value);
    KASSERT(magic == mb::kBootMagic);
    auto* info = reinterpret_cast<const mb::Info*>(uintptr_t{info_phys});   // identity-mapped
    if (info->flags & mb::kFlagMem) {
        kprintf("memory: lower %u KiB, upper %u KiB\n", info->mem_lower, info->mem_upper);
    }
    kprintf("sections: .text %p .rodata %p .data %p .bss %p-%p\n", static_cast<void*>(__text_start),
            static_cast<void*>(__rodata_start), static_cast<void*>(__data_start),
            static_cast<void*>(__bss_start), static_cast<void*>(__bss_end));
    kprintf("bss check: %lu %lu %lu %lu\n", g_zeroed[0], g_zeroed[1], g_zeroed[2], g_zeroed[3]);
    kprintf("format check: [%5d] [%05u] [%x] [%lx] [%s] [%c] [%%]\n", -42, 99u, 0xbeefu,
            0xffffffff80000000ul, "str", 'k');
#ifdef B1_ASSERT_TEST
    int two = 2;
    KASSERT(two + two == 5);   // deliberate failure: the panic line is the expected output
#endif
    kprintf("B1 done\n");
    arch::qemu_exit(arch::kExitPass);
}
