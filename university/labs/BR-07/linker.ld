/* linker.ld - BR-07: the raspi3b program. RAM starts at 0 on this machine and the image is
 * loaded at 0x80000 (as in F4-27's linker.ld). */
ENTRY(_start)
SECTIONS
{
    . = 0x80000;
    __image_start = .;
    .text : { KEEP(*(.head.text)) *(.text .text.*) }
    .rodata ALIGN(16) : { *(.rodata .rodata.*) }
    .data ALIGN(16) : { *(.data .data.*) }
    .bss ALIGN(16) : { __bss_start = .; *(COMMON) *(.bss .bss.*) . = ALIGN(16); __bss_end = .; }
    . = ALIGN(4096);
    . += 0x4000;
    __stack_top = .;
    __image_end = .;
    __image_size = __image_end - __image_start;
    /DISCARD/ : { *(.comment) *(.note.*) *(.eh_frame*) }
}
