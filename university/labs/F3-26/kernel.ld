/* kernel.ld - the OS303 teaching kernel is identity-mapped and loaded at 1 MiB.
   (A higher-half layout, as milestone B4 asks, is left out to keep the boot path short.) */
ENTRY(_start)
SECTIONS
{
    . = 0x100000;
    __kernel_start = .;
    .text : {
        KEEP(*(.multiboot))
        *(.text.boot)
        *(.text .text.*)
    }
    .rodata : ALIGN(4096) {
        *(.rodata .rodata.*)
        __uaccess_fixups_start = .;
        KEEP(*(.uaccess_fixups))
        __uaccess_fixups_end = .;
    }
    .init_array : {
        __init_array_start = .;
        KEEP(*(.init_array .init_array.*))
        __init_array_end = .;
    }
    .data : ALIGN(4096) { *(.data .data.*) }
    .bss : ALIGN(4096) {
        __bss_start = .;
        *(.bss .bss.*) *(COMMON)
        __bss_end = .;
    }
    __kernel_end = ALIGN(4096);
    /DISCARD/ : { *(.eh_frame*) *(.comment) *(.note*) }
}
