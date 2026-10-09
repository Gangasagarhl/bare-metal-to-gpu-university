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
    /* the user region: 32 KiB, aligned to its size so one PMP entry covers it */
    . = 0x80020000;
    user_start = .;
    .user : { *(.user.text) *(.user.data) }
    user_stack_top = 0x80028000;
    user_end = 0x80028000;
}
