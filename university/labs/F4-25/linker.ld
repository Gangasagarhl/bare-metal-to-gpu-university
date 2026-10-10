/* linker.ld - F4-25: the F4-24 layout with every section starting on a 4 KiB page, so the
 * MMU can give each one its own permissions: text read-only and executable, rodata read-only,
 * data, bss and stacks read-write and never executable (W^X). */
ENTRY(_start)

SECTIONS
{
    . = 0x40080000;
    __image_start = .;
    .text : {
        __text_start = .;
        KEEP(*(.head.text))
        *(.text .text.*)
    }
    . = ALIGN(4096);
    __text_end = .;
    .rodata : {
        __rodata_start = .;
        *(.rodata .rodata.*)
        . = ALIGN(8);
        __init_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
    }
    . = ALIGN(4096);
    __rodata_end = .;
    .data : {
        __data_start = .;
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
    . += 0x4000 * 8;              /* 16 KiB boot stack per CPU, room for 8 CPUs (F4-26) */
    __stack_top = .;
    __image_end = .;
    __image_size = __image_end - __image_start;
    /DISCARD/ : { *(.comment) *(.note.*) *(.eh_frame*) }
}
