/* stage.ld - F4-32: layout of one firmware stage. TEXT_BASE (where the code runs) and RAM_BASE
   (where .data, .bss and the stack go) come from the ld command line (--defsym), so the ROM can
   run from flash with its variables in secure RAM, and BL2/BL31 run entirely from secure RAM. */
ENTRY(_start)
SECTIONS
{
    . = TEXT_BASE;
    .text : { KEEP(*(.text.head)) *(.text .text.*) }
    .rodata : ALIGN(8) { *(.rodata .rodata.*) }
    __text_end = .;
    . = (RAM_BASE == 0) ? ALIGN(16) : RAM_BASE;
    .data : ALIGN(8) { *(.data .data.*) }
    .bss (NOLOAD) : ALIGN(16) {
        __bss_start = .;
        *(.bss .bss.*) *(COMMON)
        . = ALIGN(16);
        __bss_end = .;
        . += 0x2000;
        __stack_top = .;
    }
    /DISCARD/ : { *(.comment) *(.note .note.*) *(.eh_frame*) }
}
