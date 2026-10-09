// bmc_hello.cc - the smallest program for an emulated BMC system-on-chip: QEMU's ast2600-evb
// machine (an AST2600 evaluation board model with Cortex-A7 cores). It writes a greeting to the
// UART that QEMU connects to its first serial port, prints one word read from the system
// control unit (SCU), and stops. Bare metal: no OpenBMC, no U-Boot, no Linux.
// The addresses come from QEMU's own memory map of this machine (bmc_mtree.out), not from a
// datasheet; the meaning of the SCU word is NOT verified (see F5-30).
#include <stdint.h>

namespace {

volatile uint32_t* const kUart = reinterpret_cast<volatile uint32_t*>(0x1e784000);  // "serial"
volatile uint32_t* const kScu = reinterpret_cast<volatile uint32_t*>(0x1e6e2000);   // "aspeed.scu"

void put(char c)
{
    *kUart = static_cast<uint8_t>(c);   // transmit register at offset 0 (16550-style UART model)
}

void print(const char* s)
{
    for (; *s != '\0'; ++s) {
        put(*s);
    }
}

void hex(uint32_t v)
{
    print("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        put("0123456789abcdef"[(v >> shift) & 0xf]);
    }
}

}  // namespace

extern "C" [[noreturn]] void start()
{
    print("bmc_hello: running on the emulated BMC SoC\n");
    print("bmc_hello: SCU word at offset 0x004 = ");
    hex(kScu[1]);
    print("\nbmc_hello: done, halting\n");
    for (;;) {
        __asm__ volatile("wfi");        // wait for an interrupt that never comes
    }
}
