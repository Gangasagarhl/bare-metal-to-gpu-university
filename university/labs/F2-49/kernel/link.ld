/* link.ld: the lab kernel's memory layout. Physical = virtual here (no paging yet). */
ENTRY(_start)

SECTIONS
{
    . = 1M;                          /* load the kernel at 1 MiB */
    __kernel_start = .;

    .text ALIGN(4K) : {
        __text_start = .;
        KEEP(*(.multiboot))          /* first: the loader searches the start of the file */
        *(.text .text.*)
        __text_end = .;
    }

    .rodata ALIGN(4K) : {
        __rodata_start = .;
        *(.rodata .rodata.*)
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
