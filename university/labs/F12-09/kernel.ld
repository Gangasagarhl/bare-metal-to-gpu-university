/* kernel.ld - link the CI test kernel at 1 MiB, Multiboot header first. */
ENTRY(_start)
SECTIONS
{
    . = 0x100000;
    .multiboot : { KEEP(*(.multiboot)) }
    .text : { *(.text .text.*) }
    .rodata : { *(.rodata .rodata.*) }
    .data : { *(.data .data.*) }
    .bss : { *(.bss .bss.* COMMON) }
    /DISCARD/ : { *(.comment) *(.eh_frame*) *(.note*) }
}
