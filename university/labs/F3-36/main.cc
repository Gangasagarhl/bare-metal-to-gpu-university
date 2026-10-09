// main.cc - F3-36 Listing 3: first OS305 firmware (QEMU mps2-an385). No vendor framework:
// our start-up code, our linker script, our vector table. Prints a banner and a memory
// report, proves that global constructors ran, blinks LED0 three times, then stops QEMU.
#include "board.h"

extern "C" {
extern uint32_t __text_end, __data_start, __data_end, __bss_start, __bss_end;
extern uint32_t __stack_top, __stack_limit;
extern const uint32_t vector_table[];
}

uint32_t constructorsRun;      // .bss: zeroed by Reset_Handler before constructors run
uint32_t bootCount = 41;       // .data: initial value 41 is copied from flash to RAM

namespace {

class Led {
public:
    explicit Led(uint32_t bit) : mask_(1u << bit)
    {
        board::setLeds(0);     // a hardware write: this constructor must run at start-up
        ++constructorsRun;
    }
    void on() const { board::setLeds(mask_); }
    void off() const { board::setLeds(0); }

private:
    uint32_t mask_;
};

Led led0(0);                    // a global object with a constructor: needs .init_array

uint32_t addr(const void* p) { return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p)); }

}  // namespace

int main()
{
    board::uartInit();
    ++bootCount;
    board::print("OS305 F3-36: bare metal on a microcontroller\n");
    board::print("reset vector  = "); board::printHex(vector_table[1]); board::print("\n");
    board::print("initial SP    = "); board::printHex(vector_table[0]); board::print("\n");
    board::print("flash used    = "); board::printDec(addr(&__text_end) + (addr(&__data_end) - addr(&__data_start)));
    board::print(" bytes (code + initial data)\n");
    board::print("RAM used      = "); board::printDec(addr(&__bss_end) - addr(&__data_start));
    board::print(" bytes (.data + .bss) + stack ");
    board::printDec(addr(&__stack_top) - addr(&__stack_limit)); board::print(" bytes\n");
    board::print("bootCount     = "); board::printDec(bootCount); board::print(" (expect 42)\n");
    board::print("constructors  = "); board::printDec(constructorsRun); board::print(" (expect 1)\n");

    for (int i = 0; i < 3; ++i) {
        led0.on();
        board::delayPeriods(24999, 10);   // 10 x 25,000 processor clocks on, then off
        led0.off();
        board::delayPeriods(24999, 10);
        board::print("blink "); board::printDec(static_cast<uint32_t>(i + 1)); board::print("\n");
    }
    bool ok = bootCount == 42 && constructorsRun == 1;
    board::print(ok ? "self-check PASS\n" : "self-check FAIL\n");
    board::exitEmulator(ok);
}
