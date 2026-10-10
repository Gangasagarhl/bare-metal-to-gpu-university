/* link_x86.ld - BR-06: 32-bit ELF probe for QEMU's Multiboot loader, loaded at 1 MiB. */
ENTRY(_start)
SECTIONS
{
    . = 0x100000;
    .text : { KEEP(*(.multiboot)) *(.text .text.*) }
    .rodata : { *(.rodata .rodata.*) }
    .data : { *(.data .data.*) }
    .bss : { *(.bss .bss.*) *(COMMON) }
    . = ALIGN(16);
    . += 0x4000;
    __stack_top = .;
    /DISCARD/ : { *(.comment) *(.note.*) *(.eh_frame*) }
}
