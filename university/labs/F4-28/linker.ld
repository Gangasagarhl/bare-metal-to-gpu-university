/* linker.ld - F4-28: RV64 kernel for QEMU virt. RAM starts at 0x80000000 there (F4-23
 * checklist row 3); OpenSBI occupies the start of RAM and jumps to 0x80200000, where QEMU
 * puts a -kernel image when it also loads the default firmware (checked by the lab run). */
ENTRY(_start)

SECTIONS
{
    . = 0x80200000;
    __image_start = .;
    .text : {
        KEEP(*(.text.entry))
        *(.text .text.*)
    }
    .rodata ALIGN(16) : {
        *(.srodata .srodata.*)
        *(.rodata .rodata.*)
        . = ALIGN(8);
        __init_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
    }
    .data ALIGN(16) : {
        __global_pointer$ = . + 0x800;
        *(.sdata .sdata.*)
        *(.data .data.*)
    }
    .bss ALIGN(16) : {
        __bss_start = .;
        *(.sbss .sbss.*)
        *(COMMON)
        *(.bss .bss.*)
        . = ALIGN(16);
        __bss_end = .;
    }
    . = ALIGN(4096);
    . += 0x4000 * 8;              /* 16 KiB stack per hart, room for 8 harts (F4-29) */
    __stack_top = .;
    __image_end = .;
    /DISCARD/ : { *(.comment) *(.note.*) *(.eh_frame*) *(.riscv.attributes) }
}
