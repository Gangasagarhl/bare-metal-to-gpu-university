// broken_startup.cc - F3-36 forensic evidence: the start-up code as "simplified" by a
// teammate (lab microcontroller: QEMU mps2-an385, Cortex-M3).
// Freestanding C++: no operating system, no C library, no exceptions, no RTTI.
// Vector-table layout and reset behaviour: Armv7-M Architecture Reference Manual,
// exception model and vector table sections (pending verification, see the chapter).
#include <stdint.h>

extern "C" {
extern uint32_t __stack_top;
extern uint32_t __data_load, __data_start, __data_end;
extern uint32_t __bss_start, __bss_end;
using InitFunction = void (*)();
extern InitFunction __init_array_start[];
extern InitFunction __init_array_end[];
}

int main();

extern "C" [[noreturn]] void Reset_Handler();
extern "C" [[noreturn]] void Default_Handler();

// Every handler below is "weak": an application function with exactly the same name wins.
#define OS305_WEAK_HANDLER(name) \
    extern "C" void name() __attribute__((weak, alias("Default_Handler")));
OS305_WEAK_HANDLER(NMI_Handler)
OS305_WEAK_HANDLER(HardFault_Handler)
OS305_WEAK_HANDLER(MemManage_Handler)
OS305_WEAK_HANDLER(BusFault_Handler)
OS305_WEAK_HANDLER(UsageFault_Handler)
OS305_WEAK_HANDLER(SVC_Handler)
OS305_WEAK_HANDLER(DebugMon_Handler)
OS305_WEAK_HANDLER(PendSV_Handler)
OS305_WEAK_HANDLER(SysTick_Handler)
OS305_WEAK_HANDLER(Irq0_Handler)  OS305_WEAK_HANDLER(Irq1_Handler)
OS305_WEAK_HANDLER(Irq2_Handler)  OS305_WEAK_HANDLER(Irq3_Handler)
OS305_WEAK_HANDLER(Irq4_Handler)  OS305_WEAK_HANDLER(Irq5_Handler)
OS305_WEAK_HANDLER(Irq6_Handler)  OS305_WEAK_HANDLER(Irq7_Handler)
OS305_WEAK_HANDLER(Irq8_Handler)  OS305_WEAK_HANDLER(Irq9_Handler)
OS305_WEAK_HANDLER(Irq10_Handler) OS305_WEAK_HANDLER(Irq11_Handler)
OS305_WEAK_HANDLER(Irq12_Handler) OS305_WEAK_HANDLER(Irq13_Handler)
OS305_WEAK_HANDLER(Irq14_Handler) OS305_WEAK_HANDLER(Irq15_Handler)

extern "C" [[noreturn]] void Reset_Handler()
{
    // 1. Copy initial values of initialised variables from flash to RAM (.data).
    const uint32_t* src = &__data_load;
    for (uint32_t* dst = &__data_start; dst < &__data_end; ++dst, ++src) {
        *dst = *src;
    }
    // 2. Zero the zero-initialised variables (.bss).
    for (uint32_t* dst = &__bss_start; dst < &__bss_end; ++dst) {
        *dst = 0;
    }
    // (constructors of global objects: "not needed, we have no C++ runtime")
    // 4. Run the application. Firmware never returns: sleep forever if main() does.
    main();
    for (;;) {
        asm volatile("wfi");
    }
}

extern "C" [[noreturn]] void Default_Handler()
{
    for (;;) {
        asm volatile("bkpt #0");   // with a debugger attached, an unexpected exception stops here
    }
}

using Handler = void (*)();

// Word 0: initial main stack pointer. Words 1-15: system exceptions. Words 16+: IRQ 0, 1, ...
extern "C" __attribute__((section(".vectors"), used))
const Handler vector_table[16 + 16] = {
    reinterpret_cast<Handler>(&__stack_top),       // 0  initial SP
    Reset_Handler,                                 // 1  reset
    NMI_Handler, HardFault_Handler,                // 2, 3
    MemManage_Handler, BusFault_Handler,           // 4, 5
    UsageFault_Handler,                            // 6
    nullptr, nullptr, nullptr, nullptr,            // 7-10 reserved
    SVC_Handler, DebugMon_Handler,                 // 11, 12
    nullptr,                                       // 13 reserved
    PendSV_Handler, SysTick_Handler,               // 14, 15
    Irq0_Handler,  Irq1_Handler,  Irq2_Handler,  Irq3_Handler,    // 16-19
    Irq4_Handler,  Irq5_Handler,  Irq6_Handler,  Irq7_Handler,    // 20-23
    Irq8_Handler,  Irq9_Handler,  Irq10_Handler, Irq11_Handler,   // 24-27
    Irq12_Handler, Irq13_Handler, Irq14_Handler, Irq15_Handler,   // 28-31
};
