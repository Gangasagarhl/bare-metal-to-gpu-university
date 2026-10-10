/* link_aarch64.ld - BR-06: probe image for QEMU virt; RAM starts at 0x40000000 there and the
 * header's text_offset asks for RAM base + 0x80000. */
ENTRY(_start)
SECTIONS
{
    . = 0x40080000;
    __image_start = .;
    .text : { KEEP(*(.head.text)) *(.text .text.*) }
    .rodata : { *(.rodata .rodata.*) }
    .data : { *(.data .data.*) }
    .bss : { *(.bss .bss.*) *(COMMON) }
    . = ALIGN(16);
    . += 0x4000;
    __stack_top = .;
    __image_size = . - __image_start;
    /DISCARD/ : { *(.comment) *(.note.*) *(.eh_frame*) }
}
