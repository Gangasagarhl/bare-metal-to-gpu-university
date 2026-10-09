/* link.ld: F2-46's layout plus .init_array, the table of global-constructor functions. */
ENTRY(_start)

SECTIONS
{
    . = 1M;
    .text ALIGN(4K) : {
        KEEP(*(.multiboot))
        *(.text .text.*)
    }
    .rodata ALIGN(4K) : {
        *(.rodata .rodata.*)
    }
    .init_array ALIGN(4) : {
        __init_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
    }
    .data ALIGN(4K) : {
        *(.data .data.*)
    }
    .bss ALIGN(4K) : {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    /DISCARD/ : {
        *(.comment) *(.note .note.*) *(.eh_frame*)
    }
}
