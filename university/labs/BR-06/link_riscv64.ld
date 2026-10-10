/* link_riscv64.ld - BR-06: probe image for QEMU virt; RAM starts at 0x80000000 there.
 * LOAD_ADDR comes from the command line (--defsym): 0x80200000 behind OpenSBI,
 * 0x80000000 with -bios none. */
ENTRY(_start)
SECTIONS
{
    . = LOAD_ADDR;
    .text : { KEEP(*(.text.entry)) *(.text .text.*) }
    .rodata : { *(.srodata .srodata.*) *(.rodata .rodata.*) }
    .data : { __global_pointer$ = . + 0x800; *(.sdata .sdata.*) *(.data .data.*) }
    .bss : { *(.sbss .sbss.*) *(.bss .bss.*) *(COMMON) }
    . = ALIGN(16);
    . += 0x4000;
    __stack_top = .;
    /DISCARD/ : { *(.comment) *(.note.*) *(.eh_frame*) }
}
