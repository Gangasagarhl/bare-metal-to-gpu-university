// app.cc - F3-41 Listing 7: the application that the boot loader starts. Built twice:
// APP_VERSION=1 linked for slot A, APP_VERSION=2 linked for slot B. It proves that it runs
// with its own vector table by taking SysTick interrupts through it.
#include "board.h"

extern "C" const uint32_t vector_table[];

volatile uint32_t ticks = 0;

extern "C" void SysTick_Handler()
{
    ticks = ticks + 1;
}

int main()
{
    board::uartInit();
    board::print("application version "); board::printDec(APP_VERSION);
    board::print(" running; VTOR = "); board::printHex(board::reg(0xE000ED08));
    board::print(", my vector table at "); board::printHex(reinterpret_cast<uintptr_t>(vector_table));
    board::print("\n");
    board::reg(board::kSystRvr) = 24999;
    board::reg(board::kSystCvr) = 0;
    board::reg(board::kSystCsr) = 0x7;
    while (ticks < 3) {
        asm volatile("wfi");
    }
    board::print("3 SysTick interrupts arrived through the application's own vector table\n");
    board::exitEmulator(true);
}
