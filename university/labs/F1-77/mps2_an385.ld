/* mps2_an385.ld - linker script for the emulated lab microcontroller (QEMU mps2-an385).
 * Region origins and sizes are taken from QEMU's own memory tree ("info mtree -f",
 * saved as mtree.out by run.sh): mps.ssram1 at 0x00000000 holds the code,
 * mps.ssram23 at 0x20000000 is data RAM. We use only 64 KiB of each. */
MEMORY
{
    CODE (rx)  : ORIGIN = 0x00000000, LENGTH = 64K
    RAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}

ENTRY(Reset_Handler)

SECTIONS
{
    .text :
    {
        KEEP(*(.vectors))          /* the vector table must be first: the core reads it at reset */
        *(.text .text.*)
        *(.rodata .rodata.*)
    } > CODE

    .data :
    {
        __data_start = .;
        *(.data .data.*)
        __data_end = .;
    } > RAM AT > CODE              /* runs from RAM, initial values stored in CODE */
    __data_load = LOADADDR(.data);

    .bss (NOLOAD) :
    {
        __bss_start = .;
        *(.bss .bss.* COMMON)
        __bss_end = .;
    } > RAM

    __stack_top = ORIGIN(RAM) + LENGTH(RAM);   /* the stack grows down from the end of RAM */

    /DISCARD/ : { *(.ARM.exidx*) *(.comment) }
}
