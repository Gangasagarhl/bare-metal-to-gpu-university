/* kernel.ld - DR301 lab kernel: one 32-bit image loaded at 1 MiB, no paging. */
ENTRY(_start)
SECTIONS
{
    . = 0x100000;
    .text : { KEEP(*(.multiboot)) *(.text .text.*) }
    .rodata : { *(.rodata .rodata.*) }
    .data : { *(.data .data.*) }
    .bss : { *(COMMON) *(.bss .bss.*) }
    __kernel_end = .;
    /DISCARD/ : { *(.comment) *(.note.*) *(.eh_frame*) *(.init_array*) *(.ctors*) }
}
