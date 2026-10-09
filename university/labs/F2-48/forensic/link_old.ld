/* link_old.ld: the linker script copied from an older project. */
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
    .ctors ALIGN(4) : {
        __init_array_start = .;
        KEEP(*(SORT(.ctors.*)))
        KEEP(*(.ctors))
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
