/* higher.ld: the same kernel linked for the "higher half". Code and data are linked to run at
   0xC0100000 (VMA) but are loaded at 1 MiB (LMA). This needs paging to be switched on before
   the higher addresses are used (track B), so this lab only links it and reads the result. */
ENTRY(_start)
KERNEL_VMA = 0xC0000000;

SECTIONS
{
    . = 1M + KERNEL_VMA;
    __kernel_start = .;

    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VMA) {
        __text_start = .;
        KEEP(*(.multiboot))
        *(.text .text.*)
        __text_end = .;
    }
    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VMA) {
        __rodata_start = .;
        *(.rodata .rodata.*)
    }
    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VMA) {
        __data_start = .;
        *(.data .data.*)
    }
    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VMA) {
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
