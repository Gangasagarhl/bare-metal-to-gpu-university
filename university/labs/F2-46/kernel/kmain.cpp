// kmain.cpp: prints the addresses that link.ld exported, and checks the layout it promised.
#include "console.h"

// Symbols defined by the linker script, not by C++ code. Only their ADDRESSES are meaningful:
// declaring them as arrays makes "__text_start" itself the address, with no load from memory.
extern "C" char __kernel_start[], __text_start[], __text_end[], __rodata_start[],
    __data_start[], __bss_start[], __bss_end[], __kernel_end[];

int g_initialised = 1234;   // .data: its value comes from the file
int g_zeroed;               // .bss: no bytes in the file; boot.S zeroes it

namespace {

struct Row {
    const char* name;
    const char* address;
};

uint32_t addr(const char* p)
{
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p));
}

}  // namespace

extern "C" void kmain()
{
    const Row rows[] = {
        {"__kernel_start", __kernel_start}, {"__text_start  ", __text_start},
        {"__text_end    ", __text_end},     {"__rodata_start", __rodata_start},
        {"__data_start  ", __data_start},   {"__bss_start   ", __bss_start},
        {"__bss_end     ", __bss_end},      {"__kernel_end  ", __kernel_end},
    };
    print("F2-46 kernel: symbols exported by link.ld\n");
    for (const Row& r : rows) {
        print("  ");
        print(r.name);
        print(" = ");
        printHex(addr(r.address));
        print("\n");
    }
    // A plain array, not a braced list: std::initializer_list needs a C++ library header,
    // and this freestanding target has none (chapter F2-48).
    const char* const starts[] = {__text_start, __rodata_start, __data_start, __bss_start};
    bool pageAligned = true;
    for (const char* p : starts) {
        pageAligned = pageAligned && (addr(p) % 4096 == 0);
    }
    print("section starts are 4 KiB aligned: ");
    print(pageAligned ? "yes\n" : "NO\n");
    print("kernel size in memory: ");
    printDec(addr(__kernel_end) - addr(__kernel_start));
    print(" bytes\n");
    print("g_initialised = ");
    printDec(static_cast<uint32_t>(g_initialised));
    print(" at ");
    printHex(addr(reinterpret_cast<const char*>(&g_initialised)));
    print("\ng_zeroed      = ");
    printDec(static_cast<uint32_t>(g_zeroed));
    print(" at ");
    printHex(addr(reinterpret_cast<const char*>(&g_zeroed)));
    print("\n");
    qemuExit(pageAligned ? 0x10 : 0x11);   // 0x10 -> QEMU exit status 33
}
