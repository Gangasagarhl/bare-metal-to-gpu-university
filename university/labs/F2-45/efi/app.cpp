// app.cpp: a freestanding UEFI application that does NOT use the UEFI services. It talks
// only to two devices that the lab adds to QEMU itself: the debug console (I/O port 0xe9)
// and isa-debug-exit (I/O port 0xf4), so the run proves that the firmware loaded and started
// our PE32+ file without depending on UEFI structure layouts we could not verify in this build.
#include <stdint.h>

namespace {

inline void outb(uint16_t port, uint8_t value)
{
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

void say(const char* text)
{
    for (const char* p = text; *p != '\0'; ++p) {
        outb(0xe9, static_cast<uint8_t>(*p));
    }
}

void sayHex(uint64_t value)
{
    say("0x");
    for (int shift = 60; shift >= 0; shift -= 4) {
        outb(0xe9, static_cast<uint8_t>("0123456789abcdef"[(value >> shift) & 0xf]));
    }
}

}  // namespace

// A table of absolute pointers. The entries are not const and have external linkage, so the
// compiler cannot fold them into the code: the program really reads each 64-bit address from
// memory at run time. The linker must therefore emit base relocations (.reloc) so that the
// firmware can fix the addresses when it loads the image somewhere other than ImageBase.
extern const char* g_lines[];
const char* g_lines[] = {
    "PE32+ app: started by the firmware\n",
    "PE32+ app: absolute pointers in this table were relocated correctly\n",
    nullptr,
};

extern "C" uint64_t efi_main(void* /*image_handle*/, void* /*system_table*/)
{
    for (const char** line = g_lines; *line != nullptr; ++line) {
        say(*line);
    }
    // Where did the firmware put us? The linker assumed ImageBase + entry RVA.
    say("PE32+ app: efi_main is running at ");
    sayHex(reinterpret_cast<uint64_t>(&efi_main));
    say("\n");
    outb(0xf4, 0x21);   // QEMU exits with status (0x21 << 1) | 1 = 67
    for (;;) {
        asm volatile("hlt");
    }
}
