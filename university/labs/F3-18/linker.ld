/* linker.ld - F3-18: the kernel runs at KERNEL_VMA + physical address (higher half),
 * except the small 32-bit boot stub, which runs where it is loaded (1 MiB). */
ENTRY(_start)
KERNEL_VMA = 0xffffffff80000000;

SECTIONS
{
    . = 0x100000;
    __load_start = .;
    .boot : {
        KEEP(*(.multiboot))
        *(.boot.text)
        *(.boot.data)
    }
    . += KERNEL_VMA;
    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VMA) {
        __text_start = .;
        *(.text .text.*)
        __text_end = .;
    }
    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VMA) {
        __rodata_start = .;
        *(.rodata .rodata.*)
        . = ALIGN(8);
        __init_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
        __rodata_end = .;
        . = ALIGN(4K);       /* pad to a page: the file image ends on a page boundary */
    }
    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VMA) {
        __data_start = .;
        *(.data .data.*)
        . = ALIGN(4K);
    }
    __load_end_phys = . - KERNEL_VMA;
    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VMA) {
        __bss_start = .;
        *(COMMON)
        *(.bss .bss.*)
        . = ALIGN(4K);
        __bss_end = .;
    }
    __bss_end_phys = . - KERNEL_VMA;
    __kernel_end = .;
    /DISCARD/ : { *(.comment) *(.note.*) *(.eh_frame*) }
}
