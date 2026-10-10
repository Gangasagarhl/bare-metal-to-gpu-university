/* Copied from the DR403 lab folder F4-31 (only this line added) (the course kernel base); RB403 reuses it. */
/* kernel.ld - DR403 kernel image layout. Linked at 0 and position independent (start.S applies
   the R_AARCH64_RELATIVE fix-ups), so the same bytes run wherever a loader places them. */
ENTRY(_start)
SECTIONS
{
    . = 0;
    .text : { KEEP(*(.text.head)) *(.text .text.*) }
    .rodata : ALIGN(8) { *(.rodata .rodata.*) }
    .data : ALIGN(8) { *(.data.rel.ro .data.rel.ro.*) *(.data .data.*) *(.got .got.*) }
    /* F4-35: every driver file places one descriptor here; the driver core walks the list */
    .drivers : ALIGN(8) { __drivers_start = .; KEEP(*(dr403_drivers)) __drivers_end = .; }
    .rela.dyn : ALIGN(8) { *(.rela .rela.*) }
    /* outside the section: ld sizes the dynamic relocations late, so take its final size */
    __rela_start = ADDR(.rela.dyn);
    __rela_end = ADDR(.rela.dyn) + SIZEOF(.rela.dyn);
    .dynamic : { *(.dynamic) }
    .bss (NOLOAD) : ALIGN(16) {
        __bss_start = .;
        *(.bss .bss.*) *(COMMON)
        . = ALIGN(16);
        __bss_end = .;
        . += 0x4000;
        __stack_top = .;
    }
    _image_end = .;
    /DISCARD/ : { *(.comment) *(.note .note.*) *(.eh_frame*) *(.dynsym) *(.dynstr) *(.hash) *(.gnu.hash) *(.interp) }
}
