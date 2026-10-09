// blink.cc - F1-76 Listing 2: blink an LED, then make a software PWM signal, timed by a
// hardware timer, on the emulated lab microcontroller (QEMU mps2-an385).
// Register offsets: Arm CMSDK APB timer and the MPS2 FPGA I/O block, as modelled by
// QEMU 8.2 (see the unverified box in F1-76; check the CMSDK TRM and the board's
// application note before using them on hardware).
#include <stdint.h>

namespace {

volatile uint32_t& reg(uintptr_t base, uint32_t offset)
{
    return *reinterpret_cast<volatile uint32_t*>(base + offset);
}

constexpr uintptr_t kUart0 = 0x40004000;   // "uart" in mtree.out of F1-73
constexpr uintptr_t kTimer0 = 0x40000000;  // "cmsdk-apb-timer"
constexpr uintptr_t kFpgaIo = 0x40028000;  // "mps2-fpgaio": LED register at offset 0

void putChar(char c)
{
    while ((reg(kUart0, 0x04) & 0x1u) != 0) {
    }
    reg(kUart0, 0x00) = static_cast<uint8_t>(c);
}

void putString(const char* s)
{
    while (*s != '\0') {
        putChar(*s++);
    }
}

void putDec(uint32_t v)
{
    char buf[11];
    int n = 0;
    do {
        buf[n++] = static_cast<char>('0' + v % 10);
        v /= 10;
    } while (v != 0);
    while (n > 0) {
        putChar(buf[--n]);
    }
}

// Timer 0: count down from RELOAD to 0, then reload and set the interrupt status flag.
void timerStart(uint32_t reload)
{
    reg(kTimer0, 0x00) = 0;          // CTRL: stop while configuring
    reg(kTimer0, 0x08) = reload;     // RELOAD
    reg(kTimer0, 0x04) = reload;     // VALUE: start the first period now
    reg(kTimer0, 0x0C) = 1;          // INTCLEAR: clear an old flag
    reg(kTimer0, 0x00) = (1u << 3) | (1u << 0);   // CTRL: interrupt flag enable, timer enable
}

void waitTick()
{
    while ((reg(kTimer0, 0x0C) & 0x1u) == 0) {
        // poll INTSTATUS: the timer reached zero
    }
    reg(kTimer0, 0x0C) = 1;          // INTCLEAR: write 1 to clear the flag
}

void setLed(bool on)
{
    reg(kFpgaIo, 0x00) = on ? 1u : 0u;   // LED0 = bit 0
}

}  // namespace

int main()
{
    reg(kUart0, 0x08) = 0x1;   // UART0 transmitter on
    putString("F1-76: blink and software PWM on the emulated lab MCU\n");
    timerStart(2500);          // one tick = 2501 timer clock cycles (RELOAD + 1)

    putString("blink: ");
    bool led = false;
    for (int i = 0; i < 6; ++i) {
        for (int t = 0; t < 4; ++t) {
            waitTick();        // 4 ticks per LED state
        }
        led = !led;
        setLed(led);
        putChar(led ? '1' : '0');
    }
    putString("\n");

    // Software PWM: period 10 ticks, LED on for the first `duty` ticks of each period.
    const uint32_t duties[3] = {2, 5, 8};
    for (uint32_t duty : duties) {
        putString("pwm duty ");
        putDec(duty);
        putString("/10: ");
        for (uint32_t tick = 0; tick < 20; ++tick) {
            const bool on = (tick % 10) < duty;
            setLed(on);
            putChar(on ? '#' : '_');
            waitTick();
        }
        putString("\n");
    }
    setLed(false);
    putString("done\n");
    return 0;
}
