/* linker.ld - F4-24: AArch64 kernel image for QEMU virt. RAM starts at 0x40000000 there
 * (F4-23 checklist row 3); the image header asks to be loaded at RAM base + 0x80000.
 * Identity-mapped: the kernel runs at its physical address (the MMU is switched on in F4-25
 * with an identity map, so these addresses stay valid). */
ENTRY(_start)

SECTIONS
{
    . = 0x40080000;
    __image_start = .;
    .text : {
        KEEP(*(.head.text))
        *(.text .text.*)
    }
    .rodata ALIGN(16) : {
        *(.rodata .rodata.*)
        . = ALIGN(8);
        __init_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
    }
    .data ALIGN(16) : {
        *(.data .data.*)
    }
    .bss ALIGN(16) : {
        __bss_start = .;
        *(COMMON)
        *(.bss .bss.*)
        . = ALIGN(16);
        __bss_end = .;
    }
    . = ALIGN(4096);
    . += 0x4000 * 8;              /* boot stack 16 KiB per CPU, room for 8 CPUs (F4-26) */
    __stack_top = .;
    __image_end = .;
    __image_size = __image_end - __image_start;
    /DISCARD/ : { *(.comment) *(.note.*) *(.eh_frame*) }
}
