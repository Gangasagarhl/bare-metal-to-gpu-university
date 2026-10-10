/* link_new.ld: the teammate's "tidier" layout: read-only data first, then code. */
ENTRY(_start)

SECTIONS
{
    . = 1M;
    __kernel_start = .;

    .rodata ALIGN(4K) : {
        __rodata_start = .;
        *(.rodata .rodata.*)
    }

    .text ALIGN(4K) : {
        __text_start = .;
        KEEP(*(.multiboot))
        *(.text .text.*)
        __text_end = .;
    }

    .data ALIGN(4K) : {
        __data_start = .;
        *(.data .data.*)
    }

    .bss ALIGN(4K) : {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    __kernel_end = ALIGN(4K);

    /DISCARD/ : {
        *(.comment) *(.note .note.*) *(.eh_frame*)
    }
}
