/* os305.ld - linker script for the OS305 lab microcontroller (QEMU mps2-an385, Cortex-M3).
 * Region origins come from QEMU's own memory tree ("info mtree -f", saved as mtree.out):
 * mps.ssram1 at 0x00000000 is used as code memory ("flash" in this course),
 * mps.ssram23 at 0x20000000 as data RAM. We use only 256 KiB and 64 KiB of them, to
 * behave like a small microcontroller. */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}

ENTRY(Reset_Handler)

SECTIONS
{
    .text :
    {
        KEEP(*(.vectors))              /* the vector table must be first: read by the core at reset */
        *(.text .text.*)
        *(.rodata .rodata.*)
        . = ALIGN(4);
        __init_array_start = .;        /* constructors of global C++ objects */
        KEEP(*(.init_array .init_array.*))
        __init_array_end = .;
        __text_end = .;
    } > FLASH

    .data :
    {
        . = ALIGN(4);
        __data_start = .;
        *(.data .data.*)
        . = ALIGN(4);
        __data_end = .;
    } > RAM AT > FLASH                 /* runs from RAM, initial values stored in FLASH */
    __data_load = LOADADDR(.data);

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start = .;
        *(.bss .bss.* COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > RAM

    __stack_size = 4K;
    __stack_top = ORIGIN(RAM) + LENGTH(RAM);       /* the main stack grows down from here */
    __stack_limit = __stack_top - __stack_size;
    ASSERT(__bss_end <= __stack_limit, "RAM overflow: .data + .bss collide with the stack")

    /DISCARD/ : { *(.ARM.exidx*) *(.comment) }
}
