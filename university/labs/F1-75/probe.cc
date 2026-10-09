// probe.cc - F1-75 Listing 2: touch the clock controller (RCC) and a GPIO port of the
// emulated STM32F100 and read the registers back. Offsets and bit numbers follow the
// STM32F100 reference manual's RCC and GPIO chapters as recalled by the author: NOT
// verified in this build (see the unverified box in F1-75). The run shows what QEMU does.
#include <stdint.h>

namespace {

constexpr uintptr_t kUsart1 = 0x40013800;   // modelled by QEMU ("stm32f2xx-usart")
constexpr uintptr_t kRcc = 0x40021000;      // "RCC" placeholder region in QEMU's tree
constexpr uintptr_t kGpioC = 0x40011000;    // "GPIOC" placeholder region in QEMU's tree

volatile uint32_t& reg(uintptr_t base, uint32_t offset)
{
    return *reinterpret_cast<volatile uint32_t*>(base + offset);
}

void putChar(char c)
{
    while ((reg(kUsart1, 0x00) & (1u << 7)) == 0) {
    }
    reg(kUsart1, 0x04) = static_cast<uint8_t>(c);
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

void show(const char* name, uint32_t wrote, uint32_t readBack)
{
    putString(name);
    putString(": wrote ");
    putHex(wrote);
    putString(", read back ");
    putHex(readBack);
    putString(readBack == wrote ? "  (kept)\n" : "  (NOT kept)\n");
}

}  // namespace

int main()
{
    reg(kUsart1, 0x0C) = (1u << 13) | (1u << 3);   // USART1 on, transmitter on
    putString("F1-75 probe: clock gate and GPIO registers on the emulated STM32F100\n");

    const uint32_t gate = (1u << 4);               // GPIOC clock enable (unverified bit)
    reg(kRcc, 0x18) = gate;
    show("RCC  +0x18 (APB2 clock enable)", gate, reg(kRcc, 0x18));

    const uint32_t mode = 0x00000011;              // pins 8 and 9 as outputs (unverified)
    reg(kGpioC, 0x04) = mode;
    show("GPIOC+0x04 (mode, pins 8-15) ", mode, reg(kGpioC, 0x04));

    const uint32_t out = (1u << 8) | (1u << 9);   // drive pins 8 and 9 high (unverified)
    reg(kGpioC, 0x0C) = out;
    show("GPIOC+0x0C (output data)     ", out, reg(kGpioC, 0x0C));
    putString("done\n");
    return 0;
}
