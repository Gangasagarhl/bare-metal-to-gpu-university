// main.cc - F1-74: where does everything live? Prints the address of one object of each
// kind, so the learner can match the program's own view with the map file.
// USART register offsets and bits: STM32F100 reference manual, USART chapter (pending
// verification); QEMU 8.2 models this USART with its "stm32f2xx-usart" device.
#include <stdint.h>

namespace {

constexpr uintptr_t kUsart1Base = 0x40013800;   // "stm32f2xx-usart" region in mtree.out

struct Usart {
    volatile uint32_t sr;    // 0x00 status: bit 7 = transmit data register empty (TXE)
    volatile uint32_t dr;    // 0x04 data
    volatile uint32_t brr;   // 0x08 baud rate
    volatile uint32_t cr1;   // 0x0C control 1: bit 13 = UE, bit 3 = TE
};

Usart& usart1()
{
    return *reinterpret_cast<Usart*>(kUsart1Base);
}

void putChar(char c)
{
    while ((usart1().sr & (1u << 7)) == 0) {
        // wait for TXE
    }
    usart1().dr = static_cast<uint8_t>(c);
}

void putString(const char* s)
{
    while (*s != '\0') {
        putChar(*s++);
    }
}

void putHex(uint32_t v)
{
    putString("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        putChar("0123456789abcdef"[(v >> shift) & 0xFu]);
    }
}

void report(const char* what, const void* where, uint32_t value)
{
    putString(what);
    putHex(reinterpret_cast<uintptr_t>(where));
    putString("  value ");
    putHex(value);
    putChar('\n');
}

}  // namespace

extern "C" uint32_t __data_load;
extern "C" uint32_t __ram_used;

const uint32_t kTable[4] = {0x11111111, 0x22222222, 0x33333333, 0x44444444};  // .rodata
uint32_t calibration = 0x0000C0DE;   // .data: VMA in RAM, LMA in flash
uint32_t counter;                    // .bss

int main()
{
    usart1().cr1 = (1u << 13) | (1u << 3);   // UE: enable the USART, TE: enable the transmitter
    putString("F1-74 memory map tour (QEMU stm32vldiscovery)\n");
    report("code   main()         at ", reinterpret_cast<const void*>(&main), 0);
    report("rodata kTable[0]      at ", &kTable[0], kTable[0]);
    report("data   calibration    at ", &calibration, calibration);
    report("  its initial value   at ", &__data_load, __data_load);
    report("bss    counter        at ", &counter, counter);
    uint32_t local = 0x5555AAAA;
    report("stack  local          at ", &local, local);
    putString("RAM used by .data + .bss = ");
    putHex(reinterpret_cast<uintptr_t>(&__ram_used));   // a linker symbol: its address is the value
    putChar('\n');

    // Try to change flash with an ordinary store, then read it back.
    volatile uint32_t* flashWord = const_cast<volatile uint32_t*>(&kTable[0]);
    *flashWord = 0xDEADBEEF;
    report("after a store to flash, kTable[0] at ", &kTable[0], *flashWord);

    ++counter;
    calibration += 1;
    report("RAM is writable: calibration now ", &calibration, calibration);
    putString("done\n");
    return 0;
}
