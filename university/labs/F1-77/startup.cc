// startup.cc - F1-77 start-up code for the emulated lab microcontroller (QEMU mps2-an385).
// Same as F1-73, plus the pattern many vendor start-up files use: every exception handler
// is a WEAK ALIAS of Default_Handler, so the application overrides one by defining a
// function with exactly the same name. Vector-table layout: Armv7-M Architecture Reference
// Manual, "vector table" section (pending verification, see F1-77 sources).
#include <stdint.h>

extern "C" uint32_t __stack_top;
extern "C" uint32_t __data_load;
extern "C" uint32_t __data_start;
extern "C" uint32_t __data_end;
extern "C" uint32_t __bss_start;
extern "C" uint32_t __bss_end;

int main();

extern "C" [[noreturn]] void Reset_Handler();
extern "C" void Default_Handler();
extern "C" void HardFault_Handler() __attribute__((weak, alias("Default_Handler")));
extern "C" void SysTick_Handler() __attribute__((weak, alias("Default_Handler")));

extern "C" [[noreturn]] void Reset_Handler()
{
    const uint32_t* src = &__data_load;
    for (uint32_t* dst = &__data_start; dst < &__data_end; ++dst, ++src) {
        *dst = *src;
    }
    for (uint32_t* dst = &__bss_start; dst < &__bss_end; ++dst) {
        *dst = 0;
    }
    main();
    for (;;) {
        asm volatile("wfi");
    }
}

// Any exception without its own handler ends here: stop for the debugger.
extern "C" void Default_Handler()
{
    for (;;) {
        asm volatile("bkpt #0");
    }
}

using Handler = void (*)();

extern "C" __attribute__((section(".vectors"), used))
const Handler vector_table[16] = {
    reinterpret_cast<Handler>(&__stack_top), // 0  initial SP
    Reset_Handler,                           // 1  reset
    Default_Handler,                         // 2  NMI
    HardFault_Handler,                       // 3  HardFault
    Default_Handler,                         // 4  MemManage
    Default_Handler,                         // 5  BusFault
    Default_Handler,                         // 6  UsageFault
    nullptr, nullptr, nullptr, nullptr,      // 7-10 reserved
    Default_Handler,                         // 11 SVCall
    Default_Handler,                         // 12 DebugMonitor
    nullptr,                                 // 13 reserved
    Default_Handler,                         // 14 PendSV
    SysTick_Handler,                         // 15 SysTick
};
