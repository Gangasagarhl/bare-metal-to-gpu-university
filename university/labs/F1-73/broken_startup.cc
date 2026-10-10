// broken_startup.cc - FORENSIC EVIDENCE: a copy of startup.cc with one "harmless" edit.
// Used only by the F1-73 forensic lab. Do not use it as a template.
// Freestanding C++: no operating system, no C library, no exceptions, no RTTI.
// Vector-table layout and reset behaviour: Armv7-M Architecture Reference Manual,
// "exception model" and "vector table" sections (pending verification, see F1-73 sources).
#include <stdint.h>

// Symbols defined by the linker script (mps2_an385.ld). Only their addresses matter.
extern "C" uint32_t __stack_top;
extern "C" uint32_t __data_load;   // where the initial values of .data sit in code memory
extern "C" uint32_t __data_start;  // where .data must be in RAM
extern "C" uint32_t __data_end;
extern "C" uint32_t __bss_start;
extern "C" uint32_t __bss_end;

int main();

extern "C" [[noreturn]] void Reset_Handler();
extern "C" [[noreturn]] void Default_Handler();

extern "C" [[noreturn]] void Reset_Handler()
{
    // 1. Copy the initial values of initialised variables from code memory to RAM.
    const uint32_t* src = &__data_load;
    for (uint32_t* dst = &__data_start; dst < &__data_end; ++dst, ++src) {
        *dst = *src;
    }
    // 2. Fill the zero-initialised variables with zeros.
    for (uint32_t* dst = &__bss_start; dst < &__bss_end; ++dst) {
        *dst = 0;
    }
    // 3. Run the program.
    main();
    // 4. Firmware never returns. (Edited: "wfi is not needed, an empty loop is enough".)
    for (;;) {
    }
}

extern "C" [[noreturn]] void Default_Handler()
{
    for (;;) {
        asm volatile("bkpt #0");
    }
}

using Handler = void (*)();

// The vector table: word 0 is the initial stack pointer, word 1 the reset handler,
// then the handlers of the other system exceptions (NMI, HardFault, ...).
extern "C" __attribute__((section(".vectors"), used))
const Handler vector_table[16] = {
    reinterpret_cast<Handler>(&__stack_top), // 0  initial value of SP
    Reset_Handler,                           // 1  reset
    Default_Handler,                         // 2  NMI
    Default_Handler,                         // 3  HardFault
    Default_Handler,                         // 4  MemManage
    Default_Handler,                         // 5  BusFault
    Default_Handler,                         // 6  UsageFault
    nullptr, nullptr, nullptr, nullptr,      // 7-10 reserved
    Default_Handler,                         // 11 SVCall
    Default_Handler,                         // 12 DebugMonitor
    nullptr,                                 // 13 reserved
    Default_Handler,                         // 14 PendSV
    Default_Handler,                         // 15 SysTick
};
