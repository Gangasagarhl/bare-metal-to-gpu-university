/* stm32f100.ld - linker script for the second emulated lab microcontroller
 * (QEMU stm32vldiscovery machine, an STM32F100 model with a Cortex-M3 core).
 * Region origins and sizes are copied from QEMU's memory tree (mtree.out of this lab):
 * STM32F100.flash at 0x08000000 (128 KiB, also visible at address 0)
 * STM32F100.sram  at 0x20000000 (8 KiB). */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 128K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 8K
}

ENTRY(Reset_Handler)

SECTIONS
{
    .text :
    {
        KEEP(*(.vectors))          /* vector table first: the core reads words 0 and 1 at reset */
        *(.text .text.*)
        *(.rodata .rodata.*)
    } > FLASH

    .data :
    {
        __data_start = .;
        *(.data .data.*)
        __data_end = .;
    } > RAM AT > FLASH             /* VMA in RAM, LMA in FLASH */
    __data_load = LOADADDR(.data);

    .bss (NOLOAD) :
    {
        __bss_start = .;
        *(.bss .bss.* COMMON)
        __bss_end = .;
    } > RAM

    __stack_top = ORIGIN(RAM) + LENGTH(RAM);

    /DISCARD/ : { *(.ARM.exidx*) *(.comment) }
}
