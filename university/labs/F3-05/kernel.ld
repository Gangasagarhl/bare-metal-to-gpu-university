ENTRY(_start)
SECTIONS
{
    . = 0x80000000;
    .text : { *(.text.entry) *(.text .text.*) }
    .rodata : { *(.rodata .rodata.*) *(.srodata .srodata.*) }
    .data : { *(.data .data.*) *(.sdata .sdata.*) }
    .bss : { *(.bss .bss.*) *(.sbss .sbss.*) *(COMMON) }
    . = ALIGN(16);
    stacks = .;
    . += 4096 * 4;
}
