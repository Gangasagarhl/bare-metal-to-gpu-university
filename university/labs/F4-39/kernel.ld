/* kernel.ld - DR404 lab kernels: identity-mapped, loaded at 1 MiB (Multiboot). */
ENTRY(_start)
SECTIONS
{
    . = 0x100000;
    .text : {
        KEEP(*(.multiboot))
        *(.text.boot)
        *(.text .text.*)
    }
    .rodata : ALIGN(4096) { *(.rodata .rodata.*) }
    .data : ALIGN(4096) { *(.data .data.*) }
    .bss : ALIGN(4096) {
        __bss_start = .;
        *(.bss .bss.*) *(COMMON)
        __bss_end = .;
    }
    __kernel_end = ALIGN(4096);
    /DISCARD/ : { *(.eh_frame*) *(.comment) *(.note*) }
}
