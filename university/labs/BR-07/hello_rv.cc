// hello_rv.cc - BR-07 forensic lab: Lena's first kernel for the board.
// Written and tested on QEMU's riscv64 virt machine; then run, unchanged, on the board.
#include <stdint.h>

namespace {

constexpr uintptr_t kUart = 0x10000000;   // the ns16550 console "of the machine"

void putc(char c)
{
    auto* thr = reinterpret_cast<volatile uint8_t*>(kUart + 0);   // transmit holding register
    auto* lsr = reinterpret_cast<volatile uint8_t*>(kUart + 5);   // line status register
    while ((*lsr & 0x20) == 0) {                                   // wait: transmitter empty
    }
    *thr = static_cast<uint8_t>(c);
}

}  // namespace

extern "C" void kmain()
{
    for (const char* s = "hello from Lena's kernel\n"; *s != '\0'; ++s) {
        putc(*s);
    }
    for (;;) {
    }
}
