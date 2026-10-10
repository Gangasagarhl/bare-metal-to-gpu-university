/* ns.ld - F11-05: the normal-world program in normal RAM at 0x40200000 (QEMU puts its device tree at the start of RAM). */
ENTRY(_start)
SECTIONS
{
    . = 0x40200000;
    .text : { *(.text.boot) *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) *(COMMON) }
    /DISCARD/ : { *(.comment) }
}
