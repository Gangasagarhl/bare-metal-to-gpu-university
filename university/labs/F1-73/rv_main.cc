// rv_main.cc - the same banner on the RISC-V virt machine (UART "serial" region in QEMU's
// memory tree; a 16550-compatible UART whose transmit register is at offset 0).
#include <stdint.h>

namespace {

constexpr uintptr_t kUartBase = 0x10000000;   // "serial" in info mtree
constexpr uintptr_t kTestBase = 0x00100000;   // "riscv.sifive.test" in info mtree

void putChar(char c)
{
    *reinterpret_cast<volatile uint8_t*>(kUartBase) = static_cast<uint8_t>(c);
}

void putString(const char* s)
{
    while (*s != '\0') {
        putChar(*s++);
    }
}

}  // namespace

uint64_t ticks;   // .bss, zeroed by rv_start.S

extern "C" int main()
{
    putString("HW303 RISC-V virt: hello from bare metal\n");
    putString(ticks == 0 ? "ticks is 0: .bss was cleared\n" : "ticks is NOT 0\n");
    // Ask QEMU's test device to stop the emulator with "pass" (value 0x5555).
    *reinterpret_cast<volatile uint32_t*>(kTestBase) = 0x5555;
    return 0;
}
